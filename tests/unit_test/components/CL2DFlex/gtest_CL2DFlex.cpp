// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "gtest/gtest.h"
#include <chrono>
#include <cmath>
#include <stdio.h>
#include <string>

#include "QC/component/CL2DFlex.hpp"
#include "kernel/CL2DFlex.cl.h"
#include "md5_utils.hpp"

using namespace QC;
using namespace QC::component;
using namespace QC::test::utils;

QCStatus_e LoadFile( QCSharedBuffer_t buffer, std::string path )
{
    QCStatus_e ret = QC_STATUS_OK;
    FILE *file = nullptr;
    size_t length = 0;
    size_t size = buffer.size;

    file = fopen( path.c_str(), "rb" );
    if ( nullptr == file )
    {
        printf( "Failed to open file %s", path.c_str() );
        ret = QC_STATUS_FAIL;
    }

    if ( QC_STATUS_OK == ret )
    {
        fseek( file, 0, SEEK_END );
        length = (size_t) ftell( file );
        if ( size != length )
        {
            printf( "Invalid file size for %s, need %d but got %d", path.c_str(), (int) size,
                    (int) length );
            ret = QC_STATUS_FAIL;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        fseek( file, 0, SEEK_SET );
        auto r = fread( buffer.data(), 1, length, file );
        if ( length != r )
        {
            printf( "failed to read map table file %s", path.c_str() );
            ret = QC_STATUS_FAIL;
        }
    }

    if ( nullptr != file )
    {
        fclose( file );
    }

    return ret;
}

void ROITest( uint32_t numberTest, CL2DFlex_ROIConfig_t *pROITest, CL2DFlex_Work_Mode_e modeTest,
              QCImageFormat_e inputFormatTest, QCImageFormat_e outputFormatTest,
              uint32_t inputWidthTest, uint32_t inputHeightTest, uint32_t outputWidthTest,
              uint32_t outputHeightTest, uint32_t times, bool checkAccuracy )
{
    QCStatus_e ret = QC_STATUS_OK;

    CL2DFlex CL2DFlexObj;
    CL2DFlex_Config_t CL2DFlexConfig;
    char pName[20] = "CL2DFlex";

    CL2DFlexConfig.numOfInputs = 1;
    CL2DFlexConfig.workModes[0] = modeTest;
    CL2DFlexConfig.inputWidths[0] = inputWidthTest;
    CL2DFlexConfig.inputHeights[0] = inputHeightTest;
    CL2DFlexConfig.inputFormats[0] = inputFormatTest;
    CL2DFlexConfig.ROIs[0].x = pROITest[0].x;
    CL2DFlexConfig.ROIs[0].y = pROITest[0].y;
    CL2DFlexConfig.ROIs[0].width = pROITest[0].width;
    CL2DFlexConfig.ROIs[0].height = pROITest[0].height;
    CL2DFlexConfig.outputWidth = outputWidthTest;
    CL2DFlexConfig.outputHeight = outputHeightTest;
    CL2DFlexConfig.outputFormat = outputFormatTest;
    CL2DFlexConfig.letterboxPaddingValue = 0;

    QCImageProps_t imgProp1;
    imgProp1.batchSize = 1;
    imgProp1.width = CL2DFlexConfig.inputWidths[0];
    imgProp1.height = CL2DFlexConfig.inputHeights[0];
    imgProp1.format = CL2DFlexConfig.inputFormats[0];
    if ( QC_IMAGE_FORMAT_NV12 == CL2DFlexConfig.inputFormats[0] )
    {
        imgProp1.stride[0] = CL2DFlexConfig.inputWidths[0];
        imgProp1.stride[1] = CL2DFlexConfig.inputWidths[0];
        imgProp1.actualHeight[0] = CL2DFlexConfig.inputHeights[0];
        imgProp1.actualHeight[1] = CL2DFlexConfig.inputHeights[0] / 2;
        imgProp1.planeBufSize[0] = 0;
        imgProp1.planeBufSize[1] = 0;
        imgProp1.numPlanes = 2;
    }
    else if ( QC_IMAGE_FORMAT_UYVY == CL2DFlexConfig.inputFormats[0] )
    {
        imgProp1.stride[0] = CL2DFlexConfig.inputWidths[0] * 2;
        imgProp1.actualHeight[0] = CL2DFlexConfig.inputHeights[0];
        imgProp1.planeBufSize[0] = 0;
        imgProp1.numPlanes = 1;
    }
    else if ( QC_IMAGE_FORMAT_RGB888 == CL2DFlexConfig.inputFormats[0] )
    {
        imgProp1.stride[0] = CL2DFlexConfig.inputWidths[0] * 3;
        imgProp1.actualHeight[0] = CL2DFlexConfig.inputHeights[0];
        imgProp1.planeBufSize[0] = 0;
        imgProp1.numPlanes = 1;
    }

    QCSharedBuffer_t input;
    ret = input.Allocate( &imgProp1 );
    ASSERT_EQ( QC_STATUS_OK, ret );

    if ( ( inputWidthTest == 1920 ) && ( inputHeightTest == 1024 ) &&
         ( inputFormatTest == QC_IMAGE_FORMAT_NV12 ) && ( true == checkAccuracy ) )
    {
        std::string pathTest = "./data/test/CL2DFlex/0.nv12";
        FILE *file1 = nullptr;
        size_t length1 = 0;
        file1 = fopen( pathTest.c_str(), "rb" );
        if ( nullptr == file1 )
        {
            printf( "could not open image file %s\n", pathTest.c_str() );
        }
        else
        {
            fseek( file1, 0, SEEK_END );
            length1 = (size_t) ftell( file1 );
            if ( input.size != length1 )
            {
                printf( "image file %s size not match, need %d but got %d\n", pathTest.c_str(),
                        (int) input.size, (int) length1 );
            }
            else
            {
                fseek( file1, 0, SEEK_SET );
                auto r = fread( input.data(), 1, length1, file1 );
                if ( length1 != r )
                {
                    printf( "failed to read image file %s, need %d but read %d\n", pathTest.c_str(),
                            (int) length1, (int) r );
                }
            }
            fclose( file1 );
        }
    }

    QCImageProps_t imgProp2;
    imgProp2.batchSize = numberTest;
    imgProp2.width = CL2DFlexConfig.outputWidth;
    imgProp2.height = CL2DFlexConfig.outputHeight;
    imgProp2.format = CL2DFlexConfig.outputFormat;
    if ( ( QC_IMAGE_FORMAT_RGB888 == CL2DFlexConfig.outputFormat ) ||
         ( QC_IMAGE_FORMAT_BGR888 == CL2DFlexConfig.outputFormat ) )
    {
        imgProp2.stride[0] = CL2DFlexConfig.outputWidth * 3;
        imgProp2.actualHeight[0] = CL2DFlexConfig.outputHeight;
        imgProp2.planeBufSize[0] = 0;
        imgProp2.numPlanes = 1;
    }
    else if ( QC_IMAGE_FORMAT_NV12 == CL2DFlexConfig.outputFormat )
    {
        imgProp2.stride[0] = CL2DFlexConfig.outputWidth;
        imgProp2.stride[1] = CL2DFlexConfig.outputWidth;
        imgProp2.actualHeight[0] = CL2DFlexConfig.outputHeight;
        imgProp2.actualHeight[1] = CL2DFlexConfig.outputHeight / 2;
        imgProp2.planeBufSize[0] = 0;
        imgProp2.planeBufSize[1] = 0;
        imgProp2.numPlanes = 2;
    }

    QCSharedBuffer_t output;
    ret = output.Allocate( &imgProp2 );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = CL2DFlexObj.Init( pName, &CL2DFlexConfig );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = CL2DFlexObj.RegisterBuffers( &input, 1 );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = CL2DFlexObj.RegisterBuffers( &output, 1 );
    ASSERT_EQ( QC_STATUS_OK, ret );

    auto start = std::chrono::high_resolution_clock::now();
    for ( int i = 0; i < times; i++ )
    {
        ret = CL2DFlexObj.ExecuteWithROI( &input, &output, pROITest, numberTest );
    }
    auto end = std::chrono::high_resolution_clock::now();
    double duration_ms = std::chrono::duration<double, std::milli>( end - start ).count();
    printf( "number of ROI = %d, %d*%d to %d*%d, execute time = %f ms\n", numberTest,
            inputWidthTest, inputHeightTest, outputWidthTest, outputHeightTest,
            (float) duration_ms / (float) times );
    ASSERT_EQ( QC_STATUS_OK, ret );

    if ( ( inputWidthTest == 1920 ) && ( inputHeightTest == 1024 ) &&
         ( inputFormatTest == QC_IMAGE_FORMAT_NV12 ) && ( true == checkAccuracy ) )
    {
        uint32_t sizeOne = output.size / output.imgProps.batchSize;
        for ( uint32_t i = 0; i < output.imgProps.batchSize; i++ )
        {
            std::string path = "/tmp/ROI_test_" + std::to_string( i ) + ".raw";
            uint8_t *ptr = (uint8_t *) output.data() + sizeOne * i;
            FILE *fp = fopen( path.c_str(), "wb" );
            if ( nullptr != fp )
            {
                fwrite( ptr, sizeOne, 1, fp );
                fclose( fp );
            }
            else
            {
                printf( "failed to create file: %s", path.c_str() );
            }
        }
    }

    ret = CL2DFlexObj.DeRegisterBuffers( &input, 1 );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = CL2DFlexObj.DeRegisterBuffers( &output, 1 );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = CL2DFlexObj.Deinit();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = input.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = output.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );

    return;
}

