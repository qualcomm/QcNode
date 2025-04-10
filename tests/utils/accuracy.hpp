// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef RIDEHAL_TEST_ACCURACY_UTILS
#define RIDEHAL_TEST_ACCURACY_UTILS

#include <math.h>
#include <stdint.h>
#include <string>

namespace ridehal
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
}   // namespace ridehal
#endif /* RIDEHAL_TEST_ACCURACY_UTILS */