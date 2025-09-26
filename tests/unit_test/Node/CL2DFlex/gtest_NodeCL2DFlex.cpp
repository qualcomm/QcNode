// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause



#include "gtest/gtest.h"
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <stdio.h>
#include <string>

#include "QC/Node/CL2DFlex.hpp"
#include "QC/sample/BufferManager.hpp"
#include "md5_utils.hpp"

using namespace QC::Node;
using namespace QC::test::utils;
using namespace QC::sample;

QCStatus_e LoadImage( ImageDescriptor_t imageDesc, std::string path )
{
    QCStatus_e status = QC_STATUS_OK;
    FILE *file = nullptr;
    size_t length = 0;
    file = fopen( path.c_str(), "rb" );
    if ( nullptr == file )
    {
        printf( "could not open image file %s\n", path.c_str() );
        status = QC_STATUS_FAIL;
    }
    else
    {
        fseek( file, 0, SEEK_END );
        length = (size_t) ftell( file );
        if ( imageDesc.size != length )
        {
            printf( "image file %s size not match, need %d but got %d\n", path.c_str(),
                    (int) imageDesc.size, (int) length );
            status = QC_STATUS_FAIL;
        }
        else
        {
            fseek( file, 0, SEEK_SET );
            auto r = fread( imageDesc.pBuf, 1, length, file );
            if ( length != r )
            {
                printf( "failed to read image file %s, need %d but read %d\n", path.c_str(),
                        (int) length, (int) r );
                status = QC_STATUS_FAIL;
            }
        }
        fclose( file );
    }

    return status;
}

QCStatus_e LoadMap( TensorDescriptor_t buffer, std::string path )
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
        auto r = fread( buffer.pBuf, 1, length, file );
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

void SetConfigCL2D( CL2DFlex_Config_t *pCL2DFlexConfig, DataTree *pdt )
{
    pdt->Set<uint32_t>( "static.outputWidth", pCL2DFlexConfig->outputWidth );
    pdt->Set<uint32_t>( "static.outputHeight", pCL2DFlexConfig->outputHeight );
    pdt->SetImageFormat( "static.outputFormat", pCL2DFlexConfig->outputFormat );

    std::vector<DataTree> inputDts;
    for ( int i = 0; i < pCL2DFlexConfig->numOfInputs; i++ )
    {
        DataTree inputDt;
        inputDt.Set<uint32_t>( "inputWidth", pCL2DFlexConfig->inputWidths[i] );
        inputDt.Set<uint32_t>( "inputHeight", pCL2DFlexConfig->inputHeights[i] );
        inputDt.SetImageFormat( "inputFormat", pCL2DFlexConfig->inputFormats[i] );
        inputDt.Set<uint32_t>( "roiX", pCL2DFlexConfig->ROIs[i].x );
        inputDt.Set<uint32_t>( "roiY", pCL2DFlexConfig->ROIs[i].y );
        inputDt.Set<uint32_t>( "roiWidth", pCL2DFlexConfig->ROIs[i].width );
        inputDt.Set<uint32_t>( "roiHeight", pCL2DFlexConfig->ROIs[i].height );

        if ( pCL2DFlexConfig->workModes[i] == CL2DFLEX_WORK_MODE_CONVERT )
        {
            inputDt.Set<std::string>( "workMode", "convert" );
        }
        else if ( pCL2DFlexConfig->workModes[i] == CL2DFLEX_WORK_MODE_RESIZE_NEAREST )
        {
            inputDt.Set<std::string>( "workMode", "resize_nearest" );
        }
        else if ( pCL2DFlexConfig->workModes[i] == CL2DFLEX_WORK_MODE_LETTERBOX_NEAREST )
        {
            inputDt.Set<std::string>( "workMode", "letterbox_nearest" );
        }
        else if ( pCL2DFlexConfig->workModes[i] == CL2DFLEX_WORK_MODE_LETTERBOX_NEAREST_MULTIPLE )
        {
            inputDt.Set<std::string>( "workMode", "letterbox_nearest_multiple" );
        }
        else if ( pCL2DFlexConfig->workModes[i] == CL2DFLEX_WORK_MODE_RESIZE_NEAREST_MULTIPLE )
        {
            inputDt.Set<std::string>( "workMode", "resize_nearest_multiple" );
        }
        else if ( pCL2DFlexConfig->workModes[i] == CL2DFLEX_WORK_MODE_CONVERT_UBWC )
        {
            inputDt.Set<std::string>( "workMode", "convert_ubwc" );
        }
        else if ( pCL2DFlexConfig->workModes[i] == CL2DFLEX_WORK_MODE_REMAP_NEAREST )
        {
            inputDt.Set<std::string>( "workMode", "remap_nearest" );
            inputDt.Set<uint32_t>( "mapXBufferId", pCL2DFlexConfig->numOfInputs + 1 );
            inputDt.Set<uint32_t>( "mapYBufferId", pCL2DFlexConfig->numOfInputs + 2 );
        }

        inputDts.push_back( inputDt );
    }
    pdt->Set( "static.inputs", inputDts );
}