void AccuracyTest( CL2DFlex_Work_Mode_e modeTest, QCImageFormat_e inputFormatTest,
                   QCImageFormat_e outputFormatTest, uint32_t inputWidthTest,
                   uint32_t inputHeightTest, uint32_t outputWidthTest, uint32_t outputHeightTest,
                   std::string pathTest, std::string goldenPath, bool saveOutput )
{
    QCStatus_e ret = QC_STATUS_OK;

    CL2DFlex CL2DFlexObj;
    CL2DFlex_Config_t CL2DFlexConfig;
    char pName[20] = "CL2DFlex";

    CL2DFlexConfig.numOfInputs = 1;
    for ( int i = 0; i < CL2DFlexConfig.numOfInputs; i++ )
    {
        CL2DFlexConfig.workModes[i] = modeTest;
        CL2DFlexConfig.inputWidths[i] = inputWidthTest;
        CL2DFlexConfig.inputHeights[i] = inputHeightTest;
        CL2DFlexConfig.inputFormats[i] = inputFormatTest;
        CL2DFlexConfig.ROIs[i].x = 0;
        CL2DFlexConfig.ROIs[i].y = 0;
        CL2DFlexConfig.ROIs[i].width = inputWidthTest;
        CL2DFlexConfig.ROIs[i].height = inputHeightTest;
    }
    CL2DFlexConfig.outputWidth = outputWidthTest;
    CL2DFlexConfig.outputHeight = outputHeightTest;
    CL2DFlexConfig.outputFormat = outputFormatTest;

    QCSharedBuffer_t mapXBuffer;
    QCSharedBuffer_t mapYBuffer;
    for ( int i = 0; i < CL2DFlexConfig.numOfInputs; i++ )
    {
        if ( CL2DFLEX_WORK_MODE_REMAP_NEAREST == CL2DFlexConfig.workModes[i] )
        {
            QCTensorProps_t mapXProp = {
                    QC_TENSOR_TYPE_FLOAT_32,
                    { CL2DFlexConfig.outputHeight * CL2DFlexConfig.outputWidth, 0 },
                    1,
            };
            ret = mapXBuffer.Allocate( &mapXProp );
            ASSERT_EQ( QC_STATUS_OK, ret );
            ret = LoadFile( mapXBuffer, "./data/test/CL2DFlex/mapX.raw" );
            ASSERT_EQ( QC_STATUS_OK, ret );
            CL2DFlexConfig.remapTable[i].pMapX = &mapXBuffer;

            QCTensorProps_t mapYProp = {
                    QC_TENSOR_TYPE_FLOAT_32,
                    { CL2DFlexConfig.outputHeight * CL2DFlexConfig.outputWidth, 0 },
                    1,
            };
            ret = mapYBuffer.Allocate( &mapYProp );
            ASSERT_EQ( QC_STATUS_OK, ret );
            ret = LoadFile( mapYBuffer, "./data/test/CL2DFlex/mapY.raw" );
            ASSERT_EQ( QC_STATUS_OK, ret );
            CL2DFlexConfig.remapTable[i].pMapY = &mapYBuffer;
        }
    }

    QCSharedBuffer_t inputs[CL2DFlexConfig.numOfInputs];
    for ( int i = 0; i < CL2DFlexConfig.numOfInputs; i++ )
    {
        QCImageProps_t imgProp1;
        imgProp1.batchSize = 1;
        imgProp1.width = CL2DFlexConfig.inputWidths[i];
        imgProp1.height = CL2DFlexConfig.inputHeights[i];
        imgProp1.format = CL2DFlexConfig.inputFormats[i];
        if ( QC_IMAGE_FORMAT_NV12 == CL2DFlexConfig.inputFormats[i] )
        {
            imgProp1.stride[0] = CL2DFlexConfig.inputWidths[i];
            imgProp1.stride[1] = CL2DFlexConfig.inputWidths[i];
            imgProp1.actualHeight[0] = CL2DFlexConfig.inputHeights[i];
            imgProp1.actualHeight[1] = CL2DFlexConfig.inputHeights[i] / 2;
            imgProp1.planeBufSize[0] = 0;
            imgProp1.planeBufSize[1] = 0;
            imgProp1.numPlanes = 2;
        }
        else if ( QC_IMAGE_FORMAT_UYVY == CL2DFlexConfig.inputFormats[i] )
        {
            imgProp1.stride[0] = CL2DFlexConfig.inputWidths[i] * 2;
            imgProp1.actualHeight[0] = CL2DFlexConfig.inputHeights[i];
            imgProp1.planeBufSize[0] = 0;
            imgProp1.numPlanes = 1;
        }
        else if ( QC_IMAGE_FORMAT_RGB888 == CL2DFlexConfig.inputFormats[i] )
        {
            imgProp1.stride[0] = CL2DFlexConfig.inputWidths[i] * 3;
            imgProp1.actualHeight[0] = CL2DFlexConfig.inputHeights[i];
            imgProp1.planeBufSize[0] = 0;
            imgProp1.numPlanes = 1;
        }

        QCSharedBuffer_t input;
        if ( QC_IMAGE_FORMAT_NV12_UBWC == CL2DFlexConfig.inputFormats[i] )
        {
            ret = input.Allocate( CL2DFlexConfig.inputWidths[i], CL2DFlexConfig.inputHeights[i],
                                  CL2DFlexConfig.inputFormats[i] );
        }
        else
        {
            ret = input.Allocate( &imgProp1 );
        }
        ASSERT_EQ( QC_STATUS_OK, ret );
        memset( input.data(), 0, input.size );

        FILE *file1 = nullptr;
        size_t length1 = 0;
        file1 = fopen( pathTest.c_str(), "rb" );
        if ( nullptr == file1 )
        {
            printf( "could not open image file %s\n", pathTest.c_str() );
        }
        else
        {
            fseek( file1, 0, SEEK_END );
            length1 = (size_t) ftell( file1 );
            if ( input.size != length1 )
            {
                printf( "image file %s size not match, need %d but got %d\n", pathTest.c_str(),
                        (int) input.size, (int) length1 );
            }
            else
            {
                fseek( file1, 0, SEEK_SET );
                auto r = fread( input.data(), 1, length1, file1 );
                if ( length1 != r )
                {
                    printf( "failed to read image file %s, need %d but read %d\n", pathTest.c_str(),
                            (int) length1, (int) r );
                }
            }
            fclose( file1 );
        }

        inputs[i] = input;
    }

    QCImageProps_t imgProp2;
    imgProp2.batchSize = CL2DFlexConfig.numOfInputs;
    imgProp2.width = CL2DFlexConfig.outputWidth;
    imgProp2.height = CL2DFlexConfig.outputHeight;
    imgProp2.format = CL2DFlexConfig.outputFormat;
    if ( ( QC_IMAGE_FORMAT_RGB888 == CL2DFlexConfig.outputFormat ) ||
         ( QC_IMAGE_FORMAT_BGR888 == CL2DFlexConfig.outputFormat ) )
    {
        imgProp2.stride[0] = CL2DFlexConfig.outputWidth * 3;
        imgProp2.actualHeight[0] = CL2DFlexConfig.outputHeight;
        imgProp2.planeBufSize[0] = 0;
        imgProp2.numPlanes = 1;
    }
    else if ( QC_IMAGE_FORMAT_NV12 == CL2DFlexConfig.outputFormat )
    {
        imgProp2.stride[0] = CL2DFlexConfig.outputWidth;
        imgProp2.stride[1] = CL2DFlexConfig.outputWidth;
        imgProp2.actualHeight[0] = CL2DFlexConfig.outputHeight;
        imgProp2.actualHeight[1] = CL2DFlexConfig.outputHeight / 2;
        imgProp2.planeBufSize[0] = 0;
        imgProp2.planeBufSize[1] = 0;
        imgProp2.numPlanes = 2;
    }

    QCSharedBuffer_t output;
    ret = output.Allocate( &imgProp2 );
    ASSERT_EQ( QC_STATUS_OK, ret );
    memset( output.data(), 0, output.size );

    ret = CL2DFlexObj.Init( pName, &CL2DFlexConfig );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = CL2DFlexObj.Execute( inputs, CL2DFlexConfig.numOfInputs, &output );
    ASSERT_EQ( QC_STATUS_OK, ret );

    if ( true == saveOutput )
    {
        uint8_t *ptr = (uint8_t *) output.data();
        FILE *fp = fopen( goldenPath.c_str(), "wb" );
        if ( nullptr != fp )
        {
            fwrite( ptr, output.size, 1, fp );
            fclose( fp );
        }
    }

    QCSharedBuffer_t golden;
    ret = golden.Allocate( &imgProp2 );
    ASSERT_EQ( QC_STATUS_OK, ret );

    FILE *file2 = nullptr;
    size_t length2 = 0;
    file2 = fopen( goldenPath.c_str(), "rb" );
    if ( nullptr == file2 )
    {
        printf( "could not open golden file %s\n", goldenPath.c_str() );
    }
    else
    {
        fseek( file2, 0, SEEK_END );
        length2 = (size_t) ftell( file2 );
        if ( golden.size != length2 )
        {
            printf( "golden file %s size not match, need %d but got %d\n", goldenPath.c_str(),
                    (int) golden.size, (int) length2 );
        }
        else
        {
            fseek( file2, 0, SEEK_SET );
            auto r = fread( golden.data(), 1, length2, file2 );
            if ( length2 != r )
            {
                printf( "failed to read golden file %s, need %d but read %d\n", pathTest.c_str(),
                        (int) length2, (int) r );
            }
        }
        fclose( file2 );
    }

    std::string md5Output = MD5Sum( output.data(), output.size );
    printf( "output md5 = %s\n", md5Output.c_str() );
    std::string md5Golden = MD5Sum( golden.data(), output.size );
    printf( "golden md5 = %s\n", md5Golden.c_str() );

    if ( md5Output != md5Golden )   // check cosine similarity if md5 not match
    {
        size_t outputSize = output.size;
        uint8_t *outputData = (uint8_t *) output.data();
        uint8_t *goldenData = (uint8_t *) golden.data();
        float dot = 0.0;
        float norm1 = 1e-10;
        float norm2 = 1e-10;
        int miss = 0;
        for ( int i = 0; i < outputSize; i++ )
        {
            if ( outputData[i] != goldenData[i] )
            {
                miss++;
                if ( miss < 10 )
                {
                    printf( "data not match at i=%d, output=%d, golden=%d\n", i, outputData[i],
                            goldenData[i] );
                }
            }
            dot = dot + outputData[i] * goldenData[i];
            norm1 = norm1 + outputData[i] * outputData[i];
            norm2 = norm2 + goldenData[i] * goldenData[i];
        }
        float cos = dot / sqrt( norm1 * norm2 );
        printf( "cosine similarity = %f\n", cos );
        printf( "miss data number = %d\n", miss );
    }
    ASSERT_EQ( md5Output, md5Golden );

    ret = CL2DFlexObj.Deinit();
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( int i = 0; i < CL2DFlexConfig.numOfInputs; i++ )
    {
        ret = inputs[i].Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    ret = output.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );

    return;
}

