#pragma once

#include <stdint.h>
#include <stdbool.h>

#define ALPHABET_LENGTH 256

const uint8_t badec_signature[6] = { 0xB, 0xA, 0xD, 0xE, 0xC, 0x1};

const uint8_t no_compression_encoding_code = 0;
const uint8_t huffman_encoding_code        = 1;
const uint8_t arithmetic_encoding_code     = 2;
const uint8_t shannon_encoding_code        = 3;

typedef struct {
        uint8_t   signature[6];
        uint16_t  version;
        bool      directory_is_archived; // Directory vs single file
        bool      frequency_table_is_general;
        // Data block for decoding:

        // If all files share same frequency table:
        // uint8_t encoding_algorithm;
        // uint8_t byte_frequencies[256];
        // File 1:
        // size_t filename_length;
        // char*    filename;
        // uint32_t file_size;
        // uint8_t*  data
        // File 2
        // ...

        // If files are encoded independently:
        // File 1:
        // uint8_t encoding_algorithm;
        // uint8_t byte_frequencies[256];
        // size_t filename_length;
        // char*    filename;
        // uint32_t file_size;
        // uint8_t*  data
        // File 2
        // ...
}header;