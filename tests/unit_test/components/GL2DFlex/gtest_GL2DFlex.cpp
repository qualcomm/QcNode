// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "gtest/gtest.h"
#include <stdio.h>

#include "ridehal/component/GL2DFlex.hpp"

using namespace ridehal::common;
using namespace ridehal::component;

void GL2DFlexTestNormal( GL2DFlex_Config_t *pConfig, RideHal_ImageFormat_e outputFormat,
                         uint32_t outputWidth, uint32_t outputHeight )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    GL2DFlex GL2DFlexObj;
    char pName[12] = "GL2DFlex";
    uint32_t numInputs = pConfig->numOfInputs;
    RideHal_SharedBuffer_t inputs[numInputs];
    RideHal_SharedBuffer_t output;

    for ( size_t i = 0; i < numInputs; i++ )
    {
        ret = inputs[i].Allocate( pConfig->inputConfigs[i].inputResolution.width,
                                  pConfig->inputConfigs[i].inputResolution.height,
                                  pConfig->inputConfigs[i].inputFormat );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }
    pConfig->outputFormat = outputFormat;
    pConfig->outputResolution.width = outputWidth;
    pConfig->outputResolution.height = outputHeight;

    ret = output.Allocate( outputWidth, outputHeight, pConfig->outputFormat );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = GL2DFlexObj.Init( pName, pConfig );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    for ( size_t i = 0; i < numInputs; i++ )
    {
        ret = GL2DFlexObj.RegisterInputBuffers( &inputs[i], 1 );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = GL2DFlexObj.RegisterOutputBuffers( &output, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = GL2DFlexObj.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = GL2DFlexObj.Execute( inputs, numInputs, &output );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = GL2DFlexObj.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    for ( size_t i = 0; i < numInputs; i++ )
    {
        ret = GL2DFlexObj.DeregisterInputBuffers( &inputs[i], 1 );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = GL2DFlexObj.DeregisterOutputBuffers( &output, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = GL2DFlexObj.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
}

TEST( GL2DFlex, SANITY_ConvertNV12toRGB )
{
    GL2DFlex_Config_t GL2DFlexConfig;
    GL2DFlex_Config_t *pConfig = &GL2DFlexConfig;

    GL2DFlexConfig.numOfInputs = 1;
    for ( size_t i = 0; i < GL2DFlexConfig.numOfInputs; i++ )
    {
        GL2DFlexConfig.inputConfigs[i].inputFormat = RIDEHAL_IMAGE_FORMAT_NV12;
        GL2DFlexConfig.inputConfigs[i].inputResolution.width = 1920;
        GL2DFlexConfig.inputConfigs[i].inputResolution.height = 1080;
        GL2DFlexConfig.inputConfigs[i].ROI.topX = 100;
        GL2DFlexConfig.inputConfigs[i].ROI.topY = 100;
        GL2DFlexConfig.inputConfigs[i].ROI.width = 1080;
        GL2DFlexConfig.inputConfigs[i].ROI.height = 720;
    }

    RideHal_ImageFormat_e outputFormat = RIDEHAL_IMAGE_FORMAT_RGB888;
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
        GL2DFlexConfig.inputConfigs[i].inputFormat = RIDEHAL_IMAGE_FORMAT_RGB888;
        GL2DFlexConfig.inputConfigs[i].inputResolution.width = 1920;
        GL2DFlexConfig.inputConfigs[i].inputResolution.height = 1080;
        GL2DFlexConfig.inputConfigs[i].ROI.topX = 100;
        GL2DFlexConfig.inputConfigs[i].ROI.topY = 100;
        GL2DFlexConfig.inputConfigs[i].ROI.width = 1080;
        GL2DFlexConfig.inputConfigs[i].ROI.height = 720;
    }

    RideHal_ImageFormat_e outputFormat = RIDEHAL_IMAGE_FORMAT_NV12;
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
        GL2DFlexConfig.inputConfigs[i].inputFormat = RIDEHAL_IMAGE_FORMAT_UYVY;
        GL2DFlexConfig.inputConfigs[i].inputResolution.width = 1920;
        GL2DFlexConfig.inputConfigs[i].inputResolution.height = 1080;
        GL2DFlexConfig.inputConfigs[i].ROI.topX = 100;
        GL2DFlexConfig.inputConfigs[i].ROI.topY = 100;
        GL2DFlexConfig.inputConfigs[i].ROI.width = 1080;
        GL2DFlexConfig.inputConfigs[i].ROI.height = 720;
    }

    RideHal_ImageFormat_e outputFormat = RIDEHAL_IMAGE_FORMAT_NV12;
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
        GL2DFlexConfig.inputConfigs[i].inputFormat = RIDEHAL_IMAGE_FORMAT_UYVY;
        GL2DFlexConfig.inputConfigs[i].inputResolution.width = 1920;
        GL2DFlexConfig.inputConfigs[i].inputResolution.height = 1080;
        GL2DFlexConfig.inputConfigs[i].ROI.topX = 200;
        GL2DFlexConfig.inputConfigs[i].ROI.topY = 200;
        GL2DFlexConfig.inputConfigs[i].ROI.width = 1080;
        GL2DFlexConfig.inputConfigs[i].ROI.height = 720;
    }

    RideHal_ImageFormat_e outputFormat = RIDEHAL_IMAGE_FORMAT_RGB888;
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
        GL2DFlexConfig.inputConfigs[i].inputFormat = RIDEHAL_IMAGE_FORMAT_UYVY;
        GL2DFlexConfig.inputConfigs[i].inputResolution.width = 1920;
        GL2DFlexConfig.inputConfigs[i].inputResolution.height = 1080;
        GL2DFlexConfig.inputConfigs[i].ROI.topX = 0;
        GL2DFlexConfig.inputConfigs[i].ROI.topY = 0;
        GL2DFlexConfig.inputConfigs[i].ROI.width = 0;
        GL2DFlexConfig.inputConfigs[i].ROI.height = 0;
    }

    RideHal_ImageFormat_e outputFormat = RIDEHAL_IMAGE_FORMAT_NV12;
    uint32_t outputWidth = 1080;
    uint32_t outputHeight = 720;

    GL2DFlexTestNormal( pConfig, outputFormat, outputWidth, outputHeight );
}

