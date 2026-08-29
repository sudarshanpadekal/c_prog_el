#include "../include/common.h"
#include "../include/huffman.h"
#include "../include/rle.h"
#include "../include/lzw.h"
#include "../include/analyzer.h"

static const char *recommendation_name(int recommendation)
{
    switch (recommendation)
    {
        case 1:
            return "RLE";

        case 2:
            return "Huffman";

        case 3:
            return "LZW";

        default:
            return "None";
    }
}

int main(void)
{
    printf("\n");
    printf("============================================\n");
    printf("       INTELLICOMPRESS C BACKEND\n");
    printf("============================================\n");

    const char *test_text =
        "AAAAAABBBBBBBBBCCCCCCCCCCCCDDDDDD";

    size_t length = strlen(test_text);

    printf("\nInput:\n");
    printf("%s\n", test_text);

    printf("\nInput size: %zu bytes\n", length);

    AnalysisResult analysis =
        analyze_file(
            (const unsigned char *)test_text,
            length
        );

    printf("\n--- Analysis ---\n");

    printf("Entropy:             %.4f bits/symbol\n",
           analysis.entropy);

    printf("Redundancy:          %.2f%%\n",
           analysis.redundancy * 100.0);

    printf("Consecutive ratio:   %.2f%%\n",
           analysis.consecutive_ratio * 100.0);

    printf("Unique symbols:      %zu\n",
           analysis.unique_chars);

    printf("Recommendation:      %s\n",
           recommendation_name(analysis.recommendation));

    printf("Confidence:          %d%%\n",
           analysis.confidence);

    printf("\n--- Modules ---\n");

    printf("Huffman: initialized\n");
    printf("RLE:     initialized\n");
    printf("LZW:     initialized\n");
    printf("Analyzer: initialized\n");

    printf("\n============================================\n");
    printf("       BACKEND INITIALIZATION COMPLETE\n");
    printf("============================================\n\n");

    return 0;
}