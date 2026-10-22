#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "shannon_codes.h"


// Descending frequency, equal frequencies — ascending byte value
int cmp_shannon_symbols(const void* a, const void* b) {
        const shannon_symbol* a_symbol = (const shannon_symbol*)a;
        const shannon_symbol* b_symbol = (const shannon_symbol*)b;
        if (a_symbol->frequency > b_symbol->frequency) {
                return -1;
        }
        else if (a_symbol->frequency < b_symbol->frequency) {
                return 1;
        }
        return (int)a_symbol->value - (int)b_symbol->value;
}

uint8_t get_byte_from_code(uint64_t alphabet_length, huffman_code code, huffman_code* codes, bool* success) {
        for (uint64_t i = 0; i < alphabet_length; i++) {
                if (codes[i].code == code.code && codes[i].length == code.length) {
                        *success = true;
                        return i;
                }
        }
        *success = false;
        return 0;
}

// Shannon code of symbol a with frequency f(a), total = sum of all frequencies:
//   length(a) = ceil(log2(total / f(a)))
//   code(a)   = first length(a) bits of the binary fraction P(a) = cum(a) / total,
//               where cum(a) is the sum of frequencies of symbols before a in sorted order.
// Everything is computed in integers: code(a) = floor(cum(a) * 2^length(a) / total).
void get_huffman_codes(uint64_t alphabet_length, uint8_t* frequencies_table, huffman_code* codes) {
        shannon_symbol sorted_symbols[alphabet_length];
        size_t symbols_count = 0;
        uint64_t total = 0;

        for (size_t i = 0; i < alphabet_length; i++) {
                codes[i] = empty_huffman_code;
                if (frequencies_table[i] == 0) continue;

                sorted_symbols[symbols_count].value     = i;
                sorted_symbols[symbols_count].frequency = frequencies_table[i];
                symbols_count++;
                total += frequencies_table[i];
        }
        qsort(sorted_symbols, symbols_count, sizeof(shannon_symbol), cmp_shannon_symbols);

        uint64_t cumulative = 0;
        for (size_t i = 0; i < symbols_count; i++) {
                uint64_t frequency = sorted_symbols[i].frequency;

                // Smallest length with 2^length >= total / frequency
                uint8_t length = 0;
                while (((uint64_t)1 << length) * frequency < total) {
                        length++;
                }
                // A file of one repeated byte: p = 1 gives length 0, but at least 1 bit is needed
                if (length == 0) {
                        length = 1;
                }

                codes[sorted_symbols[i].value].code   = (cumulative << length) / total;
                codes[sorted_symbols[i].value].length = length;
                cumulative += frequency;
        }
}
