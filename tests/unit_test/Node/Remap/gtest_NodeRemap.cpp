// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "gtest/gtest.h"
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <stdio.h>
#include <string>

#include "QC/Node/Remap.hpp"
#include "md5_utils.hpp"

using namespace QC::Node;
using namespace QC::component;
using namespace QC::test::utils;

QCStatus_e LoadMapRemap( QCSharedBuffer_t buffer, std::string path )
{
    QCStatus_e status = QC_STATUS_OK;
    FILE *file = nullptr;
    size_t length = 0;
    size_t size = buffer.size;

    file = fopen( path.c_str(), "rb" );
    if ( nullptr == file )
    {
        printf( "Failed to open file %s", path.c_str() );
        status = QC_STATUS_FAIL;
    }

    if ( QC_STATUS_OK == status )
    {
        fseek( file, 0, SEEK_END );
        length = (size_t) ftell( file );
        if ( size != length )
        {
            printf( "Invalid file size for %s, need %d but got %d", path.c_str(), (int) size,
                    (int) length );
            status = QC_STATUS_FAIL;
        }
    }

    if ( QC_STATUS_OK == status )
    {
        fseek( file, 0, SEEK_SET );
        auto r = fread( buffer.data(), 1, length, file );
        if ( length != r )
        {
            printf( "failed to read map table file %s", path.c_str() );
            status = QC_STATUS_FAIL;
        }
    }

    if ( nullptr != file )
    {
        fclose( file );
    }

    return status;
}

void SetConfigRemap( Remap_Config_t *pRemapConfig, DataTree *pdt )
{
    pdt->Set<uint32_t>( "static.outputWidth", pRemapConfig->outputWidth );
    pdt->Set<uint32_t>( "static.outputHeight", pRemapConfig->outputHeight );
    pdt->SetImageFormat( "static.outputFormat", pRemapConfig->outputFormat );
    pdt->SetProcessorType( "static.processorType", pRemapConfig->processor );
    pdt->Set<bool>( "static.bEnableUndistortion", pRemapConfig->bEnableUndistortion );
    pdt->Set<bool>( "static.bEnableNormalize", pRemapConfig->bEnableNormalize );

    if ( true == pRemapConfig->bEnableNormalize )
    {
        pdt->Set<float>( "static.RSub", pRemapConfig->normlzR.sub );
        pdt->Set<float>( "static.RMul", pRemapConfig->normlzR.mul );
        pdt->Set<float>( "static.RAdd", pRemapConfig->normlzR.add );
        pdt->Set<float>( "static.GSub", pRemapConfig->normlzG.sub );
        pdt->Set<float>( "static.GMul", pRemapConfig->normlzG.mul );
        pdt->Set<float>( "static.GAdd", pRemapConfig->normlzG.add );
        pdt->Set<float>( "static.BSub", pRemapConfig->normlzB.sub );
        pdt->Set<float>( "static.BMul", pRemapConfig->normlzB.mul );
        pdt->Set<float>( "static.BAdd", pRemapConfig->normlzB.add );
    }

    std::vector<DataTree> inputDts;
    for ( int i = 0; i < pRemapConfig->numOfInputs; i++ )
    {
        DataTree inputDt;
        inputDt.Set<uint32_t>( "inputWidth", pRemapConfig->inputConfigs[i].inputWidth );
        inputDt.Set<uint32_t>( "inputHeight", pRemapConfig->inputConfigs[i].inputHeight );
        inputDt.SetImageFormat( "inputFormat", pRemapConfig->inputConfigs[i].inputFormat );
        inputDt.Set<uint32_t>( "roiX", pRemapConfig->inputConfigs[i].ROI.x );
        inputDt.Set<uint32_t>( "roiY", pRemapConfig->inputConfigs[i].ROI.y );
        inputDt.Set<uint32_t>( "roiWidth", pRemapConfig->inputConfigs[i].ROI.width );
        inputDt.Set<uint32_t>( "roiHeight", pRemapConfig->inputConfigs[i].ROI.height );
        inputDt.Set<uint32_t>( "mapWidth", pRemapConfig->inputConfigs[i].mapWidth );
        inputDt.Set<uint32_t>( "mapHeight", pRemapConfig->inputConfigs[i].mapHeight );

        if ( pRemapConfig->bEnableUndistortion == true )
        {
            inputDt.Set<uint32_t>( "mapX", 0 );
            inputDt.Set<uint32_t>( "mapY", 1 );
        }

        inputDts.push_back( inputDt );
    }
    pdt->Set( "static.inputs", inputDts );
}

