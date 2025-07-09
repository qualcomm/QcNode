
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( MEMORY_HEADERS
        ${HEADERS_DIR}/QC/Infras/Memory/BufferDescriptor.hpp
        ${HEADERS_DIR}/QC/Infras/Memory/ImageDescriptor.hpp
        ${HEADERS_DIR}/QC/Infras/Memory/TensorDescriptor.hpp
)

set( MEMORY_SOURCES
    BufferDescriptor.cpp
    ImageDescriptor.cpp
    TensorDescriptor.cpp
)

set( TARGET_LIBRARIES QCNodeCommon )

add_library( QCNodeMemory OBJECT ${MEMORY_SOURCES} )

set_property( TARGET QCNodeMemory PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET QCNodeMemory PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( QCNodeMemory PUBLIC ${HEADERS_DIR} )

target_link_libraries( QCNodeMemory PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${MEMORY_HEADERS} DESTINATION include/QC/Infras/Memory )
