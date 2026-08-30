#ifndef HUFFMAN_H
#define HUFFMAN_H

#include "common.h"

/*
 * Huffman tree node.
 */
typedef struct HuffmanNode
{
    unsigned char symbol;
    uint64_t frequency;

    struct HuffmanNode *left;
    struct HuffmanNode *right;

} HuffmanNode;


/*
 * Compress a file using Huffman coding.
 *
 * input_file  -> original text file
 * output_file -> compressed .huff file
 *
 * Returns:
 *   0  success
 *  -1  failure
 */
int compress_huffman(
    const char *input_file,
    const char *output_file
);


/*
 * Decompress a Huffman file.
 *
 * input_file  -> .huff file
 * output_file -> reconstructed file
 *
 * Returns:
 *   0  success
 *  -1  failure
 */
int decompress_huffman(
    const char *input_file,
    const char *output_file
);


/*
 * Free an entire Huffman tree.
 */
void free_huffman_tree(HuffmanNode *root);


/*
 * Print the Huffman tree.
 * Useful during development/debugging.
 */
void print_huffman_tree(
    HuffmanNode *root,
    char *code,
    int depth
);

#endif