void SanityRemap()
{
    QCStatus_e ret;
    std::string errors;
    QCNodeIfs *pRemap = new QC::Node::Remap();

    Remap_Config_t RemapConfig;
    RemapConfig.numOfInputs = 2;
    for ( int i = 0; i < RemapConfig.numOfInputs; i++ )
    {
        RemapConfig.inputConfigs[i].inputWidth = 128;
        RemapConfig.inputConfigs[i].inputHeight = 128;
        RemapConfig.inputConfigs[i].inputFormat = QC_IMAGE_FORMAT_UYVY;
        RemapConfig.inputConfigs[i].ROI.x = 0;
        RemapConfig.inputConfigs[i].ROI.y = 0;
        RemapConfig.inputConfigs[i].ROI.width = 64;
        RemapConfig.inputConfigs[i].ROI.height = 64;
        RemapConfig.inputConfigs[i].mapWidth = 64;
        RemapConfig.inputConfigs[i].mapHeight = 64;
    }
    RemapConfig.outputWidth = 64;
    RemapConfig.outputHeight = 64;
    RemapConfig.outputFormat = QC_IMAGE_FORMAT_RGB888;
    RemapConfig.processor = QC_PROCESSOR_HTP0;
    RemapConfig.bEnableUndistortion = false;
    RemapConfig.bEnableNormalize = false;

    DataTree dt;
    dt.Set<std::string>( "static.name", "Remap" );
    dt.Set<uint32_t>( "static.id", 0 );
    SetConfigRemap( &RemapConfig, &dt );

    QCNodeInit_t config = { dt.Dump() };
    printf( "config: %s\n", config.config.c_str() );

    ret = pRemap->Initialize( config );
    ASSERT_EQ( QC_STATUS_OK, ret );

    QCImageProps_t imgPropInputs[RemapConfig.numOfInputs];
    for ( int i = 0; i < RemapConfig.numOfInputs; i++ )
    {
        imgPropInputs[i].batchSize = 1;
        imgPropInputs[i].width = RemapConfig.inputConfigs[i].inputWidth;
        imgPropInputs[i].height = RemapConfig.inputConfigs[i].inputHeight;
        imgPropInputs[i].format = RemapConfig.inputConfigs[i].inputFormat;
        if ( QC_IMAGE_FORMAT_NV12 == RemapConfig.inputConfigs[i].inputFormat )
        {
            imgPropInputs[i].stride[0] = RemapConfig.inputConfigs[i].inputWidth;
            imgPropInputs[i].stride[1] = RemapConfig.inputConfigs[i].inputWidth;
            imgPropInputs[i].actualHeight[0] = RemapConfig.inputConfigs[i].inputHeight;
            imgPropInputs[i].actualHeight[1] = RemapConfig.inputConfigs[i].inputHeight / 2;
            imgPropInputs[i].planeBufSize[0] = 0;
            imgPropInputs[i].planeBufSize[1] = 0;
            imgPropInputs[i].numPlanes = 2;
        }
        else if ( QC_IMAGE_FORMAT_UYVY == RemapConfig.inputConfigs[i].inputFormat )
        {
            imgPropInputs[i].stride[0] = RemapConfig.inputConfigs[i].inputWidth * 2;
            imgPropInputs[i].actualHeight[0] = RemapConfig.inputConfigs[i].inputHeight;
            imgPropInputs[i].planeBufSize[0] = 0;
            imgPropInputs[i].numPlanes = 1;
        }
        else if ( QC_IMAGE_FORMAT_RGB888 == RemapConfig.inputConfigs[i].inputFormat )
        {
            imgPropInputs[i].stride[0] = RemapConfig.inputConfigs[i].inputWidth * 3;
            imgPropInputs[i].actualHeight[0] = RemapConfig.inputConfigs[i].inputHeight;
            imgPropInputs[i].planeBufSize[0] = 0;
            imgPropInputs[i].numPlanes = 1;
        }
    }

    QCImageProps_t imgPropOutput;
    imgPropOutput.batchSize = RemapConfig.numOfInputs;
    imgPropOutput.width = RemapConfig.outputWidth;
    imgPropOutput.height = RemapConfig.outputHeight;
    imgPropOutput.format = RemapConfig.outputFormat;
    if ( ( QC_IMAGE_FORMAT_RGB888 == RemapConfig.outputFormat ) ||
         ( QC_IMAGE_FORMAT_BGR888 == RemapConfig.outputFormat ) )
    {
        imgPropOutput.stride[0] = RemapConfig.outputWidth * 3;
        imgPropOutput.actualHeight[0] = RemapConfig.outputHeight;
        imgPropOutput.planeBufSize[0] = 0;
        imgPropOutput.numPlanes = 1;
    }
    else if ( QC_IMAGE_FORMAT_NV12 == RemapConfig.outputFormat )
    {
        imgPropOutput.stride[0] = RemapConfig.outputWidth;
        imgPropOutput.stride[1] = RemapConfig.outputWidth;
        imgPropOutput.actualHeight[0] = RemapConfig.outputHeight;
        imgPropOutput.actualHeight[1] = RemapConfig.outputHeight / 2;
        imgPropOutput.planeBufSize[0] = 0;
        imgPropOutput.planeBufSize[1] = 0;
        imgPropOutput.numPlanes = 2;
    }

    std::vector<QCImageProps_t> inputsProps;
    std::vector<QCSharedBufferDescriptor_t> inputs;
    for ( int i = 0; i < RemapConfig.numOfInputs; i++ )
    {
        inputsProps.push_back( imgPropInputs[i] );
        QCSharedBufferDescriptor_t buffer;
        ret = buffer.buffer.Allocate( &imgPropInputs[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
        inputs.push_back( buffer );
    }
    std::vector<QCImageProps_t> outputsProps;
    std::vector<QCSharedBufferDescriptor_t> outputs;
    outputsProps.push_back( imgPropOutput );
    QCSharedBufferDescriptor_t buffer;
    ret = buffer.buffer.Allocate( &imgPropOutput );
    outputs.push_back( buffer );

    ret = pRemap->Start();
    ASSERT_EQ( QC_STATUS_OK, ret );

    QCSharedFrameDescriptorNode frameDesc( inputs.size() + outputs.size() );
    uint32_t globalIdx = 0;
    for ( auto &buffer : inputs )
    {
        ret = frameDesc.SetBuffer( globalIdx, buffer );
        ASSERT_EQ( QC_STATUS_OK, ret );
        globalIdx++;
    }
    for ( auto &buffer : outputs )
    {
        ret = frameDesc.SetBuffer( globalIdx, buffer );
        ASSERT_EQ( QC_STATUS_OK, ret );
        globalIdx++;
    }

    ret = pRemap->ProcessFrameDescriptor( frameDesc );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = pRemap->Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = pRemap->DeInitialize();
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( auto &buffer : inputs )
    {
        ret = buffer.buffer.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    for ( auto &buffer : outputs )
    {
        ret = buffer.buffer.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    reinterpret_cast<QC::Node::Remap *>( pRemap )->~Remap();
}

void AccuracyRemap( uint32_t inputNumberTest, QCProcessorType_e processorTest,
                    QCImageFormat_e inputFormatTest, QCImageFormat_e outputFormatTest,
                    uint32_t inputWidthTest, uint32_t inputHeightTest, uint32_t outputWidthTest,
                    uint32_t outputHeightTest, bool normalizationTest, bool undistortionTest,
                    std::string pathTest, std::string goldenPath, bool saveOutput )
{
    QCStatus_e ret;
    std::string errors;
    QCNodeIfs *pRemap = new QC::Node::Remap();

    Remap_Config_t RemapConfig;
    RemapConfig.numOfInputs = inputNumberTest;
    for ( int i = 0; i < RemapConfig.numOfInputs; i++ )
    {
        RemapConfig.inputConfigs[i].inputWidth = inputWidthTest;
        RemapConfig.inputConfigs[i].inputHeight = inputHeightTest;
        RemapConfig.inputConfigs[i].inputFormat = inputFormatTest;
        RemapConfig.inputConfigs[i].ROI.x = 0;
        RemapConfig.inputConfigs[i].ROI.y = 0;
        RemapConfig.inputConfigs[i].ROI.width = outputWidthTest;
        RemapConfig.inputConfigs[i].ROI.height = outputHeightTest;
        RemapConfig.inputConfigs[i].mapWidth = outputWidthTest;
        RemapConfig.inputConfigs[i].mapHeight = outputHeightTest;
    }
    RemapConfig.outputWidth = outputWidthTest;
    RemapConfig.outputHeight = outputHeightTest;
    RemapConfig.outputFormat = outputFormatTest;
    RemapConfig.processor = processorTest;
    RemapConfig.bEnableUndistortion = undistortionTest;
    RemapConfig.bEnableNormalize = normalizationTest;

    if ( true == RemapConfig.bEnableNormalize )
    {
        RemapConfig.normlzR.sub = 123.675;
        RemapConfig.normlzR.mul = 1.f / 58.395;
        RemapConfig.normlzR.add = 0.f;
        RemapConfig.normlzG.sub = 116.28;
        RemapConfig.normlzG.mul = 1.f / 57.12;
        RemapConfig.normlzG.add = 0.f;
        RemapConfig.normlzB.sub = 103.53;
        RemapConfig.normlzB.mul = 1.f / 57.375;
        RemapConfig.normlzB.add = 0.f;
        float quantScale = 0.0186584480106831f;
        int32_t quantOffset = 114;
        RemapConfig.normlzR.add = RemapConfig.normlzR.add / quantScale + quantOffset;
        RemapConfig.normlzR.mul = RemapConfig.normlzR.mul / quantScale;
        RemapConfig.normlzG.add = RemapConfig.normlzG.add / quantScale + quantOffset;
        RemapConfig.normlzG.mul = RemapConfig.normlzG.mul / quantScale;
        RemapConfig.normlzB.add = RemapConfig.normlzB.add / quantScale + quantOffset;
        RemapConfig.normlzB.mul = RemapConfig.normlzB.mul / quantScale;
    }

    DataTree dt;
    dt.Set<std::string>( "static.name", "Remap" );
    dt.Set<uint32_t>( "static.id", 0 );
    SetConfigRemap( &RemapConfig, &dt );

    QCNodeInit_t config = { dt.Dump() };
    printf( "config: %s\n", config.config.c_str() );

    QCSharedBufferDescriptor_t mapXBufferDesc;
    QCSharedBufferDescriptor_t mapYBufferDesc;
    if ( true == RemapConfig.bEnableUndistortion )
    {
        QCTensorProps_t mapXProp = {
                QC_TENSOR_TYPE_FLOAT_32,
                { RemapConfig.inputConfigs[0].mapWidth, RemapConfig.inputConfigs[0].mapHeight, 0 },
                2,
        };
        ret = mapXBufferDesc.buffer.Allocate( &mapXProp );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = LoadMapRemap( mapXBufferDesc.buffer, "./data/test/Remap/mapX.raw" );
        ASSERT_EQ( QC_STATUS_OK, ret );

        QCTensorProps_t mapYProp = {
                QC_TENSOR_TYPE_FLOAT_32,
                { RemapConfig.inputConfigs[0].mapWidth, RemapConfig.inputConfigs[0].mapHeight, 0 },
                2,
        };
        ret = mapYBufferDesc.buffer.Allocate( &mapYProp );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = LoadMapRemap( mapYBufferDesc.buffer, "./data/test/Remap/mapY.raw" );
        ASSERT_EQ( QC_STATUS_OK, ret );
        if ( nullptr != mapXBufferDesc.buffer.data() )
        {
            config.buffers.push_back( mapXBufferDesc );
        }
        else
        {
            printf( "mapXBufferDesc is nullptr\n" );
        }
        if ( nullptr != mapYBufferDesc.buffer.data() )
        {
            config.buffers.push_back( mapYBufferDesc );
        }
        else
        {
            printf( "mapYBufferDesc is nullptr\n" );
        }
    }

    ret = pRemap->Initialize( config );
    ASSERT_EQ( QC_STATUS_OK, ret );

    QCImageProps_t imgPropInputs[RemapConfig.numOfInputs];
    for ( int i = 0; i < RemapConfig.numOfInputs; i++ )
    {
        imgPropInputs[i].batchSize = 1;
        imgPropInputs[i].width = RemapConfig.inputConfigs[i].inputWidth;
        imgPropInputs[i].height = RemapConfig.inputConfigs[i].inputHeight;
        imgPropInputs[i].format = RemapConfig.inputConfigs[i].inputFormat;
        if ( QC_IMAGE_FORMAT_NV12 == RemapConfig.inputConfigs[i].inputFormat )
        {
            imgPropInputs[i].stride[0] = RemapConfig.inputConfigs[i].inputWidth;
            imgPropInputs[i].stride[1] = RemapConfig.inputConfigs[i].inputWidth;
            imgPropInputs[i].actualHeight[0] = RemapConfig.inputConfigs[i].inputHeight;
            imgPropInputs[i].actualHeight[1] = RemapConfig.inputConfigs[i].inputHeight / 2;
            imgPropInputs[i].planeBufSize[0] = 0;
            imgPropInputs[i].planeBufSize[1] = 0;
            imgPropInputs[i].numPlanes = 2;
        }
        else if ( QC_IMAGE_FORMAT_UYVY == RemapConfig.inputConfigs[i].inputFormat )
        {
            imgPropInputs[i].stride[0] = RemapConfig.inputConfigs[i].inputWidth * 2;
            imgPropInputs[i].actualHeight[0] = RemapConfig.inputConfigs[i].inputHeight;
            imgPropInputs[i].planeBufSize[0] = 0;
            imgPropInputs[i].numPlanes = 1;
        }
        else if ( QC_IMAGE_FORMAT_RGB888 == RemapConfig.inputConfigs[i].inputFormat )
        {
            imgPropInputs[i].stride[0] = RemapConfig.inputConfigs[i].inputWidth * 3;
            imgPropInputs[i].actualHeight[0] = RemapConfig.inputConfigs[i].inputHeight;
            imgPropInputs[i].planeBufSize[0] = 0;
            imgPropInputs[i].numPlanes = 1;
        }
    }

    QCImageProps_t imgPropOutput;
    imgPropOutput.batchSize = RemapConfig.numOfInputs;
    imgPropOutput.width = RemapConfig.outputWidth;
    imgPropOutput.height = RemapConfig.outputHeight;
    imgPropOutput.format = RemapConfig.outputFormat;
    if ( ( QC_IMAGE_FORMAT_RGB888 == RemapConfig.outputFormat ) ||
         ( QC_IMAGE_FORMAT_BGR888 == RemapConfig.outputFormat ) )
    {
        imgPropOutput.stride[0] = RemapConfig.outputWidth * 3;
        imgPropOutput.actualHeight[0] = RemapConfig.outputHeight;
        imgPropOutput.planeBufSize[0] = 0;
        imgPropOutput.numPlanes = 1;
    }
    else if ( QC_IMAGE_FORMAT_NV12 == RemapConfig.outputFormat )
    {
        imgPropOutput.stride[0] = RemapConfig.outputWidth;
        imgPropOutput.stride[1] = RemapConfig.outputWidth;
        imgPropOutput.actualHeight[0] = RemapConfig.outputHeight;
        imgPropOutput.actualHeight[1] = RemapConfig.outputHeight / 2;
        imgPropOutput.planeBufSize[0] = 0;
        imgPropOutput.planeBufSize[1] = 0;
        imgPropOutput.numPlanes = 2;
    }

    std::vector<QCImageProps_t> inputsProps;
    std::vector<QCSharedBufferDescriptor_t> inputs;
    for ( int i = 0; i < RemapConfig.numOfInputs; i++ )
    {
        inputsProps.push_back( imgPropInputs[i] );
        QCSharedBufferDescriptor_t buffer;
        if ( QC_IMAGE_FORMAT_NV12_UBWC == RemapConfig.inputConfigs[i].inputFormat )
        {
            ret = buffer.buffer.Allocate( RemapConfig.inputConfigs[i].inputWidth,
                                          RemapConfig.inputConfigs[i].inputHeight,
                                          RemapConfig.inputConfigs[i].inputFormat );
        }
        else
        {
            ret = buffer.buffer.Allocate( &imgPropInputs[i] );
        }
        ASSERT_EQ( QC_STATUS_OK, ret );
        memset( buffer.buffer.data(), 0, buffer.buffer.size );

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
            if ( buffer.buffer.size != length1 )
            {
                printf( "image file %s size not match, need %d but got %d\n", pathTest.c_str(),
                        (int) buffer.buffer.size, (int) length1 );
            }
            else
            {
                fseek( file1, 0, SEEK_SET );
                auto r = fread( buffer.buffer.data(), 1, length1, file1 );
                if ( length1 != r )
                {
                    printf( "failed to read image file %s, need %d but read %d\n", pathTest.c_str(),
                            (int) length1, (int) r );
                }
            }
            fclose( file1 );
        }

        inputs.push_back( buffer );
    }

    std::vector<QCImageProps_t> outputsProps;
    std::vector<QCSharedBufferDescriptor_t> outputs;
    outputsProps.push_back( imgPropOutput );
    QCSharedBufferDescriptor_t buffer;
    ret = buffer.buffer.Allocate( &imgPropOutput );
    outputs.push_back( buffer );

    ret = pRemap->Start();
    ASSERT_EQ( QC_STATUS_OK, ret );

    QCSharedFrameDescriptorNode frameDesc( inputs.size() + outputs.size() );
    uint32_t globalIdx = 0;
    for ( auto &buffer : inputs )
    {
        ret = frameDesc.SetBuffer( globalIdx, buffer );
        ASSERT_EQ( QC_STATUS_OK, ret );
        globalIdx++;
    }
    for ( auto &buffer : outputs )
    {
        ret = frameDesc.SetBuffer( globalIdx, buffer );
        ASSERT_EQ( QC_STATUS_OK, ret );
        globalIdx++;
    }

    ret = pRemap->ProcessFrameDescriptor( frameDesc );
    ASSERT_EQ( QC_STATUS_OK, ret );

    if ( true == saveOutput )
    {
        uint8_t *ptr = (uint8_t *) outputs[0].buffer.data();
        FILE *fp = fopen( goldenPath.c_str(), "wb" );
        if ( nullptr != fp )
        {
            fwrite( ptr, outputs[0].buffer.size, 1, fp );
            fclose( fp );
        }
    }

    QCSharedBuffer_t golden;
    (void) golden.Allocate( &imgPropOutput );

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

    std::string md5Output = MD5Sum( outputs[0].buffer.data(), outputs[0].buffer.size );
    printf( "output md5 = %s\n", md5Output.c_str() );
    std::string md5Golden = MD5Sum( golden.data(), outputs[0].buffer.size );
    printf( "golden md5 = %s\n", md5Golden.c_str() );

    if ( md5Output != md5Golden )   // check cosine similarity if md5 not match
    {
        size_t outputSize = outputs[0].buffer.size;
        uint8_t *outputData = (uint8_t *) outputs[0].buffer.data();
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

    ret = pRemap->Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = pRemap->DeInitialize();
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( auto &buffer : inputs )
    {
        ret = buffer.buffer.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    for ( auto &buffer : outputs )
    {
        ret = buffer.buffer.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    (void) mapXBufferDesc.buffer.Free();
    (void) mapYBufferDesc.buffer.Free();
    (void) golden.Free();

    reinterpret_cast<QC::Node::Remap *>( pRemap )->~Remap();
}

TEST( NodeRemap, Sanity )
{
    SanityRemap();
}

// md5 of 0.uyvy is 5b1ae2203a9d97aeafe65e997f3beebc
// md5 of golden_cpu.rgb is 59760700b59beb67227d305b317dcec6
// md5 of golden_dsp.rgb is f139fb73234986a15e340afe1058522e
// md5 of golden_gpu.rgb is 59760700b59beb67227d305b317dcec6
TEST( NodeRemap, AccuracyHTP )
{
    AccuracyRemap( 2, QC_PROCESSOR_HTP0, QC_IMAGE_FORMAT_UYVY, QC_IMAGE_FORMAT_RGB888, 1920, 1024,
                   1152, 800, true, false, "./data/test/remap/0.uyvy",
                   "./data/test/remap/golden_dsp.rgb", false );
}
TEST( NodeRemap, AccuracyCPU )
{
    AccuracyRemap( 2, QC_PROCESSOR_CPU, QC_IMAGE_FORMAT_UYVY, QC_IMAGE_FORMAT_RGB888, 1920, 1024,
                   1152, 800, true, false, "./data/test/remap/0.uyvy",
                   "./data/test/remap/golden_cpu.rgb", false );
}
#if defined( USE_ENG_FADAS_GPU )
TEST( NodeRemap, AccuracyGPU )
{
    AccuracyRemap( 2, QC_PROCESSOR_GPU, QC_IMAGE_FORMAT_UYVY, QC_IMAGE_FORMAT_RGB888, 1920, 1024,
                   1152, 800, true, false, "./data/test/remap/0.uyvy",
                   "./data/test/remap/golden_gpu.rgb", false );
}
#endif

#ifndef GTEST_QCNODE
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif