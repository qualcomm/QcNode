// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


#ifndef QC_NODE_CONFIG_BASE_HPP
#define QC_NODE_CONFIG_BASE_HPP

#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "QC/Common/DataTree.hpp"
#include "QC/Infras/Log/Logger.hpp"
#include "QC/Node/Ifs/QCNodeDefs.hpp"
#include "QC/Node/Ifs/QCNodeIfs.hpp"

namespace QC
{
namespace Node
{

class NodeConfigBase : public QCNodeConfigIfs
{
public:
    /**
     * @brief NodeConfigBase Constructor.
     * @param[in] logger A reference to a QC logger shared to be used by NodeConfigBase.
     * @return None.
     */
    NodeConfigBase( Logger &logger ) : m_logger( logger ) {}

    /**
     * @brief NodeConfigBase Destructor
     * @return None
     */
    ~NodeConfigBase() {}


    NodeConfigBase( const NodeConfigBase &other ) = delete;


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
    virtual QCStatus_e VerifyAndSet( const std::string config, std::string &errors );

    /**
     * @brief Get Configuration Options
     * @return A reference string to the JSON configuration options.
     */
    virtual const std::string &GetOptions() = 0;

    /**
     * @brief Get the Configuration Structure.
     * @return A reference to the Configuration Structure.
     */
    virtual const QCNodeConfigBase_t &Get() = 0;

protected:
    bool m_bLoggerInit = false;
    DataTree m_dataTree;
    Logger &m_logger;
    static const std::string s_QC_STATUS_UNSUPPORTED;   //= "QC_STATUS_UNSUPPORTED";
};

// @deprecated TO BE removed after phase 2 dev done
using NodeConfigIfs = NodeConfigBase;

}   // namespace Node
}   // namespace QC

#endif   // QC_NODE_CONFIG_BASE_HPP
