// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QC/Infras/Memory/ManagerLocal.hpp"
#include "QC/Infras/Memory/Pool.hpp"
#include "gtest/gtest.h"

using namespace QC;
using namespace QC::Memory;

class Test_QCMemorymanager : public testing::Test
{
protected:
    void SetUp() override
    {
        std::array<std::reference_wrapper<QCMemoryAllocatorIfs>, QC_MEMORY_ALLOCATOR_LAST>
                allocators = { allocatorIfs1, allocatorIfs2, allocatorIfs1, allocatorIfs2,
                               allocatorIfs1, allocatorIfs2, allocatorIfs1 };

        Ifs = reinterpret_cast<QCMemoryManagerIfs *>( &mm );
        ASSERT_NE( nullptr, Ifs );

        QCMemoryManagerInit_t memorymanagerInit( 4, allocators );

        status = Ifs->Initialize( memorymanagerInit );
        ASSERT_EQ( QC_STATUS_OK, status );
    }

    void TearDown() override
    {
        status = Ifs->DeInitialize();
        ASSERT_EQ( QC_STATUS_OK, status );
    }

    QCStatus_e status;
    ManagerLocal mm;
    QCMemoryManagerIfs *Ifs;
    HeapAllocator allocatorIfs1;
    HeapAllocator allocatorIfs2;
};


TEST_F( Test_QCMemorymanager, SANITY_creation_and_destruction )
{
    {

        QCNodeID_t node1 = { "Test node1", QC_NODE_TYPE_FADAS_REMAP, 3 };
        QCMemoryHandle_t handle1;

        status = Ifs->Register( node1, handle1 );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( QC_NODE_TYPE_FADAS_REMAP, handle1.GetNodeType() );
        ASSERT_EQ( 1, handle1.GetNodeCount() );

        QCNodeID_t nodeExtra = { "Test node", QC_NODE_TYPE_LAST, 1 };
        QCMemoryHandle_t handleExtra;
        handleExtra.SetHandle( 0x100d100100000000 );

        status = Ifs->Register( nodeExtra, handleExtra );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        nodeExtra.type = QC_NODE_TYPE_QNN;
        nodeExtra.id = 5;
        status = Ifs->Register( nodeExtra, handleExtra );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        nodeExtra.id = 4;
        status = Ifs->Register( nodeExtra, handleExtra );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        status = Ifs->UnRegister( handleExtra );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        status = Ifs->UnRegister( handle1 );
        ASSERT_EQ( QC_STATUS_OK, status );
    }

    {
        ManagerLocal instance;
        std::array<std::reference_wrapper<QCMemoryAllocatorIfs>, QC_MEMORY_ALLOCATOR_LAST>
                allocators = { allocatorIfs1, allocatorIfs2, allocatorIfs1, allocatorIfs2,
                               allocatorIfs1, allocatorIfs2, allocatorIfs1 };

        QCMemoryManagerInit_t mmInit( 0, allocators );

        status = Ifs->Initialize( mmInit );
        ASSERT_EQ( QC_STATUS_BAD_STATE, status );

        status = instance.Initialize( mmInit );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );
    }

    {
        ManagerLocal instance;

        status = instance.DeInitialize();
        ASSERT_EQ( QC_STATUS_BAD_STATE, status );
    }

    {
        ManagerLocal *instance = new ManagerLocal();
        std::array<std::reference_wrapper<QCMemoryAllocatorIfs>, QC_MEMORY_ALLOCATOR_LAST>
                allocators = { allocatorIfs1, allocatorIfs2, allocatorIfs1, allocatorIfs2,
                               allocatorIfs1, allocatorIfs2, allocatorIfs1 };

        QCMemoryManagerInit_t mmInit( 4, allocators );

        status = instance->Initialize( mmInit );
        ASSERT_EQ( QC_STATUS_OK, status );
        instance->~ManagerLocal();
    }
}


