// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QC/Infras/Memory/HeapAllocator.hpp"

#include "gtest/gtest.h"

using namespace QC;
using namespace QC::Memory;

class Test_HeapAllocator : public testing::Test
{

protected:
    void SetUp() override {}

    void TearDown() override {}
};

TEST_F( Test_HeapAllocator, SANITY )
{
    {
        HeapAllocator allocatorIfs;

        QCBufferPropBase_t request;
        request.size = 10;
        request.alignment = QC_MEMORY_DEFAULT_ALLIGNMENT;
        request.cache = QC_CACHEABLE;
        QCBufferDescriptorBase_t response;

        QCStatus_e status = allocatorIfs.Allocate( request, response );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( (long long unsigned int) response.pBuf,
                   (long long unsigned int) response.pBuf & ( ~( request.alignment - 1 ) ) );
        ASSERT_EQ( response.size, 10 );
        ASSERT_EQ( response.cache, QC_CACHEABLE );
        status = allocatorIfs.Free( response );
        ASSERT_EQ( QC_STATUS_OK, status );

        allocatorIfs.~HeapAllocator();
    }

    {
        HeapAllocator allocatorIfs;

        QCBufferPropBase_t request;
        request.size = 128;
        request.alignment = QC_MEMORY_DEFAULT_ALLIGNMENT;
        request.cache = QC_CACHEABLE;
        QCBufferDescriptorBase_t response;

        QCStatus_e status = allocatorIfs.Allocate( request, response );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( (long long unsigned int) response.pBuf,
                   (long long unsigned int) response.pBuf & ( ~( request.alignment - 1 ) ) );
        ASSERT_EQ( response.size, 128 );
        ASSERT_EQ( response.cache, QC_CACHEABLE );
        status = allocatorIfs.Free( response );
        ASSERT_EQ( QC_STATUS_OK, status );

        allocatorIfs.~HeapAllocator();
    }

    {
        QCMemoryAllocatorIfs *allocatorIfsPtr = new HeapAllocator();

        QCBufferPropBase_t request;
        request.size = 10;
        request.alignment = QC_MEMORY_DEFAULT_ALLIGNMENT;
        request.cache = QC_CACHEABLE;
        QCBufferDescriptorBase_t response;

        QCStatus_e status = allocatorIfsPtr->Allocate( request, response );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( (long long unsigned int) response.pBuf,
                   (long long unsigned int) response.pBuf & ( ~( request.alignment - 1 ) ) );
        ASSERT_EQ( response.size, 10 );
        ASSERT_EQ( response.cache, QC_CACHEABLE );
        status = allocatorIfsPtr->Free( response );
        ASSERT_EQ( QC_STATUS_OK, status );

        delete allocatorIfsPtr;
    }

    {
        QCMemoryAllocatorIfs *allocatorIfsPtr = new HeapAllocator();

        QCBufferPropBase_t request;
        request.size = 10;
        request.alignment = QC_MEMORY_DEFAULT_ALLIGNMENT;
        request.cache = QC_CACHEABLE_NON;
        QCBufferDescriptorBase_t response;

        QCStatus_e status = allocatorIfsPtr->Allocate( request, response );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        delete allocatorIfsPtr;
    }
}