void PerformanceTest( CL2DFlex_Work_Mode_e modeTest, QCImageFormat_e inputFormatTest,
                      QCImageFormat_e outputFormatTest, uint32_t inputWidthTest,
                      uint32_t inputHeightTest, uint32_t roiWidthTest, uint32_t roiHeightTest,
                      uint32_t outputWidthTest, uint32_t outputHeightTest, uint32_t times )
{
    QCStatus_e ret = QC_STATUS_OK;

    CL2DFlex CL2DFlexObj;
    CL2DFlex_Config_t CL2DFlexConfig;
    char pName[20] = "CL2DFlex";

    CL2DFlexConfig.numOfInputs = 1;
    for ( int i = 0; i < CL2DFlexConfig.numOfInputs; i++ )
    {
        CL2DFlexConfig.workModes[i] = modeTest;
        CL2DFlexConfig.inputWidths[i] = inputWidthTest;
        CL2DFlexConfig.inputHeights[i] = inputHeightTest;
        CL2DFlexConfig.inputFormats[i] = inputFormatTest;
        CL2DFlexConfig.ROIs[i].x = 0;
        CL2DFlexConfig.ROIs[i].y = 0;
        CL2DFlexConfig.ROIs[i].width = roiWidthTest;
        CL2DFlexConfig.ROIs[i].height = roiHeightTest;
    }
    CL2DFlexConfig.outputWidth = outputWidthTest;
    CL2DFlexConfig.outputHeight = outputHeightTest;
    CL2DFlexConfig.outputFormat = outputFormatTest;

    QCSharedBuffer_t mapXBuffer;
    QCSharedBuffer_t mapYBuffer;
    for ( int i = 0; i < CL2DFlexConfig.numOfInputs; i++ )
    {
        if ( CL2DFLEX_WORK_MODE_REMAP_NEAREST == CL2DFlexConfig.workModes[i] )
        {
            QCTensorProps_t mapXProp = {
                    QC_TENSOR_TYPE_FLOAT_32,
                    { CL2DFlexConfig.outputHeight * CL2DFlexConfig.outputWidth, 0 },
                    1,
            };
            ret = mapXBuffer.Allocate( &mapXProp );
            ASSERT_EQ( QC_STATUS_OK, ret );
            ret = LoadFile( mapXBuffer, "./data/test/CL2DFlex/mapX.raw" );
            ASSERT_EQ( QC_STATUS_OK, ret );
            CL2DFlexConfig.remapTable[i].pMapX = &mapXBuffer;

            QCTensorProps_t mapYProp = {
                    QC_TENSOR_TYPE_FLOAT_32,
                    { CL2DFlexConfig.outputHeight * CL2DFlexConfig.outputWidth, 0 },
                    1,
            };
            ret = mapYBuffer.Allocate( &mapYProp );
            ASSERT_EQ( QC_STATUS_OK, ret );
            ret = LoadFile( mapYBuffer, "./data/test/CL2DFlex/mapY.raw" );
            ASSERT_EQ( QC_STATUS_OK, ret );
            CL2DFlexConfig.remapTable[i].pMapY = &mapYBuffer;
        }
    }

    QCSharedBuffer_t inputs[CL2DFlexConfig.numOfInputs];
    for ( int i = 0; i < CL2DFlexConfig.numOfInputs; i++ )
    {
        QCImageProps_t imgProp1;
        imgProp1.batchSize = 1;
        imgProp1.width = CL2DFlexConfig.inputWidths[i];
        imgProp1.height = CL2DFlexConfig.inputHeights[i];
        imgProp1.format = CL2DFlexConfig.inputFormats[i];
        if ( QC_IMAGE_FORMAT_NV12 == CL2DFlexConfig.inputFormats[i] )
        {
            imgProp1.stride[0] = CL2DFlexConfig.inputWidths[i];
            imgProp1.stride[1] = CL2DFlexConfig.inputWidths[i];
            imgProp1.actualHeight[0] = CL2DFlexConfig.inputHeights[i];
            imgProp1.actualHeight[1] = CL2DFlexConfig.inputHeights[i] / 2;
            imgProp1.planeBufSize[0] = 0;
            imgProp1.planeBufSize[1] = 0;
            imgProp1.numPlanes = 2;
        }
        else if ( QC_IMAGE_FORMAT_UYVY == CL2DFlexConfig.inputFormats[i] )
        {
            imgProp1.stride[0] = CL2DFlexConfig.inputWidths[i] * 2;
            imgProp1.actualHeight[0] = CL2DFlexConfig.inputHeights[i];
            imgProp1.planeBufSize[0] = 0;
            imgProp1.numPlanes = 1;
        }
        else if ( QC_IMAGE_FORMAT_RGB888 == CL2DFlexConfig.inputFormats[i] )
        {
            imgProp1.stride[0] = CL2DFlexConfig.inputWidths[i] * 3;
            imgProp1.actualHeight[0] = CL2DFlexConfig.inputHeights[i];
            imgProp1.planeBufSize[0] = 0;
            imgProp1.numPlanes = 1;
        }

        QCSharedBuffer_t input;
        if ( QC_IMAGE_FORMAT_NV12_UBWC == CL2DFlexConfig.inputFormats[i] )
        {
            ret = input.Allocate( CL2DFlexConfig.inputWidths[i], CL2DFlexConfig.inputHeights[i],
                                  CL2DFlexConfig.inputFormats[i] );
        }
        else
        {
            ret = input.Allocate( &imgProp1 );
        }
        ASSERT_EQ( QC_STATUS_OK, ret );
        inputs[i] = input;
    }

    QCImageProps_t imgProp2;
    imgProp2.batchSize = CL2DFlexConfig.numOfInputs;
    imgProp2.width = CL2DFlexConfig.outputWidth;
    imgProp2.height = CL2DFlexConfig.outputHeight;
    imgProp2.format = CL2DFlexConfig.outputFormat;
    if ( ( QC_IMAGE_FORMAT_RGB888 == CL2DFlexConfig.outputFormat ) ||
         ( QC_IMAGE_FORMAT_BGR888 == CL2DFlexConfig.outputFormat ) )
    {
        imgProp2.stride[0] = CL2DFlexConfig.outputWidth * 3;
        imgProp2.actualHeight[0] = CL2DFlexConfig.outputHeight;
        imgProp2.planeBufSize[0] = 0;
        imgProp2.numPlanes = 1;
    }
    else if ( QC_IMAGE_FORMAT_NV12 == CL2DFlexConfig.outputFormat )
    {
        imgProp2.stride[0] = CL2DFlexConfig.outputWidth;
        imgProp2.stride[1] = CL2DFlexConfig.outputWidth;
        imgProp2.actualHeight[0] = CL2DFlexConfig.outputHeight;
        imgProp2.actualHeight[1] = CL2DFlexConfig.outputHeight / 2;
        imgProp2.planeBufSize[0] = 0;
        imgProp2.planeBufSize[1] = 0;
        imgProp2.numPlanes = 2;
    }

    QCSharedBuffer_t output;
    ret = output.Allocate( &imgProp2 );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = CL2DFlexObj.Init( pName, &CL2DFlexConfig );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = CL2DFlexObj.RegisterBuffers( inputs, CL2DFlexConfig.numOfInputs );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = CL2DFlexObj.RegisterBuffers( &output, 1 );
    ASSERT_EQ( QC_STATUS_OK, ret );

    auto start = std::chrono::high_resolution_clock::now();
    for ( int i = 0; i < times; i++ )
    {
        ret = CL2DFlexObj.Execute( inputs, CL2DFlexConfig.numOfInputs, &output );
    }
    auto end = std::chrono::high_resolution_clock::now();
    double duration_ms = std::chrono::duration<double, std::milli>( end - start ).count();
    printf( "%d*%d to %d*%d, execute time = %f ms\n", inputWidthTest, inputHeightTest,
            outputWidthTest, outputHeightTest, (float) duration_ms / (float) times );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = CL2DFlexObj.DeRegisterBuffers( inputs, CL2DFlexConfig.numOfInputs );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = CL2DFlexObj.DeRegisterBuffers( &output, 1 );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = CL2DFlexObj.Deinit();
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( int i = 0; i < CL2DFlexConfig.numOfInputs; i++ )
    {
        ret = inputs[i].Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    ret = output.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );

    return;
}

