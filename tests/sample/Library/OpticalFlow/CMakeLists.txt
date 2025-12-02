
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( OFL_SOURCES OpticalFlow.cpp )

list( APPEND TARGET_LIBRARIES QCNode evaEpl evaPlatform devioClient )

add_library( SampleOpticalFlow OBJECT ${OFL_SOURCES} )

set_property( TARGET SampleOpticalFlow PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET SampleOpticalFlow PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( SampleOpticalFlow PUBLIC ${HEADERS_DIR} )
target_include_directories( SampleOpticalFlow PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )

target_link_libraries( SampleOpticalFlow PUBLIC ${TARGET_LIBRARIES} )

add_executable( gtest_OpticalFlow gtest_OpticalFlow.cpp )
target_link_libraries( gtest_OpticalFlow gtest SampleOpticalFlow QCNodeTestUtils BufferManager )
install(TARGETS gtest_OpticalFlow DESTINATION bin)