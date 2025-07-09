// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "QC/sample/shared_ring/SharedMemory.hpp"
#include "QC/sample/shared_ring/SharedRing.hpp"

#include <sstream>

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

using namespace QC;

using namespace QC::sample;
using namespace QC::sample::shared_ring;

static int Usage( const char *program, int error )
{
    printf( "Usage: %s -t topic_name [-h]\n"
            "examples:\n"
            "%s -t /sensor/camera/CAM0/raw\n",
            program, program );
    return error;
}

static bool IsAllZero( void *pData, size_t size )
{
    bool bAllZero = true;
    uint8_t *pU8 = (uint8_t *) pData;

    for ( size_t i = 0; i < size; i++ )
    {
        if ( 0 != pU8[i] )
        {
            bAllZero = false;
            break;
        }
    }

    return bAllZero;
}

static void DumpRing( std::string name, SharedRing_Ring_t *pRing )
{
    bool bAllZero = IsAllZero( pRing, sizeof( SharedRing_Ring_t ) );
    if ( bAllZero )
    {
        return;
    }
    printf( "%s name: '%s'\n", name.c_str(), pRing->name );
    uint8_t *pU8 = (uint8_t *) &pRing->spinlock;
    printf( "  spinlock raw: [" );
    for ( int i = 0; i < sizeof( QCSpinLock_t ); i++ )
    {
        printf( "0x%" PRIx8 ", ", pU8[i] );
    }
    printf( "]\n" );
    printf( "  reserved: %" PRIi32, pRing->reserved );
    printf( "  status: %" PRIi32, pRing->status );
    printf( "  queueDepth: %" PRIu32, pRing->queueDepth );
    printf( "\n  readIdx: 0x%" PRIx16 " %" PRIu16 " -> %" PRIu16, pRing->readIdx,
            pRing->readIdx % SHARED_RING_NUM_DESC,
            pRing->ring[pRing->readIdx % SHARED_RING_NUM_DESC] );
    printf( "  writeIdx: 0x%" PRIx16 " %" PRIu16 " -> %" PRIu16, pRing->writeIdx,
            pRing->writeIdx % SHARED_RING_NUM_DESC,
            pRing->ring[pRing->writeIdx % SHARED_RING_NUM_DESC] );
    printf( "  size: %" PRIu16, pRing->writeIdx - pRing->readIdx );
    printf( "\n  ring: [" );
    for ( int i = 0; i < SHARED_RING_NUM_DESC; i++ )
    {
        printf( "%" PRIu16 ", ", pRing->ring[i] );
        if ( ( i > 0 ) && ( 0 == ( i % 32 ) ) )
        {
            printf( "\n         " );
        }
    }
    printf( "]\n" );
    bAllZero = IsAllZero( pRing->reserved0, sizeof( pRing->reserved0 ) );
    printf( "  reserved0(%" PRIu64 ") all is zero: %s\n", sizeof( pRing->reserved0 ),
            bAllZero ? "true" : "false" );
}

static std::string GetBufferTextInfo( const QCSharedBuffer_t *pSharedBuffer )
{
    std::string str = "";
    std::stringstream ss;

    if ( QC_BUFFER_TYPE_RAW == pSharedBuffer->type )
    {
        str = "Raw";
    }
    else if ( QC_BUFFER_TYPE_IMAGE == pSharedBuffer->type )
    {
        ss << "Image format=" << pSharedBuffer->imgProps.format
           << " batch=" << pSharedBuffer->imgProps.batchSize
           << " resolution=" << pSharedBuffer->imgProps.width << "x"
           << pSharedBuffer->imgProps.height;
        if ( pSharedBuffer->imgProps.format < QC_IMAGE_FORMAT_MAX )
        {
            ss << " stride=[";
            for ( uint32_t i = 0; i < pSharedBuffer->imgProps.numPlanes; i++ )
            {
                ss << pSharedBuffer->imgProps.stride[i] << ", ";
            }
            ss << "] actual height=[";
            for ( uint32_t i = 0; i < pSharedBuffer->imgProps.numPlanes; i++ )
            {
                ss << pSharedBuffer->imgProps.actualHeight[i] << ", ";
            }
            ss << "] plane size=[";
            for ( uint32_t i = 0; i < pSharedBuffer->imgProps.numPlanes; i++ )
            {
                ss << pSharedBuffer->imgProps.planeBufSize[i] << ", ";
            }
            ss << "]";
        }
        else
        {
            ss << " plane size=[";
            for ( uint32_t i = 0; i < pSharedBuffer->imgProps.numPlanes; i++ )
            {
                ss << pSharedBuffer->imgProps.planeBufSize[i] << ", ";
            }
            ss << "]";
        }
        str = ss.str();
    }
    else if ( QC_BUFFER_TYPE_TENSOR == pSharedBuffer->type )
    {
        ss << "Tensor type=" << pSharedBuffer->tensorProps.type << " dims=[";
        for ( uint32_t i = 0; i < pSharedBuffer->tensorProps.numDims; i++ )
        {
            ss << pSharedBuffer->tensorProps.dims[i] << ", ";
        }
        ss << "]";
        str = ss.str();
    }
    else
    {
        /* Invalid type, impossible case */
    }

    return str;
}