void SanityTest()
{
    QCStatus_e ret = QC_STATUS_OK;

    CL2DFlex CL2DFlexObj;
    CL2DFlex_Config_t CL2DFlexConfig;
    char pName[20] = "CL2DFlex";
    CL2DFlexConfig.numOfInputs = 2;
    for ( int i = 0; i < CL2DFlexConfig.numOfInputs; i++ )
    {
        CL2DFlexConfig.workModes[i] = CL2DFLEX_WORK_MODE_RESIZE_NEAREST;
        CL2DFlexConfig.inputWidths[i] = 128;
        CL2DFlexConfig.inputHeights[i] = 128;
        CL2DFlexConfig.inputFormats[i] = QC_IMAGE_FORMAT_NV12;
        CL2DFlexConfig.ROIs[i].x = 64;
        CL2DFlexConfig.ROIs[i].y = 64;
        CL2DFlexConfig.ROIs[i].width = 64;
        CL2DFlexConfig.ROIs[i].height = 64;
    }
    CL2DFlexConfig.outputWidth = 64;
    CL2DFlexConfig.outputHeight = 64;
    CL2DFlexConfig.outputFormat = QC_IMAGE_FORMAT_RGB888;

    QCSharedBuffer_t inputs[CL2DFlexConfig.numOfInputs];
    for ( int i = 0; i < CL2DFlexConfig.numOfInputs; i++ )
    {
        QCImageProps_t imgProp1;
        imgProp1.batchSize = 1;
        imgProp1.width = CL2DFlexConfig.inputWidths[i];
        imgProp1.height = CL2DFlexConfig.inputHeights[i];
        imgProp1.format = CL2DFlexConfig.inputFormats[i];
        if ( QC_IMAGE_FORMAT_NV12 == CL2DFlexConfig.inputFormats[i] )
        {
            imgProp1.stride[0] = CL2DFlexConfig.inputWidths[i];
            imgProp1.stride[1] = CL2DFlexConfig.inputWidths[i];
            imgProp1.actualHeight[0] = CL2DFlexConfig.inputHeights[i];
            imgProp1.actualHeight[1] = CL2DFlexConfig.inputHeights[i] / 2;
            imgProp1.planeBufSize[0] = 0;
            imgProp1.planeBufSize[1] = 0;
            imgProp1.numPlanes = 2;
        }
        else if ( QC_IMAGE_FORMAT_UYVY == CL2DFlexConfig.inputFormats[i] )
        {
            imgProp1.stride[0] = CL2DFlexConfig.inputWidths[i] * 2;
            imgProp1.actualHeight[0] = CL2DFlexConfig.inputHeights[i];
            imgProp1.planeBufSize[0] = 0;
            imgProp1.numPlanes = 1;
        }
        else if ( QC_IMAGE_FORMAT_RGB888 == CL2DFlexConfig.inputFormats[i] )
        {
            imgProp1.stride[0] = CL2DFlexConfig.inputWidths[i] * 3;
            imgProp1.actualHeight[0] = CL2DFlexConfig.inputHeights[i];
            imgProp1.planeBufSize[0] = 0;
            imgProp1.numPlanes = 1;
        }

        QCSharedBuffer_t input;
        ret = input.Allocate( &imgProp1 );
        ASSERT_EQ( QC_STATUS_OK, ret );
        inputs[i] = input;
    }

    QCImageProps_t imgProp2;
    imgProp2.batchSize = CL2DFlexConfig.numOfInputs;
    imgProp2.width = CL2DFlexConfig.outputWidth;
    imgProp2.height = CL2DFlexConfig.outputHeight;
    imgProp2.format = CL2DFlexConfig.outputFormat;
    if ( ( QC_IMAGE_FORMAT_RGB888 == CL2DFlexConfig.outputFormat ) ||
         ( QC_IMAGE_FORMAT_BGR888 == CL2DFlexConfig.outputFormat ) )
    {
        imgProp2.stride[0] = CL2DFlexConfig.outputWidth * 3;
        imgProp2.actualHeight[0] = CL2DFlexConfig.outputHeight;
        imgProp2.planeBufSize[0] = 0;
        imgProp2.numPlanes = 1;
    }
    else if ( QC_IMAGE_FORMAT_NV12 == CL2DFlexConfig.outputFormat )
    {
        imgProp2.stride[0] = CL2DFlexConfig.outputWidth;
        imgProp2.stride[1] = CL2DFlexConfig.outputWidth;
        imgProp2.actualHeight[0] = CL2DFlexConfig.outputHeight;
        imgProp2.actualHeight[1] = CL2DFlexConfig.outputHeight / 2;
        imgProp2.planeBufSize[0] = 0;
        imgProp2.planeBufSize[1] = 0;
        imgProp2.numPlanes = 2;
    }

    QCSharedBuffer_t output;
    ret = output.Allocate( &imgProp2 );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = CL2DFlexObj.Init( pName, &CL2DFlexConfig );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = CL2DFlexObj.Start();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = CL2DFlexObj.RegisterBuffers( inputs, CL2DFlexConfig.numOfInputs );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = CL2DFlexObj.RegisterBuffers( &output, 1 );
    ASSERT_EQ( QC_STATUS_OK, ret );

    CL2DFlexConfig.ROIs[0].x = 0;
    CL2DFlexConfig.ROIs[0].y = 0;
    CL2DFlexConfig.ROIs[0].width = 64;
    CL2DFlexConfig.ROIs[0].height = 64;
    ret = CL2DFlexObj.Execute( inputs, CL2DFlexConfig.numOfInputs, &output );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = CL2DFlexObj.DeRegisterBuffers( inputs, CL2DFlexConfig.numOfInputs );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = CL2DFlexObj.DeRegisterBuffers( &output, 1 );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = CL2DFlexObj.Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = CL2DFlexObj.Deinit();
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( int i = 0; i < CL2DFlexConfig.numOfInputs; i++ )
    {
        ret = inputs[i].Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    ret = output.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );

    return;
}

