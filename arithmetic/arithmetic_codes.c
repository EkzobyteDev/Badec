#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "arithmetic_codes.h"


// cumulative[i] = sum of frequencies of bytes 0..i-1, so byte b owns [cumulative[b], cumulative[b + 1])
static uint32_t build_cumulative_table(uint8_t* frequencies_table, uint32_t* cumulative) {
        cumulative[0] = 0;
        for (size_t i = 0; i < ARITHMETIC_ALPHABET_LENGTH; i++) {
                cumulative[i + 1] = cumulative[i] + frequencies_table[i];
        }
        return cumulative[ARITHMETIC_ALPHABET_LENGTH];
}

static void put_bit(bit_stream* stream, int bit) {
        stream->byte = (stream->byte << 1) | bit;
        stream->bits_count++;
        if (stream->bits_count == 8) {
                fputc(stream->byte, stream->file);
                stream->byte = 0;
                stream->bits_count = 0;
        }
}

// Writes bit and then all pending bits, which are always opposite to it
static void put_bit_with_pending(bit_stream* stream, int bit, uint64_t* pending_bits) {
        put_bit(stream, bit);
        for (; *pending_bits > 0; (*pending_bits)--) {
                put_bit(stream, !bit);
        }
}

static int get_bit(bit_stream* stream) {
        if (stream->bits_count == 0) {
                int byte = fgetc(stream->file);
                stream->byte = (byte == EOF) ? 0 : (uint8_t)byte;
                stream->bits_count = 8;
        }
        stream->bits_count--;
        return (stream->byte >> stream->bits_count) & 1;
}

void arithmetic_encode_file(FILE* file, off_t file_size, uint8_t* frequencies_table, FILE* archive) {
        uint32_t cumulative[ARITHMETIC_ALPHABET_LENGTH + 1];
        uint32_t total = build_cumulative_table(frequencies_table, cumulative);

        bit_stream stream = { archive, 0, 0 };
        uint32_t low = 0;
        uint32_t high = ARITHMETIC_TOP_VALUE;
        uint64_t pending_bits = 0;

        for (off_t i = 0; i < file_size; i++) {
                uint8_t byte;
                fread(&byte, sizeof(uint8_t), 1, file);
                // A byte missing from the frequency table has an empty interval and cannot be encoded
                if (frequencies_table[byte] == 0) continue;

                // Narrow [low, high] to the byte's share of it
                uint64_t range = (uint64_t)high - low + 1;
                high = low + (uint32_t)(range * cumulative[byte + 1] / total - 1);
                low  = low + (uint32_t)(range * cumulative[byte] / total);

                // Output the bits that are already known and stretch the interval back
                for (;;) {
                        if (high < ARITHMETIC_HALF) {
                                put_bit_with_pending(&stream, 0, &pending_bits);
                        }
                        else if (low >= ARITHMETIC_HALF) {
                                put_bit_with_pending(&stream, 1, &pending_bits);
                                low  -= ARITHMETIC_HALF;
                                high -= ARITHMETIC_HALF;
                        }
                        else if (low >= ARITHMETIC_FIRST_QUARTER && high < ARITHMETIC_THIRD_QUARTER) {
                                // Interval straddles the middle: the next bit is unknown yet,
                                // but the bit after it will be the opposite one
                                pending_bits++;
                                low  -= ARITHMETIC_FIRST_QUARTER;
                                high -= ARITHMETIC_FIRST_QUARTER;
                        }
                        else {
                                break;
                        }
                        low  = low << 1;
                        high = (high << 1) | 1;
                }
        }

        // Two final bits put the number inside [low, high]
        pending_bits++;
        put_bit_with_pending(&stream, low < ARITHMETIC_FIRST_QUARTER ? 0 : 1, &pending_bits);

        // The decoder reads CODE_BITS bits ahead, the encoder has written 2 of them:
        // pad so both use the same number of bytes and the next file in the archive starts right after
        for (int i = 0; i < ARITHMETIC_CODE_BITS - 2; i++) {
                put_bit(&stream, 0);
        }
        while (stream.bits_count != 0) {
                put_bit(&stream, 0);
        }
}

void arithmetic_decode_file(FILE* archive, off_t file_size, uint8_t* frequencies_table, FILE* file) {
        uint32_t cumulative[ARITHMETIC_ALPHABET_LENGTH + 1];
        uint32_t total = build_cumulative_table(frequencies_table, cumulative);

        bit_stream stream = { archive, 0, 0 };
        uint32_t low = 0;
        uint32_t high = ARITHMETIC_TOP_VALUE;
        uint32_t value = 0;
        for (int i = 0; i < ARITHMETIC_CODE_BITS; i++) {
                value = (value << 1) | get_bit(&stream);
        }
        // Empty frequency table: the encoder could not encode any byte
        if (total == 0) {
                return;
        }

        for (off_t i = 0; i < file_size; i++) {
                uint64_t range = (uint64_t)high - low + 1;

                // Which byte's share does value fall into
                uint32_t scaled = (uint32_t)((((uint64_t)value - low + 1) * total - 1) / range);
                size_t byte = 0;
                while (byte < ARITHMETIC_ALPHABET_LENGTH - 1 && cumulative[byte + 1] <= scaled) {
                        byte++;
                }
                fputc((int)byte, file);

                // Same narrowing and stretching as in the encoder
                high = low + (uint32_t)(range * cumulative[byte + 1] / total - 1);
                low  = low + (uint32_t)(range * cumulative[byte] / total);

                for (;;) {
                        if (high < ARITHMETIC_HALF) {
                                // nothing to subtract
                        }
                        else if (low >= ARITHMETIC_HALF) {
                                value -= ARITHMETIC_HALF;
                                low   -= ARITHMETIC_HALF;
                                high  -= ARITHMETIC_HALF;
                        }
                        else if (low >= ARITHMETIC_FIRST_QUARTER && high < ARITHMETIC_THIRD_QUARTER) {
                                value -= ARITHMETIC_FIRST_QUARTER;
                                low   -= ARITHMETIC_FIRST_QUARTER;
                                high  -= ARITHMETIC_FIRST_QUARTER;
                        }
                        else {
                                break;
                        }
                        low   = low << 1;
                        high  = (high << 1) | 1;
                        value = (value << 1) | get_bit(&stream);
                }
        }
}
