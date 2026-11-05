
#include <stdio.h>
#include <stdlib.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <dirent.h>
#include <alloca.h>
#include <string.h>
#include <stdbool.h>
#include "header.h"
#include "../path_extractor.h"


void handle_dir(const char* home_dir_name, const char* current_dir_name, FILE* archive, bool do_not_analyze);
void handle_file(const char* home_directory, const char* relative_file_path, FILE* archive, bool do_not_analyze);

int main(int argc, char** argv) {
        char* archive_path;
        char* path_to_archiving_subject;
        bool  do_not_analyze;
        get_paths_from_args(argc, argv, &archive_path, &path_to_archiving_subject, &do_not_analyze);
        
        struct stat Q_stat;
        if (stat(path_to_archiving_subject, &Q_stat) != 0) {
                fprintf(stderr, "Failed to open %s\n", path_to_archiving_subject);
                return 1;
        }

        header R_header;
        memcpy(R_header.signature, badec_signature, sizeof(badec_signature));
        R_header.version = 3;
        R_header.directory_is_archived = S_ISDIR(Q_stat.st_mode);
        R_header.frequency_table_is_general = false;
        
        FILE* R = fopen(archive_path, "wb");
        fwrite(&R_header, sizeof(header), 1, R);
        fflush(R);

        if (R_header.directory_is_archived) {
                handle_dir(path_to_archiving_subject, "", R, do_not_analyze);
        }
        else {
                char* file_name_substr = strrchr(path_to_archiving_subject, '/');
                *file_name_substr = '\0';
                file_name_substr++;

                handle_file(path_to_archiving_subject, file_name_substr, R, do_not_analyze);
        }

        fclose(R);
}

void handle_dir(const char* home_dir_name, const char* current_dir_name, FILE* archive, bool do_not_analyze) {
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
                        handle_dir(home_dir_name, entry_name, archive, do_not_analyze);
                        continue;
                }
                
                handle_file(home_dir_name, entry_name, archive, do_not_analyze);
        }
        closedir(dir);
}

void handle_file(const char* home_directory, const char* relative_file_path, FILE* archive, bool do_not_analyze) {
        char* absolute_file_path = (char*)alloca(strlen(home_directory) + 1 + strlen(relative_file_path) + 1);
        sprintf(absolute_file_path, "%s/%s", home_directory, relative_file_path);
        
        struct stat file_stat;
        stat(absolute_file_path, &file_stat);
        
        FILE* file = fopen(absolute_file_path, "rb");

        size_t relative_file_path_length = strlen(relative_file_path) + 1;

        fwrite(&rle_encoding_code, sizeof(uint8_t), 1, archive);
        fwrite(&relative_file_path_length, sizeof(size_t), 1, archive);
        fwrite(relative_file_path, sizeof(char), relative_file_path_length, archive);
        fwrite(&file_stat.st_size, sizeof(off_t), 1, archive);

        uint8_t lcPair[2] = {0, 0};
        uint8_t currByte;
        uint8_t firstIteration = 1;
        uint8_t isEof = 0;
        while (isEof == 0) {
                lcPair[1] = currByte;
                isEof = !fread(&currByte, sizeof(uint8_t), 1, file);
                if (isEof) currByte = lcPair[1] + 1;

                if (firstIteration) {      
                        firstIteration = 0;
                        continue;
                }

                if (lcPair[1] == currByte && lcPair[0] != 255) {
                        lcPair[0] += 1;
                }
                else {
                        //printf("%d:%d ", lcPair[0], lcPair[1]);
                        fwrite(lcPair, sizeof(uint8_t), 2, archive);
                        lcPair[0] = 0;
                }
        }

        fflush(archive);
        fclose(file);
}