void CoverageTest()
{
    QCStatus_e ret = QC_STATUS_OK;

    CL2DFlex CL2DFlexObj;
    CL2DFlex_Config_t CL2DFlexConfig;
    char pName[20] = "CL2DFlex";
    CL2DFlexConfig.numOfInputs = 1;
    CL2DFlexConfig.workModes[0] = CL2DFLEX_WORK_MODE_RESIZE_NEAREST;
    CL2DFlexConfig.inputWidths[0] = 128;
    CL2DFlexConfig.inputHeights[0] = 128;
    CL2DFlexConfig.inputFormats[0] = QC_IMAGE_FORMAT_NV12;
    CL2DFlexConfig.outputWidth = 128;
    CL2DFlexConfig.outputHeight = 128;
    CL2DFlexConfig.outputFormat = QC_IMAGE_FORMAT_RGB888;
    CL2DFlexConfig.ROIs[0].x = 0;
    CL2DFlexConfig.ROIs[0].y = 0;
    CL2DFlexConfig.ROIs[0].width = 128;
    CL2DFlexConfig.ROIs[0].height = 128;
    QCSharedBuffer_t input;
    QCSharedBuffer_t output;

    ret = CL2DFlexObj.Start();
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );   // start before init

    ret = CL2DFlexObj.Stop();
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );   // stop before init

    ret = CL2DFlexObj.Deinit();
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );   // deinit before init

    ret = CL2DFlexObj.RegisterBuffers( &output, 1 );   // register buffer before init
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

    ret = CL2DFlexObj.DeRegisterBuffers( &output, 1 );   // deregister buffer before init
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

    ret = CL2DFlexObj.Execute( &input, 1, &output );   // execute before init
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

    ret = CL2DFlexObj.Init( pName, &CL2DFlexConfig );   // success init
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = CL2DFlexObj.Init( pName, &CL2DFlexConfig );   // init twice, wrong status
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

    ret = CL2DFlexObj.Deinit();   // success deinit
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = CL2DFlexObj.Init( pName, nullptr );   // null pointer for CL2DFlex configuration
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    CL2DFlexConfig.inputWidths[0] = 1;
    ret = CL2DFlexObj.Init( pName, &CL2DFlexConfig );   // wrong input width
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    CL2DFlexConfig.inputWidths[0] = 128;

    CL2DFlexConfig.inputHeights[0] = 1;
    ret = CL2DFlexObj.Init( pName, &CL2DFlexConfig );   // wrong input height
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    CL2DFlexConfig.inputHeights[0] = 128;

    CL2DFlexConfig.inputFormats[0] = QC_IMAGE_FORMAT_MAX;
    ret = CL2DFlexObj.Init( pName, &CL2DFlexConfig );   // wrong input format
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    CL2DFlexConfig.inputFormats[0] = QC_IMAGE_FORMAT_NV12;

    CL2DFlexConfig.outputFormat = QC_IMAGE_FORMAT_MAX;
    ret = CL2DFlexObj.Init( pName, &CL2DFlexConfig );   // wrong output format
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    CL2DFlexConfig.outputFormat = QC_IMAGE_FORMAT_RGB888;

    CL2DFlexConfig.ROIs[0].x = 1;
    ret = CL2DFlexObj.Init( pName, &CL2DFlexConfig );   // wrong roi.x
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    CL2DFlexConfig.ROIs[0].x = 0;

    CL2DFlexConfig.ROIs[0].y = 1;
    ret = CL2DFlexObj.Init( pName, &CL2DFlexConfig );   // wrong roi.y
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    CL2DFlexConfig.ROIs[0].y = 0;

    CL2DFlexConfig.workModes[0] = CL2DFLEX_WORK_MODE_MAX;
    ret = CL2DFlexObj.Init( pName, &CL2DFlexConfig );   // wrong work mode
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    CL2DFlexConfig.workModes[0] = CL2DFLEX_WORK_MODE_RESIZE_NEAREST;

    ret = CL2DFlexObj.Init( pName, &CL2DFlexConfig,
                            LOGGER_LEVEL_MAX );   // success init with invalid logger level
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = CL2DFlexObj.RegisterBuffers( nullptr, 1 );   // null pointer for buffer to be register
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    ret = CL2DFlexObj.DeRegisterBuffers( nullptr, 1 );   // null pointer for buffer to be deregister
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    ret = CL2DFlexObj.Execute( nullptr, 1,
                               &output );   // null pointer for input buffer
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    ret = CL2DFlexObj.Execute( &input, 1,
                               nullptr );   // null pointer for output buffer
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    ret = CL2DFlexObj.ExecuteWithROI( nullptr, &output, CL2DFlexConfig.ROIs,
                                      1 );   // null pointer for input buffer
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    ret = CL2DFlexObj.ExecuteWithROI( &input, nullptr, CL2DFlexConfig.ROIs,
                                      1 );   // null pointer for output buffer
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    ret = CL2DFlexObj.Deinit();   // success deinit
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = CL2DFlexObj.Init( pName, &CL2DFlexConfig );   // success init
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = input.Allocate( CL2DFlexConfig.inputWidths[0], CL2DFlexConfig.inputHeights[0],
                          CL2DFlexConfig.inputFormats[0] );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = output.Allocate( CL2DFlexConfig.outputWidth, CL2DFlexConfig.outputHeight,
                           CL2DFlexConfig.outputFormat );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = CL2DFlexObj.RegisterBuffers( &input, 1 );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = CL2DFlexObj.RegisterBuffers( &input, 1 );   // register twice
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = CL2DFlexObj.DeRegisterBuffers( &input, 1 );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = CL2DFlexObj.DeRegisterBuffers( &input, 1 );   // deregister twice
    ASSERT_EQ( QC_STATUS_OK, ret );

    input.type = QC_BUFFER_TYPE_TENSOR;
    ret = CL2DFlexObj.Execute( &input, 1,
                               &output );   // execute with wrong input buffer type
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    ret = CL2DFlexObj.ExecuteWithROI( &input, &output, CL2DFlexConfig.ROIs,
                                      1 );   // execute with wrong input buffer type
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    input.type = QC_BUFFER_TYPE_IMAGE;

    output.type = QC_BUFFER_TYPE_TENSOR;
    ret = CL2DFlexObj.Execute( &input, 1,
                               &output );   // execute with wrong output buffer type
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    ret = CL2DFlexObj.ExecuteWithROI( &input, &output, CL2DFlexConfig.ROIs,
                                      1 );   // execute with wrong output buffer type
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    output.type = QC_BUFFER_TYPE_IMAGE;

    input.imgProps.format = QC_IMAGE_FORMAT_RGB888;
    ret = CL2DFlexObj.Execute( &input, 1,
                               &output );   // execute with wrong input image format
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    ret = CL2DFlexObj.ExecuteWithROI( &input, &output, CL2DFlexConfig.ROIs,
                                      1 );   // execute with wrong input image format
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    input.imgProps.format = QC_IMAGE_FORMAT_NV12;

    input.imgProps.width = input.imgProps.width + 1;
    ret = CL2DFlexObj.Execute( &input, 1,
                               &output );   // execute with wrong input image width
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    ret = CL2DFlexObj.ExecuteWithROI( &input, &output, CL2DFlexConfig.ROIs,
                                      1 );   // execute with wrong input image width
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    input.imgProps.width = input.imgProps.width - 1;

    input.imgProps.height = input.imgProps.height + 1;
    ret = CL2DFlexObj.Execute( &input, 1,
                               &output );   // execute with wrong input image height
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    ret = CL2DFlexObj.ExecuteWithROI( &input, &output, CL2DFlexConfig.ROIs,
                                      1 );   // execute with wrong input image height
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    input.imgProps.height = input.imgProps.height - 1;

    output.imgProps.format = QC_IMAGE_FORMAT_NV12;
    ret = CL2DFlexObj.Execute( &input, 1,
                               &output );   // execute with wrong output image format
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    ret = CL2DFlexObj.ExecuteWithROI( &input, &output, CL2DFlexConfig.ROIs,
                                      1 );   // execute with wrong output image format
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    output.imgProps.format = QC_IMAGE_FORMAT_RGB888;

    output.imgProps.width = output.imgProps.width + 1;
    ret = CL2DFlexObj.Execute( &input, 1,
                               &output );   // execute with wrong output image width
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    ret = CL2DFlexObj.ExecuteWithROI( &input, &output, CL2DFlexConfig.ROIs,
                                      1 );   // execute with wrong output image width
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    output.imgProps.width = output.imgProps.width - 1;

    output.imgProps.height = output.imgProps.height + 1;
    ret = CL2DFlexObj.Execute( &input, 1,
                               &output );   // execute with wrong output image height
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    ret = CL2DFlexObj.ExecuteWithROI( &input, &output, CL2DFlexConfig.ROIs,
                                      1 );   // execute with wrong output image height
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    output.imgProps.height = output.imgProps.height - 1;

    ret = CL2DFlexObj.ExecuteWithROI( &input, &output, CL2DFlexConfig.ROIs,
                                      2 );   // execute with roi number
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    ret = CL2DFlexObj.ExecuteWithROI( &input, &output, CL2DFlexConfig.ROIs,
                                      1 );   // execute with wrong pipeline
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    ret = CL2DFlexObj.RegisterBuffers( &input, 1 );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = CL2DFlexObj.Deinit();   // success deinit with registered buffer
    ASSERT_EQ( QC_STATUS_OK, ret );

    CL2DFlexConfig.workModes[0] = CL2DFLEX_WORK_MODE_LETTERBOX_NEAREST_MULTIPLE;
    ret = CL2DFlexObj.Init( pName, &CL2DFlexConfig );   // success init
    ASSERT_EQ( QC_STATUS_OK, ret );

    CL2DFlexConfig.ROIs[0].height = 128 + 1;
    ret = CL2DFlexObj.ExecuteWithROI( &input, &output, CL2DFlexConfig.ROIs,
                                      1 );   // execute with wrong roi height
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    CL2DFlexConfig.ROIs[0].height = 128;

    CL2DFlexConfig.ROIs[0].width = 128 + 1;
    ret = CL2DFlexObj.ExecuteWithROI( &input, &output, CL2DFlexConfig.ROIs,
                                      1 );   // execute with wrong roi width
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    CL2DFlexConfig.ROIs[0].width = 128;

    ret = CL2DFlexObj.Deinit();   // success deinit
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = input.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = output.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );

    OpenclSrv OpenclSrvObj;
    ret = OpenclSrvObj.Init( pName, LOGGER_LEVEL_ERROR,
                             OPENCLIFACE_PERF_LOW );   // success init OpenclSrv with low priority
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = OpenclSrvObj.LoadFromSource( "" );   // create program with null source
    ASSERT_EQ( QC_STATUS_FAIL, ret );

    cl_kernel kernel;
    ret = OpenclSrvObj.CreateKernel( &kernel,
                                     "" );   // create program with null kernel
    ASSERT_EQ( QC_STATUS_FAIL, ret );

    ret = OpenclSrvObj.LoadFromBinary(
            (const unsigned char *) "" );   // create program with null binary
    ASSERT_EQ( QC_STATUS_FAIL, ret );

    ret = OpenclSrvObj.RegBuf( (QCBuffer_t *) nullptr,
                               nullptr );   // register buffer with null host buffer
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    ret = OpenclSrvObj.DeregBuf(
            (QCBuffer_t *) nullptr );   // deregister buffer with null null host buffer
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    ret = OpenclSrvObj.RegImage(
            nullptr, 0, (cl_mem *) nullptr, (cl_image_format *) nullptr,
            (cl_image_desc *) nullptr );   // register image with null host buffer
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    ret = OpenclSrvObj.RegPlane(
            nullptr, (cl_mem *) nullptr, (cl_image_format *) nullptr,
            (cl_image_desc *) nullptr );   // register plane with null host buffer
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    OpenclIfcae_Arg_t OpenclArg;
    OpenclArg.pArg = nullptr;
    OpenclArg.argSize = 0;
    OpenclIface_WorkParams_t OpenclWorkParams;
    ret = OpenclSrvObj.Execute( &kernel, &OpenclArg, 1,
                                &OpenclWorkParams );   // execute with null args
    ASSERT_EQ( QC_STATUS_FAIL, ret );

    int arg = 1;
    OpenclArg.pArg = &arg;
    OpenclArg.argSize = sizeof( cl_int );
    OpenclWorkParams.workDim = 0;
    OpenclWorkParams.pGlobalWorkSize = nullptr;
    OpenclWorkParams.pGlobalWorkOffset = nullptr;
    OpenclWorkParams.pLocalWorkSize = nullptr;
    ret = OpenclSrvObj.Execute( &kernel, &OpenclArg, 1,
                                &OpenclWorkParams );   // execute with null work params
    ASSERT_EQ( QC_STATUS_FAIL, ret );

    ret = OpenclSrvObj.Deinit();   // success deinit
    ASSERT_EQ( QC_STATUS_OK, ret );

    cl_mem *clMem;
    ret = OpenclSrvObj.RegBuf( &( input.buffer ), clMem );   // register without init
    ASSERT_EQ( QC_STATUS_FAIL, ret );

    return;
}

