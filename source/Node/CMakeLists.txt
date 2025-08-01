set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )
set( NODEIF_HEADERS ${HEADERS_DIR}/QC/Node/NodeBase.hpp)
set( NODEIF_SOURCES
    NodeBase.cpp
    DataTree.cpp
)
set( TARGET_LIBRARIES QCNodeCommon QCNodeVideoCodec )
add_library( QCNodeBase STATIC ${NODEIF_SOURCES} )
set_property( TARGET QCNodeBase PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET QCNodeBase PROPERTY POSITION_INDEPENDENT_CODE ON )
find_package(nlohmann_json 3.2.0 REQUIRED)
list( APPEND TARGET_LIBRARIES nlohmann_json::nlohmann_json)
target_link_libraries( QCNodeBase PUBLIC ${TARGET_LIBRARIES} )
install( FILES ${NODEIF_HEADERS} DESTINATION include/QC/Node )
install( FILES  ${HEADERS_DIR}/QC/Common/DataTree.hpp
                ${HEADERS_DIR}/QC/Common/QCDefs.hpp
                DESTINATION include/QC/Common )
install( FILES  ${HEADERS_DIR}/QC/Infras/Log/Logger.hpp
                DESTINATION include/QC/Infras/Log )
install( FILES  ${HEADERS_DIR}/QC/Infras/Memory/Ifs/QCBufferDescriptorBase.hpp
                ${HEADERS_DIR}/QC/Infras/Memory/Ifs/QCMemoryAllocatorIfs.hpp
                ${HEADERS_DIR}/QC/Infras/Memory/Ifs/QCMemoryDefs.hpp
                ${HEADERS_DIR}/QC/Infras/Memory/Ifs/QCMemoryManagerIfs.hpp
                ${HEADERS_DIR}/QC/Infras/Memory/Ifs/QCMemoryPoolIfs.hpp
                ${HEADERS_DIR}/QC/Infras/Memory/Ifs/QCMemoryUtilsIfs.hpp
                DESTINATION include/QC/Infras/Memory/Ifs )
install( FILES  ${HEADERS_DIR}/QC/Node/Ifs/QCFrameDescriptorNodeIfs.hpp
                ${HEADERS_DIR}/QC/Node/Ifs/QCNodeConfigIfs.hpp
                ${HEADERS_DIR}/QC/Node/Ifs/QCNodeDefs.hpp
                ${HEADERS_DIR}/QC/Node/Ifs/QCNodeIfs.hpp
                ${HEADERS_DIR}/QC/Node/Ifs/QCNodeMonitoringIfs.hpp
                DESTINATION include/QC/Node/Ifs )
add_subdirectory(QNN)
add_subdirectory(CL2DFlex)
add_subdirectory(Camera)
add_subdirectory(VideoEncoder)
add_subdirectory(VideoDecoder)
if ( ENABLE_EVA )
add_subdirectory(OpticalFlow)
add_subdirectory(DepthFromStereo)
endif()
add_subdirectory(Voxelization)
if ( ENABLE_FADAS )
add_subdirectory(Remap)
endif()
if ( ENABLE_RADAR AND NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" )
add_subdirectory(Radar)
endif()
