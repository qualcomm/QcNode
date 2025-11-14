// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "QnnModel.hpp"
#include "QnnOpDef.h"

typedef enum
{
    MOCK_ADD_MODEL_CONTROL_API_NONE,
    MOCK_ADD_MODEL_CONTROL_API_RETURN,
    MOCK_ADD_MODEL_CONTROL_API_OUT_PARAM0,
    MOCK_ADD_MODEL_CONTROL_API_OUT_PARAM1,
    MOCK_ADD_MODEL_CONTROL_API_OUT_PARAM2,
    MOCK_ADD_MODEL_CONTROL_API_OUT_PARAM3,
    MOCK_ADD_MODEL_CONTROL_API_OUT_PARAM5,
    MOCK_ADD_MODEL_CONTROL_API_OUT_PARAM6,
    MOCK_ADD_MODEL_CONTROL_API_OUT_PARAM7,
    MOCK_ADD_MODEL_CONTROL_API_OUT_PARAM8,
} MockAddModelAPI_Action_e;

typedef enum
{
    MOCK_ADD_MODEL_API_COMPOSE_GRAPH,
    MOCK_ADD_MODEL_API_MAX
} MockAddModelAPI_ID_e;

typedef struct
{
    MockAddModelAPI_Action_e action;
    void *param; /* a pointer to a control parameter, how to use it
                  * depends on the code coverage test */
} MockAddModelControlParam_t;

// Flag to determine if Backend should do node validation for each opNode added
#define DO_GRAPH_NODE_VALIDATIONS 1

using namespace qnn_wrapper_api;


static MockAddModelControlParam_t s_MockParams[MOCK_ADD_MODEL_API_MAX];

extern "C" QNN_API void MockAddModelApi_ControlFnc( MockAddModelAPI_ID_e apiId,
                                                    MockAddModelAPI_Action_e action, void *param )
{
    if ( apiId < MOCK_ADD_MODEL_API_MAX )
    {
        s_MockParams[apiId].action = action;
        s_MockParams[apiId].param = param;
    }
}

static ModelError_t addTensor_input1( QnnModel &model )
{
    ModelError_t err = MODEL_NO_ERROR;
    uint32_t dimensions_input1[] = { 1, 128, 3, 128 };
    VALIDATE(
            model.addTensor(
                    "input1",   // Tensor Name
                    (Qnn_Tensor_t) {
                            .version = QNN_TENSOR_VERSION_2,
                            { .v2 = { .id = 0,
                                      .name = "input1",
                                      .type = QNN_TENSOR_TYPE_APP_WRITE,
                                      .dataFormat = QNN_TENSOR_DATA_FORMAT_DENSE,
                                      .dataType = QNN_DATATYPE_FLOAT_32,
                                      .quantizeParams =
                                              { QNN_DEFINITION_UNDEFINED,
                                                QNN_QUANTIZATION_ENCODING_UNDEFINED,
                                                { .scaleOffsetEncoding =
                                                          { .scale =
                                                                    0.0000000000000000000000000000000000000000f,
                                                            .offset = 0 } } },
                                      .rank = 4,
                                      .dimensions = dimensions_input1,
                                      .memType = QNN_TENSORMEMTYPE_RAW,
                                      { .clientBuf = { .data = nullptr, .dataSize = 0 } },
                                      .isDynamicDimensions = nullptr,
                                      .sparseParams = { QNN_SPARSE_LAYOUT_UNDEFINED,
                                                        .hybridCoo = { .numSpecifiedElements = 0,
                                                                       .numSparseDimensions = 0 } },
                                      .isProduced = 0 } } } ),
            err );
    return err;
}

static ModelError_t addTensor_input2( QnnModel &model )
{
    ModelError_t err = MODEL_NO_ERROR;
    uint32_t dimensions_input2[] = { 1, 128, 3, 128 };
    VALIDATE(
            model.addTensor(
                    "input2",   // Tensor Name
                    (Qnn_Tensor_t) {
                            .version = QNN_TENSOR_VERSION_2,
                            { .v2 = { .id = 0,
                                      .name = "input2",
                                      .type = QNN_TENSOR_TYPE_APP_WRITE,
                                      .dataFormat = QNN_TENSOR_DATA_FORMAT_DENSE,
                                      .dataType = QNN_DATATYPE_FLOAT_32,
                                      .quantizeParams =
                                              { QNN_DEFINITION_UNDEFINED,
                                                QNN_QUANTIZATION_ENCODING_UNDEFINED,
                                                { .scaleOffsetEncoding =
                                                          { .scale =
                                                                    0.0000000000000000000000000000000000000000f,
                                                            .offset = 0 } } },
                                      .rank = 4,
                                      .dimensions = dimensions_input2,
                                      .memType = QNN_TENSORMEMTYPE_RAW,
                                      { .clientBuf = { .data = nullptr, .dataSize = 0 } },
                                      .isDynamicDimensions = nullptr,
                                      .sparseParams = { QNN_SPARSE_LAYOUT_UNDEFINED,
                                                        .hybridCoo = { .numSpecifiedElements = 0,
                                                                       .numSparseDimensions = 0 } },
                                      .isProduced = 0 } } } ),
            err );
    return err;
}