TEST( CL2DFlex, SanityTest )
{
    SanityTest();
}

TEST( CL2DFlex, CoverageTest )
{
    CoverageTest();
}

TEST( CL2DFlex, ConvertAccuracyTest )
{
    // md5 of 0.nv12 is a1591f4b8c196a47628f0ef6bc3a721c
    // md5 of 0.uyvy is 5b1ae2203a9d97aeafe65e997f3beebc
    // md5 of golden1.rgb is 4b528d54b7c5d5164f66719a89f988be
    // md5 of golden2.rgb is f94a6aeae302add0424821660bcd2684
    // md5 of golden3.nv12 is 91ed68589443b87bcfff8ae7e69b03b2
    // md5 of 0.ubwc is ce5f81f72f9c0ec0c347b1ee55d13584
    // md5 of golden8.nv12 is 43d33b3417b0b53c594269e5df95e035
    AccuracyTest( CL2DFLEX_WORK_MODE_CONVERT, QC_IMAGE_FORMAT_NV12, QC_IMAGE_FORMAT_RGB888, 1920,
                  1024, 1920, 1024, "./data/test/CL2DFlex/0.nv12",
                  "./data/test/CL2DFlex/golden1.rgb", false );
    AccuracyTest( CL2DFLEX_WORK_MODE_CONVERT, QC_IMAGE_FORMAT_UYVY, QC_IMAGE_FORMAT_RGB888, 1920,
                  1024, 1920, 1024, "./data/test/CL2DFlex/0.uyvy",
                  "./data/test/CL2DFlex/golden2.rgb", false );
    AccuracyTest( CL2DFLEX_WORK_MODE_CONVERT, QC_IMAGE_FORMAT_UYVY, QC_IMAGE_FORMAT_NV12, 1920,
                  1024, 1920, 1024, "./data/test/CL2DFlex/0.uyvy",
                  "./data/test/CL2DFlex/golden3.nv12", false );
    AccuracyTest( CL2DFLEX_WORK_MODE_CONVERT_UBWC, QC_IMAGE_FORMAT_NV12_UBWC, QC_IMAGE_FORMAT_NV12,
                  3840, 2160, 3840, 2160, "./data/test/CL2DFlex/0.ubwc",
                  "./data/test/CL2DFlex/golden8.nv12", false );
}

