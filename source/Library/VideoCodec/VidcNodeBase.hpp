// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef QC_VIDEO_CODEC_NODE_BASE_HPP
#define QC_VIDEO_CODEC_NODE_BASE_HPP

#include "QC/Node/NodeBase.hpp"
#include "QC/Node/Ifs/QCNodeIfs.hpp"
#include "VidcDrvClient.hpp"
#include <mutex>
#include <queue>
#include <sys/uio.h>
#include <unordered_map>

namespace QC::Node
{

/**
 * @brief Video Encoder Node Configuration Data Structure
 * @param params The QC component Video Encoder configuration data structure.
 */
typedef struct VidcNodeBase_Config : public QCNodeConfigBase_t
{
    uint32_t width;     /**< in pixels */
    uint32_t height;    /**< in pixels */
    uint32_t frameRate; /**< fps */
    uint32_t numInputBufferReq;
    uint32_t numOutputBufferReq;
    bool bInputDynamicMode;
    bool bOutputDynamicMode;
    QCImageFormat_e inFormat;  /**< uncompressed type */
    QCImageFormat_e outFormat; /**< compressed type */
    Logger_Level_e logLevel;
} VidcNodeBase_Config_t;

class VidcNodeBaseConfigIfs : public NodeConfigIfs
{
public:
    /**
     * @brief VideoDecoderConfigIfs Constructor
     * @param[in] logger A reference to the logger to be shared and used by VideoDecoderConfigIfs.
     * @param[in] vide A reference to the RideHal Video Decoder component to be used by VideoDecoderConfigIfs.
     * VideoDecoderConfigIfs.
     * @return None
     */
    VidcNodeBaseConfigIfs( Logger &logger ) : NodeConfigIfs( logger ) {}

    /**
     * @brief VideoDecoderConfigIfs Destructor
     * @return None
     */
    virtual ~VidcNodeBaseConfigIfs() {}

    /**
     * @brief Verify and Load the json string
     *
     * @param[in] config the json configuration string
     * @param[out] errors the error string to be used to return readable error information
     *
     * @return QC_STATUS_OK on success, others on failure
     *
     * @note This API will also initialize the logger only once.
     * And this API can be called multiple times to apply dynamic parameter settings during runtime
     * after initialization.
     */
    QCStatus_e VerifyAndSet( const std::string cfg, std::string &errors,
                             VidcNodeBase_Config_t &config );

    /**
     * @brief Get Configuration Options
     * @return A reference string to the JSON configuration options.
     * @note
     * TODO: Provide a more detailed introduction about the JSON configuration options.
     */
    const virtual std::string& GetOptions( ) = 0;

    /**
     * @brief Get the Configuration Structure.
     * @return A reference to the Configuration Structure.
     */
    const virtual QCNodeConfigBase_t& Get( ) = 0;

protected:
    QCStatus_e ParseStaticConfig( DataTree &dt, std::string &errors, VidcNodeBase_Config_t &config );
    QCStatus_e ApplyDynamicConfig( DataTree &dt, std::string &errors, VidcNodeBase_Config_t &config );
};

/** @brief base class for video codec component */
class VidcNodeBase : public NodeBase
{
public:
    /** @brief Default constructor */
    VidcNodeBase() : m_state (QC_OBJECT_STATE_INITIAL) {}

    /** @brief Default destructor */
    virtual ~VidcNodeBase() = default;

    /**
     * @brief Init the video codec node
     * @param pName the video codec unique instance name
     * @param level the log level used , default is error level
     * @return QC_STATUS_OK on success, others on failure
     */
    virtual QCStatus_e Init( const VidcNodeBase_Config_t& config );

    /**
     * @brief deinitialize the video codec
     * @return QC_STATUS_OK on success, others on failure
     */
    virtual QCStatus_e DeInitialize();

    virtual QCStatus_e Start();
    virtual QCStatus_e Stop();

    QCStatus_e PostInit( void );

    QCStatus_e AllocateBuffer( const std::vector<std::reference_wrapper<QCBufferDescriptorBase_t>> &buffers,
                               uint32_t bufferIdx, VideoCodec_BufType_e bufferType );

    /**
     * @brief Inform the video codec node of an externally allocated buffer address.
     *        i.e register a buffer address with video codec node.
     *        to be used only if an external allocator allocated this buffer.
     * @note NegotiateBufferReq() should be called before to get the buffer requirements
     *       from the driver.
     * @note API type: Synchronous
     */
    QCStatus_e SetBuffer( VideoCodec_BufType_e bufferType );

    QCStatus_e FreeOutputBuffer( );
    QCStatus_e FreeInputBuffer( );

    QCStatus_e ValidateBuffer( const VideoFrameDescriptor &vidFrmDesc, VideoCodec_BufType_e bufferType );
    QCStatus_e ValidateBuffers ( );
    QCStatus_e NegotiateBufferReq( VideoCodec_BufType_e bufType );

    QCStatus_e WaitForState( QCObjectState_e expectedState );

    vidc_color_format_type GetVidcFormat( QCImageFormat_e format );

protected:
    std::string m_name;
    QCObjectState_e m_state = QC_OBJECT_STATE_INITIAL;

    void EventCallback( const VideoCodec_EventType_e eventId, const void *pPayload );

    VidcDrvClient m_drvClient;

    uint32_t m_bufSize[VIDEO_CODEC_BUF_TYPE_NUM];

    const VidcNodeBase_Config_t *m_pConfig = nullptr;

    std::vector<std::reference_wrapper<VideoFrameDescriptor_t>> m_inputBufferList;  /**< set input descriptors in non-dynamic mode */
    std::vector<std::reference_wrapper<VideoFrameDescriptor_t>> m_outputBufferList; /**< set output descriptors in non-dynamic mode */

    QCStatus_e ValidateFrameSubmission( const VideoFrameDescriptor_t &frameDesc,
                                        VideoCodec_BufType_e bufferType,
                                        bool requireNonZeroSize = true );

private:
    typedef enum
    {
        COMPRESSED_DATA_MODE = 0,
        YUV_OR_RGB_MODE
    } VideoCodec_BufAllocMode_e;

    QCStatus_e InitBufferForNonDynamicMode( const std::vector<std::reference_wrapper<QCBufferDescriptorBase_t>> &buffers,
                                            uint32_t bufferIdx, VideoCodec_BufType_e bufferType );

    void PrintConfig();
};

}   // namespace QC::Node

#endif   // QC_VIDEO_CODEC_NODE_BASE_HPP