static ModelError_t addNode_Add_0( QnnModel &model )
{
    ModelError_t err = MODEL_NO_ERROR;

    /* ADDING NODE FOR Add_0 */
    Qnn_Param_t params_Add_0[] = {
            { .paramType = QNN_PARAMTYPE_SCALAR,
              .name = "operation",
              { .scalarParam = (Qnn_Scalar_t) { QNN_DATATYPE_UINT_32, { .uint32Value = 0 } } } } };
    const char *inputs_Add_0[] = { "input1", "input2" };
    uint32_t dimensions_output[] = { 1, 128, 3, 128 };
    Qnn_Tensor_t outputs_Add_0[] = { (Qnn_Tensor_t) {
            .version = QNN_TENSOR_VERSION_2,
            { .v2 = { .id = 0,
                      .name = "output",
                      .type = QNN_TENSOR_TYPE_APP_READ,
                      .dataFormat = QNN_TENSOR_DATA_FORMAT_DENSE,
                      .dataType = QNN_DATATYPE_FLOAT_32,
                      .quantizeParams =
                              { QNN_DEFINITION_UNDEFINED,
                                QNN_QUANTIZATION_ENCODING_UNDEFINED,
                                { .scaleOffsetEncoding =
                                          { .scale = 0.0000000000000000000000000000000000000000f,
                                            .offset = 0 } } },
                      .rank = 4,
                      .dimensions = dimensions_output,
                      .memType = QNN_TENSORMEMTYPE_RAW,
                      { .clientBuf = { .data = nullptr, .dataSize = 0 } },
                      .isDynamicDimensions = nullptr,
                      .sparseParams = { QNN_SPARSE_LAYOUT_UNDEFINED,
                                        .hybridCoo = { .numSpecifiedElements = 0,
                                                       .numSparseDimensions = 0 } },
                      .isProduced = 0 } } } };
    VALIDATE( model.addNode( QNN_OPCONFIG_VERSION_1,   // Op_Config_t Version
                             "Add_0",                  // Node Name
                             "qti.aisw",               // Package Name
                             "ElementWiseBinary",      // Qnn Node Type
                             params_Add_0,             // Node Params
                             1,                        // Num Node Params
                             inputs_Add_0,             // Input Tensor Names
                             2,                        // Num Input Tensor Names
                             outputs_Add_0,            // Output Tensors
                             1                         // Num Output Tensors
                             ),
              err );
    return err;
}

extern "C" QNN_API ModelError_t QnnModel_composeGraphs(
        Qnn_BackendHandle_t backendHandle, QNN_INTERFACE_VER_TYPE interface,
        Qnn_ContextHandle_t contextHandle, const GraphConfigInfo_t **graphsConfigInfo,
        const uint32_t numGraphsConfigInfo, GraphInfoPtr_t **graphsInfo, uint32_t *numGraphsInfo,
        bool debug, QnnLog_Callback_t logCallback, QnnLog_Level_t maxLogLevel )
{

    ModelError_t err = MODEL_NO_ERROR;

    /* model/graph for add_model*/
    QnnModel add_model;
    const QnnGraph_Config_t **graphConfigs = nullptr;
    VALIDATE( getQnnGraphConfigFromInfo( "add_model", graphsConfigInfo, numGraphsConfigInfo,
                                         graphConfigs ),
              err );
    VALIDATE( add_model.initialize( backendHandle, interface, contextHandle, "add_model", debug,
                                    DO_GRAPH_NODE_VALIDATIONS, graphConfigs ),
              err );
    VALIDATE( addTensor_input1( add_model ), err );
    VALIDATE( addTensor_input2( add_model ), err );
    VALIDATE( addNode_Add_0( add_model ), err );

    // Add all models to array to get graphsInfo
    QnnModel *models[] = { &add_model };
    uint32_t numModels = 1;

    // Populate the constructed graphs in provided output variables
    VALIDATE( getGraphInfoFromModels( *models, numModels, graphsInfo ), err );
    *numGraphsInfo = numModels;

    if ( MOCK_ADD_MODEL_CONTROL_API_NONE != s_MockParams[MOCK_ADD_MODEL_API_COMPOSE_GRAPH].action )
    {
        switch ( s_MockParams[MOCK_ADD_MODEL_API_COMPOSE_GRAPH].action )
        {
            case MOCK_ADD_MODEL_CONTROL_API_RETURN:
                err = *(ModelError_t *) s_MockParams[MOCK_ADD_MODEL_API_COMPOSE_GRAPH].param;
                break;
            case MOCK_ADD_MODEL_CONTROL_API_OUT_PARAM1:
                *numGraphsInfo = *(uint32_t *) s_MockParams[MOCK_ADD_MODEL_API_COMPOSE_GRAPH].param;
                break;
            default:
                break;
        }
        s_MockParams[MOCK_ADD_MODEL_API_COMPOSE_GRAPH].action =
                MOCK_ADD_MODEL_CONTROL_API_NONE; /* consume it and invalide the control */
    }

    return err;
}
