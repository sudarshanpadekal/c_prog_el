#include "../include/rle.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/*
 * RLE file format:
 *
 * [count - uint32_t][character - 1 byte]
 *
 * Example:
 *
 * AAAAAABBCCC
 *
 * becomes:
 *
 * [6][A][2][B][3][C]
 *
 * A uint32_t is used for the count so that
 * long runs are supported.
 */


/* ============================================================
   COMPRESSION
   ============================================================ */

int compress_rle(
    const char *input_file,
    const char *output_file
)
{
    FILE *in = fopen(input_file, "rb");

    if (!in)
    {
        fprintf(
            stderr,
            "Unable to open input file.\n"
        );

        return -1;
    }

    FILE *out = fopen(output_file, "wb");

    if (!out)
    {
        fclose(in);

        fprintf(
            stderr,
            "Unable to create output file.\n"
        );

        return -1;
    }

    int current = fgetc(in);

    /*
     * Empty file.
     */
    if (current == EOF)
    {
        fclose(in);
        fclose(out);

        return 0;
    }

    uint32_t count = 1;

    int next;

    while ((next = fgetc(in)) != EOF)
    {
        if (next == current)
        {
            count++;
        }
        else
        {
            /*
             * Write the completed run.
             */
            fwrite(
                &count,
                sizeof(uint32_t),
                1,
                out
            );

            unsigned char character =
                (unsigned char)current;

            fwrite(
                &character,
                sizeof(unsigned char),
                1,
                out
            );

            /*
             * Start a new run.
             */
            current = next;
            count = 1;
        }
    }

    /*
     * Write final run.
     */
    fwrite(
        &count,
        sizeof(uint32_t),
        1,
        out
    );

    unsigned char character =
        (unsigned char)current;

    fwrite(
        &character,
        sizeof(unsigned char),
        1,
        out
    );

    fclose(in);
    fclose(out);

    return 0;
}


/* ============================================================
   DECOMPRESSION
   ============================================================ */

int decompress_rle(
    const char *input_file,
    const char *output_file
)
{
    FILE *in = fopen(input_file, "rb");

    if (!in)
    {
        fprintf(
            stderr,
            "Unable to open RLE file.\n"
        );

        return -1;
    }

    FILE *out = fopen(output_file, "wb");

    if (!out)
    {
        fclose(in);

        fprintf(
            stderr,
            "Unable to create output file.\n"
        );

        return -1;
    }

    uint32_t count;

    unsigned char character;

    while (
        fread(
            &count,
            sizeof(uint32_t),
            1,
            in
        ) == 1
    )
    {
        /*
         * Every count must have
         * a corresponding character.
         */
        if (
            fread(
                &character,
                sizeof(unsigned char),
                1,
                in
            ) != 1
        )
        {
            fprintf(
                stderr,
                "Invalid RLE file.\n"
            );

            fclose(in);
            fclose(out);

            return -1;
        }

        /*
         * Expand the run.
         */
        for (
            uint32_t i = 0;
            i < count;
            i++
        )
        {
            fputc(
                character,
                out
            );
        }
    }

    fclose(in);
    fclose(out);

    return 0;
}