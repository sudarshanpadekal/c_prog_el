#ifndef RLE_H
#define RLE_H

#include "common.h"

/*
 * RLE compression and decompression
 *
 * Full implementation will be added in Phase 2.
 */

int compress_rle(
    const char *input_file,
    const char *output_file
);

int decompress_rle(
    const char *input_file,
    const char *output_file
);

#endif