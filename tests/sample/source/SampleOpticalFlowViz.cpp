// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QC/sample/SampleOpticalFlowViz.hpp"
#include <algorithm>
#include <math.h>

namespace QC
{
namespace sample
{

#define KernelCode( ... ) #__VA_ARGS__

static const char *s_pSourceMvColor = KernelCode(
        __inline__ float ConvertMvFloat( short x ) {
            return (float) ( (float) ( x >> 4 ) + (float) ( x & 0xF ) / 16 );
        }


        //-----------------------------------------------------------------------
        /// @brief Converts motion vectors to rgb pixel
        /// @param mv motion vector
        /// @param conf confidence value array for each pixel
        /// @param rgbImage RGB888 in uchar format
        /// @param colorWheel a color wheel constant
        /// @param imageStride (srcHeightStride, srcWidthStride, confStride, dstWidthStride) in
        /// uint4 format
        /// @param confThreshold minimum confidence value
        /// @param ncols number of colors in the wheel
        /// @param nTransparency value for the alpha channel
        //-----------------------------------------------------------------------
        __kernel void MvColorConvert( __global const short *pMv, __global const uchar *pConf,
                                      __global uchar *pRgb, __constant uchar *pColorWheel,
                                      uint4 imageStride, uchar confThreshold, uint ncols,
                                      uchar nTransparency ) {
            const int MVCOLOR_MAXCOLS = 60;
            uint2 pos = (uint2) ( get_global_id( 0 ), get_global_id( 1 ) );

            short sx = pMv[pos.y * ( imageStride.s1 >> 1 ) + pos.x];
            short sy = pMv[( imageStride.s0 + pos.y ) * ( imageStride.s1 >> 1 ) + pos.x];
            uchar confValue = pConf[pos.y * imageStride.s2 + pos.x];

            if ( ( confThreshold != 0 ) && ( confValue <= confThreshold ) )
            {
                vstore3( (uchar3) ( nTransparency ), 0, pRgb + imageStride.s3 * pos.y + pos.x * 3 );
            }
            else
            {
                float x = ConvertMvFloat( sx ) / 128;
                float y = ConvertMvFloat( sy ) / 64;

                float rad = sqrt( x * x + y * y );
                float a = atan2( -y, -x ) / M_PI_F;
                float fk = ( a + 1.0 ) / 2.0 * ( ncols - 1 );
                int k0 = (int) fk;
                int k1 = ( k0 + 1 ) % ncols;
                float f = fk - k0;

                if ( ( k0 < MVCOLOR_MAXCOLS ) && ( k1 < MVCOLOR_MAXCOLS ) )
                {
                    float3 col0 = convert_float3( vload3( k0, pColorWheel ) ) / 255.0;
                    float3 col1 = convert_float3( vload3( k1, pColorWheel ) ) / 255.0;
                    float3 col = (float3) ( 1 - f ) * col0 + ( (float3) f * col1 );

                    if ( rad <= 1 )
                    {
                        col = (float3) 1 - (float3) ( rad ) * ( (float3) 1 - col );
                    }
                    else
                    {
                        col = col * (float3) ( 0.75 );
                    }

                    uchar3 p = convert_uchar3_sat( col * (float3) 255.0 );
                    vstore3( (uchar3) ( p.s2, p.s1, p.s0 ), 0,
                             pRgb + imageStride.s3 * pos.y + pos.x * 3 );
                }
            }
        } );

SampleOpticalFlowViz::SampleOpticalFlowViz() {}
SampleOpticalFlowViz::~SampleOpticalFlowViz() {}

QCStatus_e SampleOpticalFlowViz::ParseConfig( SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    m_processor = Get( config, "processor", QC_PROCESSOR_GPU );
    if ( ( QC_PROCESSOR_CPU != m_processor ) && ( QC_PROCESSOR_GPU != m_processor ) )
    {
        QC_ERROR( "invalid processor type" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    m_width = Get( config, "width", 1920u / 2 );
    m_height = Get( config, "height", 1024u / 2 );

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

void SampleOpticalFlowViz::MvSetcols( uint32_t r, uint32_t g, uint32_t b, uint32_t k )
{
    m_nColorwheel[k][0] = r;
    m_nColorwheel[k][1] = g;
    m_nColorwheel[k][2] = b;
}

void SampleOpticalFlowViz::MvMakeColorWheel( void )
{
    // relative lengths of color transitions:
    // these are chosen based on perceptual similarity
    // (e.g. one can distinguish more shades between red and yellow
    //  than between yellow and green)
    uint32_t RY = 15;
    uint32_t YG = 6;
    uint32_t GC = 4;
    uint32_t CB = 11;
    uint32_t BM = 13;
    uint32_t MR = 6;
    m_ncols = RY + YG + GC + CB + BM + MR;

    uint32_t i;
    uint32_t k = 0;

    for ( i = 0; i < RY; i++ )
    {
        MvSetcols( 255, 255 * i / RY, 0, k++ );
    }
    for ( i = 0; i < YG; i++ )
    {
        MvSetcols( 255 - 255 * i / YG, 255, 0, k++ );
    }
    for ( i = 0; i < GC; i++ )
    {
        MvSetcols( 0, 255, 255 * i / GC, k++ );
    }
    for ( i = 0; i < CB; i++ )
    {
        MvSetcols( 0, 255 - 255 * i / CB, 255, k++ );
    }
    for ( i = 0; i < BM; i++ )
    {
        MvSetcols( 255 * i / BM, 0, 255, k++ );
    }
    for ( i = 0; i < MR; i++ )
    {
        MvSetcols( 255, 0, 255 - 255 * i / MR, k++ );
    }
}

QCStatus_e SampleOpticalFlowViz::MvComputeColor( float fx, float fy, uint8_t *pix )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( 0 == m_ncols )
    {
        QC_ERROR( "error: m_ncols = 0" );
        ret = QC_STATUS_FAIL;
    }
    else
    {
        float rad = sqrt( fx * fx + fy * fy );
        float a = atan2f( -fy, -fx ) / M_PI;

        float fk = ( a + 1.0 ) / 2.0 * ( m_ncols - 1 );
        int32_t k0 = static_cast<int32_t>( fk );
        int32_t k1 = ( k0 + 1 ) % m_ncols;
        float f = fk - k0;
        // f = 0; // uncomment to see original color wheel

        if ( ( k0 >= MVCOLOR_MAXCOLS ) || ( k1 >= MVCOLOR_MAXCOLS ) )
        {
            QC_ERROR( "error: beyond range: k0 = %d k1 = %d", k0, k1 );
            ret = QC_STATUS_FAIL;
        }
        else
        {
            for ( int b = 0; b < 3; b++ )
            {
                float col0 = m_nColorwheel[k0][b] / 255.0;
                float col1 = m_nColorwheel[k1][b] / 255.0;
                float col = ( 1 - f ) * col0 + ( f * col1 );
                if ( rad <= 1 )
                {
                    col = 1 - rad * ( 1 - col );   // increase saturation with radius
                }
                else
                {
                    col *= .75;   // out of range
                }

                pix[2 - b] = (uint8_t) ( 255.0 * col );
            }
        }
    }

    return ret;
}

QCStatus_e SampleOpticalFlowViz::Init( std::string name, SampleConfig_t &config )
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
        MvMakeColorWheel();
        if ( QC_PROCESSOR_GPU == m_processor )
        {
            ret = m_openclSrvObj.Init( name.c_str(), LOGGER_LEVEL_ERROR );

            if ( QC_STATUS_OK == ret )
            {
                ret = m_openclSrvObj.LoadFromSource( s_pSourceMvColor );
                if ( QC_STATUS_OK != ret )
                {
                    QC_ERROR( "Failed to load kernel source code" );
                }
            }
            if ( QC_STATUS_OK == ret )
            {
                ret = m_openclSrvObj.CreateKernel( &m_kernel, "MvColorConvert" );
                if ( QC_STATUS_OK != ret )
                {
                    QC_ERROR( "Failed to create kernel" );
                }
            }
            if ( QC_STATUS_OK == ret )
            {
                m_pBufMgr = BufferManager::Get( m_nodeId, m_logger.GetLevel() );
                if ( nullptr == m_pBufMgr )
                {
                    QC_ERROR( "Failed to create buffer manager!" );
                    ret = QC_STATUS_NOMEM;
                }
            }
            if ( QC_STATUS_OK == ret )
            {
                ret = m_pBufMgr->Allocate( BufferProps_t( sizeof( m_nColorwheel ) ),
                                           m_colorwheelBuf );
                if ( QC_STATUS_OK != ret )
                {
                    QC_ERROR( "Failed to allocate color wheel buffer" );
                }
                else
                {
                    memcpy( m_colorwheelBuf.GetDataPtr(), m_nColorwheel, m_colorwheelBuf.size );
                    ret = m_openclSrvObj.RegBufferDesc( m_colorwheelBuf, m_clMemColorWheel );
                    if ( QC_STATUS_OK != ret )
                    {
                        QC_ERROR( "Failed to create cl color wheel mem" );
                    }
                }
            }
        }
    }

    return ret;
}

QCStatus_e SampleOpticalFlowViz::Start()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( QC_STATUS_OK == ret )
    {
        m_stop = false;
        m_thread = std::thread( &SampleOpticalFlowViz::ThreadMain, this );
    }