void Sanity()
{
    QCStatus_e ret;
    std::string errors;
    QCNodeIfs *pCL2DFlex = new QC::Node::CL2DFlex();
    BufferManager bufMgr( { "MANAGER", QC_NODE_TYPE_CL_2D_FLEX, 0 } );

    CL2DFlex_Config_t CL2DFlexConfig;
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

    DataTree dt;
    dt.Set<std::string>( "static.name", "CL2D" );
    dt.Set<uint32_t>( "static.id", 0 );
    SetConfigCL2D( &CL2DFlexConfig, &dt );

    QCNodeInit_t config = { dt.Dump() };
    printf( "config: %s\n", config.config.c_str() );

    ImageProps_t imgPropInputs[CL2DFlexConfig.numOfInputs];
    for ( int i = 0; i < CL2DFlexConfig.numOfInputs; i++ )
    {
        imgPropInputs[i].batchSize = 1;
        imgPropInputs[i].width = CL2DFlexConfig.inputWidths[i];
        imgPropInputs[i].height = CL2DFlexConfig.inputHeights[i];
        imgPropInputs[i].format = CL2DFlexConfig.inputFormats[i];
        if ( QC_IMAGE_FORMAT_NV12 == CL2DFlexConfig.inputFormats[i] )
        {
            imgPropInputs[i].stride[0] = CL2DFlexConfig.inputWidths[i];
            imgPropInputs[i].stride[1] = CL2DFlexConfig.inputWidths[i];
            imgPropInputs[i].actualHeight[0] = CL2DFlexConfig.inputHeights[i];
            imgPropInputs[i].actualHeight[1] = CL2DFlexConfig.inputHeights[i] / 2;
            imgPropInputs[i].planeBufSize[0] = 0;
            imgPropInputs[i].planeBufSize[1] = 0;
            imgPropInputs[i].numPlanes = 2;
        }
        else if ( QC_IMAGE_FORMAT_UYVY == CL2DFlexConfig.inputFormats[i] )
        {
            imgPropInputs[i].stride[0] = CL2DFlexConfig.inputWidths[i] * 2;
            imgPropInputs[i].actualHeight[0] = CL2DFlexConfig.inputHeights[i];
            imgPropInputs[i].planeBufSize[0] = 0;
            imgPropInputs[i].numPlanes = 1;
        }
        else if ( QC_IMAGE_FORMAT_RGB888 == CL2DFlexConfig.inputFormats[i] )
        {
            imgPropInputs[i].stride[0] = CL2DFlexConfig.inputWidths[i] * 3;
            imgPropInputs[i].actualHeight[0] = CL2DFlexConfig.inputHeights[i];
            imgPropInputs[i].planeBufSize[0] = 0;
            imgPropInputs[i].numPlanes = 1;
        }
    }

    ImageProps_t imgPropOutput;
    imgPropOutput.batchSize = CL2DFlexConfig.numOfInputs;
    imgPropOutput.width = CL2DFlexConfig.outputWidth;
    imgPropOutput.height = CL2DFlexConfig.outputHeight;
    imgPropOutput.format = CL2DFlexConfig.outputFormat;
    if ( ( QC_IMAGE_FORMAT_RGB888 == CL2DFlexConfig.outputFormat ) ||
         ( QC_IMAGE_FORMAT_BGR888 == CL2DFlexConfig.outputFormat ) )
    {
        imgPropOutput.stride[0] = CL2DFlexConfig.outputWidth * 3;
        imgPropOutput.actualHeight[0] = CL2DFlexConfig.outputHeight;
        imgPropOutput.planeBufSize[0] = 0;
        imgPropOutput.numPlanes = 1;
    }
    else if ( QC_IMAGE_FORMAT_NV12 == CL2DFlexConfig.outputFormat )
    {
        imgPropOutput.stride[0] = CL2DFlexConfig.outputWidth;
        imgPropOutput.stride[1] = CL2DFlexConfig.outputWidth;
        imgPropOutput.actualHeight[0] = CL2DFlexConfig.outputHeight;
        imgPropOutput.actualHeight[1] = CL2DFlexConfig.outputHeight / 2;
        imgPropOutput.planeBufSize[0] = 0;
        imgPropOutput.planeBufSize[1] = 0;
        imgPropOutput.numPlanes = 2;
    }

    uint32_t globalIdx = 0;
    QCSharedFrameDescriptorNode frameDesc( CL2DFlexConfig.numOfInputs + 1 );
    std::vector<ImageDescriptor_t> inputs;
    inputs.reserve( CL2DFlexConfig.numOfInputs );
    for ( int i = 0; i < CL2DFlexConfig.numOfInputs; i++ )
    {
        ImageDescriptor_t imageDesc;
        ret = bufMgr.Allocate( imgPropInputs[i], imageDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
        inputs.push_back( imageDesc );
        ret = frameDesc.SetBuffer( globalIdx, inputs.back() );
        ASSERT_EQ( QC_STATUS_OK, ret );
        globalIdx++;
    }
    std::vector<ImageDescriptor_t> outputs;
    outputs.reserve( 1 );
    ImageDescriptor_t imageDesc;
    ret = bufMgr.Allocate( imgPropOutput, imageDesc );
    outputs.push_back( imageDesc );
    ret = frameDesc.SetBuffer( globalIdx, outputs.back() );
    ASSERT_EQ( QC_STATUS_OK, ret );
    globalIdx++;

    ret = pCL2DFlex->Initialize( config );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = pCL2DFlex->Start();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = pCL2DFlex->ProcessFrameDescriptor( frameDesc );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = pCL2DFlex->Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = pCL2DFlex->DeInitialize();
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( auto imageDesc : inputs )
    {
        ret = bufMgr.Free( imageDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    for ( auto imageDesc : outputs )
    {
        ret = bufMgr.Free( imageDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    reinterpret_cast<QC::Node::CL2DFlex *>( pCL2DFlex )->~CL2DFlex();
}

void Accuracy( uint32_t inputNumberTest, CL2DFlex_Work_Mode_e modeTest,
               QCImageFormat_e inputFormatTest, QCImageFormat_e outputFormatTest,
               uint32_t inputWidthTest, uint32_t inputHeightTest, uint32_t outputWidthTest,
               uint32_t outputHeightTest, std::string pathTest, std::string goldenPath,
               bool saveOutput )
{
    QCStatus_e ret;
    std::string errors;
    QCNodeIfs *pCL2DFlex = new QC::Node::CL2DFlex();
    BufferManager bufMgr( { "MANAGER", QC_NODE_TYPE_CL_2D_FLEX, 0 } );
    uint32_t globalIdx = 0;
    std::vector<uint32_t> bufferIds;
    QCNodeInit_t config;
    config.buffers.clear();

    CL2DFlex_Config_t CL2DFlexConfig;
    CL2DFlexConfig.numOfInputs = inputNumberTest;
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

    DataTree dt;
    dt.Set<std::string>( "static.name", "CL2D" );
    dt.Set<uint32_t>( "static.id", 0 );
    SetConfigCL2D( &CL2DFlexConfig, &dt );

    ImageProps_t imgPropInputs[CL2DFlexConfig.numOfInputs];
    for ( int i = 0; i < CL2DFlexConfig.numOfInputs; i++ )
    {
        imgPropInputs[i].batchSize = 1;
        imgPropInputs[i].width = CL2DFlexConfig.inputWidths[i];
        imgPropInputs[i].height = CL2DFlexConfig.inputHeights[i];
        imgPropInputs[i].format = CL2DFlexConfig.inputFormats[i];
        if ( QC_IMAGE_FORMAT_NV12 == CL2DFlexConfig.inputFormats[i] )
        {
            imgPropInputs[i].stride[0] = CL2DFlexConfig.inputWidths[i];
            imgPropInputs[i].stride[1] = CL2DFlexConfig.inputWidths[i];
            imgPropInputs[i].actualHeight[0] = CL2DFlexConfig.inputHeights[i];
            imgPropInputs[i].actualHeight[1] = CL2DFlexConfig.inputHeights[i] / 2;
            imgPropInputs[i].planeBufSize[0] = 0;
            imgPropInputs[i].planeBufSize[1] = 0;
            imgPropInputs[i].numPlanes = 2;
        }
        else if ( QC_IMAGE_FORMAT_UYVY == CL2DFlexConfig.inputFormats[i] )
        {
            imgPropInputs[i].stride[0] = CL2DFlexConfig.inputWidths[i] * 2;
            imgPropInputs[i].actualHeight[0] = CL2DFlexConfig.inputHeights[i];
            imgPropInputs[i].planeBufSize[0] = 0;
            imgPropInputs[i].numPlanes = 1;
        }
        else if ( QC_IMAGE_FORMAT_RGB888 == CL2DFlexConfig.inputFormats[i] )
        {
            imgPropInputs[i].stride[0] = CL2DFlexConfig.inputWidths[i] * 3;
            imgPropInputs[i].actualHeight[0] = CL2DFlexConfig.inputHeights[i];
            imgPropInputs[i].planeBufSize[0] = 0;
            imgPropInputs[i].numPlanes = 1;
        }
    }

    ImageProps_t imgPropOutput;
    imgPropOutput.batchSize = CL2DFlexConfig.numOfInputs;
    imgPropOutput.width = CL2DFlexConfig.outputWidth;
    imgPropOutput.height = CL2DFlexConfig.outputHeight;
    imgPropOutput.format = CL2DFlexConfig.outputFormat;
    if ( ( QC_IMAGE_FORMAT_RGB888 == CL2DFlexConfig.outputFormat ) ||
         ( QC_IMAGE_FORMAT_BGR888 == CL2DFlexConfig.outputFormat ) )
    {
        imgPropOutput.stride[0] = CL2DFlexConfig.outputWidth * 3;
        imgPropOutput.actualHeight[0] = CL2DFlexConfig.outputHeight;
        imgPropOutput.planeBufSize[0] = 0;
        imgPropOutput.numPlanes = 1;
    }
    else if ( QC_IMAGE_FORMAT_NV12 == CL2DFlexConfig.outputFormat )
    {
        imgPropOutput.stride[0] = CL2DFlexConfig.outputWidth;
        imgPropOutput.stride[1] = CL2DFlexConfig.outputWidth;
        imgPropOutput.actualHeight[0] = CL2DFlexConfig.outputHeight;
        imgPropOutput.actualHeight[1] = CL2DFlexConfig.outputHeight / 2;
        imgPropOutput.planeBufSize[0] = 0;
        imgPropOutput.planeBufSize[1] = 0;
        imgPropOutput.numPlanes = 2;
    }

    uint32_t frameIdx = 0;
    QCSharedFrameDescriptorNode frameDesc( CL2DFlexConfig.numOfInputs + 1 );

    std::vector<ImageDescriptor_t> inputs;
    inputs.reserve( CL2DFlexConfig.numOfInputs );
    for ( int i = 0; i < CL2DFlexConfig.numOfInputs; i++ )
    {
        ImageDescriptor_t imageDesc;
        if ( QC_IMAGE_FORMAT_NV12_UBWC == CL2DFlexConfig.inputFormats[i] )
        {
            ret = bufMgr.Allocate( ImageBasicProps_t( CL2DFlexConfig.inputWidths[i],
                                                      CL2DFlexConfig.inputHeights[i],
                                                      CL2DFlexConfig.inputFormats[i] ),
                                   imageDesc );
        }
        else
        {
            ret = bufMgr.Allocate( imgPropInputs[i], imageDesc );
        }
        ASSERT_EQ( QC_STATUS_OK, ret );
        inputs.push_back( imageDesc );
        ret = frameDesc.SetBuffer( frameIdx, inputs.back() );
        ASSERT_EQ( QC_STATUS_OK, ret );
        frameIdx++;
        config.buffers.push_back( inputs.back() );
        bufferIds.push_back( globalIdx );
        globalIdx++;
    }

    std::vector<ImageDescriptor_t> outputs;
    outputs.reserve( 1 );
    ImageDescriptor_t imageDesc;
    ret = bufMgr.Allocate( imgPropOutput, imageDesc );
    outputs.push_back( imageDesc );
    ret = frameDesc.SetBuffer( frameIdx, outputs.back() );
    ASSERT_EQ( QC_STATUS_OK, ret );
    frameIdx++;
    config.buffers.push_back( outputs.back() );
    bufferIds.push_back( globalIdx );
    globalIdx++;

    TensorDescriptor_t mapXBufferDesc;
    TensorDescriptor_t mapYBufferDesc;
    if ( CL2DFLEX_WORK_MODE_REMAP_NEAREST == CL2DFlexConfig.workModes[0] )
    {
        TensorProps_t mapXProp = { QC_TENSOR_TYPE_FLOAT_32,
                                   { CL2DFlexConfig.outputHeight * CL2DFlexConfig.outputWidth } };
        ret = bufMgr.Allocate( mapXProp, mapXBufferDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );

        TensorProps_t mapYProp = { QC_TENSOR_TYPE_FLOAT_32,
                                   { CL2DFlexConfig.outputHeight * CL2DFlexConfig.outputWidth }

        };
        ret = bufMgr.Allocate( mapYProp, mapYBufferDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
        if ( nullptr != mapXBufferDesc.pBuf )
        {
            config.buffers.push_back( mapXBufferDesc );
            bufferIds.push_back( globalIdx );
            globalIdx++;
        }
        else
        {
            printf( "mapXBufferDesc is nullptr\n" );
        }
        if ( nullptr != mapYBufferDesc.pBuf )
        {
            config.buffers.push_back( mapYBufferDesc );
            bufferIds.push_back( globalIdx );
            globalIdx++;
        }
        else
        {
            printf( "mapYBufferDesc is nullptr\n" );
        }
    }

    dt.Set<uint32_t>( "static.bufferIds", bufferIds );
    config.config = dt.Dump();
    printf( "config: %s\n", config.config.c_str() );

    ret = pCL2DFlex->Initialize( config );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = pCL2DFlex->Start();
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( int i = 0; i < CL2DFlexConfig.numOfInputs; i++ )
    {
        memset( inputs[i].pBuf, 0, inputs[i].size );
        ret = LoadImage( inputs[i], pathTest );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
    memset( outputs[0].pBuf, 0, outputs[0].size );
    if ( CL2DFLEX_WORK_MODE_REMAP_NEAREST == CL2DFlexConfig.workModes[0] )
    {
        ret = LoadMap( mapXBufferDesc, "./data/test/CL2DFlex/mapX.raw" );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = LoadMap( mapYBufferDesc, "./data/test/CL2DFlex/mapY.raw" );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    ret = pCL2DFlex->ProcessFrameDescriptor( frameDesc );
    ASSERT_EQ( QC_STATUS_OK, ret );

    if ( true == saveOutput )
    {
        uint8_t *ptr = (uint8_t *) outputs[0].pBuf;
        FILE *fp = fopen( goldenPath.c_str(), "wb" );
        if ( nullptr != fp )
        {
            fwrite( ptr, outputs[0].size, 1, fp );
            fclose( fp );
        }
    }

    ImageDescriptor_t golden;
    (void) bufMgr.Allocate( imgPropOutput, golden );
    LoadImage( golden, goldenPath );

    std::string md5Output = MD5Sum( outputs[0].pBuf, outputs[0].size );
    printf( "output md5 = %s\n", md5Output.c_str() );
    std::string md5Golden = MD5Sum( golden.pBuf, outputs[0].size );
    printf( "golden md5 = %s\n", md5Golden.c_str() );

    if ( md5Output != md5Golden )   // check cosine similarity if md5 not match
    {
        size_t outputSize = outputs[0].size;
        uint8_t *outputData = (uint8_t *) outputs[0].pBuf;
        uint8_t *goldenData = (uint8_t *) golden.pBuf;
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

    ret = pCL2DFlex->Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = pCL2DFlex->DeInitialize();
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( auto imageDesc : inputs )
    {
        ret = bufMgr.Free( imageDesc );
    }

    for ( auto imageDesc : outputs )
    {
        ret = bufMgr.Free( imageDesc );
    }

    (void) bufMgr.Free( mapXBufferDesc );
    (void) bufMgr.Free( mapYBufferDesc );
    (void) bufMgr.Free( golden );

    reinterpret_cast<QC::Node::CL2DFlex *>( pCL2DFlex )->~CL2DFlex();
}

TEST( NodeCL2D, Sanity )
{
    Sanity();
}

// md5 of 0.nv12 is a1591f4b8c196a47628f0ef6bc3a721c
// md5 of 0.uyvy is 5b1ae2203a9d97aeafe65e997f3beebc
// md5 of 0.ubwc is ce5f81f72f9c0ec0c347b1ee55d13584
// md5 of golden1.rgb is 4b528d54b7c5d5164f66719a89f988be
// md5 of golden2.rgb is f94a6aeae302add0424821660bcd2684
// md5 of golden3.nv12 is 91ed68589443b87bcfff8ae7e69b03b2
// md5 of golden4.rgb is 9382129ca960b8ff22e3c135e3eb12b7
// md5 of golden5.rgb is 520d1039f96107ef4db669e9644250fd
// md5 of golden6.nv12 is 91cdd0def0f40ce3c0fec070c2bccd01
// md5 of golden7.rgb is a906c3bc49c7b91ec25d6474311d8031
// md5 of golden8.nv12 is 43d33b3417b0b53c594269e5df95e035
// md5 of golden9.rgb is cdb33ad92de656964c3be1d67fff25fb
// md5 of golden10.nv12 is e5396625f90a049b625b65bc1fdf8183

TEST( NodeCL2D, Accuracy )
{
    Accuracy( 1, CL2DFLEX_WORK_MODE_CONVERT, QC_IMAGE_FORMAT_NV12, QC_IMAGE_FORMAT_RGB888, 1920,
              1024, 1920, 1024, "./data/test/CL2DFlex/0.nv12", "./data/test/CL2DFlex/golden1.rgb",
              false );
    Accuracy( 1, CL2DFLEX_WORK_MODE_CONVERT, QC_IMAGE_FORMAT_UYVY, QC_IMAGE_FORMAT_RGB888, 1920,
              1024, 1920, 1024, "./data/test/CL2DFlex/0.uyvy", "./data/test/CL2DFlex/golden2.rgb",
              false );
    Accuracy( 1, CL2DFLEX_WORK_MODE_CONVERT, QC_IMAGE_FORMAT_UYVY, QC_IMAGE_FORMAT_NV12, 1920, 1024,
              1920, 1024, "./data/test/CL2DFlex/0.uyvy", "./data/test/CL2DFlex/golden3.nv12",
              false );
    Accuracy( 1, CL2DFLEX_WORK_MODE_RESIZE_NEAREST, QC_IMAGE_FORMAT_NV12, QC_IMAGE_FORMAT_RGB888,
              1920, 1024, 1152, 800, "./data/test/CL2DFlex/0.nv12",
              "./data/test/CL2DFlex/golden4.rgb", false );
    Accuracy( 1, CL2DFLEX_WORK_MODE_RESIZE_NEAREST, QC_IMAGE_FORMAT_UYVY, QC_IMAGE_FORMAT_RGB888,
              1920, 1024, 1152, 800, "./data/test/CL2DFlex/0.uyvy",
              "./data/test/CL2DFlex/golden5.rgb", false );
    Accuracy( 1, CL2DFLEX_WORK_MODE_RESIZE_NEAREST, QC_IMAGE_FORMAT_UYVY, QC_IMAGE_FORMAT_NV12,
              1920, 1024, 1152, 800, "./data/test/CL2DFlex/0.uyvy",
              "./data/test/CL2DFlex/golden6.nv12", false );
    Accuracy( 1, CL2DFLEX_WORK_MODE_RESIZE_NEAREST, QC_IMAGE_FORMAT_RGB888, QC_IMAGE_FORMAT_RGB888,
              1920, 1024, 1152, 800, "./data/test/CL2DFlex/golden1.rgb",
              "./data/test/CL2DFlex/golden4.rgb", false );
    Accuracy( 1, CL2DFLEX_WORK_MODE_RESIZE_NEAREST, QC_IMAGE_FORMAT_NV12, QC_IMAGE_FORMAT_NV12,
              1920, 1024, 1152, 800, "./data/test/CL2DFlex/0.nv12",
              "./data/test/CL2DFlex/golden10.nv12", false );
    Accuracy( 1, CL2DFLEX_WORK_MODE_LETTERBOX_NEAREST, QC_IMAGE_FORMAT_NV12, QC_IMAGE_FORMAT_RGB888,
              1920, 1024, 1152, 800, "./data/test/CL2DFlex/0.nv12",
              "./data/test/CL2DFlex/golden7.rgb", false );
}

TEST( NodeCL2D, RemapAccuracy )
{
    Accuracy( 1, CL2DFLEX_WORK_MODE_REMAP_NEAREST, QC_IMAGE_FORMAT_NV12, QC_IMAGE_FORMAT_RGB888,
              1920, 1024, 1152, 800, "./data/test/CL2DFlex/0.nv12",
              "./data/test/CL2DFlex/golden9.rgb", false );
}

TEST( NodeCL2D, UBWCAccuracy )
{
    Accuracy( 1, CL2DFLEX_WORK_MODE_CONVERT_UBWC, QC_IMAGE_FORMAT_NV12_UBWC, QC_IMAGE_FORMAT_NV12,
              3840, 2160, 3840, 2160, "./data/test/CL2DFlex/0.ubwc",
              "./data/test/CL2DFlex/golden8.nv12", false );
}

#ifndef GTEST_QCNODE
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif