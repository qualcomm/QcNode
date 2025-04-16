// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "QC/component/C2D.hpp"
#include "md5_utils.hpp"
#include "gtest/gtest.h"

using namespace QC::common;
using namespace QC::component;
using namespace QC::test::utils;

static void readTXT( char *file_path, char *data_list[] )
{
    char line[100];
    FILE *file = fopen( file_path, "r" );

    if ( file != NULL )
    {
        int i = 0;
        while ( fgets( line, sizeof( line ), file ) != NULL )
        {
            data_list[i] = (char *) malloc( strlen( line ) + 1 );
            line[strcspn( line, "\n" )] = '\0';
            memcpy( data_list[i++], line, strlen( line ) * sizeof( char ) );
        }
        fclose( file );
    }
    else
    {
        printf( "Could not open %s\n", file_path );
        return;
    }

    return;
}

static void loadRawData( char *file_path, void *pData, size_t data_size )
{
    FILE *file = fopen( file_path, "rb" );

    if ( file != NULL )
    {
        fseek( file, 0, SEEK_END );
        size_t fsize = ftell( file );
        fseek( file, 0, SEEK_SET );
        if ( fsize == data_size )
        {
            if ( pData != nullptr )
            {
                size_t readSize = fread( pData, 1, fsize, file );
                if ( fsize != readSize )
                {
                    printf( "read size for %s does not match, read size: %lu, file size: %lu\n",
                            file_path, readSize, fsize );
                }
            }
            else
            {
                printf( "pData is null pointer\n" );
            }
        }
        else
        {
            printf( "data size for %s does not match, input size: %lu, file size: %lu\n", file_path,
                    data_size, fsize );
        }
    }
    fclose( file );
}


