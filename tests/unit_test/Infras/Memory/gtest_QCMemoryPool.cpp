// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "QC/Infras/Memory/HeapAllocator.hpp"
#include "QC/Infras/Memory/Pool.hpp"

#include "gtest/gtest.h"

using namespace QC;
using namespace QC::Memory;

class Test_QCMemoryPool : public testing::Test
{

protected:
    void SetUp() override {}

    void TearDown() override {}
};

TEST_F( Test_QCMemoryPool, SANITY )
{
    {
        HeapAllocator allocatorIfs;

        QCMemoryPoolConfig_t poolCfg( allocatorIfs );
        poolCfg.buff.size = 512;
        poolCfg.buff.alignment = QC_MEMORY_DEFAULT_ALLIGNMENT;
        poolCfg.buff.cache = QC_CACHEABLE;
        poolCfg.maxElements = 10;
        poolCfg.name = "Test Pool";

        Pool memoryPool( poolCfg );
        QCStatus_e status = memoryPool.Init();
        ASSERT_EQ( QC_STATUS_OK, status );

        QCBufferDescriptorBase_t buffer[10];
        status = memoryPool.GetElement( buffer[0] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = memoryPool.GetElement( buffer[1] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = memoryPool.GetElement( buffer[2] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = memoryPool.GetElement( buffer[3] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = memoryPool.GetElement( buffer[4] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = memoryPool.GetElement( buffer[5] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = memoryPool.GetElement( buffer[6] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = memoryPool.GetElement( buffer[7] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = memoryPool.GetElement( buffer[8] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = memoryPool.GetElement( buffer[9] );
        ASSERT_EQ( QC_STATUS_OK, status );

        // get element beyond available (11/10)
        QCBufferDescriptorBase_t extraBuffer;
        status = memoryPool.GetElement( extraBuffer );
        ASSERT_EQ( QC_STATUS_NO_RESOURCE, status );

        // return wrong element
        // wrong allocator
        extraBuffer.allocatorType = QC_MEMORY_ALLOCATOR_DMA_EVA;
        status = memoryPool.PutElement( extraBuffer );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        // wrong pBuf
        extraBuffer.allocatorType = QC_MEMORY_ALLOCATOR_HEAP;
        extraBuffer.pBuf = nullptr;
        status = memoryPool.PutElement( extraBuffer );
        ASSERT_EQ( QC_STATUS_NULL_PTR, status );

        // returning correct buffers
        status = memoryPool.PutElement( buffer[0] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = memoryPool.PutElement( buffer[1] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = memoryPool.PutElement( buffer[2] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = memoryPool.PutElement( buffer[3] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = memoryPool.PutElement( buffer[4] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = memoryPool.PutElement( buffer[5] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = memoryPool.PutElement( buffer[6] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = memoryPool.PutElement( buffer[7] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = memoryPool.PutElement( buffer[8] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = memoryPool.PutElement( buffer[9] );
        ASSERT_EQ( QC_STATUS_OK, status );
    }
}
