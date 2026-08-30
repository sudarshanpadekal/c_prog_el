#ifndef RLE_H
#define RLE_H

#include "common.h"

/*
 * Compress a file using Run-Length Encoding.
 */
int compress_rle(
    const char *input_file,
    const char *output_file
);

/*
 * Decompress an RLE file.
 */
int decompress_rle(
    const char *input_file,
    const char *output_file
);

#endif