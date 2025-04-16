// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef QC_TEST_MD5_UTILS
#define QC_TEST_MD5_UTILS

#include <stdint.h>
#include <string>

namespace QC
{
namespace test
{
namespace utils
{

std::string MD5Sum( const void *data, uint32_t length );

}   // namespace utils
}   // namespace test
}   // namespace QC
#endif /* QC_TEST_MD5_UTILS */