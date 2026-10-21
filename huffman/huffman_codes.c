#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "huffman_codes.h"


int cmp_huffman_tree_nodes(const void* a, const void* b) {
        huffman_tree_node* a_node = (huffman_tree_node*)a;
        huffman_tree_node* b_node = (huffman_tree_node*)b;
        if (a_node->frequency > b_node->frequency) {
                return -1;
        }
        else if (a_node->frequency < b_node->frequency) {
                return 1;
        }
        return 0;
        //return (uint16_t)a_node->value - b_node->value;
}

void convert_tree_to_codes(huffman_code current_code, huffman_tree_node* current_node, uint64_t alphabet_length, huffman_code* codes) {
        if (current_node->node_on_0_side == NULL && current_node->node_on_1_side == NULL) {
                codes[current_node->value] = current_code;
                return;
        }

        huffman_code path_0 = current_code;
        path_0.code <<= 1;
        path_0.length++;
        convert_tree_to_codes(path_0, current_node->node_on_0_side, alphabet_length, codes);

        huffman_code path_1 = current_code;
        path_1.code <<= 1;
        path_1.code |= 1;
        path_1.length++;
        convert_tree_to_codes(path_1, current_node->node_on_1_side, alphabet_length, codes);
}

uint8_t get_byte_from_code(uint64_t alphabet_length, huffman_code code, huffman_code* codes, bool* success) {
        for (uint64_t i = 0; i < alphabet_length; i++) {
                if (codes[i].code == code.code && codes[i].length == code.length) {
                        *success = true;
                        //printf("%3d ", i);
                        return i;
                }
        }
        *success = false;
}

void get_huffman_codes(uint64_t alphabet_length, uint8_t* frequencies_table, huffman_code* codes) {      
        huffman_tree_node sorted_frequencies_table[alphabet_length];
        for (size_t i = 0; i < alphabet_length; i++) {
                sorted_frequencies_table[i].frequency = frequencies_table[i];
                sorted_frequencies_table[i].value = i;
                
                sorted_frequencies_table[i].node_on_0_side = NULL;
                sorted_frequencies_table[i].node_on_1_side = NULL;
        } 
        
        uint16_t elements_left = alphabet_length;
        qsort(sorted_frequencies_table, elements_left, sizeof(huffman_tree_node), cmp_huffman_tree_nodes);
        for (int i = 0; i < 256; i++) {
                //printf("%d: %d\n", sorted_frequencies_table[i].value, sorted_frequencies_table[i].frequency);
        }
        
        // Finding out count of elements with non-zero frequency
        for (elements_left = 0; elements_left < alphabet_length && sorted_frequencies_table[elements_left].frequency != 0; elements_left++);
        
        huffman_tree_node temp;
        while (elements_left != 1) {
                huffman_tree_node* a_copy = (huffman_tree_node*)alloca(sizeof(huffman_tree_node));
                huffman_tree_node* b_copy = (huffman_tree_node*)alloca(sizeof(huffman_tree_node));

                *a_copy = sorted_frequencies_table[elements_left - 2];
                *b_copy = sorted_frequencies_table[elements_left - 1];

                sorted_frequencies_table[elements_left - 2].node_on_0_side = a_copy;
                sorted_frequencies_table[elements_left - 2].node_on_1_side = b_copy;
                sorted_frequencies_table[elements_left - 2].frequency      = a_copy->frequency + b_copy->frequency;

                elements_left--;
                qsort(sorted_frequencies_table, elements_left, sizeof(huffman_tree_node), cmp_huffman_tree_nodes); 
        }

        convert_tree_to_codes(empty_huffman_code, &sorted_frequencies_table[0], alphabet_length, codes);
}
