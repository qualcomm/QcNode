set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )
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
add_subdirectory(Base)
add_subdirectory(QNN)
add_subdirectory(CL2DFlex)
add_subdirectory(Camera)
add_subdirectory(VideoEncoder)
add_subdirectory(VideoDecoder)
if ( ENABLE_EVA )
add_subdirectory(OpticalFlow)
add_subdirectory(DepthFromStereo)
endif()
if ( ENABLE_FADAS )
add_subdirectory(Remap)
add_subdirectory(Voxelization)
endif()
if ( ENABLE_RADAR AND NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" )
add_subdirectory(Radar)
endif()
