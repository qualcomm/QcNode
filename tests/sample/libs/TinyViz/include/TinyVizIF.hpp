// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef RIDEHAL_SAMPLE_TINYVIZ_IF_HPP
#define RIDEHAL_SAMPLE_TINYVIZ_IF_HPP

#include <list>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "ridehal/sample/DataTypes.hpp"

using namespace ridehal::sample;

namespace ridehal
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
}   // namespace ridehal

#endif   // #ifndef RIDEHAL_SAMPLE_TINYVIZ_IF_HPP