    return ret;
}

QCStatus_e SampleOpticalFlowViz::ConvertToRgbCPU( QCBufferDescriptorBase_t &Mv,
                                                  QCBufferDescriptorBase_t &MvConf,
                                                  QCBufferDescriptorBase_t &RGB )
{
    QCStatus_e ret = QC_STATUS_OK;
    uint32_t strideH = 0;
    uint32_t strideW = 0;
    uint32_t strideW_MV = 0;

    TensorDescriptor_t *pMv = dynamic_cast<TensorDescriptor_t *>( &Mv );
    TensorDescriptor_t *pMvConf = dynamic_cast<TensorDescriptor_t *>( &MvConf );

    if ( ( nullptr == pMv ) || ( nullptr == pMvConf ) )
    {
        QC_ERROR( "motion vector or conf is not a valid tensor" );
        ret = QC_STATUS_INVALID_BUF;
    }
    else
    {
        strideH = pMvConf->dims[1];
        strideW = pMvConf->dims[2];
        strideW_MV = pMv->dims[2];
    }


    int16_t *pOutMVX = static_cast<int16_t *>( Mv.GetDataPtr() );
    int16_t *pOutMVY = reinterpret_cast<int16_t *>( reinterpret_cast<uint8_t *>( pOutMVX ) +
                                                    ( strideW_MV * strideH ) );
    uint8_t *pOutCONF = static_cast<uint8_t *>( MvConf.GetDataPtr() );
    uint8_t *pPixs = static_cast<uint8_t *>( RGB.GetDataPtr() );

    MvColorMvMaxValue_t sLiveMaxMvValue = m_sLiveMaxMvValue;
    for ( uint32_t h = 0; ( h < m_height ) && ( QC_STATUS_OK == ret ); h++ )
    {
        auto pMVX = (int16_t *) ( ( (uint8_t *) pOutMVX ) + h * strideW_MV );
        auto pMVY = (int16_t *) ( ( (uint8_t *) pOutMVY ) + h * strideW_MV );
        auto pCONF = pOutCONF + h * strideW;
        uint8_t *pColorPix = pPixs + h * m_width * 3;
        for ( uint32_t w = 0; w < m_width; w++ )
        {
            if ( *pCONF > m_nConfMapThreshold )
            {
                int16_t flowX = *pMVX;
                int16_t flowY = *pMVY;
                float fx = (float) ( flowX >> 4 ) + (float) ( flowX & 0xF ) / 16;
                float fy = (float) ( flowY >> 4 ) + (float) ( flowY & 0xF ) / 16;
                /* collect live max mvx and mvy */
                if ( abs( fx ) > (float) m_sLiveMaxMvValue.nMvX )
                {
                    m_sLiveMaxMvValue.nMvX = (uint32_t) ( abs( fx ) + 1 );
                }
                if ( abs( fy ) > (float) m_sLiveMaxMvValue.nMvY )
                {
                    m_sLiveMaxMvValue.nMvY = (uint32_t) ( abs( fy ) + 1 );
                }

                uint32_t nMvMag = ( fx ) * ( fx ) + ( fy ) * ( fy );
                if ( nMvMag > m_nMagThreshold )
                {
                    /* normalize */
                    if ( true == m_bSaturateColor )
                    {
                        fx = fx / sLiveMaxMvValue.nMvX;
                        fy = fy / sLiveMaxMvValue.nMvY;
                    }
                    else
                    {
                        fx = fx / m_sMaxMvValue.nMvX;
                        fy = fy / m_sMaxMvValue.nMvY;
                    }
                    (void) MvComputeColor( fx, fy, pColorPix );
                }
                else
                {
                    pColorPix[0] = m_nTransparency;
                    pColorPix[1] = m_nTransparency;
                    pColorPix[2] = m_nTransparency;
                }
            }
            else
            {
                pColorPix[0] = m_nTransparency;
                pColorPix[1] = m_nTransparency;
                pColorPix[2] = m_nTransparency;
            }
            pColorPix += 3;
            pCONF += 1;
            pMVX += 1;
            pMVY += 1;
        }
    }

    return ret;
}

QCStatus_e SampleOpticalFlowViz::ConvertToRgbGPU( QCBufferDescriptorBase_t &Mv,
                                                  QCBufferDescriptorBase_t &MvConf,
                                                  QCBufferDescriptorBase_t &RGB )
{
    QCStatus_e ret = QC_STATUS_OK;
    OpenclIfcae_Arg_t openclArgs[8];
    OpenclIface_WorkParams_t openclWorkParams;
    openclWorkParams.workDim = 2;
    size_t globalWorkSize[2] = { m_width, m_height };
    size_t globalWorkOffset[2] = { 0, 0 };
    openclWorkParams.pGlobalWorkSize = globalWorkSize;
    openclWorkParams.pGlobalWorkOffset = globalWorkOffset;
    openclWorkParams.pLocalWorkSize = NULL;
    cl_uint4 imageStride;
    cl_mem clMemMv;
    cl_mem clMemMvConf;
    cl_mem clMemRGB;
    uint32_t strideH = 0;
    uint32_t strideW = 0;
    uint32_t strideW_MV = 0;

    TensorDescriptor_t *pMv = dynamic_cast<TensorDescriptor_t *>( &Mv );
    TensorDescriptor_t *pMvConf = dynamic_cast<TensorDescriptor_t *>( &MvConf );

    if ( ( nullptr == pMv ) || ( nullptr == pMvConf ) )
    {
        QC_ERROR( "motion vector or conf is not a valid tensor" );
        ret = QC_STATUS_INVALID_BUF;
    }
    else
    {
        strideH = pMvConf->dims[1];
        strideW = pMvConf->dims[2];
        strideW_MV = pMv->dims[2];

        imageStride.s[0] = strideH;
        imageStride.s[1] = strideW_MV;
        imageStride.s[2] = strideW;
        imageStride.s[3] = m_width * 3;

        ret = m_openclSrvObj.RegBufferDesc( Mv, clMemMv );
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "Failed to create cl mv mem" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_openclSrvObj.RegBufferDesc( MvConf, clMemMvConf );
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
        openclArgs[0].pArg = static_cast<void *>( &clMemMv );
        openclArgs[0].argSize = sizeof( cl_mem );

        openclArgs[1].pArg = static_cast<void *>( &clMemMvConf );
        openclArgs[1].argSize = sizeof( cl_mem );

        openclArgs[2].pArg = static_cast<void *>( &clMemRGB );
        openclArgs[2].argSize = sizeof( cl_mem );

        openclArgs[3].pArg = static_cast<void *>( &m_clMemColorWheel );
        openclArgs[3].argSize = sizeof( cl_mem );

        openclArgs[4].pArg = static_cast<void *>( &imageStride );
        openclArgs[4].argSize = sizeof( cl_uint4 );

        openclArgs[5].pArg = static_cast<void *>( &m_nConfMapThreshold );
        openclArgs[5].argSize = sizeof( cl_uchar );

        openclArgs[6].pArg = static_cast<void *>( &m_ncols );
        openclArgs[6].argSize = sizeof( cl_uint );

        openclArgs[7].pArg = static_cast<void *>( &m_nTransparency );
        openclArgs[7].argSize = sizeof( cl_uchar );


        ret = m_openclSrvObj.Execute( &m_kernel, openclArgs, 8, &openclWorkParams );
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "Failed to execute kernel" );
        }
    }

    return ret;
}

