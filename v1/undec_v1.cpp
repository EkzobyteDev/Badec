
#include <cstdio>
#include <sys/sendfile.h>
#include "header_v1.h"
#include "path_extractor.h"


bool bytes_equal(const uint8_t* a, const uint8_t* b, size_t bytes_count) {
        for (size_t i = 0; i < bytes_count; i++) {
                if (a[i] != b[i]) {
                        return false;
                }
        }
        return true;
}

int main(int argc, char** argv) {
        char*    path_to_R;
        char*    path_to_Q;
        
        get_paths_from_args(argc, argv, &path_to_R, &path_to_Q);

        FILE* Q = fopen(path_to_Q, "wb");
        FILE* R = fopen(path_to_R, "rb");

        header R_header;
        size_t bytes_read = fread(&R_header, sizeof(header), 1, R);

        if (bytes_read != 1) {
                fprintf(stderr, "Failed to unarchive %s. Wrong file header\n", path_to_R);
                return 1;
        }

        if (bytes_equal(R_header.signature, badec_signature, sizeof(badec_signature)) == false) {
                fprintf(stderr, "Failed to unarchive %s. Wrong signature\n", path_to_R);
                return 1;
        }

        if (R_header.version != 0) {
                fprintf(stderr, "Failed to unarchive %s. Version %d is incompatible with this decoder \n", path_to_R, R_header.version);
                return 1;
        }

        off_t read_offset = sizeof(header);
        sendfile(fileno(Q), fileno(R), &read_offset, R_header.file_length);
}
