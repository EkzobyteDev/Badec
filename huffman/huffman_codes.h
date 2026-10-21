#pragma once
#include <stdint.h>
#include <stddef.h>

#define empty_huffman_code (huffman_code){0}


typedef struct huffman_tree_node{
        uint8_t frequency;
        uint8_t value;
        struct huffman_tree_node* node_on_0_side;
        struct huffman_tree_node* node_on_1_side;
}huffman_tree_node;



typedef struct {
        uint64_t code;
        uint8_t  length;
}huffman_code;

void get_huffman_codes(uint64_t alphabet_length, uint8_t* frequencies_table, huffman_code* codes);
uint8_t get_byte_from_code(uint64_t alphabet_length, huffman_code code, huffman_code* codes, bool* success);