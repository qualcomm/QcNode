// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_BUFFER_DESCRIPTOR_BASE_HPP
#define QC_BUFFER_DESCRIPTOR_BASE_HPP

#include "QC/Common/QCDefs.hpp"

namespace QC
{

/**
 * @brief QCNode Buffer Descriptor Base
 * @param name The buffer name.
 * @param pBuf The buffer virtual address.
 * @param size The buffer size.
 * @param type The buffer type.
 */
typedef struct QCBufferDescriptorBase
{
public:
    virtual ~QCBufferDescriptorBase() = default;
    std::string name;
    void *pBuf;
    uint32_t size;
    QCBufferDataFormat_e type;
} QCBufferDescriptorBase_t;

}   // namespace QC

#endif   // QC_BUFFER_DESCRIPTOR_BASE_HPP
