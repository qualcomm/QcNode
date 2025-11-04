
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( DFS_SOURCES DepthFromStereo.cpp )

list( APPEND TARGET_LIBRARIES QCNode evaEpl evaPlatform devioClient )

add_library( SampleDepthFromStereo OBJECT ${DFS_SOURCES} )

set_property( TARGET SampleDepthFromStereo PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET SampleDepthFromStereo PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( SampleDepthFromStereo PUBLIC ${HEADERS_DIR} )
target_include_directories( SampleDepthFromStereo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )

target_link_libraries( SampleDepthFromStereo PUBLIC ${TARGET_LIBRARIES} )

add_executable( gtest_DepthFromStereo gtest_DepthFromStereo.cpp )
target_link_libraries( gtest_DepthFromStereo gtest SampleDepthFromStereo QCNodeTestUtils BufferManager )
install(TARGETS gtest_DepthFromStereo DESTINATION bin)