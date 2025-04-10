// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef RIDEHAL_TEST_MD5_UTILS
#define RIDEHAL_TEST_MD5_UTILS

#include <stdint.h>
#include <string>

namespace ridehal
{
namespace test
{
namespace utils
{

std::string MD5Sum( const void *data, uint32_t length );

}   // namespace utils
}   // namespace test
}   // namespace ridehal
#endif /* RIDEHAL_TEST_MD5_UTILS */