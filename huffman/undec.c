
#include <stdio.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <dirent.h>
#include "header.h"
#include "huffman_codes.h"
#include "../path_extractor.h"


void create_file_from_archive(const char* home_directory_path, FILE* archive);
void mk_path(const char* path_to_file);
bool write_encoded_byte_to_file(uint8_t byte_from_archive, huffman_code* codes, size_t file_size, FILE* file, bool start_over);

bool bytes_equal(const uint8_t* a, const uint8_t* b, size_t bytes_count) {
        for (size_t i = 0; i < bytes_count; i++) {
                if (a[i] != b[i]) {
                        return false;
                }
        }
        return true;
}

int main(int argc, char** argv) {
        char* archive_path;
        char* output_path;
        get_paths_from_args(argc, argv, &archive_path, &output_path);
        
        if (output_path[strlen(output_path) - 1] != '/') {
                fprintf(stderr, "Failed to unarchive %s. -Q argument has to be a folder (i.e has to end with \"/\")\n", archive_path);
                return 1;
        }

        header R_header;
        
        FILE* archive = fopen(archive_path, "rb");
        setvbuf(archive, NULL, _IONBF, 0);

        
        size_t bytes_read = fread(&R_header, sizeof(header), 1, archive);
        if (bytes_read != 1) {
                fprintf(stderr, "Failed to unarchive %s. Wrong file header\n", archive_path);
                return 1;
        }
        if (bytes_equal(R_header.signature, badec_signature, sizeof(badec_signature)) == false) {
                fprintf(stderr, "Failed to unarchive %s. Wrong signature\n", archive_path);
                return 1;
        }

        if (R_header.version != 2) {
                fprintf(stderr, "Failed to unarchive %s. Version %d is incompatible with this decoder\n", archive_path, R_header.version);
                return 1;
        }


        if (R_header.directory_is_archived) {
                while(feof(archive) == false) {
                        create_file_from_archive(output_path, archive);
                }
        }
        else {
                create_file_from_archive(output_path, archive);
        }

        fclose(archive);
}

void mk_path(const char* path_to_file) {
        char path_to_file_copy[strlen(path_to_file) + 1];
        strcpy(path_to_file_copy, path_to_file);

        char* start_ptr = path_to_file_copy;
        char* end_ptr;

        while((end_ptr = strchr(start_ptr + 1, '/')) != NULL) {
                *end_ptr = '\0';
                mkdir(path_to_file_copy, S_IXUSR | S_IWUSR | S_IRUSR);
                *end_ptr = '/';
                start_ptr = end_ptr;
        }
                
}

void create_file_from_archive(const char* home_directory_path, FILE* archive) {
        uint8_t encoding_code;
        fread(&encoding_code, sizeof(encoding_code), 1, archive);

        if (feof(archive)) {
                return;
        }

        uint8_t frequencies_table[ALPHABET_LENGTH];
        fread(frequencies_table, sizeof(uint8_t), ALPHABET_LENGTH, archive);
        
        size_t relative_file_path_length;
        fread(&relative_file_path_length, sizeof(size_t), 1, archive);
        
        char relative_file_path[relative_file_path_length];
        fread(relative_file_path, sizeof(char), relative_file_path_length, archive);
        
        off_t file_size;
        fread(&file_size, sizeof(off_t), 1, archive);

        char absolute_file_path[strlen(home_directory_path) + strlen(relative_file_path) + 1];
        sprintf(absolute_file_path, "%s%s", home_directory_path, relative_file_path);

        if (encoding_code > 1) {
                fprintf(stderr, "Failed to unarchive file %s. Wrong compression algorithm code\n", relative_file_path);
                return;
        }


        mk_path(absolute_file_path);
        FILE* file = fopen(absolute_file_path, "wb");

        if (encoding_code == 0) {
                sendfile(fileno(file), fileno(archive), NULL, (size_t)file_size);
        }
        else if (encoding_code == 1) {
                huffman_code codes[ALPHABET_LENGTH];
                get_huffman_codes(ALPHABET_LENGTH, frequencies_table, codes);
                write_encoded_byte_to_file(0, NULL, 0, NULL, true);

                uint8_t byte;
                do {
                        fread(&byte, sizeof(byte), 1, archive);
                }
                while (write_encoded_byte_to_file(byte, codes, (size_t)file_size, file, false) == false);
        }
        
        fclose(file);
}

bool write_encoded_byte_to_file(uint8_t byte_from_archive, huffman_code* codes, size_t file_size, FILE* file, bool start_over) {
        static huffman_code code;
        static size_t bytes_decoded;
        if (start_over) {
                code = empty_huffman_code;
                bytes_decoded = 0;
                return false;
        }

        for (int i = 0; i < 8; i++) {
                code.code <<= 1;
                code.code |= ((byte_from_archive >> (8 - i - 1)) & 1);
                code.length++;
                
                bool success;
                uint8_t byte = get_byte_from_code(ALPHABET_LENGTH, code, codes, &success);

                if (success == false) continue;
                
                fwrite(&byte, sizeof(byte), 1, file);
                code = empty_huffman_code;
                bytes_decoded++;

                if (bytes_decoded == file_size) {
                        return true;
                }
        }
        return false;
}
