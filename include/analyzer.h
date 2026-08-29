#ifndef ANALYZER_H
#define ANALYZER_H

#include "common.h"

typedef struct
{
    double entropy;
    double redundancy;
    double consecutive_ratio;

    int recommendation;
    int confidence;

    size_t char_count;
    size_t unique_chars;
} AnalysisResult;

/*
 * Shannon entropy calculation
 */
double calculate_entropy(
    const unsigned char *data,
    size_t length
);

/*
 * Complete file analysis
 */
AnalysisResult analyze_file(
    const unsigned char *data,
    size_t length
);

#endif