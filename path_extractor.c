
#include <getopt.h>
#include <stdbool.h>
#include <stddef.h>

void get_paths_from_args(int argc, char** argv, char** path_to_R, char** path_to_Q, bool* do_not_analyze) {

        int option_id = 0;
        if (do_not_analyze != NULL) {
                *do_not_analyze = false;
        }
        while ((option_id = getopt(argc, argv, "R:Q:N")) != -1) {
                switch (option_id) {
                        case 'R':
                                *path_to_R = optarg;
                                break;
                        case 'Q':
                                *path_to_Q = optarg;
                                break;
                        case 'N':
                                if (do_not_analyze != NULL) {
                                        *do_not_analyze = true;
                                }
                        default:
                                return;
                }
        }
}