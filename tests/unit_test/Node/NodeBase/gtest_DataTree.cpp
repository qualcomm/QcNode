// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


#include "gtest/gtest.h"
#include <chrono>
#include <cmath>
#include <stdio.h>
#include <string>

#include "QC/Common/DataTree.hpp"

using namespace QC;

TEST( NodeBase, Sanity_DataTree )
{
    QCStatus_e status;
    std::string context;
    std::string errors;
    {
        DataTree dt;
        context = R"({"A": "This is A"})";
        status = dt.Load( context, errors );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( dt.Exists( "A" ), true );
        ASSERT_EQ( dt.Exists( "B" ), false );
        ASSERT_EQ( dt.Exists( "A.B" ), false );

        std::string value;
        value = dt.Get<std::string>( "A", "" );
        ASSERT_EQ( value, "This is A" );

        uint32_t u32V;
        u32V = dt.Get<uint32_t>( "A", UINT32_MAX );
        ASSERT_EQ( UINT32_MAX, u32V );

        DataTree dt2 = dt;
        ASSERT_EQ( dt2.Dump(), dt.Dump() );

        DataTree *pDt = &dt2;
        dt2 = *pDt;
    }

    {
        DataTree dt;
        context = R"({"A": x"This is A"})";
        status = dt.Load( context, errors );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );
    }

    {
        DataTree dt;
        context = R"({"A": [0, 1, 2], "B": 3, "C": ["a", "b"]})";
        status = dt.Load( context, errors );
        ASSERT_EQ( QC_STATUS_OK, status );
        std::vector<uint32_t> numbers;

        numbers = dt.Get<uint32_t>( "A", std::vector<uint32_t>( {} ) );
        ASSERT_EQ( 3, numbers.size() );
        ASSERT_EQ( 0, numbers[0] );
        ASSERT_EQ( 1, numbers[1] );
        ASSERT_EQ( 2, numbers[2] );

        numbers = dt.Get<uint32_t>( "B", std::vector<uint32_t>( {} ) );
        ASSERT_EQ( 0, numbers.size() );

        numbers = dt.Get<uint32_t>( "C", std::vector<uint32_t>( {} ) );
        ASSERT_EQ( 0, numbers.size() );

        DataTree dtr;
        status = dt.Get( "A", dtr );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = dt.Get( "A.FF", dtr );
        ASSERT_EQ( QC_STATUS_OUT_OF_BOUND, status );


        std::vector<DataTree> dts;
        status = dt.Get( "A", dts );
        ASSERT_EQ( QC_STATUS_OK, status );
        status = dt.Get( "A.FF", dts );
        ASSERT_EQ( QC_STATUS_OUT_OF_BOUND, status );
        status = dt.Get( "B", dts );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );
    }

    {
        DataTree dt;
        dt.Set<uint32_t>( "A.B.C", 1234 );
        ASSERT_EQ( R"({"A":{"B":{"C":1234}}})", dt.Dump() );

        DataTree dtB;
        dtB.Set( "K.aa", dt );
        ASSERT_EQ( R"({"K":{"aa":{"A":{"B":{"C":1234}}}}})", dtB.Dump() );

        std::vector<uint32_t> numbers = { 1, 2, 3 };
        dt.Set<uint32_t>( "A.B.D.E", numbers );
        ASSERT_EQ( R"({"A":{"B":{"C":1234,"D":{"E":[1,2,3]}}}})", dt.Dump() );
    }

    {
        std::map<QCImageFormat_e, std::string> formatMap = {
                { QC_IMAGE_FORMAT_RGB888, "rgb" },
                { QC_IMAGE_FORMAT_BGR888, "bgr" },
                { QC_IMAGE_FORMAT_UYVY, "uyvy" },
                { QC_IMAGE_FORMAT_NV12, "nv12" },
                { QC_IMAGE_FORMAT_NV12_UBWC, "nv12_ubwc" },
                { QC_IMAGE_FORMAT_P010, "p010" },
                { QC_IMAGE_FORMAT_TP10_UBWC, "tp10_ubwc" },
                { QC_IMAGE_FORMAT_COMPRESSED_H264, "h264" },
                { QC_IMAGE_FORMAT_COMPRESSED_H265, "h265" },
                { QC_IMAGE_FORMAT_MAX, "unknown" },
        };
        for ( auto &kv : formatMap )
        {
            DataTree dt;
            dt.SetImageFormat( "format", kv.first );
            ASSERT_EQ( kv.second, dt.Get<std::string>( "format", "" ) );

            QCImageFormat_e format = dt.GetImageFormat( "format", QC_IMAGE_FORMAT_MAX );
            ASSERT_EQ( kv.first, format );

            format = dt.GetImageFormat( "formatx", QC_IMAGE_FORMAT_MAX );
            ASSERT_EQ( QC_IMAGE_FORMAT_MAX, format );

            dt.SetImageFormat( "A.B.format", kv.first );
            ASSERT_EQ( kv.second, dt.Get<std::string>( "A.B.format", "" ) );
        }
    }

    {
        std::map<QCTensorType_e, std::string> tensorTypeMap = {
                { QC_TENSOR_TYPE_INT_8, "int8" },
                { QC_TENSOR_TYPE_INT_16, "int16" },
                { QC_TENSOR_TYPE_INT_32, "int32" },
                { QC_TENSOR_TYPE_INT_64, "int64" },
                { QC_TENSOR_TYPE_UINT_8, "uint8" },
                { QC_TENSOR_TYPE_UINT_16, "uint16" },
                { QC_TENSOR_TYPE_UINT_32, "uint32" },
                { QC_TENSOR_TYPE_UINT_64, "uint64" },
                { QC_TENSOR_TYPE_FLOAT_16, "float16" },
                { QC_TENSOR_TYPE_FLOAT_32, "float32" },
                { QC_TENSOR_TYPE_FLOAT_64, "float64" },
                { QC_TENSOR_TYPE_SFIXED_POINT_8, "sfixed_point8" },
                { QC_TENSOR_TYPE_SFIXED_POINT_16, "sfixed_point16" },
                { QC_TENSOR_TYPE_SFIXED_POINT_32, "sfixed_point32" },
                { QC_TENSOR_TYPE_UFIXED_POINT_8, "ufixed_point8" },
                { QC_TENSOR_TYPE_UFIXED_POINT_16, "ufixed_point16" },
                { QC_TENSOR_TYPE_UFIXED_POINT_32, "ufixed_point32" },
                { QC_TENSOR_TYPE_MAX, "unknown" },
        };
        for ( auto &kv : tensorTypeMap )
        {
            DataTree dt;
            dt.SetTensorType( "tensorType", kv.first );
            ASSERT_EQ( kv.second, dt.Get<std::string>( "tensorType", "" ) );

            QCTensorType_e tensorType = dt.GetTensorType( "tensorType", QC_TENSOR_TYPE_MAX );
            ASSERT_EQ( kv.first, tensorType );

            tensorType = dt.GetTensorType( "tensorTypex", QC_TENSOR_TYPE_MAX );
            ASSERT_EQ( QC_TENSOR_TYPE_MAX, tensorType );

            dt.SetTensorType( "A.B.tensorType", kv.first );
            ASSERT_EQ( kv.second, dt.Get<std::string>( "A.B.tensorType", "" ) );
        }
    }

    {
        std::map<QCProcessorType_e, std::string> processorTypeMap = {
                { QC_PROCESSOR_HTP0, "htp0" },   { QC_PROCESSOR_HTP1, "htp1" },
                { QC_PROCESSOR_CPU, "cpu" },     { QC_PROCESSOR_GPU, "gpu" },
                { QC_PROCESSOR_MAX, "unknown" },
        };
        for ( auto &kv : processorTypeMap )
        {
            DataTree dt;
            dt.SetProcessorType( "processor", kv.first );
            ASSERT_EQ( kv.second, dt.Get<std::string>( "processor", "" ) );

            QCProcessorType_e processor = dt.GetProcessorType( "processor", QC_PROCESSOR_MAX );
            ASSERT_EQ( kv.first, processor );

            processor = dt.GetProcessorType( "processorx", QC_PROCESSOR_MAX );
            ASSERT_EQ( QC_PROCESSOR_MAX, processor );

            dt.SetProcessorType( "A.B.processor", kv.first );
            ASSERT_EQ( kv.second, dt.Get<std::string>( "A.B.processor", "" ) );
        }
    }
}