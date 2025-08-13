
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( TARGET_LIBRARIES QCNode )
list( APPEND TARGET_LIBRARIES EGL GLESv2)
if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" AND "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "aarch64" )
list( APPEND TARGET_LIBRARIES GLESv2_adreno drm gbm  )
endif()
list( APPEND HEADERS_DIR ${CMAKE_SYSROOT}/usr/include/drm )
list( APPEND HEADERS_DIR ${CMAKE_SYSROOT}/usr/include/libdrm )

set( SAMPLE_GL2DFLEX_SOURCES
     GL2DFlex.cpp
)

add_library( SampleGL2DFlex OBJECT ${SAMPLE_GL2DFLEX_SOURCES} )

set_property( TARGET SampleGL2DFlex PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET SampleGL2DFlex PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( SampleGL2DFlex PUBLIC ${HEADERS_DIR} )
target_include_directories( SampleGL2DFlex PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )

target_link_libraries( SampleGL2DFlex PUBLIC ${TARGET_LIBRARIES} )

add_executable( gtest_GL2DFlex gtest_GL2DFlex.cpp )
target_include_directories( gtest_GL2DFlex PUBLIC ${HEADERS_DIR} )
target_link_libraries( gtest_GL2DFlex gtest SampleGL2DFlex QCNodeTestUtils BufferManager )

install(TARGETS gtest_GL2DFlex DESTINATION bin)
