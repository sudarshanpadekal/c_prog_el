#include "../include/huffman.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================
   MIN HEAP
   ============================================================ */

typedef struct
{
    HuffmanNode **nodes;
    int size;
    int capacity;

} MinHeap;


static MinHeap *create_heap(int capacity)
{
    MinHeap *heap = malloc(sizeof(MinHeap));

    if (!heap)
        return NULL;

    heap->nodes = malloc(
        sizeof(HuffmanNode *) * capacity
    );

    if (!heap->nodes)
    {
        free(heap);
        return NULL;
    }

    heap->size = 0;
    heap->capacity = capacity;

    return heap;
}


static void free_heap(MinHeap *heap)
{
    if (!heap)
        return;

    free(heap->nodes);
    free(heap);
}


static void swap_nodes(
    HuffmanNode **a,
    HuffmanNode **b
)
{
    HuffmanNode *temp = *a;
    *a = *b;
    *b = temp;
}


static void heapify_up(MinHeap *heap, int index)
{
    while (index > 0)
    {
        int parent = (index - 1) / 2;

        if (
            heap->nodes[parent]->frequency <=
            heap->nodes[index]->frequency
        )
        {
            break;
        }

        swap_nodes(
            &heap->nodes[parent],
            &heap->nodes[index]
        );

        index = parent;
    }
}


static void heapify_down(MinHeap *heap, int index)
{
    while (1)
    {
        int smallest = index;

        int left = 2 * index + 1;
        int right = 2 * index + 2;

        if (
            left < heap->size &&
            heap->nodes[left]->frequency <
            heap->nodes[smallest]->frequency
        )
        {
            smallest = left;
        }

        if (
            right < heap->size &&
            heap->nodes[right]->frequency <
            heap->nodes[smallest]->frequency
        )
        {
            smallest = right;
        }

        if (smallest == index)
            break;

        swap_nodes(
            &heap->nodes[index],
            &heap->nodes[smallest]
        );

        index = smallest;
    }
}


static int heap_push(
    MinHeap *heap,
    HuffmanNode *node
)
{
    if (heap->size >= heap->capacity)
        return -1;

    heap->nodes[heap->size] = node;

    heapify_up(
        heap,
        heap->size
    );

    heap->size++;

    return 0;
}


static HuffmanNode *heap_pop(MinHeap *heap)
{
    if (!heap || heap->size == 0)
        return NULL;

    HuffmanNode *result = heap->nodes[0];

    heap->size--;

    if (heap->size > 0)
    {
        heap->nodes[0] =
            heap->nodes[heap->size];

        heapify_down(heap, 0);
    }

    return result;
}


/* ============================================================
   NODE CREATION
   ============================================================ */

static HuffmanNode *create_node(
    unsigned char symbol,
    uint64_t frequency
)
{
    HuffmanNode *node =
        malloc(sizeof(HuffmanNode));

    if (!node)
        return NULL;

    node->symbol = symbol;
    node->frequency = frequency;

    node->left = NULL;
    node->right = NULL;

    return node;
}


/* ============================================================
   FREQUENCY TABLE
   ============================================================ */

static int build_frequency_table(
    const unsigned char *data,
    size_t size,
    uint64_t frequencies[MAX_SYMBOLS]
)
{
    if (!data || !frequencies)
        return -1;

    for (int i = 0; i < MAX_SYMBOLS; i++)
        frequencies[i] = 0;

    for (size_t i = 0; i < size; i++)
    {
        frequencies[data[i]]++;
    }

    return 0;
}


/* ============================================================
   BUILD HUFFMAN TREE
   ============================================================ */

static HuffmanNode *build_huffman_tree(
    uint64_t frequencies[MAX_SYMBOLS]
)
{
    MinHeap *heap =
        create_heap(MAX_SYMBOLS);

    if (!heap)
        return NULL;

    /*
     * Create a leaf for every symbol
     * appearing in the input.
     */
    for (int i = 0; i < MAX_SYMBOLS; i++)
    {
        if (frequencies[i] == 0)
            continue;

        HuffmanNode *node =
            create_node(
                (unsigned char)i,
                frequencies[i]
            );

        if (!node ||
            heap_push(heap, node) != 0)
        {
            free(node);
            free_heap(heap);
            return NULL;
        }
    }

    /*
     * Empty input.
     */
    if (heap->size == 0)
    {
        free_heap(heap);
        return NULL;
    }

    /*
     * Special case:
     * only one unique character.
     */
    if (heap->size == 1)
    {
        HuffmanNode *only =
            heap_pop(heap);

        HuffmanNode *root =
            create_node(0, only->frequency);

        if (!root)
        {
            free(only);
            free_heap(heap);
            return NULL;
        }

        root->left = only;

        free_heap(heap);

        return root;
    }

    /*
     * Standard Huffman tree construction.
     */
    while (heap->size > 1)
    {
        HuffmanNode *left =
            heap_pop(heap);

        HuffmanNode *right =
            heap_pop(heap);

        HuffmanNode *parent =
            create_node(
                0,
                left->frequency +
                right->frequency
            );

        if (!parent)
        {
            free_huffman_tree(left);
            free_huffman_tree(right);
            free_heap(heap);
            return NULL;
        }

        parent->left = left;
        parent->right = right;

        if (heap_push(heap, parent) != 0)
        {
            free_huffman_tree(parent);
            free_heap(heap);
            return NULL;
        }
    }

    HuffmanNode *root =
        heap_pop(heap);

    free_heap(heap);

    return root;
}


