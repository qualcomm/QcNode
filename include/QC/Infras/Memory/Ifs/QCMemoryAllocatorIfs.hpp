// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_MEMORY_ALLOCATOR_IFS_HPP
#define QC_MEMORY_ALLOCATOR_IFS_HPP

#include "QC/Infras/Memory/Ifs/QCMemoryDefs.hpp"

namespace QC
{

class QCMemoryAllocatorIfs
{
protected:
    const std::string m_name;

public:
    QCMemoryAllocatorIfs( const std::string &name ) : m_name( name ) {};

    virtual QCStatus_e Allocate( const QCBufferPropBase_t &request,
                                 QCBufferDescriptorBase_t &response ) = 0;
    virtual QCStatus_e Free( const QCBufferDescriptorBase_t &buff ) = 0;
};

}   // namespace QC

#endif   // QC_MEMORY_ALLOCATOR_IFS_HPP
