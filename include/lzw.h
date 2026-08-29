#ifndef LZW_H
#define LZW_H

#include "common.h"

/*
 * LZW compression and decompression
 *
 * Full implementation will be added in Phase 3.
 */

int compress_lzw(
    const char *input_file,
    const char *output_file
);

int decompress_lzw(
    const char *input_file,
    const char *output_file
);

#endif