/* ============================================================
   CODE TABLE
   ============================================================ */

static void generate_codes_recursive(
    HuffmanNode *node,
    char *code,
    int depth,
    char *codes[MAX_SYMBOLS]
)
{
    if (!node)
        return;

    /*
     * Leaf node.
     */
    if (
        node->left == NULL &&
        node->right == NULL
    )
    {
        /*
         * Single-character files get code "0".
         */
        if (depth == 0)
        {
            code[0] = '0';
            code[1] = '\0';
        }
        else
        {
            code[depth] = '\0';
        }

        codes[node->symbol] =
            malloc(strlen(code) + 1);

        if (codes[node->symbol])
        {
            strcpy(
                codes[node->symbol],
                code
            );
        }

        return;
    }

    /*
     * Left = 0
     */
    if (node->left)
    {
        code[depth] = '0';

        generate_codes_recursive(
            node->left,
            code,
            depth + 1,
            codes
        );
    }

    /*
     * Right = 1
     */
    if (node->right)
    {
        code[depth] = '1';

        generate_codes_recursive(
            node->right,
            code,
            depth + 1,
            codes
        );
    }
}


static int generate_codes(
    HuffmanNode *root,
    char *codes[MAX_SYMBOLS]
)
{
    char code[MAX_SYMBOLS];

    for (int i = 0; i < MAX_SYMBOLS; i++)
        codes[i] = NULL;

    generate_codes_recursive(
        root,
        code,
        0,
        codes
    );

    return 0;
}


static void free_codes(
    char *codes[MAX_SYMBOLS]
)
{
    for (int i = 0; i < MAX_SYMBOLS; i++)
    {
        free(codes[i]);
        codes[i] = NULL;
    }
}


/* ============================================================
   FILE HELPERS
   ============================================================ */

static unsigned char *read_file(
    const char *filename,
    size_t *size
)
{
    FILE *file =
        fopen(filename, "rb");

    if (!file)
        return NULL;

    fseek(file, 0, SEEK_END);

    long file_size =
        ftell(file);

    rewind(file);

    if (file_size < 0)
    {
        fclose(file);
        return NULL;
    }

    unsigned char *data =
        malloc(
            file_size > 0
                ? (size_t)file_size
                : 1
        );

    if (!data)
    {
        fclose(file);
        return NULL;
    }

    size_t bytes_read =
        fread(
            data,
            1,
            (size_t)file_size,
            file
        );

    fclose(file);

    *size = bytes_read;

    return data;
}


/* ============================================================
   BIT WRITER
   ============================================================ */

typedef struct
{
    FILE *file;

    unsigned char buffer;

    int bit_count;

} BitWriter;


static void bit_writer_init(
    BitWriter *writer,
    FILE *file
)
{
    writer->file = file;
    writer->buffer = 0;
    writer->bit_count = 0;
}


static void write_bit(
    BitWriter *writer,
    int bit
)
{
    writer->buffer <<= 1;

    if (bit)
        writer->buffer |= 1;

    writer->bit_count++;

    if (writer->bit_count == 8)
    {
        fwrite(
            &writer->buffer,
            1,
            1,
            writer->file
        );

        writer->buffer = 0;
        writer->bit_count = 0;
    }
}


static int bit_writer_flush(
    BitWriter *writer
)
{
    if (writer->bit_count == 0)
        return 0;

    int padding =
        8 - writer->bit_count;

    writer->buffer <<=
        padding;

    fwrite(
        &writer->buffer,
        1,
        1,
        writer->file
    );

    writer->buffer = 0;
    writer->bit_count = 0;

    return padding;
}


/* ============================================================
   COMPRESS
   ============================================================ */