TEST_F( Test_QCMemorymanager, SANITY_creation_and_registration )
{
    {

        QCNodeID_t node1 = { "Test node1", QC_NODE_TYPE_FADAS_REMAP, 0 };
        QCMemoryHandle_t handle1;

        QCNodeID_t node2 = { "Test node2", QC_NODE_TYPE_EVA_DFS, 1 };
        QCMemoryHandle_t handle2;

        QCNodeID_t node3 = { "Test node3", QC_NODE_TYPE_FADAS_REMAP, 2 };
        QCMemoryHandle_t handle3;

        QCNodeID_t node4 = { "Test node4", QC_NODE_TYPE_CUSTOM_3, 3 };
        QCMemoryHandle_t handle4;

        status = Ifs->Register( node1, handle1 );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( QC_NODE_TYPE_FADAS_REMAP, handle1.GetNodeType() );
        ASSERT_EQ( 1, handle1.GetNodeCount() );

        status = Ifs->Register( node2, handle2 );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( QC_NODE_TYPE_EVA_DFS, handle2.GetNodeType() );
        ASSERT_EQ( 1, handle2.GetNodeCount() );

        status = Ifs->Register( node3, handle3 );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( QC_NODE_TYPE_FADAS_REMAP, handle3.GetNodeType() );
        ASSERT_EQ( 2, handle3.GetNodeCount() );

        QCMemoryHandle_t handleDummy;
        status = Ifs->Register( node3, handleDummy );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        status = Ifs->Register( node4, handle4 );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( QC_NODE_TYPE_CUSTOM_3, handle4.GetNodeType() );
        ASSERT_EQ( 1, handle4.GetNodeCount() );

        QCNodeID_t nodeExtra = { "Test node4", QC_NODE_TYPE_CUSTOM_3, 3 };
        QCMemoryHandle_t handleExtra;

        status = Ifs->Register( nodeExtra, handleExtra );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        status = Ifs->UnRegister( handleExtra );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        status = Ifs->UnRegister( handle3 );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->UnRegister( handle2 );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->UnRegister( handle1 );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->UnRegister( handle4 );
        ASSERT_EQ( QC_STATUS_OK, status );
    }

    {
        QCNodeID_t node1 = { "Test node1", QC_NODE_TYPE_FADAS_REMAP, 0 };
        QCMemoryHandle_t handle1;
        status = Ifs->Register( node1, handle1 );
        ASSERT_EQ( QC_STATUS_OK, status );
    }
}

