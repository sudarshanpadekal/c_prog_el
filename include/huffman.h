#ifndef HUFFMAN_H
#define HUFFMAN_H

#include "common.h"

/*
 * Huffman compression and decompression
 *
 * Full implementation will be added in Phase 1.
 */

int compress_huffman(
    const char *input_file,
    const char *output_file
);

int decompress_huffman(
    const char *input_file,
    const char *output_file
);

#endif