int compress_huffman(
    const char *input_file,
    const char *output_file
)
{
    size_t input_size = 0;

    unsigned char *data =
        read_file(
            input_file,
            &input_size
        );

    if (!data && input_size != 0)
    {
        fprintf(
            stderr,
            "Unable to read input file.\n"
        );

        return -1;
    }

    /*
     * Empty file.
     */
    if (input_size == 0)
    {
        FILE *out =
            fopen(output_file, "wb");

        if (!out)
        {
            free(data);
            return -1;
        }

        /*
         * Magic.
         */
        fwrite("ICHP", 1, 4, out);

        /*
         * Original size.
         */
        uint64_t zero = 0;
        fwrite(&zero, sizeof(uint64_t), 1, out);

        /*
         * Number of symbols.
         */
        uint16_t symbols = 0;
        fwrite(&symbols, sizeof(uint16_t), 1, out);

        fclose(out);
        free(data);

        return 0;
    }

    uint64_t frequencies[MAX_SYMBOLS];

    build_frequency_table(
        data,
        input_size,
        frequencies
    );

    HuffmanNode *root =
        build_huffman_tree(frequencies);

    if (!root)
    {
        free(data);
        return -1;
    }

    char *codes[MAX_SYMBOLS];

    generate_codes(
        root,
        codes
    );

    FILE *out =
        fopen(output_file, "wb");

    if (!out)
    {
        free_codes(codes);
        free_huffman_tree(root);
        free(data);

        return -1;
    }

    /*
     * --------------------------------------------------------
     * FILE HEADER
     * --------------------------------------------------------
     *
     * 4 bytes  : Magic "ICHP"
     * 8 bytes  : Original size
     * 2 bytes  : Number of unique symbols
     * 8 bytes  : Frequency for each symbol
     * 1 byte   : Padding
     *
     * --------------------------------------------------------
     */

    fwrite(
        "ICHP",
        1,
        4,
        out
    );

    uint64_t original_size =
        (uint64_t)input_size;

    fwrite(
        &original_size,
        sizeof(uint64_t),
        1,
        out
    );

    uint16_t symbol_count = 0;

    for (int i = 0; i < MAX_SYMBOLS; i++)
    {
        if (frequencies[i] > 0)
            symbol_count++;
    }

    fwrite(
        &symbol_count,
        sizeof(uint16_t),
        1,
        out
    );

    /*
     * Store only symbols that occur.
     *
     * symbol : 1 byte
     * frequency : 8 bytes
     */
    for (int i = 0; i < MAX_SYMBOLS; i++)
    {
        if (frequencies[i] == 0)
            continue;

        unsigned char symbol =
            (unsigned char)i;

        fwrite(
            &symbol,
            1,
            1,
            out
        );

        fwrite(
            &frequencies[i],
            sizeof(uint64_t),
            1,
            out
        );
    }

    /*
     * Reserve one byte for padding.
     */
    long padding_position =
        ftell(out);

    unsigned char padding = 0;

    fwrite(
        &padding,
        1,
        1,
        out
    );

    /*
     * Write compressed bits.
     */
    BitWriter writer;

    bit_writer_init(
        &writer,
        out
    );

    for (size_t i = 0; i < input_size; i++)
    {
        char *code =
            codes[data[i]];

        for (size_t j = 0; j < strlen(code); j++)
        {
            write_bit(
                &writer,
                code[j] == '1'
            );
        }
    }

    padding =
        (unsigned char)
        bit_writer_flush(&writer);

    /*
     * Go back and write actual padding.
     */
    fseek(
        out,
        padding_position,
        SEEK_SET
    );

    fwrite(
        &padding,
        1,
        1,
        out
    );

    fclose(out);

    free_codes(codes);
    free_huffman_tree(root);
    free(data);

    return 0;
}


/* ============================================================
   BIT READER
   ============================================================ */

typedef struct
{
    FILE *file;

    unsigned char buffer;

    int bits_remaining;

} BitReader;


static void bit_reader_init(
    BitReader *reader,
    FILE *file
)
{
    reader->file = file;
    reader->buffer = 0;
    reader->bits_remaining = 0;
}


static int read_bit(
    BitReader *reader
)
{
    if (reader->bits_remaining == 0)
    {
        int result =
            fread(
                &reader->buffer,
                1,
                1,
                reader->file
            );

        if (result != 1)
            return -1;

        reader->bits_remaining = 8;
    }

    int bit =
        (reader->buffer & 0x80)
            ? 1
            : 0;

    reader->buffer <<= 1;

    reader->bits_remaining--;

    return bit;
}


/* ============================================================
   DECOMPRESS
   ============================================================ */

