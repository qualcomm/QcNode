// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


#include "QC/sample/SampleDepthFromStereoViz.hpp"
#include <algorithm>
#include <cmath>
#include <math.h>

namespace QC
{
namespace sample
{

#define KernelCode( ... ) #__VA_ARGS__

static const char *s_pSourceDispColor = KernelCode(   //
        __constant float colors[8][3] = { { 0, 0, 0 },
                                          { 0, 0, 1 },
                                          { 1, 0, 0 },
                                          { 1, 0, 1 },
                                          { 0, 1, 0 },
                                          { 0, 1, 1 },
                                          { 1, 1, 0 },
                                          { 1, 1, 1 } };
        __constant float relativeWeights[8] = { 8.77193, 5.40541, 8.77193, 5.74713, 8.77193,
                                                5.40541, 8.77193, 0.00000 };
        __constant float cumulativeWeights[8] = { 0.00000, 0.11400, 0.29900, 0.41300, 0.58700,
                                                  0.70100, 0.88600, 1.00000 };
        //-----------------------------------------------------------------------
        /// @brief Converts disparity to rgb pixel
        /// @param pDisparity the disparity buffer
        /// @param pConf confidence value array for each pixel
        /// @param rgbImage RGB888 in uchar format
        /// @param dispStride the disparity stride along width
        /// @param confStride the confidence stride along width
        /// @param rgbStride the confidence stride along width
        /// @param confThreshold minimum confidence value
        /// @param disparityMax maximum of the disparity
        /// @param nTransparency value for the alpha channel
        //-----------------------------------------------------------------------
        __kernel void DispColorConvert( __global const ushort *pDisparity,   //
                                        __global const uchar *pConf,         //
                                        __global uchar *pRgb,                //
                                        uint dispStride,                     //
                                        uint confStride,                     //
                                        uint rgbStride,                      //
                                        uchar confThreshold,                 //
                                        ushort disparityMax,                 //
                                        uchar nTransparency ) {
            uint2 pos = (uint2) ( get_global_id( 0 ), get_global_id( 1 ) );
            ushort disp = pDisparity[pos.y * dispStride + pos.x];
            uchar confValue = pConf[pos.y * confStride + pos.x];
            if ( confValue >= confThreshold )
            {
                // do normalization
                float val = fmin( fmax( (float) disp / disparityMax, 0.0f ), 1.0f );
                uint i;
                // search to find index to the colors
                for ( i = 0; i < 7; i++ )
                {
                    if ( val < cumulativeWeights[i + 1] )
                    {
                        break;
                    }
                }

                // calculte the relative RGB values according to the depth disparity
                float weight = 1.0 - ( val - cumulativeWeights[i] ) * relativeWeights[i];
                uchar r =
                        (uchar) ( ( weight * colors[i][0] + ( 1.0 - weight ) * colors[i + 1][0] ) *
                                  255.0 );
                uchar g =
                        (uchar) ( ( weight * colors[i][1] + ( 1.0 - weight ) * colors[i + 1][1] ) *
                                  255.0 );
                uchar b =
                        (uchar) ( ( weight * colors[i][2] + ( 1.0 - weight ) * colors[i + 1][2] ) *
                                  255.0 );
                vstore3( (uchar3) ( r, g, b ), 0, pRgb + rgbStride * pos.y + pos.x * 3 );
            }
            else
            {
                vstore3( (uchar3) ( nTransparency ), 0, pRgb + rgbStride * pos.y + pos.x * 3 );
            }
        } );

SampleDepthFromStereoViz::SampleDepthFromStereoViz() {}
SampleDepthFromStereoViz::~SampleDepthFromStereoViz() {}

QCStatus_e SampleDepthFromStereoViz::ParseConfig( SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    m_processor = Get( config, "processor", QC_PROCESSOR_GPU );
    if ( ( QC_PROCESSOR_CPU != m_processor ) && ( QC_PROCESSOR_GPU != m_processor ) )
    {
        QC_ERROR( "invalid processor type" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    m_width = Get( config, "width", 1280u );
    m_height = Get( config, "height", 416u );

    m_disparityMax = Get( config, "disparity_max", 1008u );
    m_nConfMapThreshold = Get( config, "conf_threshold", 0u );

    m_poolSize = Get( config, "pool_size", 4 );
    if ( 0 == m_poolSize )
    {
        QC_ERROR( "invalid pool_size = %d\n", m_poolSize );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    m_inputTopicName = Get( config, "input_topic", "" );
    if ( "" == m_inputTopicName )
    {
        QC_ERROR( "no input topic\n" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    m_outputTopicName = Get( config, "output_topic", "" );
    if ( "" == m_outputTopicName )
    {
        QC_ERROR( "no output topic\n" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    return ret;
}

QCStatus_e SampleDepthFromStereoViz::Init( std::string name, SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    ret = SampleIF::Init( name );
    if ( QC_STATUS_OK == ret )
    {
        ret = ParseConfig( config );
    }

    if ( QC_STATUS_OK == ret )
    {
        QCImageProps_t imgProp;
        imgProp.format = QC_IMAGE_FORMAT_RGB888;
        imgProp.batchSize = 1;
        imgProp.width = m_width;
        imgProp.height = m_height;
        imgProp.stride[0] = m_width * 3;
        imgProp.actualHeight[0] = m_height;
        imgProp.numPlanes = 1;
        imgProp.planeBufSize[0] = 0;
        ret = m_rgbPool.Init( name + ".rgb", m_nodeId, LOGGER_LEVEL_INFO, m_poolSize, imgProp );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_sub.Init( name, m_inputTopicName );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_pub.Init( name, m_outputTopicName );
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( QC_PROCESSOR_GPU == m_processor )
        {
            ret = m_openclSrvObj.Init( name.c_str(), LOGGER_LEVEL_ERROR );

            if ( QC_STATUS_OK == ret )
            {
                ret = m_openclSrvObj.LoadFromSource( s_pSourceDispColor );
                if ( QC_STATUS_OK != ret )
                {
                    QC_ERROR( "Failed to load kernel source code" );
                }
            }
            if ( QC_STATUS_OK == ret )
            {
                ret = m_openclSrvObj.CreateKernel( &m_kernel, "DispColorConvert" );
                if ( QC_STATUS_OK != ret )
                {
                    QC_ERROR( "Failed to create kernel" );
                }
            }
        }
    }

    return ret;
}

QCStatus_e SampleDepthFromStereoViz::Start()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( QC_STATUS_OK == ret )
    {
        m_stop = false;
        m_thread = std::thread( &SampleDepthFromStereoViz::ThreadMain, this );
    }

    return ret;
}

QCStatus_e SampleDepthFromStereoViz::ConvertToRgbCPU( QCBufferDescriptorBase_t &disparity,
                                                      QCBufferDescriptorBase_t &conf,
                                                      QCBufferDescriptorBase_t &RGB )
{
    QCStatus_e ret = QC_STATUS_OK;
    uint32_t strideW = 0;
    uint32_t strideW_Disp = 0;
    uint16_t *pOutDisp = static_cast<uint16_t *>( disparity.GetDataPtr() );
    uint8_t *pOutCONF = static_cast<uint8_t *>( conf.GetDataPtr() );
    uint8_t *pPixs = static_cast<uint8_t *>( RGB.GetDataPtr() );
    TensorDescriptor_t *pDisparity = dynamic_cast<TensorDescriptor_t *>( &disparity );
    TensorDescriptor_t *pConf = dynamic_cast<TensorDescriptor_t *>( &conf );

    if ( ( nullptr == pDisparity ) || ( nullptr == pConf ) )
    {
        QC_ERROR( "disparity or conf is not a valid tensor" );
        ret = QC_STATUS_INVALID_BUF;
    }
    else
    {
        strideW = pConf->dims[2];
        strideW_Disp = pDisparity->dims[2] * sizeof( uint16_t );
    }

    for ( uint32_t h = 0; ( h < m_height ) && ( QC_STATUS_OK == ret ); h++ )
    {
        uint16_t *pDisp = reinterpret_cast<uint16_t *>( reinterpret_cast<uint8_t *>( pOutDisp ) +
                                                        h * strideW_Disp );
        uint8_t *pCONF = pOutCONF + h * strideW;
        uint8_t *pColorPix = pPixs + h * m_width * 3;
        for ( uint32_t w = 0; w < m_width; w++ )
        {
            if ( *pCONF > m_nConfMapThreshold )
            {
                uint16_t disp = *pDisp;
                /* collect live max disparity */
                if ( disp > m_disparityMax )
                {
                    m_disparityMax = disp;
                }

                // do normalization
                float val = std::min( std::max( (float) disp / m_disparityMax, 0.0f ), 1.0f );

                // search to find index to the colors
                int32_t i;
                for ( i = 0; i < 7; i++ )
                {
                    if ( val < m_cumulativeWeights[i + 1] )
                    {
                        break;
                    }
                }

                // calculte the relative RGB values according to the depth disparity
                float weight = 1.0 - ( val - m_cumulativeWeights[i] ) * m_relativeWeights[i];
                pColorPix[0] = static_cast<uint8_t>(
                        ( weight * m_colors[i][0] + ( 1.0 - weight ) * m_colors[i + 1][0] ) *
                        255.0 );
                pColorPix[1] = static_cast<uint8_t>(
                        ( weight * m_colors[i][1] + ( 1.0 - weight ) * m_colors[i + 1][1] ) *
                        255.0 );
                pColorPix[2] = static_cast<uint8_t>(
                        ( weight * m_colors[i][2] + ( 1.0 - weight ) * m_colors[i + 1][2] ) *
                        255.0 );
            }
            else
            {
                pColorPix[0] = m_nTransparency;
                pColorPix[1] = m_nTransparency;
                pColorPix[2] = m_nTransparency;
            }
            pColorPix += 3;
            pCONF += 1;
            pDisp += 1;
        }
    }

    return ret;
}

QCStatus_e SampleDepthFromStereoViz::ConvertToRgbGPU( QCBufferDescriptorBase_t &disparity,
                                                      QCBufferDescriptorBase_t &conf,
                                                      QCBufferDescriptorBase_t &RGB )
{
    QCStatus_e ret = QC_STATUS_OK;
    OpenclIfcae_Arg_t openclArgs[9];
    OpenclIface_WorkParams_t openclWorkParams;
    openclWorkParams.workDim = 2;
    size_t globalWorkSize[2] = { m_width, m_height };
    size_t globalWorkOffset[2] = { 0, 0 };
    openclWorkParams.pGlobalWorkSize = globalWorkSize;
    openclWorkParams.pGlobalWorkOffset = globalWorkOffset;
    openclWorkParams.pLocalWorkSize = NULL;
    cl_mem clMemDisp;
    cl_mem clMemConf;
    cl_mem clMemRGB;
    TensorDescriptor_t *pDisparity = dynamic_cast<TensorDescriptor_t *>( &disparity );
    TensorDescriptor_t *pConf = dynamic_cast<TensorDescriptor_t *>( &conf );
    cl_uint strideConf = 0;
    cl_uint strideDisp = 0;
    cl_uint strideRGB = m_width * 3;

    if ( ( nullptr == pDisparity ) || ( nullptr == pConf ) )
    {
        QC_ERROR( "disparity or conf is not a valid tensor" );
        ret = QC_STATUS_INVALID_BUF;
    }
    else
    {
        strideConf = pConf->dims[2];
        strideDisp = pDisparity->dims[2];

        ret = m_openclSrvObj.RegBufferDesc( disparity, clMemDisp );
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "Failed to create cl mv mem" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_openclSrvObj.RegBufferDesc( conf, clMemConf );
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "Failed to create cl mvConf mem" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_openclSrvObj.RegBufferDesc( RGB, clMemRGB );
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "Failed to create cl rgb mem" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {

        openclArgs[0].pArg = static_cast<void *>( &clMemDisp );
        openclArgs[0].argSize = sizeof( cl_mem );

        openclArgs[1].pArg = static_cast<void *>( &clMemConf );
        openclArgs[1].argSize = sizeof( cl_mem );

        openclArgs[2].pArg = static_cast<void *>( &clMemRGB );
        openclArgs[2].argSize = sizeof( cl_mem );

        openclArgs[3].pArg = static_cast<void *>( &strideDisp );
        openclArgs[3].argSize = sizeof( cl_uint );

        openclArgs[4].pArg = static_cast<void *>( &strideConf );
        openclArgs[4].argSize = sizeof( cl_uint );

        openclArgs[5].pArg = static_cast<void *>( &strideRGB );
        openclArgs[5].argSize = sizeof( cl_uint );

        openclArgs[6].pArg = static_cast<void *>( &m_nConfMapThreshold );
        openclArgs[6].argSize = sizeof( cl_uchar );

        openclArgs[7].pArg = static_cast<void *>( &m_disparityMax );
        openclArgs[7].argSize = sizeof( cl_ushort );

        openclArgs[8].pArg = static_cast<void *>( &m_nTransparency );
        openclArgs[8].argSize = sizeof( cl_uchar );


        ret = m_openclSrvObj.Execute( &m_kernel, openclArgs, 9, &openclWorkParams );
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "Failed to execute kernel" );
        }
    }

    return ret;
}

void SampleDepthFromStereoViz::ThreadMain()
{
    QCStatus_e ret;
    while ( false == m_stop )
    {
        DataFrames_t frames;
        ret = m_sub.Receive( frames );
        if ( QC_STATUS_OK == ret )
        {
            QC_DEBUG( "receive frameId %" PRIu64 ", timestamp %" PRIu64 "\n", frames.FrameId( 0 ),
                      frames.Timestamp( 0 ) );

            std::shared_ptr<SharedBuffer_t> rgb = m_rgbPool.Get();
            if ( nullptr != rgb )
            {
                QCBufferDescriptorBase_t &disp = frames.GetBuffer( 0 );
                QCBufferDescriptorBase_t &conf = frames.GetBuffer( 1 );

                if ( QC_STATUS_OK == ret )
                {
                    PROFILER_BEGIN();
                    TRACE_BEGIN( frames.FrameId( 0 ) );
                    if ( QC_PROCESSOR_CPU == m_processor )
                    {
                        ret = ConvertToRgbCPU( disp, conf, rgb->GetBuffer() );
                    }
                    else
                    {
                        ret = ConvertToRgbGPU( disp, conf, rgb->GetBuffer() );
                    }
                    if ( QC_STATUS_OK == ret )
                    {
                        PROFILER_END();
                        TRACE_END( frames.FrameId( 0 ) );
                        DataFrames_t outFrames;
                        DataFrame_t frame;
                        frame.buffer = rgb;
                        frame.frameId = frames.FrameId( 0 );
                        frame.timestamp = frames.Timestamp( 0 );
                        outFrames.Add( frame );
                        m_pub.Publish( outFrames );
                    }
                    else
                    {
                        QC_ERROR( "DepthFromStereoViz failed for %" PRIu64 " : %d",
                                  frames.FrameId( 0 ), ret );
                    }
                }
            }
        }
    }
}

QCStatus_e SampleDepthFromStereoViz::Stop()
{
    QCStatus_e ret = QC_STATUS_OK;

    m_stop = true;
    if ( m_thread.joinable() )
    {
        m_thread.join();
    }

    PROFILER_SHOW();

    return ret;
}

QCStatus_e SampleDepthFromStereoViz::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;
    return ret;
}

REGISTER_SAMPLE( DepthFromStereoViz, SampleDepthFromStereoViz );

}   // namespace sample
}   // namespace QC
