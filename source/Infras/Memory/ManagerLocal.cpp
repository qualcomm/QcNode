// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QC/Infras/Memory/ManagerLocal.hpp"
#include "QC/Infras/Log/Logger.hpp"
#include "QC/Infras/Memory/Pool.hpp"
#include <cstdlib>
#include <random>
#include <unistd.h>


namespace QC
{
namespace Memory
{

ManagerLocal::ManagerLocal()
{
    m_state = QC_OBJECT_STATE_INITIAL;
    (void) QC_LOGGER_INIT( "ManagerLocal", LOGGER_LEVEL_ERROR );
}

QCStatus_e ManagerLocal::Initialize( const QCMemoryManagerInit_t &init )
{
    QCStatus_e status = QC_STATUS_OK;
    m_config = new QCMemoryManagerInit_t( init.numOfNodes, init.allocators );
    if ( QC_OBJECT_STATE_INITIAL != GetState() )
    {
        QC_ERROR( "QC_OBJECT_STATE_INITIAL != m_state" );
        status = QC_STATUS_BAD_STATE;
    }
    else if ( 0 == init.numOfNodes )
    {
        QC_ERROR( "0 ==  init.numOfNodes" );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( nullptr == m_config )
    {
        QC_ERROR( "nullptr == m_config" );
        status = QC_STATUS_NULL_PTR;
    }
    else
    {
        m_state = QC_OBJECT_STATE_INITIALIZING;
        m_pools.reserve( m_config->numOfNodes );
        m_allocations.reserve( m_config->numOfNodes );
        std::map<QCMemoryPoolHandle_t, std::reference_wrapper<QCMemoryPoolIfs>> poolMap;
        std::map<QCBufferDescriptorBase_t, QCMemoryAllocator_e> allocMap;

        for ( auto i = 0; i < m_config->numOfNodes; i++ )
        {
            m_pools.push_back( poolMap );
            std::map<QCMemoryPoolHandle_t, std::reference_wrapper<QCMemoryPoolIfs>> &refPoolMap =
                    m_pools.back();

            m_allocations.push_back( allocMap );
            std::map<QCBufferDescriptorBase_t, QCMemoryAllocator_e> &refAllocMap =
                    m_allocations.back();
        }
    }

    if ( QC_STATUS_OK == status )
    {
        m_state = QC_OBJECT_STATE_READY;
    }
    else if ( QC_STATUS_NULL_PTR == status )
    {
        m_state = QC_OBJECT_STATE_ERROR;
        if ( nullptr != m_config )
        {
            delete m_config;
            m_config = nullptr;
        }
    }
    else
    {
    }

    return status;
}

ManagerLocal::~ManagerLocal()
{
    if ( GetState() == QC_OBJECT_STATE_READY )
    {
        QCStatus_e status = DeInitialize();
        if ( QC_STATUS_OK != status )
        {
            QC_ERROR( "DeInitialize failed" );
        }
    }
    else
    {
        QC_INFO( "ManagerLocal::DeInitialize() called successfully" );
    }

    (void) QC_LOGGER_DEINIT();
}

QCStatus_e ManagerLocal::DeInitialize()
{
    // Free all allocated resources if not freed before
    QC_INFO();
    QCStatus_e status = QC_STATUS_OK;
    if ( ( GetState() == QC_OBJECT_STATE_INITIAL ) ||
         ( GetState() == QC_OBJECT_STATE_INITIALIZING ) )
    {
        status = QC_STATUS_BAD_STATE;
    }
    else
    {
        m_state = QC_OBJECT_STATE_DEINITIALIZING;
        for ( auto i = m_handleToNodeIdInVector.begin(); i != m_handleToNodeIdInVector.end(); i++ )
        {
            QCStatus_e statusLocal = ReclaimResources( i->first );
            QC_INFO( "ReclaimResources for QCMemoryHandle_t %d returned %d", i->first, status );
            if ( QC_STATUS_OK != statusLocal )
            {
                status = statusLocal;
                QC_ERROR( "ReclaimResources for QCMemoryHandle_t %d returned %d", i->first,
                          status );
            }
        }

        for ( auto i = 0; i < m_config->numOfNodes; i++ )
        {
            QC_DEBUG( "DB memory free itteration %d ", i );
            std::map<QCMemoryPoolHandle_t, std::reference_wrapper<QCMemoryPoolIfs>> &refPoolMap =
                    m_pools.back();
            QC_DEBUG( "refPoolMap at %p ", &refPoolMap );
            m_pools.pop_back();

            std::map<QCBufferDescriptorBase_t, QCMemoryAllocator_e> &refAllocMap =
                    m_allocations.back();
            QC_DEBUG( "refAllocMap at %p ", &refAllocMap );
            m_allocations.pop_back();
        }

        delete m_config;
        m_config = nullptr;

        if ( QC_STATUS_OK == status )
        {
            m_state = QC_OBJECT_STATE_INITIAL;
        }
        else
        {
            m_state = QC_OBJECT_STATE_ERROR;
        }
    }

    return status;
}

// Un/Registration
QCStatus_e ManagerLocal::Register( const QCNodeID_t &node, QCMemoryHandle_t &handle )
{
    QC_INFO( "node name %s node type %d node id %d", node.name.c_str(), node.type, node.id );

    QCStatus_e status = QC_STATUS_OK;
    // generate random number to used as memory handlers
    //  create random device
    std::random_device rd;
    // create Mersenne Twister engine for 32-bit integers
    // with random device as seed value
    std::mt19937 randomNumbersGenerator( rd() );
    // define result range uint64 and distribution
    std::uniform_int_distribution<uint32_t> distribution( 0, UINT32_MAX );

    // Set Node enum into memory hadler
    handle.SetNodeType( node.type );
    QC_DEBUG( "Memory Handle value after setting node type 0x%016" PRIx64 " ", handle.GetHandle() );

    // scoped lock
    std::lock_guard<std::mutex> lk( m_handle2NodeIdLock );

    // Set node instance number in hnadler
    handle.SetNodeCount( GetRegisteredNodesFromTheSameType( node ) + 1 );
    QC_DEBUG( "Memory Handle value after setting node count 0x%016" PRIx64 " ",
              handle.GetHandle() );

    // generate random number
    handle.SetRandomNumber( distribution( randomNumbersGenerator ) );
    QC_DEBUG( "Memory Handle value after setting random value 0x%016" PRIx64 " ",
              handle.GetHandle() );

    // check if handle allready exists
    std::map<QCMemoryHandle_t, uint32_t>::iterator it;
    if ( GetState() != QC_OBJECT_STATE_READY )
    {
        QC_ERROR( "GetState () != QC_OBJECT_STATE_READY, state =%d", GetState() );
        status = QC_STATUS_BAD_STATE;
    }
    else if ( ( node.type >= QC_NODE_TYPE_LAST ) || ( node.type == QC_NODE_TYPE_RESERVED ) )
    {
        QC_ERROR( "(node.type >= QC_NODE_TYPE_LAST(%d) ) || ( node.type == "
                  "QC_NODE_TYPE_RESERVED(%d) )",
                  QC_NODE_TYPE_LAST, QC_NODE_TYPE_RESERVED );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( m_handleToNodeIdInVector.size() == m_config->numOfNodes )
    {
        QC_ERROR( "m_handleToNodeIdInVector.size() == m_config->numOfNodes" );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( true == IsMemoryHandleRegistered( handle, it ) )
    {
        QC_ERROR( "COULD NOT GENERATE UNIQUE HANDLE" );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( m_config->numOfNodes <= node.id )
    {
        QC_ERROR( "node.id %d is bigger or equal to m_config->numOfNodes %d", node.id,
                  m_config->numOfNodes );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( IsNodeIdUnique( node ) == false )
    {
        QC_ERROR( "node.id %d is allready registers in m_handleToNodeIdInVector", node.id );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        QC_DEBUG( "GENERATED UNIQUE HANDLE" );

        // insert to data base
        m_handleToNodeIdInVector.insert( { handle, node.id } );

        // validate insertion
        it = m_handleToNodeIdInVector.find( handle );
        if ( it == m_handleToNodeIdInVector.end() )
        {
            QC_ERROR( "insertion to m_handleToNodeIdInVector failed" );
            status = QC_STATUS_FAIL;
        }
    }

    if ( QC_STATUS_FAIL == status )
    {
        QC_ERROR( "m_state = QC_OBJECT_STATE_ERROR" );
        m_state = QC_OBJECT_STATE_ERROR;
    }

    return status;
}

QCStatus_e ManagerLocal::UnRegister( const QCMemoryHandle_t &memHandle )
{
    QCStatus_e status = QC_STATUS_OK;
    std::map<QCMemoryHandle_t, uint32_t>::iterator handleIt;

    // scoped lock
    std::lock_guard<std::mutex> lk( m_handle2NodeIdLock );

    if ( GetState() != QC_OBJECT_STATE_READY )
    {
        QC_ERROR( "GetState () != QC_OBJECT_STATE_READY, state =%d", GetState() );
        status = QC_STATUS_BAD_STATE;
    }
    else if ( false == IsMemoryHandleRegistered( memHandle, handleIt ) )
    {
        status = QC_STATUS_BAD_ARGUMENTS;
        QC_ERROR( "memHndle %" PRIx64 " is not in data base", memHandle.GetHandle() );
    }
    else
    {
        QC_DEBUG( "memHndle %" PRIx64 " is in data base", memHandle.GetHandle() );
        // clean all allocations related to this handle
        status = ReclaimResources( memHandle );
        if ( status != QC_STATUS_OK )
        {
            QC_ERROR( "free resources failed" );
        }
        else
        {
            m_handleToNodeIdInVector.erase( memHandle );
            // validate erase success
            if ( true == IsMemoryHandleRegistered( memHandle, handleIt ) )
            {
                status = QC_STATUS_FAIL;
                QC_ERROR( "cant remove memhandle %" PRIx64 " from data base",
                          memHandle.GetHandle() );
            }
        }
    }

    if ( QC_STATUS_FAIL == status )
    {
        QC_ERROR( "m_state = QC_OBJECT_STATE_ERROR" );
        m_state = QC_OBJECT_STATE_ERROR;
    }

    return status;
}

// Pools methods
// Pools creation and destruction
QCStatus_e ManagerLocal::CreatePool( const QCMemoryHandle_t &handle,
                                     const QCMemoryPoolConfig &poolCfg,
                                     QCMemoryPoolHandle_t &poolHandle )
{
    QCStatus_e status = QC_STATUS_OK;
    uint64_t count = 0;
    // check Memory Handle correctness
    std::map<QCMemoryHandle_t, uint32_t>::iterator itNodeMap;
    // scoped lock
    std::lock_guard<std::mutex> lk( m_poolsLock );
    if ( GetState() != QC_OBJECT_STATE_READY )
    {
        QC_ERROR( "GetState () != QC_OBJECT_STATE_READY, state =%d", GetState() );
        status = QC_STATUS_BAD_STATE;
    }
    else if ( false == IsMemoryHandleRegistered( handle, itNodeMap ) )
    {
        status = QC_STATUS_BAD_ARGUMENTS;
        QC_ERROR( "not registers handle  %" PRIx64 "", handle.GetHandle() );
    }
    else if ( UINT64_MAX == GetRegisteredPoolsCountForANode( handle ) )
    {
        status = QC_STATUS_OUT_OF_BOUND;
        count = GetRegisteredPoolsCountForANode( handle );
        QC_ERROR( "Node m_pools count too hight %" PRIu64 " ", count );
    }
    else
    {
        // generate random number to used as memory handlers
        //  create random device
        std::random_device rd;
        // create Mersenne Twister engine for 64-bit integers
        // with random device as seed value
        std::mt19937 randomNumbersGenerator( rd() );
        // define result range uint64 and distribution
        std::uniform_int_distribution<uint64_t> distribution( 0, UINT32_MAX );
        // check uniqness

        // Set Node enum into memory hadler
        poolHandle.SetNodeType( handle.GetNodeType() );
        // generate and set random number
        poolHandle.SetRandomNumber( distribution( randomNumbersGenerator ) );

        // set new pool count
        poolHandle.SetPoolCount( static_cast<uint16_t>( count + 1 ) );
        QC_DEBUG( "Pool Handle created %" PRIx64 "", poolHandle.GetHandle() );

        auto it = m_pools[itNodeMap->second].find( poolHandle );
        if ( it != m_pools[itNodeMap->second].end() )
        {
            QC_ERROR( "COULD NOT GENERATE UNIQUE POOL HANDLE" );
            status = QC_STATUS_FAIL;
        }
        else
        {
            QC_DEBUG( "GENERATED UNIQUE POOL HANDLE" );
            // Create pool
            QCMemoryPoolIfs *pool = new Pool( poolCfg );
            // Initialize pool
            if ( nullptr != pool )
            {
                status = pool->Init();
                if ( status != QC_STATUS_OK )
                {
                    pool->~QCMemoryPoolIfs();
                    QC_ERROR( "Pool Creation & Initialization failed - destroying" );
                }
                else
                {
                    // insert unique pool handle & new pool into internal data base
                    m_pools[itNodeMap->second].insert(
                            { poolHandle, std::reference_wrapper<QCMemoryPoolIfs>( *pool ) } );
                    // verification of insetions
                    if ( m_pools[itNodeMap->second].find( poolHandle ) !=
                         m_pools[itNodeMap->second].end() )
                    {
                        QC_DEBUG( "SUCCESFULL INSERTION OF handle %" PRIx64 " ",
                                  poolHandle.GetHandle() );
                    }
                    else
                    {
                        QC_ERROR( "INSERTION OF handle %" PRIx64 " FAILED",
                                  poolHandle.GetHandle() );
                        status = QC_STATUS_FAIL;
                        pool->~QCMemoryPoolIfs();
                    }
                }
            }
        }
    }

    if ( QC_STATUS_FAIL == status )
    {
        QC_ERROR( "m_state = QC_OBJECT_STATE_ERROR" );
        m_state = QC_OBJECT_STATE_ERROR;
    }

    return status;
}

QCStatus_e ManagerLocal::DestroyPool( const QCMemoryHandle_t &handle,
                                      const QCMemoryPoolHandle_t &poolHandle )
{
    QCStatus_e status = QC_STATUS_OK;
    // check Memory Handle correctness
    std::map<QCMemoryHandle_t, uint32_t>::iterator itNodeMap;
    // scoped lock
    std::lock_guard<std::mutex> lk( m_poolsLock );

    if ( GetState() != QC_OBJECT_STATE_READY )
    {
        QC_ERROR( "GetState () != QC_OBJECT_STATE_READY, state =%d", GetState() );
        status = QC_STATUS_BAD_STATE;
    }
    else if ( false == IsMemoryHandleRegistered( handle, itNodeMap ) )
    {
        status = QC_STATUS_BAD_ARGUMENTS;
        QC_ERROR( "not registers handle  %" PRIx64 "", handle.GetHandle() );
    }
    else if ( m_pools[itNodeMap->second].find( poolHandle ) == m_pools[itNodeMap->second].end() )
    {
        status = QC_STATUS_BAD_ARGUMENTS;
        QC_ERROR( "not registers pool handle %" PRIx64 "", poolHandle.GetHandle() );
    }
    else
    {
        auto it = m_pools[itNodeMap->second].find( poolHandle );
        QCMemoryPoolIfs &pool = it->second.get();
        pool.~QCMemoryPoolIfs();
        m_pools[itNodeMap->second].erase( poolHandle );
        // verify erasure
        if ( m_pools[itNodeMap->second].find( poolHandle ) != m_pools[itNodeMap->second].end() )
        {
            QC_ERROR( "FAILED POOL DESTRUCTION WITH HANDLE %" PRIx64 " om NODE WITH HANDLE %" PRIx64
                      "",
                      poolHandle.GetHandle(), handle.GetHandle() );
            status = QC_STATUS_FAIL;
        }
    }

    if ( QC_STATUS_FAIL == status )
    {
        QC_ERROR( "m_state = QC_OBJECT_STATE_ERROR" );
        m_state = QC_OBJECT_STATE_ERROR;
    }

    return status;
}

QCStatus_e ManagerLocal::AllocateBufferFromPool( const QCMemoryHandle_t &memoryHandle,
                                                 const QCMemoryPoolHandle_t &poolHandle,
                                                 QCBufferDescriptorBase_t &buff )
{
    QCStatus_e status = QC_STATUS_OK;
    // check Memory Handle correctness
    std::map<QCMemoryHandle_t, uint32_t>::iterator itNodeMap;
    // scoped lock
    std::lock_guard<std::mutex> lk( m_poolsLock );

    if ( GetState() != QC_OBJECT_STATE_READY )
    {
        QC_ERROR( "GetState () != QC_OBJECT_STATE_READY, state =%d", GetState() );
        status = QC_STATUS_BAD_STATE;
    }
    else if ( false == IsMemoryHandleRegistered( memoryHandle, itNodeMap ) )
    {
        status = QC_STATUS_BAD_ARGUMENTS;
        QC_ERROR( "not registers handle  %" PRIx64 "", memoryHandle.GetHandle() );
    }
    else if ( m_pools[itNodeMap->second].find( poolHandle ) == m_pools[itNodeMap->second].end() )
    {
        status = QC_STATUS_BAD_ARGUMENTS;
        QC_ERROR( "not registers pool handle %" PRIx64 "", poolHandle.GetHandle() );
    }
    else
    {
        auto it = m_pools[itNodeMap->second].find( poolHandle );
        QCMemoryPoolIfs &pool = it->second.get();
        status = pool.GetElement( buff );
    }

    if ( QC_STATUS_FAIL == status )
    {
        QC_ERROR( "m_state = QC_OBJECT_STATE_ERROR" );
        m_state = QC_OBJECT_STATE_ERROR;
    }

    return status;
}

QCStatus_e ManagerLocal::PutBufferToPool( const QCMemoryHandle_t &memoryHandle,
                                          const QCMemoryPoolHandle_t &poolHandle,
                                          const QCBufferDescriptorBase_t &buff )
{
    QCStatus_e status = QC_STATUS_OK;
    // check Memory Handle correctness
    std::map<QCMemoryHandle_t, uint32_t>::iterator itNodeMap;
    // scoped lock
    std::lock_guard<std::mutex> lk( m_poolsLock );

    if ( GetState() != QC_OBJECT_STATE_READY )
    {
        QC_ERROR( "GetState () != QC_OBJECT_STATE_READY, state =%d", GetState() );
        status = QC_STATUS_BAD_STATE;
    }
    else if ( false == IsMemoryHandleRegistered( memoryHandle, itNodeMap ) )
    {
        status = QC_STATUS_BAD_ARGUMENTS;
        QC_ERROR( "not registers handle  %" PRIx64 "", memoryHandle.GetHandle() );
    }
    else if ( m_pools[itNodeMap->second].find( poolHandle ) == m_pools[itNodeMap->second].end() )
    {
        status = QC_STATUS_BAD_ARGUMENTS;
        QC_ERROR( "not registers pool handle %" PRIx64 "", poolHandle.GetHandle() );
    }
    else
    {
        auto it = m_pools[itNodeMap->second].find( poolHandle );
        QCMemoryPoolIfs &pool = it->second.get();
        status = pool.PutElement( buff );
    }

    if ( QC_STATUS_FAIL == status )
    {
        QC_ERROR( "m_state = QC_OBJECT_STATE_ERROR" );
        m_state = QC_OBJECT_STATE_ERROR;
    }

    return status;
}

// for constant allocations/deallocations for nodes
// should be used only in initialization / deinitialization
QCStatus_e ManagerLocal::AllocateBuffer( const QCMemoryHandle_t handle,
                                         const QCMemoryAllocator_e allocator,
                                         const QCBufferPropBase_t &request,
                                         QCBufferDescriptorBase_t &buff )
{
    QCStatus_e status = QC_STATUS_OK;
    std::map<QCMemoryHandle_t, uint32_t>::iterator handleIt;
    // scoped lock
    std::lock_guard<std::mutex> lk( m_allocationsLock );

    if ( GetState() != QC_OBJECT_STATE_READY )
    {
        QC_ERROR( "GetState () != QC_OBJECT_STATE_READY, state =%d", GetState() );
        status = QC_STATUS_BAD_STATE;
    }
    else if ( 0 == request.size )
    {
        QC_ERROR( "BAD INPUT size=%d", request.size );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( false == IsMemoryHandleRegistered( handle, handleIt ) )
    {
        QC_ERROR( "handle (%" PRIx64 ") elegal", handle.GetHandle() );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( handleIt->second >= m_config->numOfNodes )
    {
        QC_ERROR( "handleIt->second(%d) >= m_config->numOfNodes(%d)", handleIt->second,
                  m_config->numOfNodes );
        QC_ERROR( "DB OUT OF SYNC" );
        status = QC_STATUS_FAIL;
    }
    else if ( false == IsAllocatorLeagal( allocator ) )
    {
        QC_ERROR( "Wrong allocator" );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        QC_DEBUG( "Allocating buffer using allocator %d for handle 0x%016" PRIx64 "", allocator,
                  handle );
        QCMemoryAllocatorIfs &allocatorRef = m_config->allocators[allocator];
        QC_DEBUG( " allocator %s type %d", allocatorRef.GetConfiguration().name.c_str(),
                  allocatorRef.GetConfiguration().type );

        status = allocatorRef.Allocate( request, buff );
        if ( QC_STATUS_OK != status ) QC_ERROR( "BAD ALLOC RESULT" );
        else
        {
            std::map<QCBufferDescriptorBase_t, QCMemoryAllocator_e> &bufferMap =
                    m_allocations[handleIt->second];
            bufferMap.insert( { buff, allocator } );

            // validate insertion
            auto bufferTt = m_allocations[handleIt->second].find( buff );
            if ( bufferTt == m_allocations[handleIt->second].end() )
            {
                QC_ERROR( "COULD NOT FIND INSERTED buff=%p allocator=%s in map", buff.pBuf,
                          allocatorRef.GetConfiguration().name.c_str() );
                QC_ERROR( "allocator type =%d ", allocator );
                status = QC_STATUS_FAIL;
                QCStatus_e recoveryStatus = allocatorRef.Free( buff );
                if ( QC_STATUS_OK != recoveryStatus )
                {
                    QC_ERROR( "COULD NOT FREE ALLOCATED buff=%p allocator=%s in map status = %d",
                              buff.pBuf, allocator, recoveryStatus );
                }
            }
            else
            {
                QC_DEBUG( " %p inserted succesfully", buff.pBuf );
            }
        }
    }

    if ( QC_STATUS_FAIL == status )
    {
        QC_ERROR( "m_state = QC_OBJECT_STATE_ERROR" );
        m_state = QC_OBJECT_STATE_ERROR;
    }

    return status;
}

QCStatus_e ManagerLocal::FreeBuffer( const QCMemoryHandle_t handle,
                                     const QCBufferDescriptorBase_t &buff )
{
    QCStatus_e status = QC_STATUS_OK;
    std::map<QCMemoryHandle_t, uint32_t>::iterator handleIt;

    // scoped lock
    std::lock_guard<std::mutex> lk( m_allocationsLock );
    if ( GetState() != QC_OBJECT_STATE_READY )
    {
        QC_ERROR( "GetState () != QC_OBJECT_STATE_READY, state =%d", GetState() );
        status = QC_STATUS_BAD_STATE;
    }
    // validate handle
    else if ( false == IsMemoryHandleRegistered( handle, handleIt ) )
    {
        QC_ERROR( "handle (%" PRIx64 ") elegal", handle.GetHandle() );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( false == IsAllocatorLeagal( buff.allocatorType ) )
    {
        QC_ERROR( "Wrong allocator" );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        // Validate existance of the buffer pointer in the data base
        std::map<QCBufferDescriptorBase_t, QCMemoryAllocator_e> &bufferMap =
                m_allocations[handleIt->second];
        auto bufferIt = bufferMap.find( buff );
        if ( bufferIt != bufferMap.end() )
        {
            QCMemoryAllocatorIfs &allocatorRef = m_config->allocators[buff.allocatorType];
            QC_DEBUG( "allocatorRef name %s", allocatorRef.GetConfiguration().name.c_str() );
            status = allocatorRef.Free( buff );
            if ( QC_STATUS_OK == status )
            {
                // remove from data base
                bufferMap.erase( buff );
                // validate removal
                bufferIt = bufferMap.find( buff );
                if ( bufferIt != bufferMap.end() )
                {
                    QC_ERROR( "FOUND removad buff=%p alocator=%d in map", buff.pBuf,
                              buff.allocatorType );
                    status = QC_STATUS_FAIL;
                }
            }
            else
            {
                QC_ERROR( "Memory release failed %d", status );
            }
        }
        else
        {
            QC_ERROR( "Descriptor not in DB" );
            status = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_FAIL == status )
    {
        QC_ERROR( "m_state = QC_OBJECT_STATE_ERROR" );
        m_state = QC_OBJECT_STATE_ERROR;
    }

    return status;
}

//"Garbage collector"
QCStatus_e ManagerLocal::ReclaimResources( const QCMemoryHandle_t &handle )
{
    QCStatus_e status = QC_STATUS_OK;
    QCObjectState_e state = GetState();

    if ( state != QC_OBJECT_STATE_READY )
    {
        QC_DEBUG( "state != QC_OBJECT_STATE_READY" );
        // changing temporally object state to allow call to
        // memory release methods which are blocked by wrong state
        m_state = QC_OBJECT_STATE_READY;
    }

    std::map<QCMemoryHandle_t, uint32_t>::iterator handleIt;
    // validate handle
    if ( false == IsMemoryHandleRegistered( handle, handleIt ) )
    {
        QC_ERROR( "handle (%" PRIx64 ") elegal", handle.GetHandle() );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        QC_DEBUG( "handle (%" PRIx64 ") ", handle.GetHandle() );
        QC_DEBUG( "handle allocations in vector is %d ", handleIt->second );
        // reclaim stand alone allocations
        // ###############################
        std::map<QCBufferDescriptorBase_t, QCMemoryAllocator_e> &bufferMap =
                m_allocations[handleIt->second];

        if ( true == bufferMap.empty() )
        {
            QC_DEBUG( "No stand alone allocations " );
        }
        else
        {
            // itterate over map and release allocations
            uint32_t freedREsourcesCount = 0;
            for ( ; false == bufferMap.empty(); )
            {
                auto it = bufferMap.begin();
                if ( it != bufferMap.end() )
                {
                    QCBufferDescriptorBase_t buffDescriptor = it->first;
                    QC_DEBUG( "buffDescriptor.pBuf %p buffDescriptor.allocatorType %d ",
                              buffDescriptor.pBuf, buffDescriptor.allocatorType );
                    QCStatus_e localStatus = FreeBuffer( handle, buffDescriptor );
                    if ( QC_STATUS_OK != localStatus )
                    {
                        QC_ERROR( "allocatorPtr->Free(QCBufferDescriptorBase_t) returned error %d",
                                  localStatus );
                        status = localStatus;
                        break;
                    }
                    else
                    {
                        freedREsourcesCount++;
                        QC_DEBUG( "freedREsourcesCount %d", freedREsourcesCount );
                    }
                }
            }
            QC_DEBUG( "Total freedREsourcesCount %d", freedREsourcesCount );
            // verify map is empty
            if ( true != bufferMap.empty() )
            {
                status = QC_STATUS_FAIL;
                QC_ERROR( "Allocation buffer map of handle %" PRIx64 " is not empty",
                          handle.GetHandle() );
            }
            else
            {
                QC_DEBUG( "All stand alone allocations at handle (%" PRIx64 ") reclaimed",
                          handle.GetHandle() );
            }
        }

        // reclaim pool allocations & destroy pools
        // ########################################
        std::map<QCMemoryPoolHandle_t, std::reference_wrapper<QCMemoryPoolIfs>> &poolMap =
                m_pools[handleIt->second];
        // itterate over map and release allocations
        if ( true == poolMap.empty() )
        {
            QC_DEBUG( "No Pool allocations " );
        }
        else
        {
            uint32_t freedPoolsCount = 0;
            for ( ; false == poolMap.empty(); )
            {
                auto it = poolMap.begin();
                if ( it != poolMap.end() )
                {
                    QCStatus_e localStatus = DestroyPool( handle, it->first );
                    if ( QC_STATUS_OK != localStatus )
                    {
                        status = localStatus;
                        QC_ERROR( "Destroy Pool failed with memory handle %" PRIx64
                                  " and pool handle %" PRIx64 "",
                                  handle.GetHandle(), it->first.GetHandle() );
                    }
                    else
                    {
                        freedPoolsCount++;
                        QC_DEBUG( "freedPoolsCount %d", freedPoolsCount );
                    }
                }
            }
            QC_DEBUG( "Total freedPoolsCount %d", freedPoolsCount );

            // verify map is empty
            if ( true != poolMap.empty() )
            {
                status = QC_STATUS_FAIL;
                QC_ERROR( "Allocation buffer map of handle %" PRIx64 " is not empty",
                          handle.GetHandle() );
            }
        }
    }

    if ( status == QC_STATUS_FAIL )
    {
        QC_ERROR( "m_state = QC_OBJECT_STATE_ERROR" );
        m_state = QC_OBJECT_STATE_ERROR;
    }
    else
    {
        // return original state value from
        // start of the method
        m_state = state;
    }

    return status;
}

inline bool
ManagerLocal::IsMemoryHandleRegistered( const QCMemoryHandle_t &handle,
                                        std::map<QCMemoryHandle_t, uint32_t>::iterator &it )
{
    bool result = true;
    it = m_handleToNodeIdInVector.find( handle );
    if ( it == m_handleToNodeIdInVector.end() ) result = false;

    return result;
}

inline bool ManagerLocal::IsNodeIdUnique( const QCNodeID_t &node )
{
    bool result = true;
    // iterate over registered node memory handles
    for ( auto i = m_handleToNodeIdInVector.begin(); i != m_handleToNodeIdInVector.end(); i++ )
    {
        // check if node Enum is equel to the new one
        if ( node.id == i->second )
        {
            result = false;
            break;
        }
    }

    return result;
}

inline uint64_t ManagerLocal::GetRegisteredNodesFromTheSameType( const QCNodeID_t &node )
{
    uint64_t result = 0;

    // iterate over registered node memory handles
    for ( auto i = m_handleToNodeIdInVector.begin(); i != m_handleToNodeIdInVector.end(); i++ )
    {
        QCMemoryHandle_t handle = i->first;
        // check if node Enum is equel to the new one
        if ( node.type == handle.GetNodeType() )
        {
            // check if count of existing node enum is larger than the latest found so far
            if ( result < handle.GetNodeCount() )
            {
                // update the count with the largest one
                result = handle.GetNodeCount();
            }
        }
    }

    return result;
}


inline uint64_t ManagerLocal::GetRegisteredPoolsCountForANode( const QCMemoryHandle_t &handle )
{
    uint64_t result = 0;
    std::map<QCMemoryHandle_t, uint32_t>::iterator it;
    if ( true == IsMemoryHandleRegistered( handle, it ) )
    {
        result = m_pools[it->second].size();
        QC_DEBUG( "size of map for %" PRIu64 "", result );
    }
    else
    {
        QC_ERROR( "BAD memory handle %" PRIx64 "", handle.GetHandle() );
        result = UINT64_MAX;
    }

    return result;
}
inline bool ManagerLocal::IsAllocatorLeagal( const QCMemoryAllocator_e allocator )
{
    bool isLeagal = true;
    if ( QC_MEMORY_ALLOCATOR_LAST <= allocator )
    {
        isLeagal = false;
        QC_ERROR( "QC_MEMORY_ALLOCATOR_LAST <= allocator, allocator=%d", allocator );
    }

    return isLeagal;
}

}   // namespace Memory
}   // namespace QC
