// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef QC_NODE_DATA_TREE_HPP
#define QC_NODE_DATA_TREE_HPP

#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#include "QC/Common/QCDefs.hpp"
#include "QC/Common/Types.hpp"
#include "QC/Infras/Log/Logger.hpp"
#include <nlohmann/json.hpp>

namespace QC
{

using nlohmann::json;

class DataTree
{
private:
    /**
     * @brief Constructs a DataTree from a JSON object.
     * @param[in] js The JSON object.
     * @return void
     */
    DataTree( const json &js );

public:
    DataTree();
    ~DataTree();

    /**
     * @brief Constructs a DataTree from another DataTree object.
     * @param[in] rhs The DataTree object to copy from.
     * @return void
     */
    DataTree( const DataTree &rhs );

    /**
     * @brief Loads the DataTree from the input string context.
     * @param[in] context The input string context.
     * @param[out] errors The related error strings if loading the string context fails.
     * @return QC_STATUS_OK on success, or an appropriate error code on failure.
     */
    QCStatus_e Load( const std::string &context, std::string &errors );

    /**
     * @brief Dumps the DataTree to a string context.
     * @return The dumped string context.
     */
    std::string Dump();

    /**
     * @brief Checks if the key exists.
     * @param[in] key The input key string.
     * @return true if the key exists, false otherwise.
     */
    bool Exists( const std::string &key );

    /**
     * @brief Retrieves the value associated with the specified key.
     * @param[in] key The input key string.
     * @param[in] dv The default value to return if the key does not exist.
     * @return The value corresponding to the key.
     * @example
     *  DataTree dt = Load("{ \"config\": { \"A\" : \"This is A\" } }", ...);
     *  dt.Get<std::string>("config.A", "") -> "This is A"
     */
    template<typename T>
    T Get( const std::string &key, T dv );

    /**
     * @brief Retrieves the image format associated with the specified key.
     * @param[in] key The input key string.
     * @param[in] dv The default image format to return if the key does not exist.
     * @return The image format corresponding to the key.
     * @example
     *  DataTree dt = Load("{ \"format\": \"rgb\" }", ...);
     *  dt.GetImageFormat("format", QC_IMAGE_FORMAT_MAX) -> QC_IMAGE_FORMAT_RGB888
     */
    QCImageFormat_e GetImageFormat( const std::string key, QCImageFormat_e dv );

    /**
     * @brief Retrieves the tensor type associated with the specified key.
     * @param[in] key The input key string.
     * @param[in] dv The default tensor type to return if the key does not exist.
     * @return The tensor type corresponding to the key.
     * @example
     *  DataTree dt = Load("{ \"type\": \"ufixed_point8\" }", ...);
     *  dt.GetTensorType("type", QC_TENSOR_TYPE_MAX) -> QC_TENSOR_TYPE_UFIXED_POINT_8
     */
    QCTensorType_e GetTensorType( const std::string key, QCTensorType_e dv );

    /**
     * @brief Retrieves the processor type associated with the specified key.
     * @param[in] key The input key string.
     * @param[in] dv The default processor type to return if the key does not exist.
     * @return The processor type corresponding to the key.
     * @example
     *  DataTree dt = Load("{ \"type\": \"htp0\" }", ...);
     *  dt.GetProcessorType("type", QC_PROCESSOR_MAX) -> QC_PROCESSOR_HTP0
     */
    QCProcessorType_e GetProcessorType( const std::string key, QCProcessorType_e dv );

    /**
     * @brief Retrieves the DataTree associated with the specified key.
     * @param[in] key The input key string.
     * @param[out] dt The DataTree associated with the specified key.
     * @return QC_STATUS_OK on success, or an appropriate error code on failure.
     */
    QCStatus_e Get( const std::string &key, DataTree &dt );

    /**
     * @brief Retrieves the vector of values associated with the specified key.
     * @param[in] key The input key string.
     * @param[in] dv The default vector of values to return if the key does not exist.
     * @return The vector of values corresponding to the key.
     * @example
     *  DataTree dt = Load("{ \"config\": { \"A\" : [0, 1, 2, 3] } }", ...);
     *  dt.Get<uint32_t>("config.A", std::vector<uint32_t>{}) -> std::vector<uint32_t>{0, 1, 2, 3}
     */
    template<typename T>
    std::vector<T> Get( const std::string &key, std::vector<T> dv );

    /**
     * @brief Retrieves the vector of DataTree objects associated with the specified key.
     * @param[in] key The input key string.
     * @param[out] dts The vector of DataTree objects associated with the key.
     * @return QC_STATUS_OK on success, or an appropriate error code on failure.
     * @example
     *  DataTree dt = Load("{ \"config\": [{\"name\":\"A\"}, {\"name\": \"B\"}]}", ...);
     *  dt.Get("config") -> std::vector<DataTree>{dtA, dtB};
     *  // dtA and dtB can be used to access the JSON objects {"name":"A"} and {"name":"B"}
     *  respectively.
     */
    QCStatus_e Get( const std::string &key, std::vector<DataTree> &dts );

