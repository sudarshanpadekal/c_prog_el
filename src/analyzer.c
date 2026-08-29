#include "../include/analyzer.h"
#include <math.h>

double calculate_entropy(
    const unsigned char *data,
    size_t length
)
{
    if (data == NULL || length == 0)
        return 0.0;

    size_t frequency[MAX_SYMBOLS] = {0};

    for (size_t i = 0; i < length; i++)
    {
        frequency[data[i]]++;
    }

    double entropy = 0.0;

    for (int i = 0; i < MAX_SYMBOLS; i++)
    {
        if (frequency[i] == 0)
            continue;

        double probability =
            (double)frequency[i] / (double)length;

        entropy -=
            probability * (log(probability) / log(2.0));
    }

    return entropy;
}

AnalysisResult analyze_file(
    const unsigned char *data,
    size_t length
)
{
    AnalysisResult result = {0};

    if (data == NULL || length == 0)
        return result;

    size_t frequency[MAX_SYMBOLS] = {0};

    for (size_t i = 0; i < length; i++)
    {
        frequency[data[i]]++;
    }

    size_t unique = 0;

    for (int i = 0; i < MAX_SYMBOLS; i++)
    {
        if (frequency[i] > 0)
            unique++;
    }

    result.entropy = calculate_entropy(data, length);
    result.char_count = length;
    result.unique_chars = unique;

    /*
     * Maximum entropy for the observed alphabet.
     */
    double max_entropy = 0.0;

    if (unique > 1)
        max_entropy = log((double)unique) / log(2.0);

    if (max_entropy > 0.0)
    {
        result.redundancy =
            1.0 - (result.entropy / max_entropy);
    }
    else
    {
        result.redundancy = 1.0;
    }

    size_t consecutive_duplicates = 0;

    for (size_t i = 1; i < length; i++)
    {
        if (data[i] == data[i - 1])
            consecutive_duplicates++;
    }

    result.consecutive_ratio =
        (double)consecutive_duplicates / (double)length;

    /*
     * Recommendation logic will be refined
     * alongside the final analyzer implementation.
     */
    if (result.consecutive_ratio > 0.40)
    {
        result.recommendation = 1;   /* RLE */
        result.confidence = 90;
    }
    else if (result.redundancy > 0.30)
    {
        result.recommendation = 2;   /* Huffman */
        result.confidence = 85;
    }
    else
    {
        result.recommendation = 3;   /* LZW */
        result.confidence = 80;
    }

    return result;
}