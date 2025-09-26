// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause



#ifndef _QC_SAMPLE_DATA_BROKER_HPP_
#define _QC_SAMPLE_DATA_BROKER_HPP_

#include <condition_variable>
#include <cstdint>
#include <errno.h>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

#include "QC/Common/Types.hpp"

using namespace QC;

namespace QC
{
namespace sample
{

/// @brief Data Queue
///
/// light weight data queue
template<typename T>
class DataQueue final
{
public:
    DataQueue( std::string name, size_t queueDepth = 2, bool bLatest = true )
        : m_name( name ),
          m_queueDepth( queueDepth ),
          m_bLatest( bLatest )
    {}
    ~DataQueue() {}

    /// @brief publish a data into the queue
    /// @param data the data
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Publish( T &data )
    {
        QCStatus_e ret = QC_STATUS_OK;
        T oldData;
        {
            std::unique_lock<std::mutex> lock( m_mutex );
            m_queue.push( data );
            m_condVar.notify_one();
            /* with this block to activate the subscriber thread as soon as possible */
        }
        { /* using oldData to hold shared ptr thus to release the lock as soon as possible */
            std::unique_lock<std::mutex> lock( m_mutex );
            if ( m_queueDepth < m_queue.size() )
            {
                oldData = m_queue.front();
                m_queue.pop(); /* drop the oldest */
            }
        }
        return ret;
    }

    /// @brief Receive a data from the queue
    /// @param data the data returned
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Receive( T &data, uint32_t timeoutMs = 50 )
    {
        QCStatus_e ret = QC_STATUS_OK;
        std::unique_lock<std::mutex> lock( m_mutex );
        if ( m_queue.empty() )
        {
            if ( timeoutMs > 0 )
            {
                auto status = m_condVar.wait_for( lock, std::chrono::milliseconds( timeoutMs ) );
                if ( std::cv_status::no_timeout == status )
                {
                    if ( m_queue.empty() )
                    {
                        ret = QC_STATUS_FAIL;
                    }
                    else
                    {
                        pop( data );
                    }
                }
                else
                {
                    ret = QC_STATUS_TIMEOUT;
                }
            }
            else
            {
                /* queue is empty, treat it as timeout */
                ret = QC_STATUS_TIMEOUT;
            }
        }
        else
        {
            pop( data );
        }

        return ret;
    }

    /// @brief clear the data queue
    void clear()
    {
        std::unique_lock<std::mutex> lock( m_mutex );
        while ( false == m_queue.empty() )
        {
            m_queue.pop();
        }
    }

private:
    void pop( T &data )
    {
        if ( false == m_bLatest )
        {
            data = m_queue.front();
            m_queue.pop();
        }
        else
        {
            data = m_queue.back();
            while ( false == m_queue.empty() )
            {
                m_queue.pop();
            }
        }
    }

private:
    std::string m_name;
    uint32_t m_queueDepth;
    bool m_bLatest;
    std::queue<T> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_condVar;
};

template<typename T>
class DataBroker
{
public:
    DataBroker( std::string topicName ) : m_topicName( topicName ) {}
    ~DataBroker() {}

    std::shared_ptr<DataQueue<T>> CreateSubscriber( std::string subscriberName, uint32_t queueDepth,
                                                    bool bLatest )
    {
        std::shared_ptr<DataQueue<T>> sub = nullptr;
        std::unique_lock<std::mutex> lck( m_lock );

        auto it = m_dataQueueMap.find( subscriberName );
        if ( it == m_dataQueueMap.end() )
        {
            sub = std::make_shared<DataQueue<T>>( subscriberName, queueDepth, bLatest );
            m_dataQueueMap[subscriberName] = sub;
        }

        return sub;
    }

    void RemoveSubscriber( std::string subscriberName )
    {
        std::unique_lock<std::mutex> lck( m_lock );
        auto it = m_dataQueueMap.find( subscriberName );
        if ( it != m_dataQueueMap.end() )
        {
            m_dataQueueMap.erase( it );
        }
    }

