// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "md5_utils.hpp"
#include <iomanip>
#include <openssl/md5.h>
#include <sstream>
#include <stdio.h>
#include <vector>

#ifdef OSSL_DEPRECATEDIN_3_0
#include <openssl/evp.h>
#endif

namespace ridehal
{
namespace test
{
namespace utils
{

std::string MD5Sum( const void *data, uint32_t length )
{
    std::string md5Value = "";
    MD5_CTX ctx;
    uint8_t md5Digest[MD5_DIGEST_LENGTH];
    char digest[MD5_DIGEST_LENGTH * 2 + 1];
    int ret;

#ifndef OSSL_DEPRECATEDIN_3_0
    ret = MD5_Init( &ctx );
    if ( 1 == ret )
    {
        ret = MD5_Update( &ctx, data, length );
        if ( 1 == ret )
        {
            ret = MD5_Final( md5Digest, &ctx );
            if ( 1 == ret )
            {
                ret = 0;
                for ( uint32_t i = 0; i < MD5_DIGEST_LENGTH; i++ )
                {
                    ret += snprintf( &digest[ret], sizeof( digest ) - ret, "%02x", md5Digest[i] );
                }
                md5Value = digest;
            }
            else
            {
                printf( "md5 final failed: %d\n", ret );
            }
        }
        else
        {
            printf( "md5 update failed: %d\n", ret );
        }
    }
    else
    {
        printf( "md5 init failed: %d\n", ret );
    }
#else
    ret = EVP_Q_digest( NULL, "MD5", NULL, data, length, md5Digest, NULL );
    if ( 1 == ret )
    {
        ret = 0;
        for ( uint32_t i = 0; i < MD5_DIGEST_LENGTH; i++ )
        {
            ret += snprintf( &digest[ret], sizeof( digest ) - ret, "%02x", md5Digest[i] );
        }
        md5Value = digest;
    }
    else
    {
        printf( "EVP_Q_digest failed: %d\n", ret );
    }
#endif
    return md5Value;
}

}   // namespace utils
}   // namespace test
}   // namespace ridehal

#ifdef MD5_UTILS_TEST
using namespace ridehal::test::utils;
int main( int argc, char *argv[] )
{
    if ( argc != 2 )
    {
        printf( "Usage: %s file\n", argv[0] );
        return -1;
    }

    FILE *pFile = fopen( argv[1], "rb" );
    if ( nullptr != pFile )
    {
        fseek( pFile, 0, SEEK_END );
        uint32_t length = (uint32_t) ftell( pFile );
        fseek( pFile, 0, SEEK_SET );

        std::vector<uint8_t> data;
        data.resize( length );
        int r = fread( data.data(), 1, length, pFile );
        (void) r;
        std::string digest = MD5Sum( data.data(), length );
        printf( "%s %s\n", digest.c_str(), argv[1] );
    }
    else
    {
        printf( "no file: %s\n", argv[1] );
    }
    return 0;
}
#endif