TEST( CL2DFlex, ResizeAccuracyTest )
{
    // md5 of 0.nv12 is a1591f4b8c196a47628f0ef6bc3a721c
    // md5 of 0.uyvy is 5b1ae2203a9d97aeafe65e997f3beebc
    // md5 of golden4.rgb is 9382129ca960b8ff22e3c135e3eb12b7
    // md5 of golden5.rgb is 520d1039f96107ef4db669e9644250fd
    // md5 of golden6.nv12 is 91cdd0def0f40ce3c0fec070c2bccd01
    // md5 of golden7.rgb is a906c3bc49c7b91ec25d6474311d8031
    // md5 of golden10.nv12 is e5396625f90a049b625b65bc1fdf8183
    AccuracyTest( CL2DFLEX_WORK_MODE_RESIZE_NEAREST, QC_IMAGE_FORMAT_NV12, QC_IMAGE_FORMAT_RGB888,
                  1920, 1024, 1152, 800, "./data/test/CL2DFlex/0.nv12",
                  "./data/test/CL2DFlex/golden4.rgb", false );
    AccuracyTest( CL2DFLEX_WORK_MODE_RESIZE_NEAREST, QC_IMAGE_FORMAT_UYVY, QC_IMAGE_FORMAT_RGB888,
                  1920, 1024, 1152, 800, "./data/test/CL2DFlex/0.uyvy",
                  "./data/test/CL2DFlex/golden5.rgb", false );
    AccuracyTest( CL2DFLEX_WORK_MODE_RESIZE_NEAREST, QC_IMAGE_FORMAT_UYVY, QC_IMAGE_FORMAT_NV12,
                  1920, 1024, 1152, 800, "./data/test/CL2DFlex/0.uyvy",
                  "./data/test/CL2DFlex/golden6.nv12", false );
    AccuracyTest( CL2DFLEX_WORK_MODE_RESIZE_NEAREST, QC_IMAGE_FORMAT_RGB888, QC_IMAGE_FORMAT_RGB888,
                  1920, 1024, 1152, 800, "./data/test/CL2DFlex/golden1.rgb",
                  "./data/test/CL2DFlex/golden4.rgb", false );
    AccuracyTest( CL2DFLEX_WORK_MODE_RESIZE_NEAREST, QC_IMAGE_FORMAT_NV12, QC_IMAGE_FORMAT_NV12,
                  1920, 1024, 1152, 800, "./data/test/CL2DFlex/0.nv12",
                  "./data/test/CL2DFlex/golden10.nv12", false );
    AccuracyTest( CL2DFLEX_WORK_MODE_LETTERBOX_NEAREST, QC_IMAGE_FORMAT_NV12,
                  QC_IMAGE_FORMAT_RGB888, 1920, 1024, 1152, 800, "./data/test/CL2DFlex/0.nv12",
                  "./data/test/CL2DFlex/golden7.rgb", false );
}

