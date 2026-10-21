
#include <stdio.h>
#include <stdlib.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <dirent.h>
#include <alloca.h>
#include <string.h>
#include <stdbool.h>
#include "header.h"
#include "huffman_codes.h"
#include "../path_extractor.h"


void handle_dir(const char* home_dir_name, const char* current_dir_name, FILE* archive);
void handle_file(const char* home_directory, const char* relative_file_path, FILE* archive);

void add_frequencies_to_table(FILE* file, uint64_t alphabet_length, uint8_t* frequencies_table);
void write_encoded_byte_to_archive(huffman_code code_of_byte, FILE* archive, bool start_over, bool force_write);

int main(int argc, char** argv) {
        char* archive_path;
        char* path_to_archiving_subject;
        get_paths_from_args(argc, argv, &archive_path, &path_to_archiving_subject);
        
        struct stat Q_stat;
        if (stat(path_to_archiving_subject, &Q_stat) != 0) {
                fprintf(stderr, "Failed to open %s\n", path_to_archiving_subject);
                return 1;
        }

        header R_header;
        memcpy(R_header.signature, badec_signature, sizeof(badec_signature));
        R_header.version = 2;
        R_header.directory_is_archived = S_ISDIR(Q_stat.st_mode);
        R_header.frequency_table_is_general = false;
        
        FILE* R = fopen(archive_path, "wb");
        fwrite(&R_header, sizeof(header), 1, R);
        fflush(R);

        if (R_header.directory_is_archived) {
                handle_dir(path_to_archiving_subject, "", R);
        }
        else {
                char* file_name_substr = strrchr(path_to_archiving_subject, '/');
                *file_name_substr = '\0';
                file_name_substr++;

                handle_file(path_to_archiving_subject, file_name_substr, R);
        }

        fclose(R);
}

void handle_dir(const char* home_dir_name, const char* current_dir_name, FILE* archive) {
        char* current_dir_full_name = (char*)alloca(strlen(home_dir_name) + 1 + strlen(current_dir_name) + 1);
        sprintf(current_dir_full_name, "%s/%s", home_dir_name, current_dir_name);
        DIR* dir = opendir(current_dir_full_name);
        
        struct dirent* dir_entry_ptr;
        while ((dir_entry_ptr = readdir(dir)) != NULL) {
                if (strcmp(dir_entry_ptr->d_name, ".") == 0 || strcmp(dir_entry_ptr->d_name, "..") == 0) {
                        continue;
                }

                char entry_name[strlen(current_dir_name) + 1 + strlen(dir_entry_ptr->d_name) + 1];
                sprintf(entry_name, "%s/%s", current_dir_name, dir_entry_ptr->d_name);
                           
                if (dir_entry_ptr->d_type == DT_DIR) {
                        handle_dir(home_dir_name, entry_name, archive);
                        continue;
                }
                
                handle_file(home_dir_name, entry_name, archive);
        }
        closedir(dir);
}

void handle_file(const char* home_directory, const char* relative_file_path, FILE* archive) {
        char* absolute_file_path = (char*)alloca(strlen(home_directory) + 1 + strlen(relative_file_path) + 1);
        sprintf(absolute_file_path, "%s/%s", home_directory, relative_file_path);
        
        struct stat file_stat;
        stat(absolute_file_path, &file_stat);
        
        FILE* file = fopen(absolute_file_path, "rb");
        uint8_t frequencies_table[ALPHABET_LENGTH] = {0};
        add_frequencies_to_table(file, ALPHABET_LENGTH, frequencies_table);
        
        size_t relative_file_path_length = strlen(relative_file_path) + 1;
        fwrite(&huffman_encoding_code, sizeof(huffman_encoding_code), 1, archive);
        fwrite(frequencies_table, sizeof(uint8_t), ALPHABET_LENGTH, archive);
        fwrite(&relative_file_path_length, sizeof(size_t), 1, archive);
        fwrite(relative_file_path, sizeof(char), relative_file_path_length, archive);
        fwrite(&file_stat.st_size, sizeof(off_t), 1, archive);

        huffman_code codes[ALPHABET_LENGTH] = {0};
        get_huffman_codes(ALPHABET_LENGTH, frequencies_table, codes);

        size_t lsum = 0;
        size_t cnt = 0;
        for (int i = 0; i < 256; i++) {
                lsum += codes[i].length * frequencies_table[i];
                if (codes[i].length != 0) cnt += frequencies_table[i];
                //printf("%d (l=%d f=%d): %0*b\n", i, codes[i].length, frequencies_table[i], codes[i].length, codes[i].code);
        }
        //printf("%d vs %d\n", lsum, cnt * 8);
        
        for (off_t i = 0; i < file_stat.st_size; i++) {
                uint8_t byte;
                fread(&byte, sizeof(byte), 1, file);
                write_encoded_byte_to_archive(codes[byte], archive, i == 0, i == (file_stat.st_size - 1));
        }
        
        fflush(archive);
        fclose(file);
}

void add_frequencies_to_table(FILE* file, uint64_t alphabet_length, uint8_t* frequencies_table) {
        fseek(file, 0, SEEK_SET);

        size_t* unscaled_frequencies = (size_t*)alloca(sizeof(size_t) * alphabet_length);
        memset(unscaled_frequencies, 0, sizeof(size_t) * alphabet_length);
        double max_frequency_value = 0;

        uint8_t byte;
        fread(&byte, sizeof(byte), 1, file);
        while (feof(file) == 0) {
                unscaled_frequencies[byte]++;
                if (unscaled_frequencies[byte] > max_frequency_value) {
                        max_frequency_value = unscaled_frequencies[byte];
                }
                fread(&byte, sizeof(byte), 1, file);
        }

        //printf("max: %f\n", max_frequency_value);
        for (size_t byte = 0; byte < alphabet_length - 1; byte++) {
                if (unscaled_frequencies[byte] == 0) {
                        frequencies_table[byte] = 0;
                        continue;
                }

                frequencies_table[byte] = (uint8_t)(((double)unscaled_frequencies[byte] / max_frequency_value) * 255);
                if (frequencies_table[byte] == 0) {
                        frequencies_table[byte] = 1;
                }

                //printf("%d -> %d\n", unscaled_frequencies[byte], frequencies_table[byte]);
        }

        fseek(file, 0, SEEK_SET);
}

void write_encoded_byte_to_archive(huffman_code code_of_byte, FILE* archive, bool start_over, bool force_write) {
        static huffman_code curr_code;
        if (start_over) curr_code = empty_huffman_code;

        curr_code.code <<= code_of_byte.length;
        curr_code.code |= code_of_byte.code & (((uint64_t)1 << code_of_byte.length) - 1);
        curr_code.length += code_of_byte.length;

        while (curr_code.length >= 8) {
                uint64_t code_copy = curr_code.code;
                code_copy >>= (curr_code.length - 8);

                fwrite(&code_copy, sizeof(uint8_t), 1, archive);
                curr_code.length -= 8;
        }
        if (force_write && curr_code.length != 0) {
                uint64_t code_copy = curr_code.code;
                code_copy <<= (8 - curr_code.length);

                fwrite(&code_copy, sizeof(uint8_t), 1, archive);
        }
}