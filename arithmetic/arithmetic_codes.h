#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

// Integer arithmetic (range) coding with 32-bit low/high bounds.
// Symbols are bytes, their intervals come from the archive's 8-bit frequency table.

#define ARITHMETIC_ALPHABET_LENGTH 256
#define ARITHMETIC_CODE_BITS       32

#define ARITHMETIC_TOP_VALUE      0xFFFFFFFFu
#define ARITHMETIC_FIRST_QUARTER  0x40000000u
#define ARITHMETIC_HALF           0x80000000u
#define ARITHMETIC_THIRD_QUARTER  0xC0000000u

typedef struct {
        FILE*   file;
        uint8_t byte;
        uint8_t bits_count;
}bit_stream;

// Encodes file_size bytes of file into archive.
// Writes exactly as many bytes as arithmetic_decode_file reads back.
void arithmetic_encode_file(FILE* file, off_t file_size, uint8_t* frequencies_table, FILE* archive);

// Decodes file_size bytes from archive into file.
void arithmetic_decode_file(FILE* archive, off_t file_size, uint8_t* frequencies_table, FILE* file);
