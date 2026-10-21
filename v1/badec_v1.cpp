
#include <cstdio>
#include <cstring>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include "header_v1.h"
#include "path_extractor.h"

int main(int argc, char** argv) {
        char*       path_to_R;
        char*       path_to_Q;
        struct stat Q_stat;
        
        get_paths_from_args(argc, argv, &path_to_R, &path_to_Q);

        FILE* Q = fopen(path_to_Q, "rb");
        FILE* R = fopen(path_to_R, "wb");

        fstat(fileno(Q), &Q_stat);

        header R_header;
        memcpy(R_header.signature, badec_signature, sizeof(badec_signature));
        R_header.version     = 0;
        R_header.file_length = Q_stat.st_size;
        
        fwrite(&R_header, sizeof(header), 1, R);
        fflush(R);
        sendfile(fileno(R), fileno(Q), NULL, Q_stat.st_size);
}
