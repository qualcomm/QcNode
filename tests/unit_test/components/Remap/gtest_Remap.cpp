// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "gtest/gtest.h"
#include <chrono>
#include <cmath>
#include <stdio.h>
#include <string>

#include "QC/component/Remap.hpp"
#include "md5_utils.hpp"

using namespace QC;
using namespace QC::component;
using namespace QC::test::utils;

void SetCommonParam( Remap_Config_t *pRemapConfig )
{
    pRemapConfig->processor = QC_PROCESSOR_HTP0;
    pRemapConfig->numOfInputs = 2;
    for ( uint32_t inputId = 0; inputId < pRemapConfig->numOfInputs; inputId++ )
    {
        pRemapConfig->inputConfigs[inputId].inputFormat = QC_IMAGE_FORMAT_UYVY;
        pRemapConfig->inputConfigs[inputId].inputWidth = 512;
        pRemapConfig->inputConfigs[inputId].inputHeight = 512;
        pRemapConfig->inputConfigs[inputId].mapWidth = 256;
        pRemapConfig->inputConfigs[inputId].mapHeight = 256;
        pRemapConfig->inputConfigs[inputId].ROI.x = 0;
        pRemapConfig->inputConfigs[inputId].ROI.y = 0;
        pRemapConfig->inputConfigs[inputId].ROI.width = 256;
        pRemapConfig->inputConfigs[inputId].ROI.height = 256;
    }
    pRemapConfig->outputFormat = QC_IMAGE_FORMAT_RGB888;
    pRemapConfig->outputWidth = 256;
    pRemapConfig->outputHeight = 256;
    pRemapConfig->bEnableUndistortion = false;
    pRemapConfig->bEnableNormalize = false;
    return;
}

void CoverTest1()
{
    QCStatus_e ret = QC_STATUS_OK;

    Remap RemapObj;
    Remap_Config_t RemapConfig;
    char pName[10] = "Remap";
    QCSharedBuffer_t inputs[1];
    QCSharedBuffer_t output;

    ret = RemapObj.Start();
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );   // start before init

    ret = RemapObj.Stop();
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );   // stop before init

    ret = RemapObj.Deinit();
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );   // deinit before init

    ret = RemapObj.RegisterBuffers( &output, 1,
                                    FADAS_BUF_TYPE_OUT );   // register buffer before init
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

    ret = RemapObj.DeRegisterBuffers( &output, 1 );   // deregister buffer before init
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

    ret = RemapObj.Execute( inputs, 1, &output );   // execute before init
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

    SetCommonParam( &RemapConfig );
    ret = RemapObj.Init( pName, &RemapConfig );   // success init
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = RemapObj.Init( pName, &RemapConfig );   // init twice, wrong status
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

    ret = RemapObj.Deinit();   // success deinit
    ASSERT_EQ( QC_STATUS_OK, ret );

    return;
}

void CoverTest2()
{
    QCStatus_e ret = QC_STATUS_OK;

    Remap RemapObj;
    Remap_Config_t RemapConfig;
    char pName[10] = "Remap";
    QCSharedBuffer_t inputs[1];
    QCSharedBuffer_t output;

    ret = RemapObj.Init( pName, nullptr );   // null pointer for remap configuration
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    setenv( "QC_FADAS_CLIENT_ID", "15", 1 );   // wrong client id
    SetCommonParam( &RemapConfig );
    RemapConfig.processor = QC_PROCESSOR_MAX;
    ret = RemapObj.Init( pName, &RemapConfig );   // wrong processor type
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    unsetenv( "QC_FADAS_CLIENT_ID" );

    setenv( "QC_FADAS_CLIENT_ID", "-1", 1 );   // wrong client id
    SetCommonParam( &RemapConfig );
    RemapConfig.numOfInputs = QC_MAX_INPUTS + 1;
    ret = RemapObj.Init( pName, &RemapConfig );   // wrong number of inputs
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    unsetenv( "QC_FADAS_CLIENT_ID" );

    setenv( "QC_FADAS_CLIENT_ID", "a", 1 );   // wrong client id
    SetCommonParam( &RemapConfig );
    RemapConfig.outputFormat = QC_IMAGE_FORMAT_MAX;
    ret = RemapObj.Init( pName, &RemapConfig );   // wrong output format for DSP
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    unsetenv( "QC_FADAS_CLIENT_ID" );

    setenv( "QC_FADAS_CLIENT_ID", "1", 1 );
    SetCommonParam( &RemapConfig );
    RemapConfig.inputConfigs[0].inputFormat = QC_IMAGE_FORMAT_MAX;
    ret = RemapObj.Init( pName, &RemapConfig );   // wrong input format for DSP
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    unsetenv( "QC_FADAS_CLIENT_ID" );

    SetCommonParam( &RemapConfig );
    RemapConfig.outputFormat = QC_IMAGE_FORMAT_MAX;
    RemapConfig.processor = QC_PROCESSOR_CPU;
    ret = RemapObj.Init( pName, &RemapConfig );   // wrong output format for CPU
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    SetCommonParam( &RemapConfig );
    RemapConfig.inputConfigs[0].inputFormat = QC_IMAGE_FORMAT_MAX;
    RemapConfig.processor = QC_PROCESSOR_CPU;
    ret = RemapObj.Init( pName, &RemapConfig );   // wrong input format for CPU
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    SetCommonParam( &RemapConfig );
    RemapConfig.processor = QC_PROCESSOR_HTP0;
    RemapConfig.inputConfigs[0].inputFormat = QC_IMAGE_FORMAT_RGB888;
    RemapConfig.outputFormat = QC_IMAGE_FORMAT_RGB888;
    RemapConfig.bEnableNormalize = true;
    ret = RemapObj.Init( pName, &RemapConfig );   // wrong dsp pipeline
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    SetCommonParam( &RemapConfig );
    RemapConfig.processor = QC_PROCESSOR_CPU;
    RemapConfig.inputConfigs[0].inputFormat = QC_IMAGE_FORMAT_RGB888;
    RemapConfig.outputFormat = QC_IMAGE_FORMAT_RGB888;
    RemapConfig.bEnableNormalize = true;
    ret = RemapObj.Init( pName, &RemapConfig );   // wrong cpu&gpu pipeline
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    SetCommonParam( &RemapConfig );
    RemapConfig.bEnableUndistortion = true;
    RemapConfig.inputConfigs[0].remapTable.pMapX = nullptr;
    RemapConfig.inputConfigs[0].remapTable.pMapY = nullptr;
    ret = RemapObj.Init( pName, &RemapConfig );   // null pointer for map table
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    SetCommonParam( &RemapConfig );
    ret = RemapObj.Init( pName, &RemapConfig,
                         LOGGER_LEVEL_MAX );   // success init with invalid logger level and invalid
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = RemapObj.RegisterBuffers(
            nullptr, 1,
            FADAS_BUF_TYPE_OUT );   // null pointer for buffer to be register
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    ret = RemapObj.DeRegisterBuffers( nullptr, 1 );   // null pointer for buffer to be deregister
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    ret = RemapObj.Execute( nullptr, RemapConfig.numOfInputs,
                            &output );   // null pointer for input buffer
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    ret = RemapObj.Execute( inputs, RemapConfig.numOfInputs,
                            nullptr );   // null pointer for output buffer
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    ret = RemapObj.Execute( inputs, RemapConfig.numOfInputs + 1,
                            &output );   // wrong input buffer number
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    ret = RemapObj.Deinit();   // success deinit
    ASSERT_EQ( QC_STATUS_OK, ret );

    return;
}

