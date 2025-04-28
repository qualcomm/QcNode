set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( QC_NODE_CL2D_HEADERS
    ${HEADERS_DIR}/QC/node/CL2DFlex.hpp
)

set( QC_NODE_CL2D_SOURCES
    CL2DFlex.cpp
)

set( TARGET_LIBRARIES QCNodeCL2DFlex QCNodeNodeBase )

add_library( QCNodeNodeCL2DFlex OBJECT ${QC_NODE_CL2D_SOURCES} )

set_property( TARGET QCNodeNodeCL2DFlex PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET QCNodeNodeCL2DFlex PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( QCNodeNodeCL2DFlex PUBLIC ${HEADERS_DIR} )

target_link_libraries( QCNodeNodeCL2DFlex PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${QC_NODE_CL2D_HEADERS} DESTINATION include/QC/node )
