set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( QC_NODE_CL2D_HEADERS
    ${HEADERS_DIR}/QC/Node/CL2DFlex.hpp
)

set( QC_NODE_CL2D_SOURCES
    CL2DFlex.cpp
    CL2DFlexConfig.cpp
    CL2DFlexImpl.cpp
    pipeline/CL2DPipelineBase.cpp
    pipeline/CL2DPipelineConvert.cpp
    pipeline/CL2DPipelineResize.cpp
    pipeline/CL2DPipelineRemap.cpp
    pipeline/CL2DPipelineLetterbox.cpp
    pipeline/CL2DPipelineConvertUBWC.cpp
    pipeline/CL2DPipelineResizeMultiple.cpp
    pipeline/CL2DPipelineLetterboxMultiple.cpp
)

set( TARGET_LIBRARIES QCNodeBase )

add_library( QCNodeCL2DFlex OBJECT ${QC_NODE_CL2D_SOURCES} )
list( APPEND TARGET_LIBRARIES OpenclIface)
list( APPEND HEADERS_DIR ${PROJECT_SOURCE_DIR}/source/Library/OpenclIface )
list( APPEND HEADERS_DIR ${PROJECT_SOURCE_DIR}/source/Node/CL2DFlex )

set_property( TARGET QCNodeCL2DFlex PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET QCNodeCL2DFlex PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( QCNodeCL2DFlex PUBLIC ${HEADERS_DIR} )

target_link_libraries( QCNodeCL2DFlex PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${QC_NODE_CL2D_HEADERS} DESTINATION include/QC/Node )
