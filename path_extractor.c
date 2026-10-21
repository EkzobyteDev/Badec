
#include <getopt.h>

void get_paths_from_args(int argc, char** argv, char** path_to_R, char** path_to_Q) {

        int option_id = 0;
        while ((option_id = getopt(argc, argv, "R:Q:")) != -1) {
                switch (option_id) {
                        case 'R':
                                *path_to_R = optarg;
                                break;
                        case 'Q':
                                *path_to_Q = optarg;
                                break;
                        default:
                                return;
                }
        }
}