void C2DTestNormal( C2D_Config_t *c2dConfig, QCImageFormat_e outputFormat, uint32_t outputWidth,
                    uint32_t outputHeight )
{
    QCStatus_e ret = QC_STATUS_OK;

    C2D C2DObj;
    char pName[5] = "C2D";
    uint32_t numInputs = c2dConfig->numOfInputs;
    QCSharedBuffer_t inputs[numInputs];
    QCSharedBuffer_t output;

    ret = C2DObj.Init( pName, c2dConfig );
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( size_t i = 0; i < numInputs; i++ )
    {
        ret = inputs[i].Allocate( c2dConfig->inputConfigs[i].inputResolution.width,
                                  c2dConfig->inputConfigs[i].inputResolution.height,
                                  c2dConfig->inputConfigs[i].inputFormat );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    ret = output.Allocate( outputWidth, outputHeight, outputFormat );
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( size_t i = 0; i < numInputs; i++ )
    {
        ret = C2DObj.RegisterInputBuffers( &inputs[i], 1 );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    ret = C2DObj.RegisterOutputBuffers( &output, 1 );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = C2DObj.Start();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = C2DObj.Execute( inputs, numInputs, &output );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = C2DObj.Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( size_t i = 0; i < numInputs; i++ )
    {
        ret = C2DObj.DeregisterInputBuffers( &inputs[i], 1 );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    ret = C2DObj.DeregisterOutputBuffers( &output, 1 );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = C2DObj.Deinit();
    ASSERT_EQ( QC_STATUS_OK, ret );
}

void C2DTestAccuracy( C2D_Config_t *c2dConfig, QCImageFormat_e outputFormat, uint32_t outputWidth,
                      uint32_t outputHeight, char *data_list_file, char *golden_list_file )
{
    QCStatus_e ret = QC_STATUS_OK;

    C2D C2DObj;
    char pName[5] = "C2D";
    uint32_t numInputs = c2dConfig->numOfInputs;
    QCSharedBuffer_t inputs[numInputs];
    QCSharedBuffer_t output;

    char *data_list[numInputs];
    char *golden_list[numInputs];
    void *golden_data[numInputs];
    void *pOutputData = nullptr;

    readTXT( data_list_file, data_list );
    readTXT( golden_list_file, golden_list );

    ret = C2DObj.Init( pName, c2dConfig );
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( size_t i = 0; i < numInputs; i++ )
    {
        ret = inputs[i].Allocate( c2dConfig->inputConfigs[i].inputResolution.width,
                                  c2dConfig->inputConfigs[i].inputResolution.height,
                                  c2dConfig->inputConfigs[i].inputFormat );
        ASSERT_EQ( QC_STATUS_OK, ret );

        loadRawData( data_list[i], inputs[i].data(), inputs[i].size );
    }

    ret = output.Allocate( numInputs, outputWidth, outputHeight, outputFormat );
    ASSERT_EQ( QC_STATUS_OK, ret );

    memset( output.data(), 0, output.size );

    uint32_t outputSize = output.size / numInputs;
    for ( size_t i = 0; i < numInputs; i++ )
    {
        golden_data[i] = (void *) malloc( outputSize );
        loadRawData( golden_list[i], golden_data[i], outputSize );
    }

    for ( size_t i = 0; i < numInputs; i++ )
    {
        ret = C2DObj.RegisterInputBuffers( &inputs[i], 1 );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    ret = C2DObj.RegisterOutputBuffers( &output, 1 );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = C2DObj.Start();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = C2DObj.Execute( inputs, numInputs, &output );
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( size_t i = 0; i < numInputs; i++ )
    {
        pOutputData = (void *) ( (uint8_t *) output.data() + i * outputSize );
        std::string md5OutputGolden = MD5Sum( pOutputData, outputSize );
        std::string md5OutputData = MD5Sum( golden_data[i], outputSize );
        EXPECT_EQ( md5OutputGolden, md5OutputData );

        printf( "md5sum for output batch %lu: %s\n", i, md5OutputData.c_str() );
        printf( "md5sum for golden data %lu: %s\n", i, md5OutputGolden.c_str() );

        free( golden_data[i] );
    }

    ret = C2DObj.Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( size_t i = 0; i < numInputs; i++ )
    {
        ret = C2DObj.DeregisterInputBuffers( &inputs[i], 1 );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    ret = C2DObj.DeregisterOutputBuffers( &output, 1 );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = C2DObj.Deinit();
    ASSERT_EQ( QC_STATUS_OK, ret );
}

void C2DTestPerf( C2D_Config_t *c2dConfig, QCImageFormat_e outputFormat, uint32_t outputWidth,
                  uint32_t outputHeight, char *data_list_file, uint32_t times )
{
    QCStatus_e ret = QC_STATUS_OK;

    C2D C2DObj;
    char pName[5] = "C2D";
    uint32_t numInputs = c2dConfig->numOfInputs;
    QCSharedBuffer_t inputs[numInputs];
    QCSharedBuffer_t output;

    char *data_list[numInputs];
    void *pOutputData = nullptr;

    readTXT( data_list_file, data_list );

    ret = C2DObj.Init( pName, c2dConfig );
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( size_t i = 0; i < numInputs; i++ )
    {
        ret = inputs[i].Allocate( c2dConfig->inputConfigs[i].inputResolution.width,
                                  c2dConfig->inputConfigs[i].inputResolution.height,
                                  c2dConfig->inputConfigs[i].inputFormat );
        ASSERT_EQ( QC_STATUS_OK, ret );

        loadRawData( data_list[i], inputs[i].data(), inputs[i].size );
    }

    ret = output.Allocate( numInputs, outputWidth, outputHeight, outputFormat );
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( size_t i = 0; i < numInputs; i++ )
    {
        ret = C2DObj.RegisterInputBuffers( &inputs[i], 1 );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    ret = C2DObj.RegisterOutputBuffers( &output, 1 );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = C2DObj.Start();
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( int i = 0; i < times; i++ )
    {
        auto start = std::chrono::high_resolution_clock::now();
        ret = C2DObj.Execute( inputs, numInputs, &output );
        auto end = std::chrono::high_resolution_clock::now();

        double duration_ms = std::chrono::duration<double, std::milli>( end - start ).count();
        printf( "execute time for loop %d: %f ms\n", i, (float) duration_ms );
    }
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = C2DObj.Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( size_t i = 0; i < numInputs; i++ )
    {
        ret = C2DObj.DeregisterInputBuffers( &inputs[i], 1 );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    ret = C2DObj.DeregisterOutputBuffers( &output, 1 );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = C2DObj.Deinit();
    ASSERT_EQ( QC_STATUS_OK, ret );
}

TEST( C2D, SANITY_C2D_ConvertUYVYtoRGB )
{
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;

    C2DConfig.numOfInputs = 1;
    for ( size_t i = 0; i < C2DConfig.numOfInputs; i++ )
    {
        C2DConfig.inputConfigs[i].inputFormat = QC_IMAGE_FORMAT_UYVY;
        C2DConfig.inputConfigs[i].inputResolution.width = 600;
        C2DConfig.inputConfigs[i].inputResolution.height = 600;
        C2DConfig.inputConfigs[i].ROI.topX = 100;
        C2DConfig.inputConfigs[i].ROI.topY = 100;
        C2DConfig.inputConfigs[i].ROI.width = 100;
        C2DConfig.inputConfigs[i].ROI.height = 100;
    }

    QCImageFormat_e outputFormat = QC_IMAGE_FORMAT_RGB888;
    uint32_t outputWidth = 600;
    uint32_t outputHeight = 600;

    C2DTestNormal( pC2DConfig, outputFormat, outputWidth, outputHeight );
}

TEST( C2D, SANITY_C2D_ConvertRGBtoYUVY )
{
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;

    C2DConfig.numOfInputs = 1;
    for ( size_t i = 0; i < C2DConfig.numOfInputs; i++ )
    {
        C2DConfig.inputConfigs[i].inputFormat = QC_IMAGE_FORMAT_RGB888;
        C2DConfig.inputConfigs[i].inputResolution.width = 1000;
        C2DConfig.inputConfigs[i].inputResolution.height = 1000;
        C2DConfig.inputConfigs[i].ROI.topX = 150;
        C2DConfig.inputConfigs[i].ROI.topY = 150;
        C2DConfig.inputConfigs[i].ROI.width = 600;
        C2DConfig.inputConfigs[i].ROI.height = 600;
    }

    QCImageFormat_e outputFormat = QC_IMAGE_FORMAT_UYVY;
    uint32_t outputWidth = 600;
    uint32_t outputHeight = 600;

    C2DTestNormal( pC2DConfig, outputFormat, outputWidth, outputHeight );
}

TEST( C2D, SANITY_C2D_ConvertRGBtoNV12 )
{
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;

    C2DConfig.numOfInputs = 1;
    for ( size_t i = 0; i < C2DConfig.numOfInputs; i++ )
    {
        C2DConfig.inputConfigs[i].inputFormat = QC_IMAGE_FORMAT_RGB888;
        C2DConfig.inputConfigs[i].inputResolution.width = 1920;
        C2DConfig.inputConfigs[i].inputResolution.height = 1080;
        C2DConfig.inputConfigs[i].ROI.topX = 200;
        C2DConfig.inputConfigs[i].ROI.topY = 200;
        C2DConfig.inputConfigs[i].ROI.width = 1080;
        C2DConfig.inputConfigs[i].ROI.height = 720;
    }

    QCImageFormat_e outputFormat = QC_IMAGE_FORMAT_NV12;
    uint32_t outputWidth = 1080;
    uint32_t outputHeight = 720;

    C2DTestNormal( pC2DConfig, outputFormat, outputWidth, outputHeight );
}

TEST( C2D, SANITY_C2D_ConvertNV12toUYVY )
{
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;

    pC2DConfig->numOfInputs = 1;
    for ( size_t i = 0; i < C2DConfig.numOfInputs; i++ )
    {
        pC2DConfig->inputConfigs[i].inputFormat = QC_IMAGE_FORMAT_NV12;
        pC2DConfig->inputConfigs[i].inputResolution.width = 1920;
        pC2DConfig->inputConfigs[i].inputResolution.height = 1080;
        pC2DConfig->inputConfigs[i].ROI.topX = 300;
        pC2DConfig->inputConfigs[i].ROI.topY = 300;
        pC2DConfig->inputConfigs[i].ROI.width = 1080;
        pC2DConfig->inputConfigs[i].ROI.height = 720;
    }

    QCImageFormat_e outputFormat = QC_IMAGE_FORMAT_UYVY;
    uint32_t outputWidth = 1080;
    uint32_t outputHeight = 720;

    C2DTestNormal( pC2DConfig, outputFormat, outputWidth, outputHeight );
}

TEST( C2D, SANITY_C2D_ConvertUYVYtoNV12 )
{
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;

    pC2DConfig->numOfInputs = 1;
    for ( size_t i = 0; i < C2DConfig.numOfInputs; i++ )
    {
        pC2DConfig->inputConfigs[i].inputFormat = QC_IMAGE_FORMAT_UYVY;
        pC2DConfig->inputConfigs[i].inputResolution.width = 1920;
        pC2DConfig->inputConfigs[i].inputResolution.height = 1080;
        pC2DConfig->inputConfigs[i].ROI.topX = 100;
        pC2DConfig->inputConfigs[i].ROI.topY = 100;
        pC2DConfig->inputConfigs[i].ROI.width = 1080;
        pC2DConfig->inputConfigs[i].ROI.height = 720;
    }

    QCImageFormat_e outputFormat = QC_IMAGE_FORMAT_NV12;
    uint32_t outputWidth = 1080;
    uint32_t outputHeight = 720;

    C2DTestNormal( pC2DConfig, outputFormat, outputWidth, outputHeight );
}

TEST( C2D, SANITY_C2D_ConvertUYVYtoBGR )
{
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;

    pC2DConfig->numOfInputs = 1;
    for ( size_t i = 0; i < C2DConfig.numOfInputs; i++ )
    {
        pC2DConfig->inputConfigs[i].inputFormat = QC_IMAGE_FORMAT_UYVY;
        pC2DConfig->inputConfigs[i].inputResolution.width = 1920;
        pC2DConfig->inputConfigs[i].inputResolution.height = 1080;
        pC2DConfig->inputConfigs[i].ROI.topX = 100;
        pC2DConfig->inputConfigs[i].ROI.topY = 100;
        pC2DConfig->inputConfigs[i].ROI.width = 1080;
        pC2DConfig->inputConfigs[i].ROI.height = 720;
    }

    QCImageFormat_e outputFormat = QC_IMAGE_FORMAT_BGR888;
    uint32_t outputWidth = 1080;
    uint32_t outputHeight = 720;

    C2DTestNormal( pC2DConfig, outputFormat, outputWidth, outputHeight );
}

TEST( C2D, SANITY_C2D_ConvertNV12toP010 )
{
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;

    pC2DConfig->numOfInputs = 1;
    for ( size_t i = 0; i < C2DConfig.numOfInputs; i++ )
    {
        pC2DConfig->inputConfigs[i].inputFormat = QC_IMAGE_FORMAT_NV12;
        pC2DConfig->inputConfigs[i].inputResolution.width = 1920;
        pC2DConfig->inputConfigs[i].inputResolution.height = 1080;
        pC2DConfig->inputConfigs[i].ROI.topX = 100;
        pC2DConfig->inputConfigs[i].ROI.topY = 100;
        pC2DConfig->inputConfigs[i].ROI.width = 1080;
        pC2DConfig->inputConfigs[i].ROI.height = 720;
    }

    QCImageFormat_e outputFormat = QC_IMAGE_FORMAT_P010;
    uint32_t outputWidth = 1080;
    uint32_t outputHeight = 720;

    C2DTestNormal( pC2DConfig, outputFormat, outputWidth, outputHeight );
}

TEST( C2D, BOUND_C2D_ROI )
{
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;

    pC2DConfig->numOfInputs = 1;
    for ( size_t i = 0; i < C2DConfig.numOfInputs; i++ )
    {
        pC2DConfig->inputConfigs[i].inputFormat = QC_IMAGE_FORMAT_UYVY;
        pC2DConfig->inputConfigs[i].inputResolution.width = 1920;
        pC2DConfig->inputConfigs[i].inputResolution.height = 1080;
        pC2DConfig->inputConfigs[i].ROI.topX = 0;
        pC2DConfig->inputConfigs[i].ROI.topY = 0;
        pC2DConfig->inputConfigs[i].ROI.width = 0;
        pC2DConfig->inputConfigs[i].ROI.height = 0;
    }

    QCImageFormat_e outputFormat = QC_IMAGE_FORMAT_NV12;
    uint32_t outputWidth = 1080;
    uint32_t outputHeight = 720;

    C2DTestNormal( pC2DConfig, outputFormat, outputWidth, outputHeight );
}

TEST( C2D, SANITY_C2D_RegDeregBuffer )
{

    QCStatus_e ret = QC_STATUS_OK;

    C2D C2DObj;
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;
    char pName[5] = "C2D";

    pC2DConfig->numOfInputs = 1;
    QCImageFormat_e C2DOutputFormat = QC_IMAGE_FORMAT_NV12;
    uint32_t C2DOutputWidth = 1080;
    uint32_t C2DOutputHeight = 720;
    uint32_t inputBufferNum = 1;
    uint32_t outputBufferNum = 1;

    QCSharedBuffer_t inputs[pC2DConfig->numOfInputs];
    QCSharedBuffer_t output;

    for ( size_t i = 0; i < pC2DConfig->numOfInputs; i++ )
    {
        pC2DConfig->inputConfigs[i].inputFormat = QC_IMAGE_FORMAT_UYVY;
        pC2DConfig->inputConfigs[i].inputResolution.width = 1920;
        pC2DConfig->inputConfigs[i].inputResolution.height = 1080;
        pC2DConfig->inputConfigs[i].ROI.topX = 100;
        pC2DConfig->inputConfigs[i].ROI.topY = 100;
        pC2DConfig->inputConfigs[i].ROI.width = 1080;
        pC2DConfig->inputConfigs[i].ROI.height = 720;

        ret = inputs[i].Allocate( pC2DConfig->inputConfigs[i].inputResolution.width,
                                  pC2DConfig->inputConfigs[i].inputResolution.height,
                                  pC2DConfig->inputConfigs[i].inputFormat );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    ret = output.Allocate( C2DOutputWidth, C2DOutputHeight, C2DOutputFormat );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = C2DObj.Init( pName, pC2DConfig );
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( size_t i = 0; i < pC2DConfig->numOfInputs; i++ )
    {
        ret = C2DObj.RegisterInputBuffers( &inputs[i], inputBufferNum );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    ret = C2DObj.RegisterOutputBuffers( &output, outputBufferNum );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = C2DObj.Start();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = C2DObj.Execute( inputs, pC2DConfig->numOfInputs, &output );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = C2DObj.Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( size_t i = 0; i < pC2DConfig->numOfInputs; i++ )
    {
        ret = C2DObj.DeregisterInputBuffers( &inputs[i], inputBufferNum );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    ret = C2DObj.DeregisterOutputBuffers( &output, outputBufferNum );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = C2DObj.Deinit();
    ASSERT_EQ( QC_STATUS_OK, ret );
}

TEST( C2D, FAILURE_C2D_UnInitialized )
{
    QCStatus_e ret = QC_STATUS_OK;

    C2D C2DObj;
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;
    char pName[5] = "C2D";

    C2DConfig.numOfInputs = 1;
    for ( size_t i = 0; i < C2DConfig.numOfInputs; i++ )
    {
        C2DConfig.inputConfigs[i].inputFormat = QC_IMAGE_FORMAT_UYVY;
        C2DConfig.inputConfigs[i].inputResolution.width = 600;
        C2DConfig.inputConfigs[i].inputResolution.height = 600;
        C2DConfig.inputConfigs[i].ROI.topX = 100;
        C2DConfig.inputConfigs[i].ROI.topY = 100;
        C2DConfig.inputConfigs[i].ROI.width = 100;
        C2DConfig.inputConfigs[i].ROI.height = 100;
    }

    QCImageFormat_e C2DOutputFormat = QC_IMAGE_FORMAT_RGB888;
    uint32_t C2DOutputWidth = 600;
    uint32_t C2DOutputHeight = 600;

    QCSharedBuffer_t inputs[C2DConfig.numOfInputs];
    ret = inputs[0].Allocate( C2DConfig.inputConfigs[0].inputResolution.width,
                              C2DConfig.inputConfigs[0].inputResolution.height,
                              C2DConfig.inputConfigs[0].inputFormat );
    ASSERT_EQ( QC_STATUS_OK, ret );

    QCSharedBuffer_t output;
    ret = output.Allocate( C2DOutputWidth, C2DOutputHeight, C2DOutputFormat );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = C2DObj.Start();
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

    ret = C2DObj.Execute( inputs, C2DConfig.numOfInputs, &output );
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

    ret = C2DObj.Stop();
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

    ret = C2DObj.Deinit();
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );
}

TEST( C2D, FAILURE_C2D_TopXBadArgs )
{

    QCStatus_e ret = QC_STATUS_OK;

    C2D C2DObj;
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;
    char pName[5] = "C2D";

    C2DConfig.numOfInputs = 1;
    for ( size_t i = 0; i < C2DConfig.numOfInputs; i++ )
    {
        C2DConfig.inputConfigs[i].inputFormat = QC_IMAGE_FORMAT_UYVY;
        C2DConfig.inputConfigs[i].inputResolution.width = 600;
        C2DConfig.inputConfigs[i].inputResolution.height = 600;
        C2DConfig.inputConfigs[i].ROI.topX = 800;
        C2DConfig.inputConfigs[i].ROI.topY = 100;
        C2DConfig.inputConfigs[i].ROI.width = 100;
        C2DConfig.inputConfigs[i].ROI.height = 100;
    }

    QCImageFormat_e C2DOutputFormat = QC_IMAGE_FORMAT_RGB888;
    uint32_t C2DOutputWidth = 600;
    uint32_t C2DOutputHeight = 600;

    QCSharedBuffer_t inputs[C2DConfig.numOfInputs];
    ret = inputs[0].Allocate( C2DConfig.inputConfigs[0].inputResolution.width,
                              C2DConfig.inputConfigs[0].inputResolution.height,
                              C2DConfig.inputConfigs[0].inputFormat );
    ASSERT_EQ( QC_STATUS_OK, ret );

    QCSharedBuffer_t output;
    ret = output.Allocate( C2DOutputWidth, C2DOutputHeight, C2DOutputFormat );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = C2DObj.Init( pName, pC2DConfig );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    ret = C2DObj.Start();
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

    ret = C2DObj.Execute( inputs, C2DConfig.numOfInputs, &output );
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

    ret = C2DObj.Stop();
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

    ret = C2DObj.Deinit();
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );
}

TEST( C2D, FAILURE_C2D_TopYBadArgs )
{

    QCStatus_e ret = QC_STATUS_OK;

    C2D C2DObj;
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;
    char pName[5] = "C2D";

    C2DConfig.numOfInputs = 1;
    for ( size_t i = 0; i < C2DConfig.numOfInputs; i++ )
    {
        C2DConfig.inputConfigs[i].inputFormat = QC_IMAGE_FORMAT_UYVY;
        C2DConfig.inputConfigs[i].inputResolution.width = 600;
        C2DConfig.inputConfigs[i].inputResolution.height = 600;
        C2DConfig.inputConfigs[i].ROI.topX = 100;
        C2DConfig.inputConfigs[i].ROI.topY = 650;
        C2DConfig.inputConfigs[i].ROI.width = 100;
        C2DConfig.inputConfigs[i].ROI.height = 100;
    }

    QCImageFormat_e C2DOutputFormat = QC_IMAGE_FORMAT_RGB888;
    uint32_t C2DOutputWidth = 600;
    uint32_t C2DOutputHeight = 600;

    QCSharedBuffer_t inputs[C2DConfig.numOfInputs];
    ret = inputs[0].Allocate( C2DConfig.inputConfigs[0].inputResolution.width,
                              C2DConfig.inputConfigs[0].inputResolution.height,
                              C2DConfig.inputConfigs[0].inputFormat );
    ASSERT_EQ( QC_STATUS_OK, ret );

    QCSharedBuffer_t output;
    ret = output.Allocate( C2DOutputWidth, C2DOutputHeight, C2DOutputFormat );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = C2DObj.Init( pName, pC2DConfig );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    ret = C2DObj.Start();
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

    ret = C2DObj.Execute( inputs, C2DConfig.numOfInputs, &output );
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

    ret = C2DObj.Stop();
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

    ret = C2DObj.Deinit();
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );
}

TEST( C2D, FAILURE_C2D_ROIWidthBadArgs )
{

    QCStatus_e ret = QC_STATUS_OK;

    C2D C2DObj;
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;
    char pName[5] = "C2D";

    C2DConfig.numOfInputs = 1;
    for ( size_t i = 0; i < C2DConfig.numOfInputs; i++ )
    {
        C2DConfig.inputConfigs[i].inputFormat = QC_IMAGE_FORMAT_UYVY;
        C2DConfig.inputConfigs[i].inputResolution.width = 600;
        C2DConfig.inputConfigs[i].inputResolution.height = 600;
        C2DConfig.inputConfigs[i].ROI.topX = 100;
        C2DConfig.inputConfigs[i].ROI.topY = 100;
        C2DConfig.inputConfigs[i].ROI.width = 1000;
        C2DConfig.inputConfigs[i].ROI.height = 100;
    }

    QCImageFormat_e C2DOutputFormat = QC_IMAGE_FORMAT_RGB888;
    uint32_t C2DOutputWidth = 600;
    uint32_t C2DOutputHeight = 600;

    QCSharedBuffer_t inputs[C2DConfig.numOfInputs];
    ret = inputs[0].Allocate( C2DConfig.inputConfigs[0].inputResolution.width,
                              C2DConfig.inputConfigs[0].inputResolution.height,
                              C2DConfig.inputConfigs[0].inputFormat );
    ASSERT_EQ( QC_STATUS_OK, ret );

    QCSharedBuffer_t output;
    ret = output.Allocate( C2DOutputWidth, C2DOutputHeight, C2DOutputFormat );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = C2DObj.Init( pName, pC2DConfig );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    ret = C2DObj.Start();
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

    ret = C2DObj.Execute( inputs, C2DConfig.numOfInputs, &output );
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

    ret = C2DObj.Stop();
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

    ret = C2DObj.Deinit();
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );
}

TEST( C2D, FAILURE_C2D_ROIHeightBadArgs )
{
    QCStatus_e ret = QC_STATUS_OK;

    C2D C2DObj;
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;
    char pName[5] = "C2D";

    C2DConfig.numOfInputs = 1;
    for ( size_t i = 0; i < C2DConfig.numOfInputs; i++ )
    {
        C2DConfig.inputConfigs[i].inputFormat = QC_IMAGE_FORMAT_UYVY;
        C2DConfig.inputConfigs[i].inputResolution.width = 600;
        C2DConfig.inputConfigs[i].inputResolution.height = 600;
        C2DConfig.inputConfigs[i].ROI.topX = 100;
        C2DConfig.inputConfigs[i].ROI.topY = 100;
        C2DConfig.inputConfigs[i].ROI.width = 100;
        C2DConfig.inputConfigs[i].ROI.height = 1000;
    }

    QCImageFormat_e C2DOutputFormat = QC_IMAGE_FORMAT_RGB888;
    uint32_t C2DOutputWidth = 600;
    uint32_t C2DOutputHeight = 600;

    QCSharedBuffer_t inputs[C2DConfig.numOfInputs];
    ret = inputs[0].Allocate( C2DConfig.inputConfigs[0].inputResolution.width,
                              C2DConfig.inputConfigs[0].inputResolution.height,
                              C2DConfig.inputConfigs[0].inputFormat );
    ASSERT_EQ( QC_STATUS_OK, ret );

    QCSharedBuffer_t output;
    ret = output.Allocate( C2DOutputWidth, C2DOutputHeight, C2DOutputFormat );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = C2DObj.Init( pName, pC2DConfig );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    ret = C2DObj.Start();
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

    ret = C2DObj.Execute( inputs, C2DConfig.numOfInputs, &output );
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

    ret = C2DObj.Stop();
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

    ret = C2DObj.Deinit();
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );
}

TEST( C2D, FAILURE_C2D_InputNumError )
{
    QCStatus_e ret = QC_STATUS_OK;

    C2D C2DObj;
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;
    char pName[5] = "C2D";

    C2DConfig.numOfInputs = 1;
    for ( size_t i = 0; i < C2DConfig.numOfInputs; i++ )
    {
        C2DConfig.inputConfigs[i].inputFormat = QC_IMAGE_FORMAT_UYVY;
        C2DConfig.inputConfigs[i].inputResolution.width = 600;
        C2DConfig.inputConfigs[i].inputResolution.height = 600;
        C2DConfig.inputConfigs[i].ROI.topX = 100;
        C2DConfig.inputConfigs[i].ROI.topY = 100;
        C2DConfig.inputConfigs[i].ROI.width = 100;
        C2DConfig.inputConfigs[i].ROI.height = 100;
    }

    QCImageFormat_e C2DOutputFormat = QC_IMAGE_FORMAT_RGB888;
    uint32_t C2DOutputWidth = 600;
    uint32_t C2DOutputHeight = 600;

    QCSharedBuffer_t inputs[C2DConfig.numOfInputs];
    ret = inputs[0].Allocate( C2DConfig.inputConfigs[0].inputResolution.width,
                              C2DConfig.inputConfigs[0].inputResolution.height,
                              C2DConfig.inputConfigs[0].inputFormat );
    ASSERT_EQ( QC_STATUS_OK, ret );

    QCSharedBuffer_t output;
    ret = output.Allocate( C2DOutputWidth, C2DOutputHeight, C2DOutputFormat );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = C2DObj.Init( pName, pC2DConfig );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = C2DObj.Start();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = C2DObj.Execute( inputs, 2, &output );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    ret = C2DObj.Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = C2DObj.Deinit();
    ASSERT_EQ( QC_STATUS_OK, ret );
}

TEST( C2D, FAILURE_C2D_GetSourceSurf )
{
    QCStatus_e ret = QC_STATUS_OK;

    C2D C2DObj;
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;
    char pName[5] = "C2D";

    C2DConfig.numOfInputs = 2;
    for ( size_t i = 0; i < C2DConfig.numOfInputs; i++ )
    {
        C2DConfig.inputConfigs[i].inputFormat = QC_IMAGE_FORMAT_UYVY;
        C2DConfig.inputConfigs[i].inputResolution.width = 600;
        C2DConfig.inputConfigs[i].inputResolution.height = 600;
        C2DConfig.inputConfigs[i].ROI.topX = 100;
        C2DConfig.inputConfigs[i].ROI.topY = 100;
        C2DConfig.inputConfigs[i].ROI.width = 100;
        C2DConfig.inputConfigs[i].ROI.height = 100;
    }

    QCImageFormat_e C2DOutputFormat = QC_IMAGE_FORMAT_RGB888;
    uint32_t C2DOutputWidth = 600;
    uint32_t C2DOutputHeight = 600;

    QCSharedBuffer_t inputs[C2DConfig.numOfInputs];
    ret = inputs[0].Allocate( C2DConfig.inputConfigs[0].inputResolution.width,
                              C2DConfig.inputConfigs[0].inputResolution.height,
                              C2DConfig.inputConfigs[0].inputFormat );
    ASSERT_EQ( QC_STATUS_OK, ret );

    QCSharedBuffer_t output;
    ret = output.Allocate( C2DConfig.numOfInputs, C2DOutputWidth, C2DOutputHeight,
                           C2DOutputFormat );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = C2DObj.Init( pName, pC2DConfig );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = C2DObj.Start();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = C2DObj.Execute( inputs + 2, C2DConfig.numOfInputs, &output );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    ret = C2DObj.Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = C2DObj.Deinit();
    ASSERT_EQ( QC_STATUS_OK, ret );
}

TEST( C2D, FAILURE_C2D_RegInputBufferFormat )
{

    QCStatus_e ret = QC_STATUS_OK;

    C2D C2DObj;
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;
    char pName[5] = "C2D";

    pC2DConfig->numOfInputs = 1;
    QCImageFormat_e C2DOutputFormat = QC_IMAGE_FORMAT_NV12;
    uint32_t C2DOutputWidth = 1080;
    uint32_t C2DOutputHeight = 720;
    uint32_t inputBufferNum = 1;
    uint32_t outputBufferNum = 1;

    QCSharedBuffer_t inputs[pC2DConfig->numOfInputs];
    QCSharedBuffer_t output;

    for ( size_t i = 0; i < pC2DConfig->numOfInputs; i++ )
    {
        pC2DConfig->inputConfigs[i].inputFormat = QC_IMAGE_FORMAT_UYVY;
        pC2DConfig->inputConfigs[i].inputResolution.width = 1920;
        pC2DConfig->inputConfigs[i].inputResolution.height = 1080;
        pC2DConfig->inputConfigs[i].ROI.topX = 100;
        pC2DConfig->inputConfigs[i].ROI.topY = 100;
        pC2DConfig->inputConfigs[i].ROI.width = 1080;
        pC2DConfig->inputConfigs[i].ROI.height = 720;
    }

    ret = C2DObj.Init( pName, pC2DConfig );
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( size_t i = 0; i < pC2DConfig->numOfInputs; i++ )
    {
        ret = inputs[i].Allocate( pC2DConfig->inputConfigs[i].inputResolution.width,
                                  pC2DConfig->inputConfigs[i].inputResolution.height,
                                  QC_IMAGE_FORMAT_RGB888 );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    ret = output.Allocate( C2DOutputWidth, C2DOutputHeight, C2DOutputFormat );
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( size_t i = 0; i < pC2DConfig->numOfInputs; i++ )
    {
        ret = C2DObj.RegisterInputBuffers( &inputs[i], inputBufferNum );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    ret = C2DObj.RegisterOutputBuffers( &output, outputBufferNum );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = C2DObj.Start();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = C2DObj.Execute( inputs, pC2DConfig->numOfInputs, &output );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    ret = C2DObj.Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( size_t i = 0; i < pC2DConfig->numOfInputs; i++ )
    {
        ret = C2DObj.DeregisterInputBuffers( &inputs[i], inputBufferNum );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    ret = C2DObj.DeregisterOutputBuffers( &output, outputBufferNum );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = C2DObj.Deinit();
    ASSERT_EQ( QC_STATUS_OK, ret );
}

TEST( C2D, FAILURE_C2D_RegInputBufferRes )
{
    QCStatus_e ret = QC_STATUS_OK;

    C2D C2DObj;
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;
    char pName[5] = "C2D";

    pC2DConfig->numOfInputs = 1;
    QCImageFormat_e C2DOutputFormat = QC_IMAGE_FORMAT_NV12;
    uint32_t C2DOutputWidth = 1080;
    uint32_t C2DOutputHeight = 720;
    uint32_t inputBufferNum = 1;
    uint32_t outputBufferNum = 1;

    QCSharedBuffer_t inputs[pC2DConfig->numOfInputs];
    QCSharedBuffer_t output;

    for ( size_t i = 0; i < pC2DConfig->numOfInputs; i++ )
    {
        pC2DConfig->inputConfigs[i].inputFormat = QC_IMAGE_FORMAT_UYVY;
        pC2DConfig->inputConfigs[i].inputResolution.width = 1920;
        pC2DConfig->inputConfigs[i].inputResolution.height = 1080;
        pC2DConfig->inputConfigs[i].ROI.topX = 100;
        pC2DConfig->inputConfigs[i].ROI.topY = 100;
        pC2DConfig->inputConfigs[i].ROI.width = 1080;
        pC2DConfig->inputConfigs[i].ROI.height = 720;
    }

    ret = C2DObj.Init( pName, pC2DConfig );
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( size_t i = 0; i < pC2DConfig->numOfInputs; i++ )
    {
        ret = inputs[i].Allocate( 1000, 1000, pC2DConfig->inputConfigs[i].inputFormat );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    ret = output.Allocate( C2DOutputWidth, C2DOutputHeight, C2DOutputFormat );
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( size_t i = 0; i < pC2DConfig->numOfInputs; i++ )
    {
        ret = C2DObj.RegisterInputBuffers( &inputs[i], inputBufferNum );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    ret = C2DObj.RegisterOutputBuffers( &output, outputBufferNum );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = C2DObj.Start();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = C2DObj.Execute( inputs, pC2DConfig->numOfInputs, &output );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    ret = C2DObj.Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( size_t i = 0; i < pC2DConfig->numOfInputs; i++ )
    {
        ret = C2DObj.DeregisterInputBuffers( &inputs[i], inputBufferNum );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    ret = C2DObj.DeregisterOutputBuffers( &output, outputBufferNum );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = C2DObj.Deinit();
    ASSERT_EQ( QC_STATUS_OK, ret );
}

TEST( C2D, FAILURE_C2D_DeRegInputBuffer )
{
    QCStatus_e ret = QC_STATUS_OK;

    C2D C2DObj;
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;
    char pName[5] = "C2D";

    pC2DConfig->numOfInputs = 1;
    QCImageFormat_e C2DOutputFormat = QC_IMAGE_FORMAT_NV12;
    uint32_t C2DOutputWidth = 1080;
    uint32_t C2DOutputHeight = 720;
    uint32_t inputBufferNum = 1;
    uint32_t outputBufferNum = 1;

    QCSharedBuffer_t inputs[pC2DConfig->numOfInputs];
    QCSharedBuffer_t output;

    for ( size_t i = 0; i < pC2DConfig->numOfInputs; i++ )
    {
        pC2DConfig->inputConfigs[i].inputFormat = QC_IMAGE_FORMAT_UYVY;
        pC2DConfig->inputConfigs[i].inputResolution.width = 1920;
        pC2DConfig->inputConfigs[i].inputResolution.height = 1080;
        pC2DConfig->inputConfigs[i].ROI.topX = 100;
        pC2DConfig->inputConfigs[i].ROI.topY = 100;
        pC2DConfig->inputConfigs[i].ROI.width = 1080;
        pC2DConfig->inputConfigs[i].ROI.height = 720;
    }

    ret = C2DObj.Init( pName, pC2DConfig );
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( size_t i = 0; i < pC2DConfig->numOfInputs; i++ )
    {
        ret = inputs[i].Allocate( pC2DConfig->inputConfigs[i].inputResolution.width,
                                  pC2DConfig->inputConfigs[i].inputResolution.height,
                                  pC2DConfig->inputConfigs[i].inputFormat );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    ret = output.Allocate( C2DOutputWidth, C2DOutputHeight, C2DOutputFormat );
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( size_t i = 0; i < pC2DConfig->numOfInputs; i++ )
    {
        ret = C2DObj.RegisterInputBuffers( &inputs[i], inputBufferNum );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    ret = C2DObj.RegisterOutputBuffers( &output, outputBufferNum );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = C2DObj.Start();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = C2DObj.Execute( inputs, pC2DConfig->numOfInputs, &output );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = C2DObj.Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( size_t i = 0; i < pC2DConfig->numOfInputs; i++ )
    {
        ret = C2DObj.DeregisterInputBuffers( &inputs[i], inputBufferNum + 1 );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    }

    ret = C2DObj.DeregisterOutputBuffers( &output, outputBufferNum );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = C2DObj.Deinit();
    ASSERT_EQ( QC_STATUS_OK, ret );
}

TEST( C2D, FAILURE_C2D_DeRegOutputBuffer )
{
    QCStatus_e ret = QC_STATUS_OK;

    C2D C2DObj;
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;
    char pName[5] = "C2D";

    pC2DConfig->numOfInputs = 1;
    QCImageFormat_e C2DOutputFormat = QC_IMAGE_FORMAT_NV12;
    uint32_t C2DOutputWidth = 1080;
    uint32_t C2DOutputHeight = 720;
    uint32_t inputBufferNum = 1;
    uint32_t outputBufferNum = 1;

    QCSharedBuffer_t inputs[pC2DConfig->numOfInputs];
    QCSharedBuffer_t output;

    for ( size_t i = 0; i < pC2DConfig->numOfInputs; i++ )
    {
        pC2DConfig->inputConfigs[i].inputFormat = QC_IMAGE_FORMAT_UYVY;
        pC2DConfig->inputConfigs[i].inputResolution.width = 1920;
        pC2DConfig->inputConfigs[i].inputResolution.height = 1080;
        pC2DConfig->inputConfigs[i].ROI.topX = 100;
        pC2DConfig->inputConfigs[i].ROI.topY = 100;
        pC2DConfig->inputConfigs[i].ROI.width = 1080;
        pC2DConfig->inputConfigs[i].ROI.height = 720;
    }

    ret = C2DObj.Init( pName, pC2DConfig );
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( size_t i = 0; i < pC2DConfig->numOfInputs; i++ )
    {
        ret = inputs[i].Allocate( pC2DConfig->inputConfigs[i].inputResolution.width,
                                  pC2DConfig->inputConfigs[i].inputResolution.height,
                                  pC2DConfig->inputConfigs[i].inputFormat );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    ret = output.Allocate( C2DOutputWidth, C2DOutputHeight, C2DOutputFormat );
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( size_t i = 0; i < pC2DConfig->numOfInputs; i++ )
    {
        ret = C2DObj.RegisterInputBuffers( &inputs[i], inputBufferNum );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    ret = C2DObj.RegisterOutputBuffers( &output, outputBufferNum );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = C2DObj.Start();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = C2DObj.Execute( inputs, pC2DConfig->numOfInputs, &output );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = C2DObj.Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( size_t i = 0; i < pC2DConfig->numOfInputs; i++ )
    {
        ret = C2DObj.DeregisterInputBuffers( &inputs[i], inputBufferNum );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    ret = C2DObj.DeregisterOutputBuffers( &output, outputBufferNum + 1 );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    ret = C2DObj.Deinit();
    ASSERT_EQ( QC_STATUS_OK, ret );
}

TEST( C2D, ACCURACY_C2D_ConvertNV12toUYVY_4Batch_ROI )
{
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;
    char input_file_list[] = "data/test/c2d/input_data/NV12/input_data_list.txt";
    char golden_file_list[] = "data/test/c2d/golden_data/UYVY/golden_data_list.txt";

    pC2DConfig->numOfInputs = 4;
    for ( size_t i = 0; i < C2DConfig.numOfInputs; i++ )
    {
        pC2DConfig->inputConfigs[i].inputFormat = QC_IMAGE_FORMAT_NV12;
        pC2DConfig->inputConfigs[i].inputResolution.width = 1920;
        pC2DConfig->inputConfigs[i].inputResolution.height = 1024;
        pC2DConfig->inputConfigs[i].ROI.topX = 100;
        pC2DConfig->inputConfigs[i].ROI.topY = 100;
        pC2DConfig->inputConfigs[i].ROI.width = 600;
        pC2DConfig->inputConfigs[i].ROI.height = 600;
    }

    QCImageFormat_e outputFormat = QC_IMAGE_FORMAT_UYVY;
    uint32_t outputWidth = 1024;
    uint32_t outputHeight = 768;

    C2DTestAccuracy( pC2DConfig, outputFormat, outputWidth, outputHeight, input_file_list,
                     golden_file_list );
}

TEST( C2D, ACCURACY_C2D_ConvertNV12toRGB_4Batch_ROI )
{
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;
    char input_file_list[] = "data/test/c2d/input_data/NV12/input_data_list.txt";
    char golden_file_list[] = "data/test/c2d/golden_data/RGB/golden_data_list.txt";

    pC2DConfig->numOfInputs = 4;
    for ( size_t i = 0; i < C2DConfig.numOfInputs; i++ )
    {
        pC2DConfig->inputConfigs[i].inputFormat = QC_IMAGE_FORMAT_NV12;
        pC2DConfig->inputConfigs[i].inputResolution.width = 1920;
        pC2DConfig->inputConfigs[i].inputResolution.height = 1024;
        pC2DConfig->inputConfigs[i].ROI.topX = 100;
        pC2DConfig->inputConfigs[i].ROI.topY = 100;
        pC2DConfig->inputConfigs[i].ROI.width = 600;
        pC2DConfig->inputConfigs[i].ROI.height = 600;
    }

    QCImageFormat_e outputFormat = QC_IMAGE_FORMAT_RGB888;
    uint32_t outputWidth = 1024;
    uint32_t outputHeight = 768;

    C2DTestAccuracy( pC2DConfig, outputFormat, outputWidth, outputHeight, input_file_list,
                     golden_file_list );
}

TEST( C2D, PERF_C2D_ConvertNV12toUYVY_4Batch_ROI )
{
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;
    char input_file_list[] = "data/test/c2d/input_data/NV12/input_data_list.txt";

    pC2DConfig->numOfInputs = 4;
    for ( size_t i = 0; i < C2DConfig.numOfInputs; i++ )
    {
        pC2DConfig->inputConfigs[i].inputFormat = QC_IMAGE_FORMAT_NV12;
        pC2DConfig->inputConfigs[i].inputResolution.width = 1920;
        pC2DConfig->inputConfigs[i].inputResolution.height = 1024;
        pC2DConfig->inputConfigs[i].ROI.topX = 100;
        pC2DConfig->inputConfigs[i].ROI.topY = 100;
        pC2DConfig->inputConfigs[i].ROI.width = 600;
        pC2DConfig->inputConfigs[i].ROI.height = 600;
    }

    QCImageFormat_e outputFormat = QC_IMAGE_FORMAT_UYVY;
    uint32_t outputWidth = 1024;
    uint32_t outputHeight = 768;
    uint32_t times = 4;

    C2DTestPerf( pC2DConfig, outputFormat, outputWidth, outputHeight, input_file_list, times );
}

TEST( C2D, PERF_C2D_ConvertNV12toRGB_4Batch_ROI )
{
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;
    char input_file_list[] = "data/test/c2d/input_data/NV12/input_data_list.txt";

    pC2DConfig->numOfInputs = 4;
    for ( size_t i = 0; i < C2DConfig.numOfInputs; i++ )
    {
        pC2DConfig->inputConfigs[i].inputFormat = QC_IMAGE_FORMAT_NV12;
        pC2DConfig->inputConfigs[i].inputResolution.width = 1920;
        pC2DConfig->inputConfigs[i].inputResolution.height = 1024;
        pC2DConfig->inputConfigs[i].ROI.topX = 100;
        pC2DConfig->inputConfigs[i].ROI.topY = 100;
        pC2DConfig->inputConfigs[i].ROI.width = 600;
        pC2DConfig->inputConfigs[i].ROI.height = 600;
    }

    QCImageFormat_e outputFormat = QC_IMAGE_FORMAT_RGB888;
    uint32_t outputWidth = 1024;
    uint32_t outputHeight = 768;
    uint32_t times = 4;

    C2DTestPerf( pC2DConfig, outputFormat, outputWidth, outputHeight, input_file_list, times );
}


#ifndef GTEST_QCNODE
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif
