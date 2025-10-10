// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear



#include "gtest/gtest.h"
#include <chrono>
#include <cmath>
#include <stdio.h>
#include <string>

#include "QC/Common/DataTree.hpp"
#include "QC/Node/NodeBase.hpp"

using namespace QC::Node;
using namespace QC;

TEST( NodeBase, Sanity_QCSharedBufferDescriptor )
{
    QCStatus_e ret;
    QCSharedBufferDescriptor_t buffer;
    QCSharedFrameDescriptorNode *pFrameDesc;

    ret = buffer.buffer.Allocate( 1920, 1080, QC_IMAGE_FORMAT_NV12 );
    ASSERT_EQ( QC_STATUS_OK, ret );

    pFrameDesc = new QCSharedFrameDescriptorNode( 1 );

    QCBufferDescriptorBase_t &bufDescNull = pFrameDesc->GetBuffer( 0 );
    QCSharedBufferDescriptor_t *pDescCast =
            dynamic_cast<QCSharedBufferDescriptor_t *>( &bufDescNull );
    ASSERT_EQ( nullptr, pDescCast );

    ret = (QCStatus_e) pFrameDesc->SetBuffer( 0, buffer );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = (QCStatus_e) pFrameDesc->SetBuffer( 1, buffer );
    ASSERT_EQ( QC_STATUS_OUT_OF_BOUND, ret );

    QCBufferDescriptorBase_t &bufDesc = pFrameDesc->GetBuffer( 0 );
    pDescCast = dynamic_cast<QCSharedBufferDescriptor_t *>( &bufDesc );
    ASSERT_EQ( &buffer, pDescCast );

    delete pFrameDesc;

    ret = buffer.buffer.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );
}

#ifndef GTEST_QCNODE
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif