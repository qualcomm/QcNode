
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( MEMORY_HEADERS
        ${HEADERS_DIR}/QC/Infras/Memory/BufferDescriptor.hpp
        ${HEADERS_DIR}/QC/Infras/Memory/ImageDescriptor.hpp
        ${HEADERS_DIR}/QC/Infras/Memory/TensorDescriptor.hpp
        ${HEADERS_DIR}/QC/Infras/Memory/CameraFrameDescriptor.hpp
        ${HEADERS_DIR}/QC/Infras/Memory/Ifs/QCMemoryAllocatorIfs.hpp
        ${HEADERS_DIR}/QC/Infras/Memory/Ifs/QCMemoryDefs.hpp
        ${HEADERS_DIR}/QC/Infras/Memory/Ifs/QCMemoryManagerIfs.hpp
        ${HEADERS_DIR}/QC/Infras/Memory/Ifs/QCMemoryPoolIfs.hpp
        ${HEADERS_DIR}/QC/Infras/Memory/Ifs/QCMemoryUtilsIfs.hpp
        ${HEADERS_DIR}/QC/Infras/Memory/HeapAllocator.hpp
        ${HEADERS_DIR}/QC/Infras/Memory/ManagerLocal.hpp
        ${HEADERS_DIR}/QC/Infras/Memory/UtilsBase.hpp
        ${HEADERS_DIR}/QC/Infras/Memory/VideoFrameDescriptor.hpp
)

set( MEMORY_SOURCES
    BufferDescriptor.cpp
    ImageDescriptor.cpp
    TensorDescriptor.cpp
    CameraFrameDescriptor.cpp
    ManagerLocal.cpp
    HeapAllocator.cpp
    UtilsBase.cpp
    VideoFrameDescriptor.cpp
)

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" AND "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "aarch64" )
    list( APPEND MEMORY_HEADERS  
        ${HEADERS_DIR}/QC/Infras/Memory/DMABUFFAllocator.hpp
        ${HEADERS_DIR}/QC/Infras/Memory/DMABUFFUtils.hpp 
    )
else()
    list( APPEND MEMORY_HEADERS  
        ${HEADERS_DIR}/QC/Infras/Memory/PMEMAllocator.hpp 
        ${HEADERS_DIR}/QC/Infras/Memory/PMEMUtils.hpp
    )
endif()

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" AND "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "aarch64" )
    list( APPEND MEMORY_SOURCES  
        DMABUFFAllocator.cpp
        DMABUFFUtils.cpp 
    )
else()
    list( APPEND MEMORY_SOURCES  
        PMEMAllocator.cpp 
        PMEMUtils.cpp
    )
endif()

set( TARGET_LIBRARIES QCNodeCommon )

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" AND "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "aarch64" )
    list( APPEND TARGET_LIBRARIES kiumd apdf aosal )
    list( APPEND HEADERS_DIR ${CMAKE_SYSROOT}/usr/include/WF )
    list( APPEND HEADERS_DIR ${CMAKE_SYSROOT}/usr/include/linux-ark )
endif()

add_library( QCNodeMemory OBJECT ${MEMORY_SOURCES} )

set_property( TARGET QCNodeMemory PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET QCNodeMemory PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( QCNodeMemory PUBLIC ${HEADERS_DIR} )

target_link_libraries( QCNodeMemory PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${MEMORY_HEADERS} DESTINATION include/QC/Infras/Memory )

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" )
target_include_directories( QCNodeMemory PUBLIC ${CMAKE_SYSROOT}/usr/include/mm-osal/include )
target_compile_definitions( QCNodeMemory PUBLIC _VIDC_LRH_LINUX_ )
endif()
