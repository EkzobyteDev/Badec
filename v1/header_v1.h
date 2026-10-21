#pragma once

#include <cstdint>

const uint8_t badec_signature[6] = { 0xB, 0xA, 0xD, 0xE, 0xC, 0x1};

struct header {
        uint8_t   signature[6];
        uint16_t  version;
        uint64_t  file_length;
};