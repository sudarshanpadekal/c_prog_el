// #include "../include/huffman.h"
// #include "../include/rle.h"
// #include "../include/lzw.h"

// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>


// void print_usage(void)
// {
//     printf("\n");
//     printf("============================================\n");
//     printf("       INTELLICOMPRESS PLATFORM\n");
//     printf("============================================\n");

//     printf("\nUsage:\n");

//     printf("  Compress:\n");
//     printf("    intellicompress compress <algorithm> <input> <output>\n");

//     printf("\n  Decompress:\n");
//     printf("    intellicompress decompress <algorithm> <input> <output>\n");

//     printf("\nAlgorithms:\n");
//     printf("    huffman\n");
//     printf("    rle\n");
//     printf("    lzw\n");

//     printf("\nExamples:\n");

//     printf("    intellicompress compress huffman sample.txt sample.huff\n");
//     printf("    intellicompress compress rle sample.txt sample.rle\n");
//     printf("    intellicompress compress lzw sample.txt sample.lzw\n");

//     printf("\n    intellicompress decompress huffman sample.huff restored.txt\n");
//     printf("    intellicompress decompress rle sample.rle restored.txt\n");
//     printf("    intellicompress decompress lzw sample.lzw restored.txt\n");

//     printf("\n");
// }


// int compress_file(
//     const char *algorithm,
//     const char *input,
//     const char *output
// )
// {
//     if (strcmp(algorithm, "huffman") == 0)
//     {
//         return compress_huffman(input, output);
//     }

//     if (strcmp(algorithm, "rle") == 0)
//     {
//         return compress_rle(input, output);
//     }

//     if (strcmp(algorithm, "lzw") == 0)
//     {
//         return compress_lzw(input, output);
//     }

//     fprintf(
//         stderr,
//         "Unknown compression algorithm: %s\n",
//         algorithm
//     );

//     return -1;
// }


// int decompress_file(
//     const char *algorithm,
//     const char *input,
//     const char *output
// )
// {
//     if (strcmp(algorithm, "huffman") == 0)
//     {
//         return decompress_huffman(input, output);
//     }

//     if (strcmp(algorithm, "rle") == 0)
//     {
//         return decompress_rle(input, output);
//     }

//     if (strcmp(algorithm, "lzw") == 0)
//     {
//         return decompress_lzw(input, output);
//     }

//     fprintf(
//         stderr,
//         "Unknown decompression algorithm: %s\n",
//         algorithm
//     );

//     return -1;
// }


// int main(int argc, char *argv[])
// {
//     /*
//      * We need:
//      *
//      * argv[0] = program
//      * argv[1] = operation
//      * argv[2] = algorithm
//      * argv[3] = input
//      * argv[4] = output
//      */

//     if (argc != 5)
//     {
//         print_usage();
//         return EXIT_FAILURE;
//     }

//     const char *operation = argv[1];
//     const char *algorithm = argv[2];
//     const char *input = argv[3];
//     const char *output = argv[4];

//     int result;

//     printf("\n");
//     printf("Intellicompress\n");
//     printf("------------------------------\n");
//     printf("Operation : %s\n", operation);
//     printf("Algorithm : %s\n", algorithm);
//     printf("Input     : %s\n", input);
//     printf("Output    : %s\n", output);
//     printf("------------------------------\n");

//     if (strcmp(operation, "compress") == 0)
//     {
//         printf("\nCompressing...\n");

//         result = compress_file(
//             algorithm,
//             input,
//             output
//         );

//         if (result == 0)
//         {
//             printf(
//                 "Compression successful!\n"
//             );
//         }
//         else
//         {
//             printf(
//                 "Compression failed.\n"
//             );
//         }
//     }
//     else if (strcmp(operation, "decompress") == 0)
//     {
//         printf("\nDecompressing...\n");

//         result = decompress_file(
//             algorithm,
//             input,
//             output
//         );

//         if (result == 0)
//         {
//             printf(
//                 "Decompression successful!\n"
//             );
//         }
//         else
//         {
//             printf(
//                 "Decompression failed.\n"
//             );
//         }
//     }
//     else
//     {
//         fprintf(
//             stderr,
//             "Unknown operation: %s\n",
//             operation
//         );

//         print_usage();

//         return EXIT_FAILURE;
//     }

//     return result == 0
//         ? EXIT_SUCCESS
//         : EXIT_FAILURE;
// }

#include "../include/server.h"

int main(void)
{
    return start_http_server(8080);
}