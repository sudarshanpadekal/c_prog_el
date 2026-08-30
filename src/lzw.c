#include "../include/lzw.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define LZW_MAX_CODES 65536
#define LZW_HASH_SIZE 131071


/* ============================================================
   LZW DICTIONARY ENTRY
   ============================================================ */

typedef struct
{
    int prefix;
    unsigned char character;
    uint16_t code;

} LZWEntry;


/* ============================================================
   HASH TABLE
   ============================================================ */

typedef struct
{
    int prefix;
    unsigned char character;
    uint16_t code;
    int used;

} LZWHashEntry;


static unsigned long lzw_hash(
    int prefix,
    unsigned char character
)
{
    unsigned long hash =
        (unsigned long)prefix * 257UL
        + character;

    return hash % LZW_HASH_SIZE;
}


/*
 * Find a sequence in the dictionary.
 *
 * Sequence = prefix + character
 */
static int dictionary_find(
    LZWHashEntry *table,
    int prefix,
    unsigned char character
)
{
    unsigned long index =
        lzw_hash(prefix, character);

    while (table[index].used)
    {
        if (
            table[index].prefix == prefix &&
            table[index].character == character
        )
        {
            return table[index].code;
        }

        index++;

        if (index >= LZW_HASH_SIZE)
            index = 0;
    }

    return -1;
}


/*
 * Add a new sequence.
 */
static int dictionary_add(
    LZWHashEntry *table,
    int prefix,
    unsigned char character,
    uint16_t code
)
{
    unsigned long index =
        lzw_hash(prefix, character);

    while (table[index].used)
    {
        index++;

        if (index >= LZW_HASH_SIZE)
            index = 0;
    }

    table[index].used = 1;
    table[index].prefix = prefix;
    table[index].character = character;
    table[index].code = code;

    return 0;
}


/* ============================================================
   WRITE / READ 16-BIT CODES
   ============================================================ */

static int write_code(
    FILE *file,
    uint16_t code
)
{
    return fwrite(
        &code,
        sizeof(uint16_t),
        1,
        file
    ) == 1 ? 0 : -1;
}


static int read_code(
    FILE *file,
    uint16_t *code
)
{
    return fread(
        code,
        sizeof(uint16_t),
        1,
        file
    ) == 1 ? 0 : -1;
}


/* ============================================================
   COMPRESSION
   ============================================================ */

int compress_lzw(
    const char *input_file,
    const char *output_file
)
{
    FILE *in =
        fopen(input_file, "rb");

    if (!in)
    {
        fprintf(
            stderr,
            "Unable to open input file.\n"
        );

        return -1;
    }

    FILE *out =
        fopen(output_file, "wb");

    if (!out)
    {
        fclose(in);

        fprintf(
            stderr,
            "Unable to create output file.\n"
        );

        return -1;
    }

    LZWHashEntry *dictionary =
        calloc(
            LZW_HASH_SIZE,
            sizeof(LZWHashEntry)
        );

    if (!dictionary)
    {
        fclose(in);
        fclose(out);

        return -1;
    }

    /*
     * Codes 0–255 represent
     * the original ASCII/byte characters.
     *
     * New dictionary entries start at 256.
     */
    uint32_t next_code = 256;

    int current =
        fgetc(in);

    /*
     * Empty file.
     */
    if (current == EOF)
    {
        free(dictionary);
        fclose(in);
        fclose(out);

        return 0;
    }

    int prefix = current;

    int next;

    while ((next = fgetc(in)) != EOF)
    {
        unsigned char character =
            (unsigned char)next;

        int existing =
            dictionary_find(
                dictionary,
                prefix,
                character
            );

        if (existing != -1)
        {
            /*
             * prefix + character already exists.
             */
            prefix = existing;
        }
        else
        {
            /*
             * Output the code for prefix.
             */
            if (
                write_code(
                    out,
                    (uint16_t)prefix
                ) != 0
            )
            {
                free(dictionary);
                fclose(in);
                fclose(out);

                return -1;
            }

            /*
             * Add prefix + character.
             */
            // if (next_code < LZW_MAX_CODES)
            // {
            //     dictionary_add(
            //         dictionary,
            //         prefix,
            //         character,
            //         next_code
            //     );

            //     next_code++;
            // }
            if (next_code < LZW_MAX_CODES)
                {
                    dictionary_add(
                    dictionary,
                    prefix,
                    character,
                    (uint16_t)next_code
                    );

                    next_code++;
            }

            /*
             * Start a new sequence.
             */
            prefix = character;
        }
    }

    /*
     * Output final sequence.
     */
    if (
        write_code(
            out,
            (uint16_t)prefix
        ) != 0
    )
    {
        free(dictionary);
        fclose(in);
        fclose(out);

        return -1;
    }

    free(dictionary);

    fclose(in);
    fclose(out);

    return 0;
}


