
#include "huffman/huffman_codes.h"
#include <stdio.h>

#define ALPHABET_LENGTH 256


int main() {
        uint8_t frequencies_table[ALPHABET_LENGTH] = {0};
        huffman_code codes[ALPHABET_LENGTH] = {0};

        frequencies_table['r'] = 12;
        frequencies_table['a'] = 6;
        frequencies_table['b'] = 3;
        frequencies_table['c'] = 1;
        frequencies_table['d'] = 1;

        get_huffman_codes(ALPHABET_LENGTH, frequencies_table, codes);

        char str[5] = "rabcd";
        for (int i = 0; i < 5; i++) {
                char c = str[i];
                printf("%c: %0*b\n", c, codes[c].length, codes[c].code);
        }
}