TEST_F( Test_QCMemorymanager, SANITY_basic_stand_alone_buffer_allocation )
{
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

        QCNodeID_t node1 = { "Test node1", QC_NODE_TYPE_FADAS_REMAP, 0 };
        QCMemoryHandle_t handle1;

        QCNodeID_t node2 = { "Test node2", QC_NODE_TYPE_EVA_DFS, 1 };
        QCMemoryHandle_t handle2;

        QCNodeID_t node3 = { "Test node3", QC_NODE_TYPE_FADAS_REMAP, 2 };
        QCMemoryHandle_t handle3;

        QCNodeID_t node4 = { "Test node4", QC_NODE_TYPE_CUSTOM_3, 3 };
        QCMemoryHandle_t handle4;

        status = Ifs->Register( node1, handle1 );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( QC_NODE_TYPE_FADAS_REMAP, handle1.GetNodeType() );
        ASSERT_EQ( 1, handle1.GetNodeCount() );

        status = Ifs->Register( node2, handle2 );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( QC_NODE_TYPE_EVA_DFS, handle2.GetNodeType() );
        ASSERT_EQ( 1, handle2.GetNodeCount() );

        status = Ifs->Register( node3, handle3 );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( QC_NODE_TYPE_FADAS_REMAP, handle3.GetNodeType() );
        ASSERT_EQ( 2, handle3.GetNodeCount() );

        status = Ifs->Register( node4, handle4 );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( QC_NODE_TYPE_CUSTOM_3, handle4.GetNodeType() );
        ASSERT_EQ( 1, handle4.GetNodeCount() );

        status = Ifs->AllocateBuffer( handle1, QC_MEMORY_ALLOCATOR_HEAP, request[0], response[0] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->AllocateBuffer( handle2, QC_MEMORY_ALLOCATOR_HEAP, request[1], response[1] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->AllocateBuffer( handle3, QC_MEMORY_ALLOCATOR_HEAP, request[2], response[2] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->AllocateBuffer( handle4, QC_MEMORY_ALLOCATOR_HEAP, request[3], response[3] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->FreeBuffer( handle1, response[0] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->FreeBuffer( handle2, response[1] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->FreeBuffer( handle3, response[2] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->FreeBuffer( handle4, response[3] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->UnRegister( handle3 );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->UnRegister( handle2 );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->UnRegister( handle1 );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->UnRegister( handle4 );
        ASSERT_EQ( QC_STATUS_OK, status );
    }

    {
        QCBufferPropBase_t request;
        request.size = 0;
        request.alignment = QC_MEMORY_DEFAULT_ALLIGNMENT;
        request.cache = QC_CACHEABLE;

        QCBufferDescriptorBase_t response;

        QCNodeID_t node1 = { "Test node1", QC_NODE_TYPE_FADAS_REMAP, 0 };
        QCMemoryHandle_t handle1;

        status = Ifs->UnRegister( handle1 );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        status = Ifs->Register( node1, handle1 );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( QC_NODE_TYPE_FADAS_REMAP, handle1.GetNodeType() );
        ASSERT_EQ( 1, handle1.GetNodeCount() );

        status = Ifs->AllocateBuffer( handle1, QC_MEMORY_ALLOCATOR_HEAP, request, response );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );
        request.size = 10;

        QCMemoryHandle_t handle2;
        handle2.SetHandle( 0 );

        status = Ifs->AllocateBuffer( handle2, QC_MEMORY_ALLOCATOR_HEAP, request, response );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        status = Ifs->AllocateBuffer( handle1, QC_MEMORY_ALLOCATOR_LAST, request, response );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        status = Ifs->FreeBuffer( handle2, response );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        response.allocatorType = QC_MEMORY_ALLOCATOR_LAST;
        status = Ifs->FreeBuffer( handle1, response );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        status = Ifs->UnRegister( handle1 );
        ASSERT_EQ( QC_STATUS_OK, status );
    }
}

TEST_F( Test_QCMemorymanager, SANITY_stand_alone_buffer_allocation )
{
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

        QCNodeID_t node1 = { "Test node1", QC_NODE_TYPE_FADAS_REMAP, 0 };
        QCMemoryHandle_t handle1;

        QCNodeID_t node2 = { "Test node2", QC_NODE_TYPE_EVA_DFS, 1 };
        QCMemoryHandle_t handle2;

        QCNodeID_t node3 = { "Test node3", QC_NODE_TYPE_FADAS_REMAP, 2 };
        QCMemoryHandle_t handle3;

        QCNodeID_t node4 = { "Test node4", QC_NODE_TYPE_CUSTOM_3, 3 };
        QCMemoryHandle_t handle4;

        status = Ifs->Register( node1, handle1 );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( QC_NODE_TYPE_FADAS_REMAP, handle1.GetNodeType() );
        ASSERT_EQ( 1, handle1.GetNodeCount() );

        status = Ifs->Register( node2, handle2 );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( QC_NODE_TYPE_EVA_DFS, handle2.GetNodeType() );
        ASSERT_EQ( 1, handle2.GetNodeCount() );

        status = Ifs->Register( node3, handle3 );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( QC_NODE_TYPE_FADAS_REMAP, handle3.GetNodeType() );
        ASSERT_EQ( 2, handle3.GetNodeCount() );

        status = Ifs->Register( node4, handle4 );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( QC_NODE_TYPE_CUSTOM_3, handle4.GetNodeType() );
        ASSERT_EQ( 1, handle4.GetNodeCount() );

        status = Ifs->AllocateBuffer( handle1, QC_MEMORY_ALLOCATOR_HEAP, request[0], response[0] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->AllocateBuffer( handle2, QC_MEMORY_ALLOCATOR_HEAP, request[1], response[1] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->AllocateBuffer( handle3, QC_MEMORY_ALLOCATOR_HEAP, request[2], response[2] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->AllocateBuffer( handle4, QC_MEMORY_ALLOCATOR_HEAP, request[3], response[3] );
        ASSERT_EQ( QC_STATUS_OK, status );

        // incorrect free 1
        status = Ifs->FreeBuffer( handle1, response[3] );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        status = Ifs->FreeBuffer( handle2, response[2] );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        status = Ifs->FreeBuffer( handle3, response[1] );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        status = Ifs->FreeBuffer( handle4, response[0] );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        // incorrect free 2
        QCBufferDescriptorBase_t responseTmp = response[0];
        response[0].pBuf = nullptr;
        status = Ifs->FreeBuffer( handle1, response[0] );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );
        response[0] = responseTmp;

        responseTmp = response[1];
        response[1].pBuf = (void *) 0xFFFFFFFFFFFFFFFF;
        status = Ifs->FreeBuffer( handle2, response[1] );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );
        response[1] = responseTmp;

        // correct free
        status = Ifs->FreeBuffer( handle1, response[0] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->FreeBuffer( handle2, response[1] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->FreeBuffer( handle3, response[2] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->FreeBuffer( handle4, response[3] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->UnRegister( handle3 );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->UnRegister( handle2 );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->UnRegister( handle1 );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->UnRegister( handle4 );
        ASSERT_EQ( QC_STATUS_OK, status );
    }

    {
        ManagerLocal instance;

        QCNodeID_t node1 = { "Test node1", QC_NODE_TYPE_FADAS_REMAP, 0 };
        QCMemoryHandle_t handle1;

        status = instance.Register( node1, handle1 );
        ASSERT_EQ( QC_STATUS_BAD_STATE, status );

        status = instance.UnRegister( handle1 );
        ASSERT_EQ( QC_STATUS_BAD_STATE, status );

        QCBufferPropBase_t request;
        QCBufferDescriptorBase_t response;

        status = instance.AllocateBuffer( handle1, QC_MEMORY_ALLOCATOR_HEAP, request, response );
        ASSERT_EQ( QC_STATUS_BAD_STATE, status );

        status = instance.FreeBuffer( handle1, response );
        ASSERT_EQ( QC_STATUS_BAD_STATE, status );
    }
}

TEST_F( Test_QCMemorymanager, SANITY_reclaim_resources )
{
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

        QCNodeID_t node1 = { "Test node1", QC_NODE_TYPE_FADAS_REMAP, 0 };
        QCMemoryHandle_t handle1;

        QCNodeID_t node2 = { "Test node2", QC_NODE_TYPE_EVA_DFS, 1 };
        QCMemoryHandle_t handle2;

        QCNodeID_t node3 = { "Test node3", QC_NODE_TYPE_FADAS_REMAP, 2 };
        QCMemoryHandle_t handle3;

        QCNodeID_t node4 = { "Test node4", QC_NODE_TYPE_CUSTOM_3, 3 };
        QCMemoryHandle_t handle4;

        status = Ifs->Register( node1, handle1 );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( QC_NODE_TYPE_FADAS_REMAP, handle1.GetNodeType() );
        ASSERT_EQ( 1, handle1.GetNodeCount() );

        status = Ifs->Register( node2, handle2 );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( QC_NODE_TYPE_EVA_DFS, handle2.GetNodeType() );
        ASSERT_EQ( 1, handle2.GetNodeCount() );

        status = Ifs->Register( node3, handle3 );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( QC_NODE_TYPE_FADAS_REMAP, handle3.GetNodeType() );
        ASSERT_EQ( 2, handle3.GetNodeCount() );

        status = Ifs->Register( node4, handle4 );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( QC_NODE_TYPE_CUSTOM_3, handle4.GetNodeType() );
        ASSERT_EQ( 1, handle4.GetNodeCount() );

        status = Ifs->AllocateBuffer( handle1, QC_MEMORY_ALLOCATOR_HEAP, request[0], response[0] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->AllocateBuffer( handle2, QC_MEMORY_ALLOCATOR_HEAP, request[1], response[1] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->AllocateBuffer( handle3, QC_MEMORY_ALLOCATOR_HEAP, request[2], response[2] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->AllocateBuffer( handle4, QC_MEMORY_ALLOCATOR_HEAP, request[3], response[3] );
        ASSERT_EQ( QC_STATUS_OK, status );

        QCMemoryPoolConfig_t poolCfg( allocatorIfs1 );
        poolCfg.buff.size = 1024;
        poolCfg.buff.alignment = QC_MEMORY_DEFAULT_ALLIGNMENT;
        poolCfg.buff.cache = QC_MEMORY_DEFAULT_CACHE_ATTRIBUTES;
        poolCfg.maxElements = 10;
        poolCfg.name = "test pool";

        QCMemoryPoolHandle_t poolHandle;

        status = Ifs->CreatePool( handle1, poolCfg, poolHandle );
        ASSERT_EQ( QC_STATUS_OK, status );

        QCBufferDescriptorBase_t buff1;
        status = Ifs->AllocateBufferFromPool( handle1, poolHandle, buff1 );
        ASSERT_EQ( QC_STATUS_OK, status );

        QCBufferDescriptorBase_t buff2;
        status = Ifs->AllocateBufferFromPool( handle1, poolHandle, buff2 );
        ASSERT_EQ( QC_STATUS_OK, status );

        QCMemoryPoolHandle_t poolHandleDummy;
        QCBufferDescriptorBase_t buffDummy;
        poolHandleDummy.SetHandle( 0 );
        status = Ifs->AllocateBufferFromPool( handle1, poolHandleDummy, buffDummy );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        QCMemoryHandle_t handle;
        handle.SetHandle( 0 );
        status = Ifs->ReclaimResources( handle );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        status = Ifs->ReclaimResources( handle1 );
        ASSERT_EQ( QC_STATUS_OK, status );

        // correct free
        // status = Ifs->FreeBuffer( handle1, response[0] );
        // ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->FreeBuffer( handle2, response[1] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->FreeBuffer( handle3, response[2] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->FreeBuffer( handle4, response[3] );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->UnRegister( handle3 );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->UnRegister( handle2 );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->UnRegister( handle1 );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = Ifs->UnRegister( handle4 );
        ASSERT_EQ( QC_STATUS_OK, status );
    }
}

TEST_F( Test_QCMemorymanager, SANITY_pool_create_destroy )
{
    QCMemoryPoolConfig_t poolCfg( allocatorIfs1 );
    poolCfg.buff.size = 1024;
    poolCfg.buff.alignment = QC_MEMORY_DEFAULT_ALLIGNMENT;
    poolCfg.buff.cache = QC_MEMORY_DEFAULT_CACHE_ATTRIBUTES;
    poolCfg.maxElements = 10;
    poolCfg.name = "test pool";

    QCMemoryPoolHandle_t poolHandle;

    QCNodeID_t node1 = { "Test node1", QC_NODE_TYPE_FADAS_REMAP, 0 };
    QCMemoryHandle_t handle1;

    status = Ifs->Register( node1, handle1 );
    ASSERT_EQ( QC_STATUS_OK, status );

    status = Ifs->CreatePool( handle1, poolCfg, poolHandle );
    ASSERT_EQ( QC_STATUS_OK, status );

    QCBufferDescriptorBase_t buff;
    status = Ifs->AllocateBufferFromPool( handle1, poolHandle, buff );
    ASSERT_EQ( QC_STATUS_OK, status );

    QCMemoryPoolHandle_t poolHandleDummy;
    poolHandleDummy.SetHandle( 0 );
    status = Ifs->AllocateBufferFromPool( handle1, poolHandleDummy, buff );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

    status = Ifs->PutBufferToPool( handle1, poolHandle, buff );
    ASSERT_EQ( QC_STATUS_OK, status );

    status = Ifs->PutBufferToPool( handle1, poolHandleDummy, buff );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

    status = Ifs->DestroyPool( handle1, poolHandle );
    ASSERT_EQ( QC_STATUS_OK, status );

    QCMemoryHandle_t handle1Dummy;
    handle1Dummy.SetHandle( 0 );
    status = Ifs->CreatePool( handle1Dummy, poolCfg, poolHandle );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

    status = Ifs->AllocateBufferFromPool( handle1Dummy, poolHandle, buff );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

    status = Ifs->DestroyPool( handle1Dummy, poolHandleDummy );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

    status = Ifs->DestroyPool( handle1, poolHandleDummy );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

    status = Ifs->PutBufferToPool( handle1Dummy, poolHandle, buff );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

    {
        ManagerLocal instance;

        status = instance.DestroyPool( handle1Dummy, poolHandle );
        ASSERT_EQ( QC_STATUS_BAD_STATE, status );

        status = instance.CreatePool( handle1Dummy, poolCfg, poolHandle );
        ASSERT_EQ( QC_STATUS_BAD_STATE, status );

        status = instance.AllocateBufferFromPool( handle1, poolHandle, buff );
        ASSERT_EQ( QC_STATUS_BAD_STATE, status );

        status = instance.PutBufferToPool( handle1Dummy, poolHandle, buff );
        ASSERT_EQ( QC_STATUS_BAD_STATE, status );
    }
}
