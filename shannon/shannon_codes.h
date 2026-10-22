#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Shannon coding. Type and function names are kept the same as in huffman/
// so badec.c and undec.c only include this file instead of huffman_codes.h.

#define empty_huffman_code (huffman_code){0}


typedef struct {
        uint8_t  value;
        uint32_t frequency;
}shannon_symbol;



typedef struct {
        uint64_t code;
        uint8_t  length;
}huffman_code;

void get_huffman_codes(uint64_t alphabet_length, uint8_t* frequencies_table, huffman_code* codes);
uint8_t get_byte_from_code(uint64_t alphabet_length, huffman_code code, huffman_code* codes, bool* success);
