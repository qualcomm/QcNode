// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "gtest/gtest.h"
#include <stdio.h>

#include "GL2DFlex.hpp"
#include "QC/sample/BufferManager.hpp"

using namespace QC;
using namespace QC::sample;
using namespace QC::Memory;

void GL2DFlexTestNormal( GL2DFlex_Config_t *pConfig, QCImageFormat_e outputFormat,
                         uint32_t outputWidth, uint32_t outputHeight )
{
    QCStatus_e ret = QC_STATUS_OK;
    BufferManager bufMgr( { "GL2DFlex", QC_NODE_TYPE_CUSTOM_0, 0 } );
    Logger logger;
    logger.Init( "GL2DFLEX", LOGGER_LEVEL_ERROR );
    GL2DFlex GL2DFlexObj( logger );
    char pName[12] = "GL2DFlex";
    uint32_t numInputs = pConfig->numOfInputs;
    ImageDescriptor_t inputs[numInputs];
    ImageDescriptor_t output;

    for ( size_t i = 0; i < numInputs; i++ )
    {
        ret = bufMgr.Allocate( ImageBasicProps_t( pConfig->inputConfigs[i].inputResolution.width,
                                                  pConfig->inputConfigs[i].inputResolution.height,
                                                  pConfig->inputConfigs[i].inputFormat ),
                               inputs[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
    pConfig->outputFormat = outputFormat;
    pConfig->outputResolution.width = outputWidth;
    pConfig->outputResolution.height = outputHeight;

    ret = bufMgr.Allocate( ImageBasicProps_t( outputWidth, outputHeight, pConfig->outputFormat ),
                           output );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = GL2DFlexObj.Init( pName, pConfig );
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( size_t i = 0; i < numInputs; i++ )
    {
        ret = GL2DFlexObj.RegisterInputBuffers( &inputs[i], 1 );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    ret = GL2DFlexObj.RegisterOutputBuffers( &output, 1 );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = GL2DFlexObj.Start();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = GL2DFlexObj.Execute( inputs, numInputs, &output );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = GL2DFlexObj.Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( size_t i = 0; i < numInputs; i++ )
    {
        ret = GL2DFlexObj.DeregisterInputBuffers( &inputs[i], 1 );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    ret = GL2DFlexObj.DeregisterOutputBuffers( &output, 1 );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = GL2DFlexObj.Deinit();
    ASSERT_EQ( QC_STATUS_OK, ret );

    logger.Deinit();
}

TEST( GL2DFlex, SANITY_ConvertNV12toRGB )
{
    GL2DFlex_Config_t GL2DFlexConfig;
    GL2DFlex_Config_t *pConfig = &GL2DFlexConfig;

    GL2DFlexConfig.numOfInputs = 1;
    for ( size_t i = 0; i < GL2DFlexConfig.numOfInputs; i++ )
    {
        GL2DFlexConfig.inputConfigs[i].inputFormat = QC_IMAGE_FORMAT_NV12;
        GL2DFlexConfig.inputConfigs[i].inputResolution.width = 1920;
        GL2DFlexConfig.inputConfigs[i].inputResolution.height = 1080;
        GL2DFlexConfig.inputConfigs[i].ROI.topX = 100;
        GL2DFlexConfig.inputConfigs[i].ROI.topY = 100;
        GL2DFlexConfig.inputConfigs[i].ROI.width = 1080;
        GL2DFlexConfig.inputConfigs[i].ROI.height = 720;
    }

    QCImageFormat_e outputFormat = QC_IMAGE_FORMAT_RGB888;
    uint32_t outputWidth = 600;
    uint32_t outputHeight = 600;

    GL2DFlexTestNormal( pConfig, outputFormat, outputWidth, outputHeight );
}

TEST( GL2DFlex, SANITY_ConvertRGBtoNV12 )
{
    GL2DFlex_Config_t GL2DFlexConfig;
    GL2DFlex_Config_t *pConfig = &GL2DFlexConfig;

    GL2DFlexConfig.numOfInputs = 1;
    for ( size_t i = 0; i < GL2DFlexConfig.numOfInputs; i++ )
    {
        GL2DFlexConfig.inputConfigs[i].inputFormat = QC_IMAGE_FORMAT_RGB888;
        GL2DFlexConfig.inputConfigs[i].inputResolution.width = 1920;
        GL2DFlexConfig.inputConfigs[i].inputResolution.height = 1080;
        GL2DFlexConfig.inputConfigs[i].ROI.topX = 100;
        GL2DFlexConfig.inputConfigs[i].ROI.topY = 100;
        GL2DFlexConfig.inputConfigs[i].ROI.width = 1080;
        GL2DFlexConfig.inputConfigs[i].ROI.height = 720;
    }

    QCImageFormat_e outputFormat = QC_IMAGE_FORMAT_NV12;
    uint32_t outputWidth = 600;
    uint32_t outputHeight = 600;

    GL2DFlexTestNormal( pConfig, outputFormat, outputWidth, outputHeight );
}

TEST( GL2DFlex, SANITY_ConvertUYVYtoNV12 )
{
    GL2DFlex_Config_t GL2DFlexConfig;
    GL2DFlex_Config_t *pConfig = &GL2DFlexConfig;

    GL2DFlexConfig.numOfInputs = 1;
    for ( size_t i = 0; i < GL2DFlexConfig.numOfInputs; i++ )
    {
        GL2DFlexConfig.inputConfigs[i].inputFormat = QC_IMAGE_FORMAT_UYVY;
        GL2DFlexConfig.inputConfigs[i].inputResolution.width = 1920;
        GL2DFlexConfig.inputConfigs[i].inputResolution.height = 1080;
        GL2DFlexConfig.inputConfigs[i].ROI.topX = 100;
        GL2DFlexConfig.inputConfigs[i].ROI.topY = 100;
        GL2DFlexConfig.inputConfigs[i].ROI.width = 1080;
        GL2DFlexConfig.inputConfigs[i].ROI.height = 720;
    }

    QCImageFormat_e outputFormat = QC_IMAGE_FORMAT_NV12;
    uint32_t outputWidth = 1080;
    uint32_t outputHeight = 720;

    GL2DFlexTestNormal( pConfig, outputFormat, outputWidth, outputHeight );
}

TEST( GL2DFlex, SANITY_ConvertUYVYtoRGB )
{
    GL2DFlex_Config_t GL2DFlexConfig;
    GL2DFlex_Config_t *pConfig = &GL2DFlexConfig;

    GL2DFlexConfig.numOfInputs = 1;
    for ( size_t i = 0; i < GL2DFlexConfig.numOfInputs; i++ )
    {
        GL2DFlexConfig.inputConfigs[i].inputFormat = QC_IMAGE_FORMAT_UYVY;
        GL2DFlexConfig.inputConfigs[i].inputResolution.width = 1920;
        GL2DFlexConfig.inputConfigs[i].inputResolution.height = 1080;
        GL2DFlexConfig.inputConfigs[i].ROI.topX = 200;
        GL2DFlexConfig.inputConfigs[i].ROI.topY = 200;
        GL2DFlexConfig.inputConfigs[i].ROI.width = 1080;
        GL2DFlexConfig.inputConfigs[i].ROI.height = 720;
    }

    QCImageFormat_e outputFormat = QC_IMAGE_FORMAT_RGB888;
    uint32_t outputWidth = 1080;
    uint32_t outputHeight = 720;

    GL2DFlexTestNormal( pConfig, outputFormat, outputWidth, outputHeight );
}

TEST( GL2DFlex, SANITY_ROI_BOUND )
{
    GL2DFlex_Config_t GL2DFlexConfig;
    GL2DFlex_Config_t *pConfig = &GL2DFlexConfig;

    GL2DFlexConfig.numOfInputs = 1;
    for ( size_t i = 0; i < GL2DFlexConfig.numOfInputs; i++ )
    {
        GL2DFlexConfig.inputConfigs[i].inputFormat = QC_IMAGE_FORMAT_UYVY;
        GL2DFlexConfig.inputConfigs[i].inputResolution.width = 1920;
        GL2DFlexConfig.inputConfigs[i].inputResolution.height = 1080;
        GL2DFlexConfig.inputConfigs[i].ROI.topX = 0;
        GL2DFlexConfig.inputConfigs[i].ROI.topY = 0;
        GL2DFlexConfig.inputConfigs[i].ROI.width = 0;
        GL2DFlexConfig.inputConfigs[i].ROI.height = 0;
    }

    QCImageFormat_e outputFormat = QC_IMAGE_FORMAT_NV12;
    uint32_t outputWidth = 1080;
    uint32_t outputHeight = 720;

    GL2DFlexTestNormal( pConfig, outputFormat, outputWidth, outputHeight );
}

TEST( GL2DFlex, SANITY_RegDeregMultiBuffer )
{
    QCStatus_e ret = QC_STATUS_OK;
    BufferManager bufMgr( { "GL2DFlex", QC_NODE_TYPE_CUSTOM_0, 0 } );
    Logger logger;
    logger.Init( "GL2DFLEX", LOGGER_LEVEL_ERROR );
    GL2DFlex GL2DFlexObj( logger );
    GL2DFlex_Config_t GL2DFlexConfig;
    GL2DFlex_Config_t *pConfig = &GL2DFlexConfig;
    char pName[12] = "GL2DFlex";

    pConfig->numOfInputs = 2;
    QCImageFormat_e outputFormat = QC_IMAGE_FORMAT_NV12;
    uint32_t outputWidth = 1080;
    uint32_t outputHeight = 720;
    uint32_t bufferNum = 4;

    ImageDescriptor_t inputBuffers[pConfig->numOfInputs][bufferNum];
    ImageDescriptor_t outputBuffers[bufferNum];

    for ( size_t i = 0; i < pConfig->numOfInputs; i++ )
    {
        pConfig->inputConfigs[i].inputFormat = QC_IMAGE_FORMAT_UYVY;
        pConfig->inputConfigs[i].inputResolution.width = 1920;
        pConfig->inputConfigs[i].inputResolution.height = 1080;
        pConfig->inputConfigs[i].ROI.topX = 100;
        pConfig->inputConfigs[i].ROI.topY = 100;
        pConfig->inputConfigs[i].ROI.width = 1080;
        pConfig->inputConfigs[i].ROI.height = 720;
        for ( size_t k = 0; k < bufferNum; k++ )
        {
            ret = bufMgr.Allocate(
                    ImageBasicProps_t( pConfig->inputConfigs[i].inputResolution.width,
                                       pConfig->inputConfigs[i].inputResolution.height,
                                       pConfig->inputConfigs[i].inputFormat ),
                    inputBuffers[i][k] );
            ASSERT_EQ( QC_STATUS_OK, ret );
        }
    }

    pConfig->outputFormat = outputFormat;
    pConfig->outputResolution.width = outputWidth;
    pConfig->outputResolution.height = outputHeight;
    for ( size_t k = 0; k < bufferNum; k++ )
    {
        ret = bufMgr.Allocate(
                ImageBasicProps_t( pConfig->numOfInputs, outputWidth, outputHeight, outputFormat ),
                outputBuffers[k] );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    ImageDescriptor_t *inputs[pConfig->numOfInputs];
    ImageDescriptor_t *output;

    ret = GL2DFlexObj.Init( pName, pConfig );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = GL2DFlexObj.Start();
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( size_t i = 0; i < pConfig->numOfInputs; i++ )
    {
        for ( size_t k = 0; k < bufferNum; k++ )
        {
            ret = GL2DFlexObj.RegisterInputBuffers( &inputBuffers[i][k], 1 );
            ASSERT_EQ( QC_STATUS_OK, ret );
        }
    }

    ret = GL2DFlexObj.RegisterOutputBuffers( outputBuffers, bufferNum );
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( size_t k = 0; k < bufferNum; k++ )
    {
        for ( size_t i = 0; i < pConfig->numOfInputs; i++ )
        {
            inputs[i] = &inputBuffers[i][k];
        }
        output = &outputBuffers[k];

        ret = GL2DFlexObj.Execute( *inputs, pConfig->numOfInputs, output );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    for ( size_t i = 0; i < pConfig->numOfInputs; i++ )
    {
        for ( size_t k = 0; k < bufferNum; k++ )
        {
            ret = GL2DFlexObj.DeregisterInputBuffers( &inputBuffers[i][k], 1 );
            ASSERT_EQ( QC_STATUS_OK, ret );
        }
    }

    ret = GL2DFlexObj.Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = GL2DFlexObj.Deinit();
    ASSERT_EQ( QC_STATUS_OK, ret );

    logger.Deinit();
}

#ifndef GTEST_QCNODE
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif
