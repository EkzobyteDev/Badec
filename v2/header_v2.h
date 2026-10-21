#pragma once

#include <cstdint>

const uint8_t badec_signature[6] = { 0xB, 0xA, 0xD, 0xE, 0xC, 0x1};

struct header {
        uint8_t   signature[6];
        uint16_t  version                    = 1;
        uint8_t   compression_algorithm_code = 0;
        bool      directory_is_archived      = false; // Directory vs single file
        // Data block for decoding
        // File 1:
        // size_t filename_length;
        // char*    filename;
        // uint32_t file_size;
        // uint8_t*  data
        // File 2
        // ...
        // File 3
        // ...
        // ...
};