TEST_F( Test_HeapAllocator, SANITY_multiple_allocations )
{
    {
        HeapAllocator allocatorIfs;

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

        QCStatus_e status = allocatorIfs.Allocate( request[0], response[0] );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( (long long unsigned int) response[0].pBuf,
                   (long long unsigned int) response[0].pBuf & ( ~( request[0].alignment - 1 ) ) );
        ASSERT_EQ( response[0].size, request[0].size );
        ASSERT_EQ( response[0].cache, QC_CACHEABLE );

        status = allocatorIfs.Allocate( request[1], response[1] );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( (long long unsigned int) response[1].pBuf,
                   (long long unsigned int) response[1].pBuf & ( ~( request[1].alignment - 1 ) ) );
        ASSERT_EQ( response[1].size, request[1].size );
        ASSERT_EQ( response[1].cache, QC_CACHEABLE );

        status = allocatorIfs.Allocate( request[2], response[2] );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( (long long unsigned int) response[2].pBuf,
                   (long long unsigned int) response[2].pBuf & ( ~( request[2].alignment - 1 ) ) );
        ASSERT_EQ( response[2].size, request[2].size );
        ASSERT_EQ( response[2].cache, QC_CACHEABLE );

        status = allocatorIfs.Allocate( request[3], response[3] );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( (long long unsigned int) response[3].pBuf,
                   (long long unsigned int) response[3].pBuf & ( ~( request[3].alignment - 1 ) ) );
        ASSERT_EQ( response[3].size, request[3].size );
        ASSERT_EQ( response[3].cache, QC_CACHEABLE );
        status = allocatorIfs.Free( response[3] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = allocatorIfs.Free( response[2] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = allocatorIfs.Free( response[1] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = allocatorIfs.Free( response[0] );
        ASSERT_EQ( QC_STATUS_OK, status );

        response[0].allocatorType = QC_MEMORY_ALLOCATOR_DMA;
        status = allocatorIfs.Free( response[0] );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        response[0].allocatorType = QC_MEMORY_ALLOCATOR_HEAP;
        response[0].pBuf = nullptr;
        status = allocatorIfs.Free( response[0] );
        ASSERT_EQ( QC_STATUS_NULL_PTR, status );

        allocatorIfs.~HeapAllocator();
    }


    {
        QCMemoryAllocatorIfs *allocatorIfsPtr = new HeapAllocator();

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

        QCStatus_e status = allocatorIfsPtr->Allocate( request[0], response[0] );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( (long long unsigned int) response[0].pBuf,
                   (long long unsigned int) response[0].pBuf & ( ~( request[0].alignment - 1 ) ) );
        ASSERT_EQ( response[0].size, request[0].size );
        ASSERT_EQ( response[0].cache, QC_CACHEABLE );

        status = allocatorIfsPtr->Allocate( request[1], response[1] );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( (long long unsigned int) response[1].pBuf,
                   (long long unsigned int) response[1].pBuf & ( ~( request[1].alignment - 1 ) ) );
        ASSERT_EQ( response[1].size, request[1].size );
        ASSERT_EQ( response[1].cache, QC_CACHEABLE );

        status = allocatorIfsPtr->Allocate( request[2], response[2] );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( (long long unsigned int) response[2].pBuf,
                   (long long unsigned int) response[2].pBuf & ( ~( request[2].alignment - 1 ) ) );
        ASSERT_EQ( response[2].size, request[2].size );
        ASSERT_EQ( response[2].cache, QC_CACHEABLE );

        status = allocatorIfsPtr->Allocate( request[3], response[3] );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( (long long unsigned int) response[3].pBuf,
                   (long long unsigned int) response[3].pBuf & ( ~( request[3].alignment - 1 ) ) );
        ASSERT_EQ( response[3].size, request[3].size );
        ASSERT_EQ( response[3].cache, QC_CACHEABLE );

        status = allocatorIfsPtr->Free( response[3] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = allocatorIfsPtr->Free( response[2] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = allocatorIfsPtr->Free( response[1] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = allocatorIfsPtr->Free( response[0] );
        ASSERT_EQ( QC_STATUS_OK, status );

        QCBufferPropBase_t badRequest;
        request[0].size = 10;
        request[0].alignment = QC_MEMORY_DEFAULT_ALLIGNMENT;
        request[0].cache = QC_CACHEABLE_WRITE_THROUGH;

        status = allocatorIfsPtr->Allocate( badRequest, response[1] );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        delete allocatorIfsPtr;
    }
}

class Test_MemoryIfs : public testing::Test
{

protected:
    void SetUp() override {}

    void TearDown() override {}
};

TEST_F( Test_MemoryIfs, SANITY_QCBufferDescriptorBase_default_constructor )
{
    QCBufferDescriptorBase_t desc;

    // Verify initial values set by the default constructor
    ASSERT_EQ( desc.pBuf, nullptr );
    ASSERT_EQ( desc.size, 0 );
    ASSERT_EQ( desc.alignment,
               QC_MEMORY_DEFAULT_ALLIGNMENT );   // Assuming this is 4 * 1024 as per defs
    ASSERT_EQ( desc.cache, QC_MEMORY_DEFAULT_CACHE_ATTRIBUTES );   // Assuming this is QC_CACHEABLE
    ASSERT_EQ( desc.allocatorType, QC_MEMORY_ALLOCATOR_LAST );
    ASSERT_EQ( desc.type, QC_BUFFER_TYPE_LAST );
    ASSERT_EQ( desc.dmaHandle, 0 );
    ASSERT_EQ( desc.pid, 0 );
    ASSERT_TRUE( desc.name.empty() );   // std::string defaults to empty
}

TEST_F( Test_MemoryIfs, SANITY_QCBufferDescriptorBase_VirtualDestructorDoesNotCrash )
{
    QCBufferDescriptorBase_t *desc = new QCBufferDescriptorBase_t();
    // No specific assertion needed, just checking for no crash
    delete desc;   // This should call the virtual destructor
    SUCCEED();     // Indicate success if it reaches here without crashing
}

TEST_F( Test_MemoryIfs, SANITY_QCBufferDescriptorBase_OperatorLessThanComparesCorrectly )
{
    QCBufferDescriptorBase_t d1;
    QCBufferDescriptorBase_t d2;

    // Case 1: Identical objects (should be false)
    ASSERT_FALSE( d1 < d2 );
    ASSERT_FALSE( d2 < d1 );

    // Case 2: pBuf differs (should be the primary comparison)
    d1.pBuf = reinterpret_cast<void *>( 0x1000 );
    d2.pBuf = reinterpret_cast<void *>( 0x2000 );
    ASSERT_TRUE( d1 < d2 );
    ASSERT_FALSE( d2 < d1 );   // Ensure strict weak ordering
    d2.pBuf = d1.pBuf;         // Reset for next comparison

    // Case 3: size differs
    d1.size = 100;
    d2.size = 200;
    ASSERT_TRUE( d1 < d2 );
    ASSERT_FALSE( d2 < d1 );
    d2.size = d1.size;

    // Case 4: dmaHandle differs
    d1.dmaHandle = 1;
    d2.dmaHandle = 2;
    ASSERT_TRUE( d1 < d2 );
    ASSERT_FALSE( d2 < d1 );
    d2.dmaHandle = d1.dmaHandle;

    // Case 5: pid differs
    d1.pid = 1000;
    d2.pid = 2000;
    ASSERT_TRUE( d1 < d2 );
    ASSERT_FALSE( d2 < d1 );
    d2.pid = d1.pid;

    // Case 6: alignment differs
    d1.alignment = 16;
    d2.alignment = 32;
    ASSERT_TRUE( d1 < d2 );
    ASSERT_FALSE( d2 < d1 );
    d2.alignment = d1.alignment;

    // Case 7: cache differs
    d1.cache = QC_CACHEABLE_NON;   // Assuming this is 0
    d2.cache = QC_CACHEABLE;       // Assuming this is 1
    ASSERT_TRUE( d1 < d2 );
    ASSERT_FALSE( d2 < d1 );
    d2.cache = d1.cache;

    // Case 8: allocatorType differs
    d1.allocatorType = QC_MEMORY_ALLOCATOR_HEAP;   // Assuming 0
    d2.allocatorType = QC_MEMORY_ALLOCATOR_DMA;    // Assuming 1
    ASSERT_TRUE( d1 < d2 );
    ASSERT_FALSE( d2 < d1 );
    d2.allocatorType = d1.allocatorType;


    // Test a more complex scenario:
    // d1 has a smaller size, all other fields equal initially
    d1 = QCBufferDescriptorBase_t();   // Reset
    d2 = QCBufferDescriptorBase_t();   // Reset

    d1.pBuf = reinterpret_cast<void *>( 0x1000 );
    d2.pBuf = reinterpret_cast<void *>( 0x1000 );   // Equal pBuf

    d1.size = 100;
    d2.size = 200;   // d1 < d2 due to size

    d1.dmaHandle = 5;
    d2.dmaHandle = 1;   // This would make d2 < d1 if size was equal.
                        // But because pBuf and size are compared first, d1 < d2 will hold.

    ASSERT_TRUE( d1 < d2 );
    ASSERT_FALSE( d2 < d1 );

    // Verify order of comparison: pBuf, then size, etc.
    d1 = QCBufferDescriptorBase_t();
    d2 = QCBufferDescriptorBase_t();

    d1.pBuf = reinterpret_cast<void *>( 0x1000 );
    d2.pBuf = reinterpret_cast<void *>( 0x1000 );   // pBuf equal

    d1.size = 100;
    d2.size = 100;   // size equal

    d1.dmaHandle = 2;
    d2.dmaHandle = 1;   // d2 should be less than d1 now due to dmaHandle

    ASSERT_FALSE( d1 < d2 );
    ASSERT_TRUE( d2 < d1 );
}

TEST_F( Test_MemoryIfs, SANITY_QCMemoryAllocatorIfs_default_constructor )
{
    QCBufferDescriptorBase_t desc;

    // Verify initial values set by the default constructor
    ASSERT_EQ( desc.pBuf, nullptr );
    ASSERT_EQ( desc.size, 0 );
    ASSERT_EQ( desc.alignment,
               QC_MEMORY_DEFAULT_ALLIGNMENT );   // Assuming this is 4 * 1024 as per defs
    ASSERT_EQ( desc.cache, QC_MEMORY_DEFAULT_CACHE_ATTRIBUTES );   // Assuming this is QC_CACHEABLE
    ASSERT_EQ( desc.allocatorType, QC_MEMORY_ALLOCATOR_LAST );
    ASSERT_EQ( desc.type, QC_BUFFER_TYPE_LAST );
    ASSERT_EQ( desc.dmaHandle, 0 );
    ASSERT_EQ( desc.pid, 0 );
    ASSERT_TRUE( desc.name.empty() );   // std::string defaults to empty
}

// Test case for constructor initialization
TEST_F( Test_MemoryIfs, SANITY_QCMemoryAllocatorIfs_ConstructorInitializesConfiguration )
{
    QC::Memory::QCMemoryAllocatorConfigInit_t init_cfg = { "TestAllocator" };
    QC::Memory::QCMemoryAllocator_e type = QC_MEMORY_ALLOCATOR_DMA;

    QCMemoryAllocatorIfs allocator( init_cfg, type );

    const QC::Memory::QCMemoryAllocatorConfig_t &config = allocator.GetConfiguration();

    ASSERT_EQ( config.name, "TestAllocator" );
    ASSERT_EQ( config.type, QC_MEMORY_ALLOCATOR_DMA );
}

// Test case for default Allocate behavior (should return UNSUPPORTED)
TEST_F( Test_MemoryIfs, SANITY_QCMemoryAllocatorIfs_DefaultAllocateReturnsUnsupported )
{
    QC::Memory::QCMemoryAllocatorConfigInit_t init_cfg = {
            "Test",
    };
    QCMemoryAllocatorIfs allocator( init_cfg, QC_MEMORY_ALLOCATOR_HEAP );

    QC::Memory::QCBufferPropBase_t request;
    QC::Memory::QCBufferDescriptorBase_t response;

    // Call the base implementation explicitly to test the default behavior
    QCStatus_e status = allocator.Allocate( request, response );
    ASSERT_EQ( status, QC_STATUS_UNSUPPORTED );
}

// Test case for default Free behavior (should return UNSUPPORTED)
TEST_F( Test_MemoryIfs, SANITY_QCMemoryAllocatorIfs_DefaultFreeReturnsUnsupported )
{
    QC::Memory::QCMemoryAllocatorConfigInit_t init_cfg = {
            "Test",
    };
    QCMemoryAllocatorIfs allocator( init_cfg, QC_MEMORY_ALLOCATOR_HEAP );

    QC::Memory::QCBufferDescriptorBase_t buff;

    // Call the base implementation explicitly to test the default behavior
    QCStatus_e status = allocator.Free( buff );
    ASSERT_EQ( status, QC_STATUS_UNSUPPORTED );
}