TEST( CL2DFlex, RemapAccuracyTest )
{
    // md5 of 0.nv12 is a1591f4b8c196a47628f0ef6bc3a721c
    // md5 of golden9.rgb is 94d20588fa7d89cd285a819cddb0c978
    AccuracyTest( CL2DFLEX_WORK_MODE_REMAP_NEAREST, QC_IMAGE_FORMAT_NV12, QC_IMAGE_FORMAT_RGB888,
                  1920, 1024, 1152, 800, "./data/test/CL2DFlex/0.nv12",
                  "./data/test/CL2DFlex/golden9.rgb", false );
}

TEST( CL2DFlex, PerformanceTest )
{
    printf( "performance test of convert nv12 to rgb\n" );
    PerformanceTest( CL2DFLEX_WORK_MODE_CONVERT, QC_IMAGE_FORMAT_NV12, QC_IMAGE_FORMAT_RGB888, 1920,
                     1024, 1920, 1024, 1920, 1024, 100 );
    printf( "performance test of resize nv12 to rgb\n" );
    PerformanceTest( CL2DFLEX_WORK_MODE_RESIZE_NEAREST, QC_IMAGE_FORMAT_NV12,
                     QC_IMAGE_FORMAT_RGB888, 1920, 1024, 1920, 1024, 1152, 800, 100 );
    printf( "performance test of letterbox resize nv12 to rgb\n" );
    PerformanceTest( CL2DFLEX_WORK_MODE_LETTERBOX_NEAREST, QC_IMAGE_FORMAT_NV12,
                     QC_IMAGE_FORMAT_RGB888, 1920, 1024, 1920, 1024, 1152, 800, 100 );
    printf( "performance test of convert uyvy to rgb\n" );
    PerformanceTest( CL2DFLEX_WORK_MODE_CONVERT, QC_IMAGE_FORMAT_UYVY, QC_IMAGE_FORMAT_RGB888, 1920,
                     1024, 1920, 1024, 1920, 1024, 100 );
    printf( "performance test of resize nv12 to rgb\n" );
    PerformanceTest( CL2DFLEX_WORK_MODE_RESIZE_NEAREST, QC_IMAGE_FORMAT_UYVY,
                     QC_IMAGE_FORMAT_RGB888, 1920, 1024, 1920, 1024, 1152, 800, 100 );
    printf( "performance test of convert nv12 ubwc to nv12\n" );
    PerformanceTest( CL2DFLEX_WORK_MODE_CONVERT_UBWC, QC_IMAGE_FORMAT_NV12_UBWC,
                     QC_IMAGE_FORMAT_NV12, 1920, 1024, 1920, 1024, 1920, 1024, 100 );
    printf( "performance test of remap nv12 to bgr\n" );
    PerformanceTest( CL2DFLEX_WORK_MODE_REMAP_NEAREST, QC_IMAGE_FORMAT_NV12, QC_IMAGE_FORMAT_BGR888,
                     1920, 1024, 1920, 1024, 1152, 800, 100 );
}

TEST( CL2DFlex, MultipleROITest )
{
    printf( "Multiple ROI crop test of letterbox nv12 to rgb\n" );
    uint32_t numberTest = 100;
    CL2DFlex_ROIConfig_t roiTest[numberTest];
    for ( int i = 0; i < numberTest; i++ )
    {
        roiTest[i].x = i * 10;
        roiTest[i].y = i * 5;
        if ( i < numberTest / 2 )
        {
            roiTest[i].width = 200;
            roiTest[i].height = 400;
        }
        else
        {
            roiTest[i].width = 400;
            roiTest[i].height = 200;
        }
    }

    printf( "letterbox NV12 to RGB test:\n" );
    ROITest( numberTest, roiTest, CL2DFLEX_WORK_MODE_LETTERBOX_NEAREST_MULTIPLE,
             QC_IMAGE_FORMAT_NV12, QC_IMAGE_FORMAT_RGB888, 1920, 1024, 64, 64, 10, true );

    printf( "resize NV12 to RGB test:\n" );
    ROITest( numberTest, roiTest, CL2DFLEX_WORK_MODE_RESIZE_NEAREST_MULTIPLE, QC_IMAGE_FORMAT_NV12,
             QC_IMAGE_FORMAT_RGB888, 1920, 1024, 64, 64, 10, true );
}

#ifndef GTEST_QCNODE
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif