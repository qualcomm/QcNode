// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear



#ifndef QC_TEST_ACCURACY_UTILS
#define QC_TEST_ACCURACY_UTILS

#include <math.h>
#include <stdint.h>
#include <string>

namespace QC
{
namespace test
{
namespace utils
{

template<typename T>
double CosineSimilarity( const T *golden, const T *data, size_t size )
{
    double dot = 0.0;
    double norm1 = 1e-10;
    double norm2 = 1e-10;
    for ( size_t i = 0; i < size; i++ )
    {
        dot = dot + (double) data[i] * golden[i];
        norm1 = norm1 + (double) data[i] * data[i];
        norm2 = norm2 + (double) golden[i] * golden[i];
    }
    double cosSim = dot / sqrt( norm1 * norm2 );

    return cosSim;
}

}   // namespace utils
}   // namespace test
}   // namespace QC
#endif /* QC_TEST_ACCURACY_UTILS */