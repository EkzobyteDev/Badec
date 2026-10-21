
#include <cstdio>
#include <cstring>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <dirent.h>
#include "header_v2.h"
#include "path_extractor.h"


void handle_dir(const char* home_dir_name, const char* current_dir_name, FILE* archive);
void create_file_from_archive(const char* home_directory_path, const char* relative_file_path, FILE* archive);

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
        R_header.directory_is_archived = S_ISDIR(Q_stat.st_mode);
        
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

                create_file_from_archive(path_to_archiving_subject, file_name_substr, R);
        }

        fclose(R);
}

void create_file_from_archive(const char* home_directory_path, const char* relative_file_path, FILE* archive) {
        char absolute_file_path[strlen(home_directory_path) + 1 + strlen(relative_file_path) + 1];
        sprintf(absolute_file_path, "%s/%s", home_directory_path, relative_file_path);
        
        struct stat file_stat;
        stat(absolute_file_path, &file_stat);
        
        size_t relative_file_path_length = strlen(relative_file_path) + 1;
        fwrite(&relative_file_path_length, sizeof(size_t), 1, archive);
        fwrite(relative_file_path, sizeof(char), relative_file_path_length, archive);
        fwrite(&file_stat.st_size, sizeof(off_t), 1, archive);
        fflush(archive);
        
        FILE* file = fopen(absolute_file_path, "rb");
        sendfile(fileno(archive), fileno(file), NULL, file_stat.st_size);
        
        fclose(file);
}

void handle_dir(const char* home_dir_name, const char* current_dir_name, FILE* archive) {
        char current_dir_full_name[strlen(home_dir_name) + 1 + strlen(current_dir_name) + 1] = "";
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
                
                create_file_from_archive(home_dir_name, entry_name, archive);
        }
        closedir(dir);
}
