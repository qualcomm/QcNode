
#include "md5_utils.hpp"
#include "gtest/gtest.h"
#include <chrono>
#include <cstdlib>
#include <stdio.h>
#include <unistd.h>

#include "QC/Node/OpticalFlow.hpp"
#include "QC/sample/BufferManager.hpp"

using namespace QC::Node;
using namespace QC::test::utils;
using namespace QC;
using namespace QC::Memory;
using namespace QC::sample;

#define ALIGN_S( size, align ) ( ( size + align - 1 ) / align ) * align

static void LoadRaw( void *pData, uint32_t length, std::string path )
{
    printf( "  load raw from %s\n", path.c_str() );
    FILE *pFile = fopen( path.c_str(), "rb" );
    ASSERT_NE( nullptr, pFile );
    fseek( pFile, 0, SEEK_END );
    int size = ftell( pFile );
    ASSERT_LE( size, length );
    fseek( pFile, 0, SEEK_SET );
    int r = fread( pData, 1, size, pFile );
    ASSERT_EQ( r, size );
    fclose( pFile );
}

static void SaveRaw( std::string path, void *pData, size_t size )
{
    FILE *pFile = fopen( path.c_str(), "wb" );
    if ( nullptr != pFile )
    {
        fwrite( pData, 1, size, pFile );
        fclose( pFile );
        printf( "  save raw %s\n", path.c_str() );
    }
}


