
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( TARGET_LIBRARIES QCNode )

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" AND "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "aarch64" )
    list( APPEND TARGET_LIBRARIES C2D2 )
    list( APPEND HEADERS_DIR ${CMAKE_SYSROOT}/usr/include/WF )
endif()

if( DEFINED QC_C2D_LIBS )
    list( APPEND TARGET_LIBRARIES ${QC_C2D_LIBS} )
endif()

set( SAMPLE_C2D_SOURCES
    C2D.cpp
)

add_library( SampleC2D OBJECT ${SAMPLE_C2D_SOURCES} )

set_property( TARGET SampleC2D PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET SampleC2D PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( SampleC2D PUBLIC ${HEADERS_DIR} )
target_include_directories( SampleC2D PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )

target_link_libraries( SampleC2D PUBLIC ${TARGET_LIBRARIES} )

add_executable( gtest_C2D gtest_C2D.cpp )
target_link_libraries( gtest_C2D gtest SampleC2D QCNodeTestUtils BufferManager )
install(TARGETS gtest_C2D DESTINATION bin)