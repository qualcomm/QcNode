
set( HEADERS_DIR 
    ${PROJECT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include )

set ( BUFMGR_SRC BufferManager.cpp )

add_library( BufferManager STATIC ${BUFMGR_SRC} )

target_include_directories( BufferManager PUBLIC  ${HEADERS_DIR} )

set( TARGET_LIBRARIES QCNodeMemory )

set_property( TARGET BufferManager PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET BufferManager PROPERTY POSITION_INDEPENDENT_CODE ON )

target_link_libraries( BufferManager PUBLIC ${TARGET_LIBRARIES} )

