#ifndef LZW_H
#define LZW_H

#include "common.h"

/*
 * Compress a file using LZW.
 */
int compress_lzw(
    const char *input_file,
    const char *output_file
);

/*
 * Decompress an LZW file.
 */
int decompress_lzw(
    const char *input_file,
    const char *output_file
);

#endif