TEST( GL2DFlex, SANITY_RegDeregMultiBuffer )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    GL2DFlex GL2DFlexObj;
    GL2DFlex_Config_t GL2DFlexConfig;
    GL2DFlex_Config_t *pConfig = &GL2DFlexConfig;
    char pName[12] = "GL2DFlex";

    pConfig->numOfInputs = 2;
    RideHal_ImageFormat_e outputFormat = RIDEHAL_IMAGE_FORMAT_NV12;
    uint32_t outputWidth = 1080;
    uint32_t outputHeight = 720;
    uint32_t bufferNum = 4;

    RideHal_SharedBuffer_t inputBuffers[pConfig->numOfInputs][bufferNum];
    RideHal_SharedBuffer_t outputBuffers[bufferNum];

    for ( size_t i = 0; i < pConfig->numOfInputs; i++ )
    {
        pConfig->inputConfigs[i].inputFormat = RIDEHAL_IMAGE_FORMAT_UYVY;
        pConfig->inputConfigs[i].inputResolution.width = 1920;
        pConfig->inputConfigs[i].inputResolution.height = 1080;
        pConfig->inputConfigs[i].ROI.topX = 100;
        pConfig->inputConfigs[i].ROI.topY = 100;
        pConfig->inputConfigs[i].ROI.width = 1080;
        pConfig->inputConfigs[i].ROI.height = 720;
        for ( size_t k = 0; k < bufferNum; k++ )
        {
            ret = inputBuffers[i][k].Allocate( pConfig->inputConfigs[i].inputResolution.width,
                                               pConfig->inputConfigs[i].inputResolution.height,
                                               pConfig->inputConfigs[i].inputFormat );
            ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        }
    }

    pConfig->outputFormat = outputFormat;
    pConfig->outputResolution.width = outputWidth;
    pConfig->outputResolution.height = outputHeight;
    for ( size_t k = 0; k < bufferNum; k++ )
    {
        ret = outputBuffers[k].Allocate( pConfig->numOfInputs, outputWidth, outputHeight,
                                         outputFormat );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    RideHal_SharedBuffer_t *inputs[pConfig->numOfInputs];
    RideHal_SharedBuffer_t *output;

    ret = GL2DFlexObj.Init( pName, pConfig );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = GL2DFlexObj.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    for ( size_t i = 0; i < pConfig->numOfInputs; i++ )
    {
        for ( size_t k = 0; k < bufferNum; k++ )
        {
            ret = GL2DFlexObj.RegisterInputBuffers( &inputBuffers[i][k], 1 );
            ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        }
    }

    ret = GL2DFlexObj.RegisterOutputBuffers( outputBuffers, bufferNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    for ( size_t k = 0; k < bufferNum; k++ )
    {
        for ( size_t i = 0; i < pConfig->numOfInputs; i++ )
        {
            inputs[i] = &inputBuffers[i][k];
        }
        output = &outputBuffers[k];

        ret = GL2DFlexObj.Execute( *inputs, pConfig->numOfInputs, output );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    for ( size_t i = 0; i < pConfig->numOfInputs; i++ )
    {
        for ( size_t k = 0; k < bufferNum; k++ )
        {
            ret = GL2DFlexObj.DeregisterInputBuffers( &inputBuffers[i][k], 1 );
            ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        }
    }

    ret = GL2DFlexObj.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = GL2DFlexObj.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
}


#ifndef GTEST_RIDEHAL
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif

