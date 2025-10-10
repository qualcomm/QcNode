// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


#ifndef QC_SAMPLE_TINYVIZ_IF_HPP
#define QC_SAMPLE_TINYVIZ_IF_HPP

#include <list>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "QC/sample/DataTypes.hpp"

using namespace QC::sample;

namespace QC
{
namespace sample
{

class TinyVizIF
{
public:
    TinyVizIF() = default;
    TinyVizIF( const TinyVizIF & ) = delete;
    TinyVizIF &operator=( const TinyVizIF & ) = delete;
    virtual ~TinyVizIF() = default;

    virtual bool init( uint32_t winW = 1920, uint32_t winH = 1024 ) = 0;
    virtual bool start() = 0;
    virtual bool stop() = 0;

    virtual bool addCamera( const std::string camName ) = 0;
    virtual bool addData( const std::string camName, DataFrame_t & ) = 0;
    virtual bool addData( const std::string camName, Road2DObjects_t & ) = 0;
};
}   // namespace sample
}   // namespace QC

#endif   // #ifndef QC_SAMPLE_TINYVIZ_IF_HPP
