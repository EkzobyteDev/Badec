
#include <stdio.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <dirent.h>
#include "header.h"
#include "arithmetic_codes.h"
#include "../path_extractor.h"


int create_file_from_archive(const char* home_directory_path, FILE* archive);
void mk_path(const char* path_to_file);

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
                        int err = create_file_from_archive(output_path, archive);
                        if (err) {
                                fprintf(stderr, "Failed to unarchive one of files in archive. Terminating\n", archive_path, R_header.version);
                                return 1;
                        }
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

int create_file_from_archive(const char* home_directory_path, FILE* archive) {
        uint8_t encoding_code;
        fread(&encoding_code, sizeof(uint8_t), 1, archive);

        if (feof(archive)) {
                return 0;
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

        if (encoding_code != no_compression_encoding_code && encoding_code != arithmetic_encoding_code) {
                fprintf(stderr, "Failed to unarchive file %s. Wrong compression algorithm code\n", relative_file_path);
                return 1;
        }


        /*
        for (int i = 0; i < 256; i++) {
                printf("%3d ", frequencies_table[i]);
        }
        printf("\n");
        */


        mk_path(absolute_file_path);
        FILE* file = fopen(absolute_file_path, "wb");

        //printf("File %s (%d b)\n", absolute_file_path, file_size);
        if (encoding_code == no_compression_encoding_code) {
                fflush(file);
                sendfile(fileno(file), fileno(archive), NULL, (size_t)file_size);
        }
        else if (encoding_code == arithmetic_encoding_code) {
                arithmetic_decode_file(archive, file_size, frequencies_table, file);
        }
        
        fclose(file);
        return 0;
}