void SampleOpticalFlowViz::ThreadMain()
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
                QCBufferDescriptorBase_t &mv = frames.GetBuffer( 0 );
                QCBufferDescriptorBase_t &mvConf = frames.GetBuffer( 1 );

                if ( QC_STATUS_OK == ret )
                {
                    PROFILER_BEGIN();
                    TRACE_BEGIN( frames.FrameId( 0 ) );
                    if ( QC_PROCESSOR_CPU == m_processor )
                    {
                        ret = ConvertToRgbCPU( mv, mvConf, rgb->GetBuffer() );
                    }
                    else
                    {
                        ret = ConvertToRgbGPU( mv, mvConf, rgb->GetBuffer() );
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
                        QC_ERROR( "OpticalFlowViz failed for %" PRIu64 " : %d", frames.FrameId( 0 ),
                                  ret );
                    }
                }
            }
        }
    }
}

QCStatus_e SampleOpticalFlowViz::Stop()
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

QCStatus_e SampleOpticalFlowViz::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;
    if ( nullptr != m_pBufMgr )
    {
        BufferManager::Put( m_pBufMgr );
        m_pBufMgr = nullptr;
    }
    return ret;
}

REGISTER_SAMPLE( OpticalFlowViz, SampleOpticalFlowViz );

}   // namespace sample
}   // namespace QC
