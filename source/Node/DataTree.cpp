// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "QC/Common/DataTree.hpp"

namespace QC
{

DataTree::DataTree() {}

DataTree::DataTree( const json &js ) : m_json( js ) {}

DataTree::DataTree( const DataTree &rhs ) : m_json( rhs.m_json ) {}

DataTree::~DataTree() {}

QCStatus_e DataTree::Load( const std::string &context, std::string &errors )
{
    QCStatus_e status = QC_STATUS_OK;
    try
    {
        m_json = json::parse( context );
    }
    catch ( const json::parse_error &e )
    {
        // Handle the parse error
        errors = std::string( "JSON parse error: " ) + e.what() + "\n";
        errors += std::string( "Exception id: " ) + std::to_string( e.id ) + "\n";
        errors += std::string( "Byte position of error: " ) + std::to_string( e.byte ) + "\n";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    return status;
}

std::string DataTree::Dump()
{
    return m_json.dump();
}

bool DataTree::Exists( const std::string &key )
{
    std::istringstream ss( key );
    std::string token;
    const json *pCurrent = &m_json;
    bool bExists = true;

    while ( std::getline( ss, token, '.' ) )
    {
        if ( pCurrent->contains( token ) )
        {
            pCurrent = &( ( *pCurrent )[token] );
        }
        else
        {
            bExists = false;
        }
    }

    return bExists;
}

QCStatus_e DataTree::Get( const std::string &key, DataTree &dt )
{
    std::istringstream ss( key );
    std::string token;
    const json *pCurrent = &m_json;
    QCStatus_e status = QC_STATUS_OK;
    bool bHasKey = true;

    while ( std::getline( ss, token, '.' ) )
    {
        if ( pCurrent->contains( token ) )
        {
            pCurrent = &( ( *pCurrent )[token] );
        }
        else
        {
            status = QC_STATUS_OUT_OF_BOUND;
        }
    }

    if ( QC_STATUS_OK == status )
    {
        dt = DataTree( *pCurrent );
    }

    return status;
}

QCStatus_e DataTree::Get( const std::string &key, std::vector<DataTree> &dts )
{
    std::istringstream ss( key );
    std::string token;
    const json *pCurrent = &m_json;
    std::vector<DataTree> retV;
    QCStatus_e status = QC_STATUS_OK;

    while ( std::getline( ss, token, '.' ) )
    {
        if ( pCurrent->contains( token ) )
        {
            pCurrent = &( ( *pCurrent )[token] );
        }
        else
        {
            status = QC_STATUS_OUT_OF_BOUND;
        }
    }

    if ( QC_STATUS_OK == status )
    {
        if ( pCurrent->is_array() )
        {
            for ( auto &js : *pCurrent )
            {
                dts.push_back( DataTree( js ) );
            }
        }
        else
        {
            status = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    return status;
}

QCImageFormat_e DataTree::GetImageFormat( const std::string key, QCImageFormat_e dv )
{
    QCImageFormat_e retV;
    std::istringstream ss( key );
    std::string token;
    std::string format;
    const json *pCurrent = &m_json;
    bool bHasKey = true;

    while ( std::getline( ss, token, '.' ) )
    {
        if ( pCurrent->contains( token ) )
        {
            pCurrent = &( ( *pCurrent )[token] );
        }
        else
        {
            bHasKey = false;
        }
    }

    if ( bHasKey )
    {
        format = pCurrent->get<std::string>();
        if ( "rgb" == format )
        {
            retV = QC_IMAGE_FORMAT_RGB888;
        }
        else if ( "bgr" == format )
        {
            retV = QC_IMAGE_FORMAT_BGR888;
        }
        else if ( "uyvy" == format )
        {
            retV = QC_IMAGE_FORMAT_UYVY;
        }
        else if ( "nv12" == format )
        {
            retV = QC_IMAGE_FORMAT_NV12;
        }
        else if ( "nv12_ubwc" == format )
        {
            retV = QC_IMAGE_FORMAT_NV12_UBWC;
        }
        else if ( "p010" == format )
        {
            retV = QC_IMAGE_FORMAT_P010;
        }
        else if ( "tp10_ubwc" == format )
        {
            retV = QC_IMAGE_FORMAT_TP10_UBWC;
        }
        else if ( "h264" == format )
        {
            retV = QC_IMAGE_FORMAT_COMPRESSED_H264;
        }
        else if ( "h265" == format )
        {
            retV = QC_IMAGE_FORMAT_COMPRESSED_H265;
        }
        else
        {
            retV = QC_IMAGE_FORMAT_MAX;
        }
    }
    else
    {
        retV = dv;
    }

    return retV;
}

QCTensorType_e DataTree::GetTensorType( const std::string key, QCTensorType_e dv )
{
    QCTensorType_e retV;
    std::istringstream ss( key );
    std::string token;
    std::string tensorType;
    const json *pCurrent = &m_json;
    bool bHasKey = true;

    while ( std::getline( ss, token, '.' ) )
    {
        if ( pCurrent->contains( token ) )
        {
            pCurrent = &( ( *pCurrent )[token] );
        }
        else
        {
            bHasKey = false;
        }
    }

    if ( bHasKey )
    {
        tensorType = pCurrent->get<std::string>();
        if ( "int8" == tensorType )
        {
            retV = QC_TENSOR_TYPE_INT_8;
        }
        else if ( "int16" == tensorType )
        {
            retV = QC_TENSOR_TYPE_INT_16;
        }
        else if ( "int32" == tensorType )
        {
            retV = QC_TENSOR_TYPE_INT_32;
        }
        else if ( "int64" == tensorType )
        {
            retV = QC_TENSOR_TYPE_INT_64;
        }
        else if ( "uint8" == tensorType )
        {
            retV = QC_TENSOR_TYPE_UINT_8;
        }
        else if ( "uint16" == tensorType )
        {
            retV = QC_TENSOR_TYPE_UINT_16;
        }
        else if ( "uint32" == tensorType )
        {
            retV = QC_TENSOR_TYPE_UINT_32;
        }
        else if ( "uint64" == tensorType )
        {
            retV = QC_TENSOR_TYPE_UINT_64;
        }
        else if ( "float16" == tensorType )
        {
            retV = QC_TENSOR_TYPE_FLOAT_16;
        }
        else if ( "float32" == tensorType )
        {
            retV = QC_TENSOR_TYPE_FLOAT_32;
        }
        else if ( "float64" == tensorType )
        {
            retV = QC_TENSOR_TYPE_FLOAT_64;
        }
        else if ( "sfixed_point8" == tensorType )
        {
            retV = QC_TENSOR_TYPE_SFIXED_POINT_8;
        }
        else if ( "sfixed_point16" == tensorType )
        {
            retV = QC_TENSOR_TYPE_SFIXED_POINT_16;
        }
        else if ( "sfixed_point32" == tensorType )
        {
            retV = QC_TENSOR_TYPE_SFIXED_POINT_32;
        }
        else if ( "ufixed_point8" == tensorType )
        {
            retV = QC_TENSOR_TYPE_UFIXED_POINT_8;
        }
        else if ( "ufixed_point16" == tensorType )
        {
            retV = QC_TENSOR_TYPE_UFIXED_POINT_16;
        }
        else if ( "ufixed_point32" == tensorType )
        {
            retV = QC_TENSOR_TYPE_UFIXED_POINT_32;
        }
        else
        {
            retV = QC_TENSOR_TYPE_MAX;
        }
    }
    else
    {
        retV = dv;
    }

    return retV;
}

QCProcessorType_e DataTree::GetProcessorType( const std::string key, QCProcessorType_e dv )
{
    QCProcessorType_e retV;
    std::istringstream ss( key );
    std::string token;
    std::string processor;
    const json *pCurrent = &m_json;
    bool bHasKey = true;

    while ( std::getline( ss, token, '.' ) )
    {
        if ( pCurrent->contains( token ) )
        {
            pCurrent = &( ( *pCurrent )[token] );
        }
        else
        {
            bHasKey = false;
        }
    }

    if ( bHasKey )
    {
        processor = pCurrent->get<std::string>();
        if ( "htp0" == processor )
        {
            retV = QC_PROCESSOR_HTP0;
        }
        else if ( "htp1" == processor )
        {
            retV = QC_PROCESSOR_HTP1;
        }
        else if ( "cpu" == processor )
        {
            retV = QC_PROCESSOR_CPU;
        }
        else if ( "gpu" == processor )
        {
            retV = QC_PROCESSOR_GPU;
        }
        else
        {
            retV = QC_PROCESSOR_MAX;
        }
    }
    else
    {
        retV = dv;
    }

    return retV;
}

void DataTree::Set( const std::string &key, DataTree &dt )
{
    std::istringstream ss( key );
    std::string token;
    json *pCurrent = &m_json;

    while ( std::getline( ss, token, '.' ) )
    {
        if ( ss.peek() == EOF )
        {
            ( *pCurrent )[token] = dt.m_json;
        }
        else
        {
            if ( false == pCurrent->contains( token ) )
            {
                ( *pCurrent )[token] = json();
            }
            pCurrent = &( *pCurrent )[token];
        }
    }
}

void DataTree::Set( const std::string &key, std::vector<DataTree> &kv )
{
    std::istringstream ss( key );
    std::string token;
    json *pCurrent = &m_json;

    while ( std::getline( ss, token, '.' ) )
    {
        if ( ss.peek() == EOF )
        {
            std::vector<json> jss;
            for ( auto &it : kv )
            {
                jss.push_back( it.m_json );
            }
            ( *pCurrent )[token] = jss;
        }
        else
        {
            if ( false == pCurrent->contains( token ) )
            {
                ( *pCurrent )[token] = json();
            }
            pCurrent = &( *pCurrent )[token];
        }
    }
}

void DataTree::SetImageFormat( const std::string key, QCImageFormat_e kv )
{
    std::istringstream ss( key );
    std::string token;
    json *pCurrent = &m_json;
    std::string format;

    switch ( kv )
    {
        case QC_IMAGE_FORMAT_RGB888:
            format = "rgb";
            break;
        case QC_IMAGE_FORMAT_BGR888:
            format = "bgr";
            break;
        case QC_IMAGE_FORMAT_UYVY:
            format = "uyvy";
            break;
        case QC_IMAGE_FORMAT_NV12:
            format = "nv12";
            break;
        case QC_IMAGE_FORMAT_NV12_UBWC:
            format = "nv12_ubwc";
            break;
        case QC_IMAGE_FORMAT_P010:
            format = "p010";
            break;
        case QC_IMAGE_FORMAT_TP10_UBWC:
            format = "tp10_ubwc";
            break;
        case QC_IMAGE_FORMAT_COMPRESSED_H264:
            format = "h264";
            break;
        case QC_IMAGE_FORMAT_COMPRESSED_H265:
            format = "h265";
            break;
        default:
            format = "unknown";
            break;
    }


    while ( std::getline( ss, token, '.' ) )
    {
        if ( ss.peek() == EOF )
        {
            ( *pCurrent )[token] = format;
        }
        else
        {
            if ( false == pCurrent->contains( token ) )
            {
                ( *pCurrent )[token] = json();
            }
            pCurrent = &( *pCurrent )[token];
        }
    }
}

void DataTree::SetTensorType( const std::string key, QCTensorType_e kv )
{
    std::istringstream ss( key );
    std::string token;
    json *pCurrent = &m_json;
    std::string tensorType;

    switch ( kv )
    {
        case QC_TENSOR_TYPE_INT_8:
            tensorType = "int8";
            break;
        case QC_TENSOR_TYPE_INT_16:
            tensorType = "int16";
            break;
        case QC_TENSOR_TYPE_INT_32:
            tensorType = "int32";
            break;
        case QC_TENSOR_TYPE_INT_64:
            tensorType = "int64";
            break;
        case QC_TENSOR_TYPE_UINT_8:
            tensorType = "uint8";
            break;
        case QC_TENSOR_TYPE_UINT_16:
            tensorType = "uint16";
            break;
        case QC_TENSOR_TYPE_UINT_32:
            tensorType = "uint32";
            break;
        case QC_TENSOR_TYPE_UINT_64:
            tensorType = "uint64";
            break;
        case QC_TENSOR_TYPE_FLOAT_16:
            tensorType = "float16";
            break;
        case QC_TENSOR_TYPE_FLOAT_32:
            tensorType = "float32";
            break;
        case QC_TENSOR_TYPE_FLOAT_64:
            tensorType = "float64";
            break;
        case QC_TENSOR_TYPE_SFIXED_POINT_8:
            tensorType = "sfixed_point8";
            break;
        case QC_TENSOR_TYPE_SFIXED_POINT_16:
            tensorType = "sfixed_point16";
            break;
        case QC_TENSOR_TYPE_SFIXED_POINT_32:
            tensorType = "sfixed_point32";
            break;
        case QC_TENSOR_TYPE_UFIXED_POINT_8:
            tensorType = "ufixed_point8";
            break;
        case QC_TENSOR_TYPE_UFIXED_POINT_16:
            tensorType = "ufixed_point16";
            break;
        case QC_TENSOR_TYPE_UFIXED_POINT_32:
            tensorType = "ufixed_point32";
            break;
        default:
            tensorType = "unknown";
            break;
    }


    while ( std::getline( ss, token, '.' ) )
    {
        if ( ss.peek() == EOF )
        {
            ( *pCurrent )[token] = tensorType;
        }
        else
        {
            if ( false == pCurrent->contains( token ) )
            {
                ( *pCurrent )[token] = json();
            }
            pCurrent = &( *pCurrent )[token];
        }
    }
}

void DataTree::SetProcessorType( const std::string key, QCProcessorType_e kv )
{
    std::istringstream ss( key );
    std::string token;
    json *pCurrent = &m_json;
    std::string processor;

    switch ( kv )
    {
        case QC_PROCESSOR_HTP0:
            processor = "htp0";
            break;
        case QC_PROCESSOR_HTP1:
            processor = "htp1";
            break;
        case QC_PROCESSOR_CPU:
            processor = "cpu";
            break;
        case QC_PROCESSOR_GPU:
            processor = "gpu";
            break;
        default:
            processor = "unknown";
            break;
    }


    while ( std::getline( ss, token, '.' ) )
    {
        if ( ss.peek() == EOF )
        {
            ( *pCurrent )[token] = processor;
        }
        else
        {
            if ( false == pCurrent->contains( token ) )
            {
                ( *pCurrent )[token] = json();
            }
            pCurrent = &( *pCurrent )[token];
        }
    }
}

}   // namespace QC
