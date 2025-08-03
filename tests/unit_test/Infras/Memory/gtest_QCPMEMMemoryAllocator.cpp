// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "QC/Infras/Memory/PMEMAllocator.hpp"
#include "gtest/gtest.h"

using namespace QC;
using namespace QC::Memory;

class Test_PMEMAllocator : public testing::Test
{

protected:
    void SetUp() override {
        allocatorIfs = new PMEMAllocator( {"QC_MEMORY_ALLOCATOR_DMA"} , QC_MEMORY_ALLOCATOR_DMA );
    }

    void TearDown() override {
        allocatorIfs->~QCMemoryAllocatorIfs();
    }

    QCMemoryAllocatorIfs* allocatorIfs;
};


TEST_F( Test_PMEMAllocator, SANITY_1 )
{
    QCBufferPropBase_t request;
    request.size = 10;
    request.alignment = QC_MEMORY_DEFAULT_ALLIGNMENT;
    request.cache = QC_CACHEABLE;
    QCBufferDescriptorBase_t response;

    QCStatus_e status = allocatorIfs->Allocate( request, response );
    ASSERT_EQ( QC_STATUS_OK, status );
    ASSERT_EQ( (long long unsigned int) response.pBuf,
                (long long unsigned int) response.pBuf & ( ~( request.alignment - 1 ) ) );
    ASSERT_EQ( response.size, 10 );
    ASSERT_EQ( response.cache, QC_CACHEABLE );
    status = allocatorIfs->Free( response );
    ASSERT_EQ( QC_STATUS_OK, status );
}

TEST_F( Test_PMEMAllocator, SANITY_2 )
{
    QCBufferPropBase_t request;
    request.size = 128;
    request.alignment = QC_MEMORY_DEFAULT_ALLIGNMENT;
    request.cache = QC_CACHEABLE;
    QCBufferDescriptorBase_t response;

    QCStatus_e status = allocatorIfs->Allocate( request, response );
    ASSERT_EQ( QC_STATUS_OK, status );
    ASSERT_EQ( (long long unsigned int) response.pBuf,
                (long long unsigned int) response.pBuf & ( ~( request.alignment - 1 ) ) );
    ASSERT_EQ( response.size, 128 );
    ASSERT_EQ( response.cache, QC_CACHEABLE );
    status = allocatorIfs->Free( response );
    ASSERT_EQ( QC_STATUS_OK, status );
}

TEST_F( Test_PMEMAllocator, SANITY_3 )
{
    QCBufferPropBase_t request;
    request.size = 10;
    request.alignment = QC_MEMORY_DEFAULT_ALLIGNMENT;
    request.cache = QC_CACHEABLE;
    QCBufferDescriptorBase_t response;

    QCStatus_e status = allocatorIfs->Allocate( request, response );
    ASSERT_EQ( QC_STATUS_OK, status );
    ASSERT_EQ( (long long unsigned int) response.pBuf,
                (long long unsigned int) response.pBuf & ( ~( request.alignment - 1 ) ) );
    ASSERT_EQ( response.size, 10 );
    ASSERT_EQ( response.cache, QC_CACHEABLE );
    status = allocatorIfs->Free( response );
    ASSERT_EQ( QC_STATUS_OK, status );
}

TEST_F( Test_PMEMAllocator, SANITY_multiple_allocations_1 )
{
    QCBufferPropBase_t request[4];
    request[0].size = 10;
    request[0].alignment = QC_MEMORY_DEFAULT_ALLIGNMENT;
    request[0].cache = QC_CACHEABLE;

    request[1].size = 20;
    request[1].alignment = QC_MEMORY_DEFAULT_ALLIGNMENT;
    request[1].cache = QC_CACHEABLE;

    request[2].size = 1024 * 2;
    request[2].alignment = QC_MEMORY_DEFAULT_ALLIGNMENT;
    request[2].cache = QC_CACHEABLE;

    request[3].size = 1024 * 10;
    request[3].alignment = QC_MEMORY_DEFAULT_ALLIGNMENT;
    request[3].cache = QC_CACHEABLE;
    QCBufferDescriptorBase_t response[4];

    QCStatus_e status = allocatorIfs->Allocate( request[0], response[0] );
    ASSERT_EQ( QC_STATUS_OK, status );
    ASSERT_EQ( (long long unsigned int) response[0].pBuf,
                (long long unsigned int) response[0].pBuf & ( ~( request[0].alignment - 1 ) ) );
    ASSERT_EQ( response[0].size, request[0].size );
    ASSERT_EQ( response[0].cache, QC_CACHEABLE );

    status = allocatorIfs->Allocate( request[1], response[1] );
    ASSERT_EQ( QC_STATUS_OK, status );
    ASSERT_EQ( (long long unsigned int) response[1].pBuf,
                (long long unsigned int) response[1].pBuf & ( ~( request[1].alignment - 1 ) ) );
    ASSERT_EQ( response[1].size, request[1].size );
    ASSERT_EQ( response[1].cache, QC_CACHEABLE );

    status = allocatorIfs->Allocate( request[2], response[2] );
    ASSERT_EQ( QC_STATUS_OK, status );
    ASSERT_EQ( (long long unsigned int) response[2].pBuf,
                (long long unsigned int) response[2].pBuf & ( ~( request[2].alignment - 1 ) ) );
    ASSERT_EQ( response[2].size, request[2].size );
    ASSERT_EQ( response[2].cache, QC_CACHEABLE );

    status = allocatorIfs->Allocate( request[3], response[3] );
    ASSERT_EQ( QC_STATUS_OK, status );
    ASSERT_EQ( (long long unsigned int) response[3].pBuf,
                (long long unsigned int) response[3].pBuf & ( ~( request[3].alignment - 1 ) ) );
    ASSERT_EQ( response[3].size, request[3].size );
    ASSERT_EQ( response[3].cache, QC_CACHEABLE );
    status = allocatorIfs->Free( response[3] );
    ASSERT_EQ( QC_STATUS_OK, status );

    status = allocatorIfs->Free( response[2] );
    ASSERT_EQ( QC_STATUS_OK, status );

    status = allocatorIfs->Free( response[1] );
    ASSERT_EQ( QC_STATUS_OK, status );

    status = allocatorIfs->Free( response[0] );
    ASSERT_EQ( QC_STATUS_OK, status );
}