/* ============================================================
   DECOMPRESSION
   ============================================================ */

int decompress_lzw(
    const char *input_file,
    const char *output_file
)
{
    FILE *in =
        fopen(input_file, "rb");

    if (!in)
    {
        fprintf(
            stderr,
            "Unable to open LZW file.\n"
        );

        return -1;
    }

    FILE *out =
        fopen(output_file, "wb");

    if (!out)
    {
        fclose(in);

        return -1;
    }

    /*
     * Dictionary entries.
     *
     * prefix points to an earlier code.
     */
    LZWEntry *dictionary =
        malloc(
            sizeof(LZWEntry) *
            LZW_MAX_CODES
        );

    if (!dictionary)
    {
        fclose(in);
        fclose(out);

        return -1;
    }

    uint32_t next_code = 256;

    uint16_t previous_code;

    /*
     * Read first code.
     */
    if (
        read_code(
            in,
            &previous_code
        ) != 0
    )
    {
        /*
         * Empty compressed file.
         */
        free(dictionary);
        fclose(in);
        fclose(out);

        return 0;
    }

    /*
     * First code must represent
     * an original byte.
     */
    if (previous_code >= 256)
    {
        fprintf(
            stderr,
            "Invalid LZW file.\n"
        );

        free(dictionary);
        fclose(in);
        fclose(out);

        return -1;
    }

    fputc(
        (unsigned char)previous_code,
        out
    );

    uint16_t current_code;

    while (
        read_code(
            in,
            &current_code
        ) == 0
    )
    {
        unsigned char *stack =
            malloc(LZW_MAX_CODES);

        if (!stack)
        {
            free(dictionary);
            fclose(in);
            fclose(out);

            return -1;
        }

        int top = 0;

        uint16_t code =
            current_code;

        unsigned char first_character;

        /*
         * Normal case:
         * code already exists.
         */
        if (
            code < next_code &&
            code < 256
        )
        {
            /*
             * Direct character.
             */
            first_character =
                (unsigned char)code;

            stack[top++] =
                first_character;
        }
        else if (
            code >= 256 &&
            code < next_code
        )
        {
            /*
             * Follow dictionary links.
             */
            uint16_t temp =
                code;

            while (temp >= 256)
            {
                if (top >= LZW_MAX_CODES)
                {
                    free(stack);
                    free(dictionary);
                    fclose(in);
                    fclose(out);

                    return -1;
                }

                stack[top++] =
                    dictionary[temp].character;

                temp =
                    dictionary[temp].prefix;
            }

            first_character =
                (unsigned char)temp;

            stack[top++] =
                first_character;
        }
        else if (code == next_code)
        {
            /*
             * Special LZW case:
             *
             * code = previous sequence
             *        + first character
             *        of previous sequence
             */
            uint16_t temp =
                previous_code;

            while (temp >= 256)
            {
                stack[top++] =
                    dictionary[temp].character;

                temp =
                    dictionary[temp].prefix;
            }

            first_character =
                (unsigned char)temp;

            stack[top++] =
                first_character;
        }
        else
        {
            fprintf(
                stderr,
                "Invalid LZW code.\n"
            );

            free(stack);
            free(dictionary);
            fclose(in);
            fclose(out);

            return -1;
        }

        /*
         * Write sequence in reverse.
         */
        for (int i = top - 1; i >= 0; i--)
        {
            fputc(
                stack[i],
                out
            );
        }

        /*
         * Add:
         *
         * previous sequence
         * +
         * first character of current sequence
         */
        if (next_code < LZW_MAX_CODES)
        {
            dictionary[next_code].prefix =
                previous_code;

            dictionary[next_code].character =
                first_character;

            dictionary[next_code].code =
                next_code;

            next_code++;
        }

        previous_code =
            current_code;

        free(stack);
    }

    free(dictionary);

    fclose(in);
    fclose(out);

    return 0;
}