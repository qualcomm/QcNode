
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( COMMON_HEADERS ${HEADERS_DIR}/QC/common/Types.hpp )

set( COMMON_SOURCES
    Buffer.cpp
    BufferManager.cpp
    Image.cpp
    Tensor.cpp
    Logger.cpp
)

set( TARGET_LIBRARIES )

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "QNX" )
    list( APPEND COMMON_SOURCES Buffer_QNX.cpp Logger_QNX.cpp )
    list( APPEND TARGET_LIBRARIES pmem_client pmemext fastrpc_pmem apdf slog2 )
    list( APPEND HEADERS_DIR ${CMAKE_SYSROOT}/usr/include/WF )
endif()

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" AND "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "aarch64" )
    list( APPEND COMMON_SOURCES Buffer_Linux.cpp Logger_Linux.cpp )    
    list( APPEND TARGET_LIBRARIES kiumd apdf aosal pthread dl systemd )
    list( APPEND HEADERS_DIR ${CMAKE_SYSROOT}/usr/include/WF )
    list( APPEND HEADERS_DIR ${CMAKE_SYSROOT}/usr/include/linux-ark )
endif()

add_library( QCNodeCommon OBJECT ${COMMON_SOURCES} )

set_property( TARGET QCNodeCommon PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET QCNodeCommon PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( QCNodeCommon PUBLIC ${HEADERS_DIR} )

target_link_libraries( QCNodeCommon PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${COMMON_HEADERS} DESTINATION include/QC/common )
install( FILES ${PROJECT_SOURCE_DIR}/include/QC/infras/memory/BufferManager.hpp
               ${PROJECT_SOURCE_DIR}/include/QC/infras/memory/SharedBuffer.hpp
               DESTINATION include/QC/infras/memory )
install( FILES ${PROJECT_SOURCE_DIR}/include/QC/infras/logger/Logger.hpp DESTINATION include/QC/infras/logger )