static void DumpDesc( std::string name, SharedRing_Desc_t *pDesc )
{
    bool bAllZero = IsAllZero( pDesc, sizeof( SharedRing_Desc_t ) );
    if ( bAllZero )
    {
        return;
    }

    printf( "%s: number %" PRIu32, name.c_str(), pDesc->numDataFrames );
    printf( "  timestamp %" PRIu64, pDesc->timestamp );
    printf( "  ref %" PRIi32, pDesc->ref );
    bAllZero = IsAllZero( pDesc->reserved, sizeof( pDesc->reserved ) );
    printf( "  reserved(%" PRIu64 ") all is zero: %s\n", sizeof( pDesc->reserved ),
            bAllZero ? "true" : "false" );
    for ( uint32_t i = 0; i < pDesc->numDataFrames; i++ )
    {
        auto &dataFrame = pDesc->dataFrames[i];
        printf( "   [%" PRIu32 "] name='%s' frameId=%" PRIu64 " timestamp=%" PRIu64
                " quantScale=%.3f quantOffset=%" PRIi32 "\n",
                i, dataFrame.name, dataFrame.frameId, dataFrame.timestamp, dataFrame.quantScale,
                dataFrame.quantOffset );
        bAllZero = IsAllZero( dataFrame.reserved, sizeof( dataFrame.reserved ) );
        printf( "    reserved(%" PRIu64 ") all is zero: %s\n", sizeof( dataFrame.reserved ),
                bAllZero ? "true" : "false" );
        auto &buf = dataFrame.buf;
        printf( "    dma=%" PRIu64 " size=%" PRIu64 " pid=%" PRIu64 " usage=%d flags=%" PRIu32 "\n",
                buf.buffer.dmaHandle, buf.buffer.size, buf.buffer.pid, buf.buffer.usage,
                buf.buffer.flags );
        printf( "    size=%" PRIu64 " offset=%" PRIu64 " type=%d\n", buf.size, buf.offset,
                buf.type );
        printf( "    %s\n", GetBufferTextInfo( &buf ).c_str() );
    }
}

int main( int argc, char *argv[] )
{
    int opt;
    std::string topicName;
    bool bShowAll = false;

    while ( ( opt = getopt( argc, argv, "at:h" ) ) != -1 )
    {
        switch ( opt )
        {
            case 'a':
                bShowAll = true;
                break;
            case 't':
                topicName = optarg;
                break;
            case 'h':
                return Usage( argv[0], 0 );
                break;
            default:
                break;
        }
    }

    if ( topicName.empty() )
    {
        return Usage( argv[0], EINVAL );
    }

    SharedMemory shmem;
    std::replace( topicName.begin(), topicName.end(), '/', '_' );
    QCStatus_e ret = shmem.Open( topicName );

    if ( QC_STATUS_OK != ret )
    {
        printf( "Failed to open shared memory: %s", topicName.c_str() );
        return EACCES;
    }

    SharedRing_Memory_t *pRingMem = (SharedRing_Memory_t *) shmem.Data();

    printf( "topic: '%s'\n", pRingMem->name );
    printf( "status: %" PRIi32 "\n", pRingMem->status );
    bool bAllZero = IsAllZero( pRingMem->reserved0, sizeof( pRingMem->reserved0 ) );
    printf( "reserved0(%" PRIu64 ") all is zero: %s\n", sizeof( pRingMem->reserved0 ),
            bAllZero ? "true" : "false" );

    DumpRing( "avail", &pRingMem->avail );
    DumpRing( "free", &pRingMem->free );
    for ( int i = 0; i < SHARED_RING_NUM_SUBSCRIBERS; i++ )
    {
        DumpRing( "used" + std::to_string( i ), &pRingMem->used[i] );
    }

    if ( bShowAll )
    {
        for ( int i = 0; i < SHARED_RING_NUM_DESC; i++ )
        {
            DumpDesc( "desc" + std::to_string( i ), &pRingMem->descs[i] );
        }
    }

    return 0;
}