void CoverTest3()
{
    QCStatus_e ret = QC_STATUS_OK;

    Remap RemapObj;
    Remap_Config_t RemapConfig;
    char pName[10] = "Remap";

    SetCommonParam( &RemapConfig );
    ret = RemapObj.Init( pName, &RemapConfig );   // success init
    ASSERT_EQ( QC_STATUS_OK, ret );
    QCSharedBuffer_t inputs[RemapConfig.numOfInputs];
    QCSharedBuffer_t output;

    ret = output.Allocate( RemapConfig.numOfInputs, RemapConfig.outputWidth,
                           RemapConfig.outputHeight, RemapConfig.outputFormat );
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( uint32_t inputId = 0; inputId < RemapConfig.numOfInputs; inputId++ )
    {
        ret = inputs[inputId].Allocate( RemapConfig.inputConfigs[inputId].inputWidth,
                                        RemapConfig.inputConfigs[inputId].inputHeight,
                                        QC_IMAGE_FORMAT_NV12 );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
    ret = RemapObj.Execute( inputs, RemapConfig.numOfInputs, &output );   // wrong input format
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    for ( uint32_t inputId = 0; inputId < RemapConfig.numOfInputs; inputId++ )
    {
        ret = inputs[inputId].Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    for ( uint32_t inputId = 0; inputId < RemapConfig.numOfInputs; inputId++ )
    {
        ret = inputs[inputId].Allocate( RemapConfig.inputConfigs[inputId].inputWidth + 1,
                                        RemapConfig.inputConfigs[inputId].inputHeight,
                                        RemapConfig.inputConfigs[inputId].inputFormat );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
    ret = RemapObj.Execute( inputs, RemapConfig.numOfInputs, &output );   // wrong input width
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    for ( uint32_t inputId = 0; inputId < RemapConfig.numOfInputs; inputId++ )
    {
        ret = inputs[inputId].Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    for ( uint32_t inputId = 0; inputId < RemapConfig.numOfInputs; inputId++ )
    {
        ret = inputs[inputId].Allocate( RemapConfig.inputConfigs[inputId].inputWidth,
                                        RemapConfig.inputConfigs[inputId].inputHeight + 1,
                                        RemapConfig.inputConfigs[inputId].inputFormat );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
    ret = RemapObj.Execute( inputs, RemapConfig.numOfInputs, &output );   // wrong input height
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    for ( uint32_t inputId = 0; inputId < RemapConfig.numOfInputs; inputId++ )
    {
        ret = inputs[inputId].Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    for ( uint32_t inputId = 0; inputId < RemapConfig.numOfInputs; inputId++ )
    {
        ret = inputs[inputId].Allocate( 2, RemapConfig.inputConfigs[inputId].inputWidth,
                                        RemapConfig.inputConfigs[inputId].inputHeight,
                                        RemapConfig.inputConfigs[inputId].inputFormat );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
    ret = RemapObj.Execute( inputs, RemapConfig.numOfInputs, &output );   // wrong input batch
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    for ( uint32_t inputId = 0; inputId < RemapConfig.numOfInputs; inputId++ )
    {
        ret = inputs[inputId].Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    for ( uint32_t inputId = 0; inputId < RemapConfig.numOfInputs; inputId++ )
    {
        ret = inputs[inputId].Allocate( RemapConfig.inputConfigs[inputId].inputWidth,
                                        RemapConfig.inputConfigs[inputId].inputHeight,
                                        RemapConfig.inputConfigs[inputId].inputFormat );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
    ret = RemapObj.RegisterBuffers( inputs, RemapConfig.numOfInputs, FADAS_BUF_TYPE_INOUT );
    ASSERT_EQ( QC_STATUS_OK, ret );
    inputs[0].buffer.size++;
    ret = RemapObj.Execute( inputs, RemapConfig.numOfInputs, &output );   // invalid input buffer
    ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
    inputs[0].buffer.size--;
    ret = RemapObj.DeRegisterBuffers( inputs, RemapConfig.numOfInputs );
    ASSERT_EQ( QC_STATUS_OK, ret );
    for ( uint32_t inputId = 0; inputId < RemapConfig.numOfInputs; inputId++ )
    {
        ret = inputs[inputId].Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    ret = output.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( uint32_t inputId = 0; inputId < RemapConfig.numOfInputs; inputId++ )
    {
        ret = inputs[inputId].Allocate( RemapConfig.inputConfigs[inputId].inputWidth,
                                        RemapConfig.inputConfigs[inputId].inputHeight,
                                        RemapConfig.inputConfigs[inputId].inputFormat );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    ret = output.Allocate( RemapConfig.numOfInputs, RemapConfig.outputWidth,
                           RemapConfig.outputHeight, QC_IMAGE_FORMAT_BGR888 );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = RemapObj.Execute( inputs, RemapConfig.numOfInputs, &output );   // wrong output format
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    ret = output.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = output.Allocate( RemapConfig.numOfInputs, RemapConfig.outputWidth + 1,
                           RemapConfig.outputHeight, RemapConfig.outputFormat );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = RemapObj.Execute( inputs, RemapConfig.numOfInputs, &output );   // wrong output width
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    ret = output.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = output.Allocate( RemapConfig.numOfInputs, RemapConfig.outputWidth,
                           RemapConfig.outputHeight + 1, RemapConfig.outputFormat );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = RemapObj.Execute( inputs, RemapConfig.numOfInputs, &output );   // wrong output height
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    ret = output.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = output.Allocate( RemapConfig.numOfInputs + 1, RemapConfig.outputWidth,
                           RemapConfig.outputHeight, RemapConfig.outputFormat );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = RemapObj.Execute( inputs, RemapConfig.numOfInputs, &output );   // wrong output batch
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    ret = output.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = output.Allocate( RemapConfig.numOfInputs, RemapConfig.outputWidth,
                           RemapConfig.outputHeight, RemapConfig.outputFormat );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = RemapObj.RegisterBuffers( &output, 1, FADAS_BUF_TYPE_INOUT );
    ASSERT_EQ( QC_STATUS_OK, ret );
    output.buffer.size++;
    ret = RemapObj.Execute( inputs, RemapConfig.numOfInputs, &output );   // invalid  output buffer
    ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
    output.buffer.size--;
    ret = RemapObj.DeRegisterBuffers( &output, 1 );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = output.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = RemapObj.RegisterBuffers( inputs, RemapConfig.numOfInputs, FADAS_BUF_TYPE_IN );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = RemapObj.RegisterBuffers( inputs, RemapConfig.numOfInputs,
                                    FADAS_BUF_TYPE_IN );   // register twice
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = RemapObj.Deinit();   // success deinit with registered buffers
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( uint32_t inputId = 0; inputId < RemapConfig.numOfInputs; inputId++ )
    {
        ret = inputs[inputId].Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    return;
}

void CoverTest4()
{
    QCStatus_e ret = QC_STATUS_OK;

    Remap RemapObj;
    Remap_Config_t RemapConfig;
    char pName[10] = "Remap";

    SetCommonParam( &RemapConfig );
    RemapConfig.processor = QC_PROCESSOR_HTP1;
    ret = RemapObj.Init( pName, &RemapConfig );   // success init with dsp1
    ASSERT_EQ( QC_STATUS_OK, ret );
    QCSharedBuffer_t inputs[RemapConfig.numOfInputs];

    for ( uint32_t inputId = 0; inputId < RemapConfig.numOfInputs; inputId++ )
    {
        ret = inputs[inputId].Allocate( RemapConfig.inputConfigs[inputId].inputWidth,
                                        RemapConfig.inputConfigs[inputId].inputHeight,
                                        RemapConfig.inputConfigs[inputId].inputFormat );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    ret = RemapObj.RegisterBuffers( inputs, RemapConfig.numOfInputs,
                                    FADAS_BUF_TYPE_IN );   // success register buffer
    ASSERT_EQ( QC_STATUS_OK, ret );

    inputs[0].imgProps.batchSize++;
    ret = RemapObj.RegisterBuffers(
            inputs, RemapConfig.numOfInputs,
            FADAS_BUF_TYPE_IN );   // register with mismatch image batch size
    ASSERT_EQ( QC_STATUS_FAIL, ret );
    inputs[0].imgProps.batchSize--;

    ret = RemapObj.DeRegisterBuffers( inputs, RemapConfig.numOfInputs );   // deregister with dsp1
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = RemapObj.RegisterBuffers( inputs, RemapConfig.numOfInputs,
                                    FADAS_BUF_TYPE_MAX );   // register with wrong buffer type
    ASSERT_EQ( QC_STATUS_FAIL, ret );

    for ( uint32_t inputId = 0; inputId < RemapConfig.numOfInputs; inputId++ )
    {
        ret = inputs[inputId].Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    return;
}

void CoverTest5()
{
    QCStatus_e ret = QC_STATUS_OK;
    int32_t fd = 0;

    Remap RemapObj;
    Remap_Config_t RemapConfig;
    char pName[10] = "Remap";
    QCSharedBuffer_t inputs[1];
    QCSharedBuffer_t output;
    FadasRemap FadasRemapObj;

    ret = FadasRemapObj.Init( QC_PROCESSOR_HTP1, pName,
                              LOGGER_LEVEL_ERROR );   // init with dsp1
    ASSERT_EQ( QC_STATUS_OK, ret );

    fd = FadasRemapObj.RegBuf( nullptr,
                               FADAS_BUF_TYPE_OUT );   // null pointer for buffer to be register
    ASSERT_EQ( -1, fd );

    fd = FadasRemapObj.RegBuf( &inputs[0], FADAS_BUF_TYPE_OUT );   // not image buffer
    ASSERT_EQ( -1, fd );

    FadasRemapObj.DeregBuf( nullptr );   // null pointer for buffer to be deregister

    return;
}

void SuccessTest( uint32_t batchTest, QCProcessorType_e processorTest,
                  QCImageFormat_e inputFormatTest, QCImageFormat_e outputFormatTest,
                  uint32_t inputWidthTest, uint32_t inputHeightTest, uint32_t outputWidthTest,
                  uint32_t outputHeightTest, bool bEnableUndistortionTest,
                  bool bEnableNormalizeTest, bool bCheckAccuracyTest, bool bCheckPerformanceTest )
{
    QCStatus_e ret = QC_STATUS_OK;

    Remap RemapObj;
    Remap_Config_t RemapConfig;
    char pName[10] = "Remap";

    RemapConfig.processor = processorTest;
    RemapConfig.numOfInputs = batchTest;
    for ( uint32_t inputId = 0; inputId < RemapConfig.numOfInputs; inputId++ )
    {
        RemapConfig.inputConfigs[inputId].inputFormat = inputFormatTest;
        RemapConfig.inputConfigs[inputId].inputWidth = inputWidthTest;
        RemapConfig.inputConfigs[inputId].inputHeight = inputHeightTest;
        RemapConfig.inputConfigs[inputId].mapWidth = outputWidthTest;
        RemapConfig.inputConfigs[inputId].mapHeight = outputHeightTest;
        RemapConfig.inputConfigs[inputId].ROI.x = 0;
        RemapConfig.inputConfigs[inputId].ROI.y = 0;
        RemapConfig.inputConfigs[inputId].ROI.width = outputWidthTest;
        RemapConfig.inputConfigs[inputId].ROI.height = outputHeightTest;
    }
    RemapConfig.outputFormat = outputFormatTest;
    RemapConfig.outputWidth = outputWidthTest;
    RemapConfig.outputHeight = outputHeightTest;
    RemapConfig.bEnableUndistortion = bEnableUndistortionTest;
    RemapConfig.bEnableNormalize = bEnableNormalizeTest;

    if ( bEnableNormalizeTest == true )
    {
        RemapConfig.normlzR.sub = 0.0;
        RemapConfig.normlzR.mul = 1.0;
        RemapConfig.normlzR.add = 0.0;
        RemapConfig.normlzG.sub = 0.0;
        RemapConfig.normlzG.mul = 1.0;
        RemapConfig.normlzG.add = 0.0;
        RemapConfig.normlzB.sub = 0.0;
        RemapConfig.normlzB.mul = 1.0;
        RemapConfig.normlzB.add = 0.0;
    }

    QCSharedBuffer_t mapXBuffer[RemapConfig.numOfInputs];
    QCSharedBuffer_t mapYBuffer[RemapConfig.numOfInputs];
    if ( bEnableUndistortionTest == true )
    {
        for ( uint32_t inputId = 0; inputId < RemapConfig.numOfInputs; inputId++ )
        {
            uint32_t mapWidth = RemapConfig.inputConfigs[inputId].mapWidth;
            uint32_t mapHeight = RemapConfig.inputConfigs[inputId].mapHeight;
            uint32_t inputWidth = RemapConfig.inputConfigs[inputId].inputWidth;
            uint32_t inputHeight = RemapConfig.inputConfigs[inputId].inputHeight;

            QCTensorProps_t mapXProp;
            mapXProp = {
                    QC_TENSOR_TYPE_FLOAT_32,
                    { mapWidth, mapHeight, 0 },
                    2,
            };
            ret = mapXBuffer[inputId].Allocate( &mapXProp );
            ASSERT_EQ( QC_STATUS_OK, ret );
            QCTensorProps_t mapYProp;
            mapYProp = {
                    QC_TENSOR_TYPE_FLOAT_32,
                    { mapWidth, mapHeight, 0 },
                    2,
            };
            ret = mapYBuffer[inputId].Allocate( &mapYProp );
            ASSERT_EQ( QC_STATUS_OK, ret );

            float *mapX = (float *) mapXBuffer[inputId].data();
            float *mapY = (float *) mapYBuffer[inputId].data();
            for ( int i = 0; i < mapHeight; i++ )
            {
                for ( int j = 0; j < mapWidth; j++ )
                {
                    mapX[i * mapWidth + j] = (float) j / (float) mapWidth * (float) inputWidth;
                    mapY[i * mapWidth + j] = (float) i / (float) mapHeight * (float) inputHeight;
                }
            }

            RemapConfig.inputConfigs[inputId].remapTable.pMapX = &mapXBuffer[inputId];
            RemapConfig.inputConfigs[inputId].remapTable.pMapY = &mapYBuffer[inputId];
        }
    }

    QCSharedBuffer_t inputs[RemapConfig.numOfInputs];
    for ( uint32_t inputId = 0; inputId < RemapConfig.numOfInputs; inputId++ )
    {
        ret = inputs[inputId].Allocate( RemapConfig.inputConfigs[inputId].inputWidth,
                                        RemapConfig.inputConfigs[inputId].inputHeight,
                                        RemapConfig.inputConfigs[inputId].inputFormat );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    if ( bCheckAccuracyTest == true )
    {
        size_t inputSize[QC_MAX_INPUTS];
        for ( uint32_t inputId = 0; inputId < RemapConfig.numOfInputs; inputId++ )
        {
            if ( RemapConfig.inputConfigs[inputId].inputFormat == QC_IMAGE_FORMAT_UYVY )
            {
                inputSize[inputId] = RemapConfig.inputConfigs[inputId].inputWidth *
                                     RemapConfig.inputConfigs[inputId].inputHeight * 2;
            }
            else if ( RemapConfig.inputConfigs[inputId].inputFormat == QC_IMAGE_FORMAT_RGB888 )
            {
                inputSize[inputId] = RemapConfig.inputConfigs[inputId].inputWidth *
                                     RemapConfig.inputConfigs[inputId].inputHeight * 3;
            }
            else if ( RemapConfig.inputConfigs[inputId].inputFormat == QC_IMAGE_FORMAT_NV12 )
            {
                inputSize[inputId] = RemapConfig.inputConfigs[inputId].inputWidth *
                                     RemapConfig.inputConfigs[inputId].inputHeight * 1.5;
            }
            else if ( RemapConfig.inputConfigs[inputId].inputFormat == QC_IMAGE_FORMAT_NV12_UBWC )
            {
                inputSize[inputId] = inputs[inputId].size;
            }
        }

        printf( "inputData is: \n" );
        for ( uint32_t inputId = 0; inputId < RemapConfig.numOfInputs; inputId++ )
        {
            uint8_t *inputData = (uint8_t *) inputs[inputId].data();
            for ( int i = 0; i < inputSize[inputId]; i++ )
            {
                inputData[i] = i % 256;
            }
            for ( int i = 0; i < 6; i++ )
            {
                printf( "inputId = %d, i = %d, data = %d \n", inputId, i, inputData[i] );
            }
        }
    }

    QCSharedBuffer_t output;
    ret = output.Allocate( RemapConfig.numOfInputs, RemapConfig.outputWidth,
                           RemapConfig.outputHeight, RemapConfig.outputFormat );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = RemapObj.Init( pName, &RemapConfig );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = RemapObj.Start();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = RemapObj.RegisterBuffers( inputs, RemapConfig.numOfInputs, FADAS_BUF_TYPE_IN );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = RemapObj.RegisterBuffers( &output, 1, FADAS_BUF_TYPE_OUT );
    ASSERT_EQ( QC_STATUS_OK, ret );

    if ( bCheckPerformanceTest == true )
    {
        uint32_t times = 100;
        auto start = std::chrono::high_resolution_clock::now();
        for ( int i = 0; i < times; i++ )
        {
            ret = RemapObj.Execute( inputs, RemapConfig.numOfInputs, &output );
        }
        auto end = std::chrono::high_resolution_clock::now();
        double duration_ms = std::chrono::duration<double, std::milli>( end - start ).count();
        printf( "%d*%d to %d*%d, execute time = %f ms\n", inputWidthTest, inputHeightTest,
                outputWidthTest, outputHeightTest, (float) duration_ms / (float) times );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
    else
    {
        ret = RemapObj.Execute( inputs, RemapConfig.numOfInputs, &output );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    ret = RemapObj.DeRegisterBuffers( inputs, RemapConfig.numOfInputs );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = RemapObj.DeRegisterBuffers( &output, 1 );
    ASSERT_EQ( QC_STATUS_OK, ret );

    if ( bCheckAccuracyTest == true )
    {
        printf( "outputData is: \n" );
        size_t outputSize = RemapConfig.outputWidth * RemapConfig.outputHeight * 3;
        uint8_t *outputData = (uint8_t *) output.data();
        for ( uint32_t inputId = 0; inputId < RemapConfig.numOfInputs; inputId++ )
        {
            for ( int i = 0; i < 6; i++ )
            {
                printf( "inputId = %d, i = %d, data = %d \n", inputId, i,
                        outputData[inputId * outputSize + i] );
            }
        }
    }

    ret = RemapObj.Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = RemapObj.Deinit();
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( uint32_t inputId = 0; inputId < RemapConfig.numOfInputs; inputId++ )
    {
        ret = inputs[inputId].Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    ret = output.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );

    if ( bEnableUndistortionTest == true )
    {
        for ( uint32_t inputId = 0; inputId < RemapConfig.numOfInputs; inputId++ )
        {

            ret = mapXBuffer[inputId].Free();
            ASSERT_EQ( QC_STATUS_OK, ret );
            ret = mapYBuffer[inputId].Free();
            ASSERT_EQ( QC_STATUS_OK, ret );
        }
    }

    return;
}

void ImageTest( QCProcessorType_e processorTest, QCImageFormat_e inputFormatTest,
                QCImageFormat_e outputFormatTest, uint32_t inputWidthTest, uint32_t inputHeightTest,
                uint32_t outputWidthTest, uint32_t outputHeightTest, std::string pathTest,
                std::string goldenPath, bool normTest, bool saveOutput )
{
    QCStatus_e ret = QC_STATUS_OK;

    Remap RemapObj;
    Remap_Config_t RemapConfig;
    char pName[10] = "Remap";

    RemapConfig.processor = processorTest;
    RemapConfig.numOfInputs = 2;
    for ( uint32_t inputId = 0; inputId < RemapConfig.numOfInputs; inputId++ )
    {
        RemapConfig.inputConfigs[inputId].inputFormat = inputFormatTest;
        RemapConfig.inputConfigs[inputId].inputWidth = inputWidthTest;
        RemapConfig.inputConfigs[inputId].inputHeight = inputHeightTest;
        RemapConfig.inputConfigs[inputId].mapWidth = outputWidthTest;
        RemapConfig.inputConfigs[inputId].mapHeight = outputHeightTest;
        RemapConfig.inputConfigs[inputId].ROI.x = 0;
        RemapConfig.inputConfigs[inputId].ROI.y = 0;
        RemapConfig.inputConfigs[inputId].ROI.width = outputWidthTest;
        RemapConfig.inputConfigs[inputId].ROI.height = outputHeightTest;
    }
    RemapConfig.outputFormat = outputFormatTest;
    RemapConfig.outputWidth = outputWidthTest;
    RemapConfig.outputHeight = outputHeightTest;
    RemapConfig.bEnableUndistortion = false;
    if ( true == normTest )
    {
        RemapConfig.bEnableNormalize = true;
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
    else
    {
        RemapConfig.bEnableNormalize = false;
    }

    QCSharedBuffer_t inputs[RemapConfig.numOfInputs];
    for ( uint32_t inputId = 0; inputId < RemapConfig.numOfInputs; inputId++ )
    {
        QCImageProps_t imgProp;
        imgProp.batchSize = 1;
        imgProp.width = RemapConfig.inputConfigs[inputId].inputWidth;
        imgProp.height = RemapConfig.inputConfigs[inputId].inputHeight;
        imgProp.format = RemapConfig.inputConfigs[inputId].inputFormat;
        if ( QC_IMAGE_FORMAT_UYVY == RemapConfig.inputConfigs[inputId].inputFormat )
        {
            imgProp.stride[0] = RemapConfig.inputConfigs[inputId].inputWidth * 2;
            imgProp.actualHeight[0] = RemapConfig.inputConfigs[inputId].inputHeight;
            imgProp.planeBufSize[0] = 0;
            imgProp.numPlanes = 1;
        }
        else if ( QC_IMAGE_FORMAT_NV12 == RemapConfig.inputConfigs[inputId].inputFormat )
        {
            imgProp.stride[0] = RemapConfig.inputConfigs[inputId].inputWidth;
            imgProp.actualHeight[0] = RemapConfig.inputConfigs[inputId].inputHeight;
            imgProp.stride[1] = RemapConfig.inputConfigs[inputId].inputWidth;
            imgProp.actualHeight[1] = RemapConfig.inputConfigs[inputId].inputHeight / 2;
            imgProp.planeBufSize[0] = 0;
            imgProp.planeBufSize[1] = 0;
            imgProp.numPlanes = 2;
        }
        else if ( QC_IMAGE_FORMAT_RGB888 == RemapConfig.inputConfigs[inputId].inputFormat )
        {
            imgProp.stride[0] = RemapConfig.inputConfigs[inputId].inputWidth * 3;
            imgProp.actualHeight[0] = RemapConfig.inputConfigs[inputId].inputHeight;
            imgProp.planeBufSize[0] = 0;
            imgProp.numPlanes = 1;
        }

        ret = inputs[inputId].Allocate( &imgProp );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    FILE *file1 = nullptr;
    size_t length1 = 0;
    file1 = fopen( pathTest.c_str(), "rb" );
    if ( nullptr == file1 )
    {
        printf( "could not open image file %s\n", pathTest.c_str() );
    }
    else
    {
        for ( uint32_t inputId = 0; inputId < RemapConfig.numOfInputs; inputId++ )
        {
            fseek( file1, 0, SEEK_END );
            length1 = (size_t) ftell( file1 );
            if ( inputs[0].size != length1 )
            {
                printf( "image file %s size not match, need %d but got %d\n", pathTest.c_str(),
                        (int) inputs[0].size, (int) length1 );
            }
            else
            {
                fseek( file1, 0, SEEK_SET );
                auto r = fread( inputs[inputId].data(), 1, length1, file1 );
                if ( length1 != r )
                {
                    printf( "failed to read image file %s at id=%d, need %d but read %d\n",
                            pathTest.c_str(), inputId, (int) length1, (int) r );
                }
            }
        }
        fclose( file1 );
    }

    QCImageProps_t imgProp;
    imgProp.batchSize = RemapConfig.numOfInputs;
    imgProp.width = RemapConfig.outputWidth;
    imgProp.height = RemapConfig.outputHeight;
    imgProp.format = RemapConfig.outputFormat;
    imgProp.stride[0] = RemapConfig.outputWidth * 3;
    imgProp.actualHeight[0] = RemapConfig.outputHeight;
    imgProp.planeBufSize[0] = 0;
    imgProp.numPlanes = 1;

    QCSharedBuffer_t output;
    ret = output.Allocate( &imgProp );
    ASSERT_EQ( QC_STATUS_OK, ret );
    memset( output.data(), 0, output.size );

    ret = RemapObj.Init( pName, &RemapConfig );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = RemapObj.Execute( inputs, RemapConfig.numOfInputs, &output );
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
    ret = golden.Allocate( &imgProp );
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
    EXPECT_EQ( md5Output, md5Golden );

    ret = RemapObj.Deinit();
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( uint32_t inputId = 0; inputId < RemapConfig.numOfInputs; inputId++ )
    {
        ret = inputs[inputId].Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    ret = output.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = golden.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );

    return;
}

TEST( Remap, ImageAccuracyTest )   // image pipeline md5 accuracy tests
{
    // md5 of 0.uyvy is 5b1ae2203a9d97aeafe65e997f3beebc
    // md5 of golden_cpu.rgb is 59760700b59beb67227d305b317dcec6
    // md5 of golden_dsp.rgb is f139fb73234986a15e340afe1058522e
    // md5 of golden_gpu.rgb is 59760700b59beb67227d305b317dcec6
    printf( "DSP image accuracy test\n" );
    ImageTest( QC_PROCESSOR_HTP0, QC_IMAGE_FORMAT_UYVY, QC_IMAGE_FORMAT_RGB888, 1920, 1024, 1152,
               800, "./data/test/remap/0.uyvy", "./data/test/remap/golden_dsp.rgb", true, false );
    printf( "CPU image accuracy test\n" );
    ImageTest( QC_PROCESSOR_CPU, QC_IMAGE_FORMAT_UYVY, QC_IMAGE_FORMAT_RGB888, 1920, 1024, 1152,
               800, "./data/test/remap/0.uyvy", "./data/test/remap/golden_cpu.rgb", true, false );
#if defined( USE_ENG_FADAS_GPU )
    printf( "GPU image accuracy test\n" );
    ImageTest( QC_PROCESSOR_GPU, QC_IMAGE_FORMAT_UYVY, QC_IMAGE_FORMAT_RGB888, 1920, 1024, 1152,
               800, "./data/test/remap/0.uyvy", "./data/test/remap/golden_gpu.rgb", true, false );
#endif
}

TEST( Remap, GeneralAccuracyTest )   // general accuracy test for DSP&CPU backend, RGB to RGB
                                     // pipeline, no undistortion and no renormalization
{
    printf( "DSP general accuracy test\n" );
    SuccessTest( 2, QC_PROCESSOR_HTP0, QC_IMAGE_FORMAT_RGB888, QC_IMAGE_FORMAT_RGB888, 512, 512,
                 256, 256, false, false, true, false );
    printf( "CPU general accuracy test\n" );
    SuccessTest( 2, QC_PROCESSOR_CPU, QC_IMAGE_FORMAT_RGB888, QC_IMAGE_FORMAT_RGB888, 512, 512, 256,
                 256, false, false, true, false );
#if defined( USE_ENG_FADAS_GPU )
    printf( "GPU general accuracy test\n" );
    SuccessTest( 2, QC_PROCESSOR_GPU, QC_IMAGE_FORMAT_RGB888, QC_IMAGE_FORMAT_RGB888, 512, 512, 256,
                 256, false, false, true, false );
#endif
    printf( "map table general accuracy test\n" );
    SuccessTest( 2, QC_PROCESSOR_HTP0, QC_IMAGE_FORMAT_RGB888, QC_IMAGE_FORMAT_RGB888, 512, 512,
                 256, 256, true, false, true, false );
}

TEST( Remap, GeneralPerformanceTest )   // general performance test for DSP&CPU backend, UYVY to
                                        // RGB pipeline, no undistortion and no renormalization
{
    printf( "DSP UYVY general performance test\n" );
    SuccessTest( 2, QC_PROCESSOR_HTP0, QC_IMAGE_FORMAT_UYVY, QC_IMAGE_FORMAT_RGB888, 1920, 1024,
                 1152, 800, false, false, false, true );
    printf( "CPU UYVY general performance test\n" );
    SuccessTest( 2, QC_PROCESSOR_CPU, QC_IMAGE_FORMAT_UYVY, QC_IMAGE_FORMAT_RGB888, 1920, 1024,
                 1152, 800, false, false, false, true );
#if defined( USE_ENG_FADAS_GPU )
    printf( "GPU UYVY general performance test\n" );
    SuccessTest( 2, QC_PROCESSOR_GPU, QC_IMAGE_FORMAT_UYVY, QC_IMAGE_FORMAT_RGB888, 1920, 1024,
                 1152, 800, false, false, false, true );
#endif
#if defined( USE_ENG_FADAS_DSP )
    printf( "DSP NV12 general performance test\n" );
    SuccessTest( 2, QC_PROCESSOR_HTP0, QC_IMAGE_FORMAT_NV12, QC_IMAGE_FORMAT_BGR888, 1920, 1024,
                 1152, 800, false, false, false, true );
#endif
#if defined( USE_ENG_FADAS_CPU )
    printf( "CPU NV12 general performance test\n" );
    SuccessTest( 2, QC_PROCESSOR_CPU, QC_IMAGE_FORMAT_NV12, QC_IMAGE_FORMAT_RGB888, 1920, 1024,
                 1152, 800, false, false, false, true );
#endif
#if defined( USE_ENG_FADAS_GPU )
    printf( "GPU NV12 general performance test\n" );
    SuccessTest( 2, QC_PROCESSOR_GPU, QC_IMAGE_FORMAT_NV12, QC_IMAGE_FORMAT_BGR888, 1920, 1024,
                 1152, 800, false, false, false, true );
#endif
}

TEST( Remap, CoverTest )   // fail path tests
{
    CoverTest1();   // bad status error
    CoverTest2();   // bad arguments error
    CoverTest3();   // wrong input&output buffer
    CoverTest4();   // cover error paths in RegisterBuffers
    CoverTest5();   // call FadasRemap class directly
}

TEST( Remap, CPUSuccessPipeline1Test )   // general success test on CPU for UYVY/RGB to RGB, with
                                         // and without normalization, no undistortion
{
    SuccessTest( 2, QC_PROCESSOR_CPU, QC_IMAGE_FORMAT_RGB888, QC_IMAGE_FORMAT_RGB888, 512, 512, 256,
                 256, false, false, false, false );
    SuccessTest( 2, QC_PROCESSOR_CPU, QC_IMAGE_FORMAT_UYVY, QC_IMAGE_FORMAT_RGB888, 512, 512, 256,
                 256, false, false, false, false );
    SuccessTest( 2, QC_PROCESSOR_CPU, QC_IMAGE_FORMAT_UYVY, QC_IMAGE_FORMAT_RGB888, 512, 512, 256,
                 256, false, true, false, false );
}

TEST( Remap, CPUSuccessPipeline2Test )   // general success test on CPU for UYVY/RGB to RGB, with
                                         // and without normalization, undistortion
{
    SuccessTest( 2, QC_PROCESSOR_CPU, QC_IMAGE_FORMAT_RGB888, QC_IMAGE_FORMAT_RGB888, 512, 512, 256,
                 256, true, false, false, false );
    SuccessTest( 2, QC_PROCESSOR_CPU, QC_IMAGE_FORMAT_UYVY, QC_IMAGE_FORMAT_RGB888, 512, 512, 256,
                 256, true, false, false, false );
    SuccessTest( 2, QC_PROCESSOR_CPU, QC_IMAGE_FORMAT_UYVY, QC_IMAGE_FORMAT_RGB888, 512, 512, 256,
                 256, true, true, false, false );
}

#if defined( USE_ENG_FADAS_CPU )   // nv12 input and bgr output format on CPU is only supported in
                                   // eng version
TEST( Remap, CPUSuccessPipeline3Test )   // general success test on CPU for NV12/UYVY to RGB, with
                                         // and without undistortion
{
    SuccessTest( 2, QC_PROCESSOR_CPU, QC_IMAGE_FORMAT_NV12, QC_IMAGE_FORMAT_RGB888, 512, 512, 256,
                 256, false, false, false, false );
    SuccessTest( 2, QC_PROCESSOR_CPU, QC_IMAGE_FORMAT_NV12, QC_IMAGE_FORMAT_RGB888, 512, 512, 256,
                 256, true, false, false, false );
}

TEST( Remap, CPUSuccessPipeline4Test )   // general success test on CPU for NV12 to RGB, with
                                         // and without normalization, undistortion
{
    SuccessTest( 2, QC_PROCESSOR_CPU, QC_IMAGE_FORMAT_NV12, QC_IMAGE_FORMAT_RGB888, 512, 512, 256,
                 256, false, false, false, false );
    SuccessTest( 2, QC_PROCESSOR_CPU, QC_IMAGE_FORMAT_NV12, QC_IMAGE_FORMAT_RGB888, 512, 512, 256,
                 256, true, false, false, false );
    SuccessTest( 2, QC_PROCESSOR_CPU, QC_IMAGE_FORMAT_NV12, QC_IMAGE_FORMAT_RGB888, 512, 512, 256,
                 256, false, true, false, false );
    SuccessTest( 2, QC_PROCESSOR_CPU, QC_IMAGE_FORMAT_NV12, QC_IMAGE_FORMAT_RGB888, 512, 512, 256,
                 256, true, true, false, false );
}
#endif

TEST( Remap, DSPSuccessPipeline1Test )   // general success test on DSP for UYVY/RGB to RGB, with
                                         // and without normalization, no undistortion
{
    SuccessTest( 2, QC_PROCESSOR_HTP0, QC_IMAGE_FORMAT_RGB888, QC_IMAGE_FORMAT_RGB888, 512, 512,
                 256, 256, false, false, false, false );
    SuccessTest( 2, QC_PROCESSOR_HTP0, QC_IMAGE_FORMAT_UYVY, QC_IMAGE_FORMAT_RGB888, 512, 512, 256,
                 256, false, false, false, false );
    SuccessTest( 2, QC_PROCESSOR_HTP0, QC_IMAGE_FORMAT_UYVY, QC_IMAGE_FORMAT_RGB888, 512, 512, 256,
                 256, false, true, false, false );
}

TEST( Remap, DSPSuccessPipeline2Test )   // general success test on DSP for UYVY/RGB to RGB, with
                                         // and without normalization, undistortion
{
    SuccessTest( 2, QC_PROCESSOR_HTP0, QC_IMAGE_FORMAT_RGB888, QC_IMAGE_FORMAT_RGB888, 512, 512,
                 256, 256, true, false, false, false );
    SuccessTest( 2, QC_PROCESSOR_HTP0, QC_IMAGE_FORMAT_UYVY, QC_IMAGE_FORMAT_RGB888, 512, 512, 256,
                 256, true, false, false, false );
    SuccessTest( 2, QC_PROCESSOR_HTP0, QC_IMAGE_FORMAT_UYVY, QC_IMAGE_FORMAT_RGB888, 512, 512, 256,
                 256, true, true, false, false );
}

#if defined( USE_ENG_FADAS_DSP )   // nv12 input and bgr output format on DSP is only supported in
                                   // eng version
TEST( Remap, DSPSuccessPipeline3Test )   // general success test on DSP for NV12/UYVY to BGR, with
                                         // and without undistortion
{
    SuccessTest( 2, QC_PROCESSOR_HTP0, QC_IMAGE_FORMAT_NV12, QC_IMAGE_FORMAT_BGR888, 512, 512, 256,
                 256, false, false, false, false );
    SuccessTest( 2, QC_PROCESSOR_HTP0, QC_IMAGE_FORMAT_NV12, QC_IMAGE_FORMAT_BGR888, 512, 512, 256,
                 256, true, false, false, false );
    SuccessTest( 2, QC_PROCESSOR_HTP0, QC_IMAGE_FORMAT_UYVY, QC_IMAGE_FORMAT_BGR888, 512, 512, 256,
                 256, false, false, false, false );
    SuccessTest( 2, QC_PROCESSOR_HTP0, QC_IMAGE_FORMAT_UYVY, QC_IMAGE_FORMAT_BGR888, 512, 512, 256,
                 256, true, false, false, false );
}
#endif

#if defined( USE_ENG_FADAS_GPU )
TEST( Remap, GPUSuccessPipeline1Test )   // general success test on GPU for UYVY/RGB to RGB, with
                                         // and without normalization, no undistortion
{
    SuccessTest( 2, QC_PROCESSOR_GPU, QC_IMAGE_FORMAT_RGB888, QC_IMAGE_FORMAT_RGB888, 512, 512, 256,
                 256, false, false, false, false );
    SuccessTest( 2, QC_PROCESSOR_GPU, QC_IMAGE_FORMAT_UYVY, QC_IMAGE_FORMAT_RGB888, 512, 512, 256,
                 256, false, false, false, false );
    SuccessTest( 2, QC_PROCESSOR_GPU, QC_IMAGE_FORMAT_UYVY, QC_IMAGE_FORMAT_RGB888, 512, 512, 256,
                 256, false, true, false, false );
}

TEST( Remap, GPUSuccessPipeline2Test )   // general success test on GPU for UYVY/RGB to RGB, with
                                         // and without normalization, undistortion
{
    SuccessTest( 2, QC_PROCESSOR_GPU, QC_IMAGE_FORMAT_RGB888, QC_IMAGE_FORMAT_RGB888, 512, 512, 256,
                 256, true, false, false, false );
    SuccessTest( 2, QC_PROCESSOR_GPU, QC_IMAGE_FORMAT_UYVY, QC_IMAGE_FORMAT_RGB888, 512, 512, 256,
                 256, true, false, false, false );
    SuccessTest( 2, QC_PROCESSOR_GPU, QC_IMAGE_FORMAT_UYVY, QC_IMAGE_FORMAT_RGB888, 512, 512, 256,
                 256, true, true, false, false );
}

TEST( Remap, GPUSuccessPipeline3Test )   // general success test on GPU for NV12/UYVY to BGR, with
                                         // and without undistortion
{
    SuccessTest( 2, QC_PROCESSOR_GPU, QC_IMAGE_FORMAT_NV12, QC_IMAGE_FORMAT_BGR888, 512, 512, 256,
                 256, false, false, false, false );
    SuccessTest( 2, QC_PROCESSOR_GPU, QC_IMAGE_FORMAT_NV12, QC_IMAGE_FORMAT_BGR888, 512, 512, 256,
                 256, true, false, false, false );
}

TEST( Remap, GPUSuccessPipeline4Test )   // general success test on GPU for NV12 to RGB, with
                                         // and without normalization, undistortion
{
    SuccessTest( 2, QC_PROCESSOR_GPU, QC_IMAGE_FORMAT_NV12, QC_IMAGE_FORMAT_RGB888, 512, 512, 256,
                 256, false, false, false, false );
    SuccessTest( 2, QC_PROCESSOR_GPU, QC_IMAGE_FORMAT_NV12, QC_IMAGE_FORMAT_RGB888, 512, 512, 256,
                 256, true, false, false, false );
    SuccessTest( 2, QC_PROCESSOR_GPU, QC_IMAGE_FORMAT_NV12, QC_IMAGE_FORMAT_RGB888, 512, 512, 256,
                 256, false, true, false, false );
    SuccessTest( 2, QC_PROCESSOR_GPU, QC_IMAGE_FORMAT_NV12, QC_IMAGE_FORMAT_RGB888, 512, 512, 256,
                 256, true, true, false, false );
}

TEST( Remap, GPUSuccessPipeline5Test )   // general success test on GPU for NV12 UBWC to BGR
{
    SuccessTest( 2, QC_PROCESSOR_GPU, QC_IMAGE_FORMAT_NV12_UBWC, QC_IMAGE_FORMAT_BGR888, 1920, 1536,
                 1024, 768, false, false, false, false );
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