static void Eva_NodeOpticalFlowRun( std::string name, OpticalFlow_Config_t &config,
                                    std::string img1 = "", std::string img2 = "",
                                    std::string goldenMvMap = "", std::string goldenMvConf = "" )
{
    QC::Node::OpticalFlow opticalFlow;
    QCNodeIfs *node = dynamic_cast<QCNodeIfs *>( &opticalFlow );
    QCStatus_e ret;
    QCStatus_e status;

    BufferManager bufMgr( { "LME", QC_NODE_TYPE_EVA_OPTICAL_FLOW, 0 } );

    ImageDescriptor_t refImgDesc;
    ImageBasicProps_t refImgProp;
    ImageDescriptor_t curImgDesc;
    ImageBasicProps_t curImgProp;
    TensorDescriptor_t mvFwdMapDesc;
    TensorDescriptor_t mvFwdconfMapDesc;
    TensorDescriptor_t mvFwdMapDescG;
    TensorDescriptor_t mvFwdconfMapDescG;


    refImgProp.format = config.imageFormat;
    refImgProp.batchSize = 1;
    refImgProp.width = config.width;
    refImgProp.height = config.height;

    ret = bufMgr.Allocate(refImgProp, refImgDesc);
    ASSERT_EQ( QC_STATUS_OK, ret );

    curImgProp.format = config.imageFormat;
    curImgProp.batchSize = 1;
    curImgProp.width = config.width;
    curImgProp.height = config.height;

    ret = bufMgr.Allocate(curImgProp, curImgDesc);
    ASSERT_EQ( QC_STATUS_OK, ret );

    if (img1.empty() == false)
    {
        LoadRaw(refImgDesc.GetDataPtr(), refImgDesc.GetDataSize(), img1);
    }

    if (img2.empty() == false)
    {
        LoadRaw(curImgDesc.GetDataPtr(), curImgDesc.GetDataSize(), img2);
    }

    memset(mvFwdMapDesc.GetDataPtr(), 0, mvFwdMapDesc.GetDataSize());
    memset(mvFwdconfMapDesc.GetDataPtr(), 0, mvFwdconfMapDesc.GetDataSize());

    if ( (goldenMvMap.empty() == false) and (goldenMvConf.empty() == false) )
    { /* clear output buffer for accuracy test */
        memset(mvFwdMapDescG.GetDataPtr(), 0, mvFwdMapDescG.GetDataSize());
        memset(mvFwdconfMapDescG.GetDataPtr(), 0, mvFwdconfMapDescG.GetDataSize());
    }

    TensorProps_t mvFwdMapProp = { QC_TENSOR_TYPE_UINT_16,
                                    { 1, ALIGN_S( config.height, 8 ), ALIGN_S( config.width * 2, 128 ), 1 }};

    ret = bufMgr.Allocate(mvFwdMapProp, mvFwdMapDesc);
    ASSERT_EQ(QC_STATUS_OK, ret);

    TensorProps_t mvFwdConfProp = { QC_TENSOR_TYPE_UINT_8,
                                    { 1, ALIGN_S( config.height, 8 ), ALIGN_S( config.width, 128 ), 1 }};


    ret = bufMgr.Allocate(mvFwdConfProp, mvFwdconfMapDesc);
    ASSERT_EQ(QC_STATUS_OK, ret);

    printf( "-- Test for %s\n", name.c_str() );

    DataTree dt;
    DataTree top_dt;
    dt.Set<uint32_t>( "width", config.width);
    dt.Set<uint32_t>( "height", config.height);
    dt.Set<uint32_t>( "fps", config.frameRate);
    dt.Set<bool>( "confidenceOutputEn", config.confidenceOutputEn);
    dt.Set<uint8_t>("computationAccuracy", config.computationAccuracy);
    dt.Set<uint32_t>("edgeAlignMetric", config.edgeAlignMetric);
    dt.Set<float32_t>("imageSharpnessThreshold", config.imageSharpnessThreshold);
    dt.Set<float32_t>("textureThreshold", config.textureThreshold);
    dt.Set<bool>("isFirstRequest", config.isFirstRequest);
    top_dt.Set("static", dt);
    top_dt.Set<std::string>( "static.name", "SANITY" );

    QCNodeInit_t configuration = { top_dt.Dump() };
    printf( "configuration.config: %s\n", configuration.config.c_str() );
    status = node->Initialize( configuration );
    ASSERT_EQ( QC_STATUS_OK, status );


    NodeFrameDescriptor *frameDescriptor =
            new NodeFrameDescriptor( QC_NODE_OF_LAST_BUFF_ID );

    ret = frameDescriptor->SetBuffer(QC_NODE_OF_REFERENCE_IMAGE_BUFF_ID, refImgDesc);
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = frameDescriptor->SetBuffer(QC_NODE_OF_CURRENT_IMAGE_BUFF_ID, curImgDesc);
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = frameDescriptor->SetBuffer(QC_NODE_OF_FWD_MOTION_BUFF_ID, mvFwdMapDesc);
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = frameDescriptor->SetBuffer(QC_NODE_OF_FWD_CONF_BUFF_ID, mvFwdconfMapDesc);
    ASSERT_EQ( QC_STATUS_OK, ret );


    status = node->Start();
    ASSERT_EQ( QC_STATUS_OK, status );

    status = node->ProcessFrameDescriptor( *frameDescriptor );
    ASSERT_EQ( QC_STATUS_OK, status );

    if ((goldenMvMap.empty() == false) and (goldenMvConf.empty() == false))
    {
        // for the first run, execute the below to generate the golden output
        #if defined( __QNXNTO__ )
            SaveRaw(goldenMvMap, mvFwdMapDesc.GetDataPtr(), mvFwdMapDesc.GetDataSize());
            SaveRaw(goldenMvConf, mvFwdconfMapDesc.GetDataPtr(), mvFwdconfMapDesc.GetDataSize());
        #endif

        ret = bufMgr.Allocate(mvFwdMapProp, mvFwdMapDescG);
        ASSERT_EQ(QC_STATUS_OK, ret);

        ret = bufMgr.Allocate(mvFwdConfProp, mvFwdconfMapDescG);
        ASSERT_EQ(QC_STATUS_OK, ret);

        LoadRaw( mvFwdMapDescG.GetDataPtr(), mvFwdMapDescG.GetDataSize(), goldenMvMap );
        LoadRaw( mvFwdconfMapDescG.GetDataPtr(), mvFwdconfMapDescG.GetDataSize(), goldenMvConf );

        std::string md5Output = MD5Sum(mvFwdMapDesc.GetDataPtr(), mvFwdMapDesc.GetDataSize());
        std::string md5Golden = MD5Sum(mvFwdMapDescG.GetDataPtr(), mvFwdMapDescG.GetDataSize());
        ASSERT_EQ( md5Output, md5Golden );

        md5Output = MD5Sum(mvFwdconfMapDesc.GetDataPtr(), mvFwdconfMapDesc.GetDataSize());
        md5Golden = MD5Sum(mvFwdconfMapDescG.GetDataPtr(), mvFwdconfMapDescG.GetDataSize());
        ASSERT_EQ( md5Output, md5Golden );
    }

    status = node->Stop();
    ASSERT_EQ( QC_STATUS_OK, status );

    status = node->DeInitialize();
    ASSERT_EQ( QC_STATUS_OK, status );

    bufMgr.Free(refImgDesc);
    bufMgr.Free(curImgDesc);
    bufMgr.Free(mvFwdMapDesc);
    bufMgr.Free(mvFwdconfMapDesc);
    bufMgr.Free(mvFwdMapDescG);
    bufMgr.Free(mvFwdconfMapDescG);
    delete frameDescriptor;
}


TEST( EVA, SANITY_NodeOpticalFlowCPU )
{
    OpticalFlow_Config_t config;
    config.width = 1920;
    config.height = 1024;
    config.frameRate = 30;
    config.imageFormat = QC_IMAGE_FORMAT_NV12;
    config.confidenceOutputEn = true;
    config.computationAccuracy = COMPUTATION_ACCURACY_MEDIUM;
    config.edgeAlignMetric = 0;
    config.imageSharpnessThreshold = 0.0f;
    config.textureThreshold = 0.5f;
    config.isFirstRequest = true;
    Eva_NodeOpticalFlowRun( "OFL0_FWD", config, "data/test/ofl/0.nv12", "data/test/ofl/1.nv12",
                            "data/test/ofl/mv-map.raw",
                            "data/test/ofl/mv-conf.raw" );
}



#ifndef GTEST_QCNODE
#if __CTC__
extern "C" void ctc_append_all( void );
#endif
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
#if __CTC__
    ctc_append_all();
#endif
    return nVal;
}
#endif