TEST_F( Test_PMEMAllocator, SANITY_multiple_allocations_2 )
{
    QCBufferPropBase_t request[4];
    request[0].size = 10;
    request[0].alignment = QC_MEMORY_DEFAULT_ALLIGNMENT;
    request[0].cache = QC_CACHEABLE;

    request[1].size = 20;
    request[1].alignment = QC_MEMORY_DEFAULT_ALLIGNMENT;
    request[1].cache = QC_CACHEABLE;

    request[2].size = 1024 * 2;
    request[2].alignment = QC_MEMORY_DEFAULT_ALLIGNMENT;
    request[2].cache = QC_CACHEABLE;

    request[3].size = 1024 * 10;
    request[3].alignment = QC_MEMORY_DEFAULT_ALLIGNMENT;
    request[3].cache = QC_CACHEABLE;
    QCBufferDescriptorBase_t response[4];

    QCStatus_e status = allocatorIfs->Allocate( request[0], response[0] );
    ASSERT_EQ( QC_STATUS_OK, status );
    ASSERT_EQ( (long long unsigned int) response[0].pBuf,
                (long long unsigned int) response[0].pBuf & ( ~( request[0].alignment - 1 ) ) );
    ASSERT_EQ( response[0].size, request[0].size );
    ASSERT_EQ( response[0].cache, QC_CACHEABLE );

    status = allocatorIfs->Allocate( request[1], response[1] );
    ASSERT_EQ( QC_STATUS_OK, status );
    ASSERT_EQ( (long long unsigned int) response[1].pBuf,
                (long long unsigned int) response[1].pBuf & ( ~( request[1].alignment - 1 ) ) );
    ASSERT_EQ( response[1].size, request[1].size );
    ASSERT_EQ( response[1].cache, QC_CACHEABLE );

    status = allocatorIfs->Allocate( request[2], response[2] );
    ASSERT_EQ( QC_STATUS_OK, status );
    ASSERT_EQ( (long long unsigned int) response[2].pBuf,
                (long long unsigned int) response[2].pBuf & ( ~( request[2].alignment - 1 ) ) );
    ASSERT_EQ( response[2].size, request[2].size );
    ASSERT_EQ( response[2].cache, QC_CACHEABLE );

    status = allocatorIfs->Allocate( request[3], response[3] );
    ASSERT_EQ( QC_STATUS_OK, status );
    ASSERT_EQ( (long long unsigned int) response[3].pBuf,
                (long long unsigned int) response[3].pBuf & ( ~( request[3].alignment - 1 ) ) );
    ASSERT_EQ( response[3].size, request[3].size );
    ASSERT_EQ( response[3].cache, QC_CACHEABLE );

    status = allocatorIfs->Free( response[3] );
    ASSERT_EQ( QC_STATUS_OK, status );

    status = allocatorIfs->Free( response[2] );
    ASSERT_EQ( QC_STATUS_OK, status );

    status = allocatorIfs->Free( response[1] );
    ASSERT_EQ( QC_STATUS_OK, status );

    status = allocatorIfs->Free( response[0] );
    ASSERT_EQ( QC_STATUS_OK, status );

    QCBufferPropBase_t badRequest;
    request[0].size = 10;
    request[0].alignment = QC_MEMORY_DEFAULT_ALLIGNMENT;
    request[0].cache = QC_CACHEABLE_WRITE_THROUGH;

    status = allocatorIfs->Allocate( badRequest, response[1] );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );
}
