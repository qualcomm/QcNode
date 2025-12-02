// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


#ifndef QC_SAMPLE_GENIE_HPP
#define QC_SAMPLE_GENIE_HPP

#include "QC/sample/SampleIF.hpp"

#include "GenieCommon.h"
#include "GenieDialog.h"
#include "GenieEngine.h"
#include "GenieLog.h"
#include "GenieProfile.h"
#include "GenieSampler.h"

#include <map>
#include <memory>
#include <unordered_map>

namespace QC
{
namespace sample
{

/// @brief qcnode::sample::SampleGenie
///
/// SampleGenie that to demonstate how to use the QNN SDK Genie
class SampleGenie : public SampleIF
{
public:
    SampleGenie();
    ~SampleGenie();

    /// @brief Initialize the QNN SDK Genie
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the QNN SDK Genie
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Start();

    /// @brief Stop the QNN SDK Genie
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Stop();

    /// @brief deinitialize the QNN SDK Genie
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Deinit();

private:
    QCStatus_e ParseConfig( SampleConfig_t &config );
    void ThreadMain();
    GenieLog_Level_t GetGenieLogLevel();

private:
    template<class F, class T>
    void RequantEmbedding( F *from, T *to, size_t length ) const
    {
        for ( size_t i = 0; i < length; i++ )
        {
            to[i] = static_cast<T>( m_requantScale * from[i] + m_requantOffset );
        }
    }

    template<class F, class T>
    static void TokenToEmbedRequantCallback( const int32_t token, void *embedding,
                                             const uint32_t embeddingSize, const void *pPriv )
    {
        SampleGenie *pSelf = (SampleGenie *) pPriv;
        pSelf->TokenToEmbedRequantCallback<F, T>( token, embedding, embeddingSize );
    }

    template<class F, class T>
    void TokenToEmbedRequantCallback( const int32_t token, void *embedding,
                                      const uint32_t embeddingSize )
    {
        QC_TRACE_EVENT( "T2E", { QCNodeTraceArg( "token", token ) } );
        const size_t numElements = embeddingSize / sizeof( T );
        const size_t lutIndex = token * numElements;
        if ( ( lutIndex + numElements ) * sizeof( F ) <= m_embeddingLutSize )
        {
            F *embeddingSrc = static_cast<F *>( m_embeddingLut.get() ) + ( lutIndex );
            T *embeddingDst = static_cast<T *>( embedding );
            RequantEmbedding( embeddingSrc, embeddingDst, numElements );
        }
        else
        {
            QC_ERROR( "Error: T2E conversion overflow." );
        }
    }

    static void TokenToEmbedCallback( const int32_t token, void *embedding,
                                      const uint32_t embeddingSize, const void *pPriv );
    void TokenToEmbedCallback( const int32_t token, void *embedding, const uint32_t embeddingSize );

private:
    static void QueryCallback( const char *responseStr,
                               const GenieDialog_SentenceCode_t sentenceCode, const void *pPriv );

    void QueryCallback( const char *responseStr, const GenieDialog_SentenceCode_t sentenceCode );

private:
    std::string m_inputTopicName;
    std::string m_outputTopicName;

    std::string m_genieConfig;
    std::string m_embeddingTable;
    GenieDialogConfig_Handle_t m_genieConfigHandle = nullptr;
    GenieLog_Handle_t m_logHandle = nullptr;
    GenieDialog_Handle_t m_dialogHandle = nullptr;

    std::thread m_thread;
    bool m_stop;

    DataSubscriber<DataFrames_t> m_sub;
    DataPublisher<DataFrames_t> m_pub;

    float m_requantScale = 1.0;
    int32_t m_requantOffset = 0;

    size_t m_embeddingLutSize = 0;
    std::shared_ptr<void> m_embeddingLut = nullptr;

    QCTensorType_e m_lutDataType;
    float m_lutScale = 1.0;
    int32_t m_lutOffset = 0;

    QCTensorType_e m_inputDataType;
    float m_inputScale = 1.0;
    int32_t m_inputOffset = 0;

    GenieDialog_TokenToEmbeddingCallback_t m_tokenToEmbeddingCallback = nullptr;

    std::unordered_map<QCTensorType_e,
                       std::unordered_map<QCTensorType_e, GenieDialog_TokenToEmbeddingCallback_t>>
            m_t2eCallbacks;
};   // class SampleGenie

}   // namespace sample
}   // namespace QC

#endif   // QC_SAMPLE_GENIE_HPP