int decompress_huffman(
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
            "Unable to open compressed file.\n"
        );

        return -1;
    }

    /*
     * Read and verify magic.
     */
    char magic[4];

    if (
        fread(
            magic,
            1,
            4,
            in
        ) != 4
    )
    {
        fclose(in);
        return -1;
    }

    if (
        memcmp(
            magic,
            "ICHP",
            4
        ) != 0
    )
    {
        fprintf(
            stderr,
            "Invalid Huffman file.\n"
        );

        fclose(in);

        return -1;
    }

    /*
     * Original size.
     */
    uint64_t original_size;

    if (
        fread(
            &original_size,
            sizeof(uint64_t),
            1,
            in
        ) != 1
    )
    {
        fclose(in);
        return -1;
    }

    /*
     * Number of unique symbols.
     */
    uint16_t symbol_count;

    if (
        fread(
            &symbol_count,
            sizeof(uint16_t),
            1,
            in
        ) != 1
    )
    {
        fclose(in);
        return -1;
    }

    uint64_t frequencies[MAX_SYMBOLS] = {0};

    /*
     * Read frequency table.
     */
    for (
        uint16_t i = 0;
        i < symbol_count;
        i++
    )
    {
        unsigned char symbol;

        uint64_t frequency;

        if (
            fread(
                &symbol,
                1,
                1,
                in
            ) != 1
        )
        {
            fclose(in);
            return -1;
        }

        if (
            fread(
                &frequency,
                sizeof(uint64_t),
                1,
                in
            ) != 1
        )
        {
            fclose(in);
            return -1;
        }

        frequencies[symbol] =
            frequency;
    }

    /*
     * Read padding.
     */
    unsigned char padding;

    if (
        fread(
            &padding,
            1,
            1,
            in
        ) != 1
    )
    {
        fclose(in);
        return -1;
    }

    /*
     * Empty file.
     */
    if (original_size == 0)
    {
        FILE *out =
            fopen(output_file, "wb");

        if (!out)
        {
            fclose(in);
            return -1;
        }

        fclose(out);
        fclose(in);

        return 0;
    }

    /*
     * Rebuild exact same tree.
     */
    HuffmanNode *root =
        build_huffman_tree(frequencies);

    if (!root)
    {
        fclose(in);
        return -1;
    }

    FILE *out =
        fopen(output_file, "wb");

    if (!out)
    {
        free_huffman_tree(root);
        fclose(in);

        return -1;
    }

    /*
     * Single-symbol special case.
     */
    if (
        root->left &&
        root->right == NULL
    )
    {
        for (
            uint64_t i = 0;
            i < original_size;
            i++
        )
        {
            fputc(
                root->left->symbol,
                out
            );
        }

        fclose(out);
        fclose(in);

        free_huffman_tree(root);

        return 0;
    }

    /*
     * Decode bitstream.
     */
    BitReader reader;

    bit_reader_init(
        &reader,
        in
    );

    HuffmanNode *current =
        root;

    uint64_t decoded = 0;

    while (decoded < original_size)
    {
        int bit =
            read_bit(&reader);

        if (bit < 0)
        {
            fprintf(
                stderr,
                "Unexpected end of Huffman data.\n"
            );

            fclose(out);
            fclose(in);

            free_huffman_tree(root);

            return -1;
        }

        if (bit == 0)
            current = current->left;
        else
            current = current->right;

        if (!current)
        {
            fprintf(
                stderr,
                "Invalid Huffman bitstream.\n"
            );

            fclose(out);
            fclose(in);

            free_huffman_tree(root);

            return -1;
        }

        /*
         * Leaf reached.
         */
        if (
            current->left == NULL &&
            current->right == NULL
        )
        {
            fputc(
                current->symbol,
                out
            );

            decoded++;

            current = root;
        }
    }

    fclose(out);
    fclose(in);

    free_huffman_tree(root);

    return 0;
}


/* ============================================================
   TREE CLEANUP
   ============================================================ */

void free_huffman_tree(
    HuffmanNode *root
)
{
    if (!root)
        return;

    free_huffman_tree(
        root->left
    );

    free_huffman_tree(
        root->right
    );

    free(root);
}


/* ============================================================
   TREE VISUALIZATION
   ============================================================ */

void print_huffman_tree(
    HuffmanNode *root,
    char *code,
    int depth
)
{
    if (!root)
        return;

    if (
        root->left == NULL &&
        root->right == NULL
    )
    {
        code[depth] = '\0';

        printf(
            "Symbol: %3u | Frequency: %llu | Code: %s\n",
            root->symbol,
            (unsigned long long)root->frequency,
            code
        );

        return;
    }

    if (root->left)
    {
        code[depth] = '0';

        print_huffman_tree(
            root->left,
            code,
            depth + 1
        );
    }

    if (root->right)
    {
        code[depth] = '1';

        print_huffman_tree(
            root->right,
            code,
            depth + 1
        );
    }
}