    /**
     * @brief Sets the value for the specified key.
     * @param[in] key The input key string.
     * @param[in] kv The value to be set.
     * @return None.
     * @example
     *  DataTree dt;
     *  dt.Set<std::string>("config.A", "This is A");
     *  // After this, the JSON object will be:
     *  // { "config": { "A" : "This is A" } }
     */
    template<typename T>
    void Set( const std::string &key, T kv );

    /**
     * @brief Sets the DataTree associated with the specified key.
     * @param[in] key The input key string.
     * @param[in] dt The DataTree to be associated with the specified key.
     * @return None.
     */
    void Set( const std::string &key, DataTree &dt );

    /**
     * @brief Sets the image format associated with the specified key.
     * @param[in] key The input key string.
     * @param[in] kv The image format.
     * @return None.
     */
    void SetImageFormat( const std::string key, QCImageFormat_e kv );

    /**
     * @brief Sets the tensor type associated with the specified key.
     * @param[in] key The input key string.
     * @param[in] kv The tensor type.
     * @return None.
     */
    void SetTensorType( const std::string key, QCTensorType_e kv );

    /**
     * @brief Sets the processor type associated with the specified key.
     * @param[in] key The input key string.
     * @param[in] kv The processor type.
     * @return None.
     */
    void SetProcessorType( const std::string key, QCProcessorType_e kv );

    /**
     * @brief Sets the vector of values for the specified key.
     * @param[in] key The input key string.
     * @param[in] kv The vector of values to be set.
     * @return None.
     * @example
     *  DataTree dt;
     *  dt.Set<std::uint32_t>("config.A", std::vector<uint32_t>{0, 1, 2, 3});
     *  // After this, the JSON object will be:
     *  // { "config": { "A" : [0, 1, 2, 3] } }
     */
    template<typename T>
    void Set( const std::string &key, std::vector<T> &kv );

    /**
     * @brief Sets the vector of DataTree objects for the specified key.
     * @param[in] key The input key string.
     * @param[in] kv The vector of DataTree objects.
     * @return None.
     * @example
     *  DataTree dt;
     *  DataTree dtA = {"name":"A"};
     *  DataTree dtB = {"name":"B"};
     *  dt.Set("config", std::vector<DataTree>{dtA, dtB});
     *  // After this, the JSON object will be:
     *  // { "config": [{"name": "A"}, {"name": "B"}] }
     */
    void Set( const std::string &key, std::vector<DataTree> &kv );

private:
    json m_json;
};


template<typename T>
T DataTree::Get( const std::string &key, T dv )
{
    std::istringstream ss( key );
    std::string token;
    const json *pCurrent = &m_json;
    T retV;
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
        try
        {
            retV = pCurrent->get<T>();
        }
        catch ( const json::exception &e )
        {
            QC_LOG_ERROR( "DataTree: Get key<%s> of %s with error: %s", key.c_str(),
                          m_json.dump().c_str(), e.what() );
            retV = dv;
        }
    }
    else
    {
        retV = dv;
    }

    return retV;
}


template<typename T>
std::vector<T> DataTree::Get( const std::string &key, std::vector<T> dv )
{
    std::istringstream ss( key );
    std::string token;
    const json *pCurrent = &m_json;
    std::vector<T> retV;
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

    if ( bHasKey && pCurrent->is_array() )
    {
        try
        {
            retV = pCurrent->get<std::vector<T>>();
        }
        catch ( const json::exception &e )
        {
            QC_LOG_ERROR( "DataTree: Get key<%s> of %s with error: %s", key.c_str(),
                          m_json.dump().c_str(), e.what() );
            retV = dv;
        }
    }
    else
    {
        retV = dv;
    }

    return retV;
}


template<typename T>
void DataTree::Set( const std::string &key, T kv )
{
    std::istringstream ss( key );
    std::string token;
    json *pCurrent = &m_json;

    while ( std::getline( ss, token, '.' ) )
    {
        if ( ss.peek() == EOF )
        {
            ( *pCurrent )[token] = kv;
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

template<typename T>
void DataTree::Set( const std::string &key, std::vector<T> &kv )
{
    std::istringstream ss( key );
    std::string token;
    json *pCurrent = &m_json;

    while ( std::getline( ss, token, '.' ) )
    {
        if ( ss.peek() == EOF )
        {
            ( *pCurrent )[token] = kv;
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

#endif
