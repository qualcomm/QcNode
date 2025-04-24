set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( NODEIF_HEADERS ${HEADERS_DIR}/QC/node/NodeBase.hpp)

set( NODEIF_SOURCES
    NodeBase.cpp
    DataTree.cpp
)

set( TARGET_LIBRARIES QCNodeCommon QCNodeVideoCodec )

add_library( QCNodeNodeBase STATIC ${NODEIF_SOURCES} )

set_property( TARGET QCNodeNodeBase PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET QCNodeNodeBase PROPERTY POSITION_INDEPENDENT_CODE ON )

find_package(nlohmann_json 3.2.0 REQUIRED)
list( APPEND TARGET_LIBRARIES nlohmann_json::nlohmann_json)

target_link_libraries( QCNodeNodeBase PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${NODEIF_HEADERS} DESTINATION include/QC/node )
install( FILES  ${HEADERS_DIR}/QC/common/DataTree.hpp
                ${HEADERS_DIR}/QC/common/QCDefs.hpp
                DESTINATION include/QC/common )
install( FILES  ${HEADERS_DIR}/QC/infras/logger/Logger.hpp
                DESTINATION include/QC/infras/logger )
install( FILES  ${HEADERS_DIR}/QC/infras/memory/Ifs/QCMemoryAllocatorIfs.hpp
                ${HEADERS_DIR}/QC/infras/memory/Ifs/QCMemoryDefs.hpp
                DESTINATION include/QC/infras/memory/Ifs )
install( FILES  ${HEADERS_DIR}/QC/infras/memory/QCBufferDescriptorBase.hpp
                DESTINATION include/QC/infras/memory )
install( FILES  ${HEADERS_DIR}/QC/node/Ifs/QCFrameDescriptorNodeIfs.hpp
                ${HEADERS_DIR}/QC/node/Ifs/QCNodeConfigIfs.hpp
                ${HEADERS_DIR}/QC/node/Ifs/QCNodeDefs.hpp
                ${HEADERS_DIR}/QC/node/Ifs/QCNodeIfs.hpp
                ${HEADERS_DIR}/QC/node/Ifs/QCNodeMonitoringIfs.hpp
                DESTINATION include/QC/node/Ifs )

add_subdirectory(QNN)