    QCStatus_e Publish( T &data )
    {
        QCStatus_e ret = QC_STATUS_OK;

        std::unique_lock<std::mutex> lck( m_lock );
        for ( auto it : m_dataQueueMap )
        {
            ret = it.second->Publish( data );
            if ( QC_STATUS_OK != ret )
            {
                break;
            }
        }

        return ret;
    }

private:
    std::string m_topicName;
    std::mutex m_lock;
    std::map<std::string, std::shared_ptr<DataQueue<T>>> m_dataQueueMap;

public:
    static std::shared_ptr<DataBroker<T>> Add( std::string topicName );

private:
    static std::mutex s_MapLock;
    static std::map<std::string, std::shared_ptr<DataBroker<T>>> s_brokerMap;
};

template<typename T>
class DataPublisher
{
public:
    DataPublisher() {}

    ~DataPublisher() {}

    QCStatus_e Init( std::string name, std::string topicName )
    {
        QCStatus_e ret = QC_STATUS_OK;

        m_name = name;
        m_topicName = topicName;
        m_broker = DataBroker<T>::Add( topicName );
        if ( nullptr == m_broker )
        {
            ret = QC_STATUS_NOMEM;
        }

        return ret;
    }

    QCStatus_e Publish( T &data )
    {
        QCStatus_e ret = QC_STATUS_OK;
        if ( nullptr != m_broker )
        {
            m_broker->Publish( data );
        }
        else
        {
            ret = QC_STATUS_BAD_STATE;
        }

        return ret;
    }

private:
    std::string m_name;
    std::string m_topicName;
    std::shared_ptr<DataBroker<T>> m_broker = nullptr;
};

template<typename T>
class DataSubscriber
{
public:
    DataSubscriber() {}
    ~DataSubscriber()
    {
        if ( m_sub != nullptr )
        {
            m_broker->RemoveSubscriber( m_name );
        }
    }

    QCStatus_e Init( std::string name, std::string topicName, uint32_t queueDepth = 2,
                     bool bLatest = true )
    {
        QCStatus_e ret = QC_STATUS_OK;

        m_name = name;
        m_topicName = topicName;
        m_broker = DataBroker<T>::Add( topicName );
        if ( nullptr != m_broker )
        {
            m_sub = m_broker->CreateSubscriber( m_name, queueDepth, bLatest );
            if ( nullptr == m_sub )
            {
                ret = QC_STATUS_ALREADY;
            }
        }
        else
        {
            ret = QC_STATUS_NOMEM;
        }

        return ret;
    }

    QCStatus_e Receive( T &out, uint32_t timeoutMs = 1000 )
    {
        QCStatus_e ret = QC_STATUS_OK;

        if ( nullptr != m_sub )
        {
            ret = m_sub->Receive( out, timeoutMs );
        }
        else
        {
            ret = QC_STATUS_BAD_STATE;
        }

        return ret;
    }

private:
    std::string m_name;
    std::string m_topicName;
    std::shared_ptr<DataBroker<T>> m_broker = nullptr;
    std::shared_ptr<DataQueue<T>> m_sub = nullptr;
};

template<typename T>
std::mutex DataBroker<T>::s_MapLock;
template<typename T>
std::map<std::string, std::shared_ptr<DataBroker<T>>> DataBroker<T>::s_brokerMap;

template<typename T>
std::shared_ptr<DataBroker<T>> DataBroker<T>::Add( std::string topicName )
{
    std::shared_ptr<DataBroker<T>> ptr = nullptr;

    std::unique_lock<std::mutex> lck( s_MapLock );
    auto it = s_brokerMap.find( topicName );
    if ( it == s_brokerMap.end() )
    {
        ptr = std::make_shared<DataBroker<T>>( topicName );
        s_brokerMap[topicName] = ptr;
    }
    else
    {
        ptr = it->second;
    }

    return ptr;
}
}   // namespace sample
}   // namespace QC

#endif   // _QC_SAMPLE_DATA_BROKER_HPP_
