
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( POST_CENTER_POINT_SOURCES PostCenterPoint.cpp FadasPlrPostProc.cpp )

list( APPEND TARGET_LIBRARIES QCNode FadasIface BufferManager )

add_library( SamplePostCenterPoint OBJECT ${POST_CENTER_POINT_SOURCES} )

set_property( TARGET SamplePostCenterPoint PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET SamplePostCenterPoint PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( SamplePostCenterPoint PUBLIC ${HEADERS_DIR} )
target_include_directories( SamplePostCenterPoint PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )

target_link_libraries( SamplePostCenterPoint PUBLIC ${TARGET_LIBRARIES} )

add_executable( gtest_PostCenterPoint gtest_PostCenterPoint.cpp )
target_include_directories( gtest_PostCenterPoint PUBLIC ${HEADERS_DIR} )
target_link_libraries( gtest_PostCenterPoint gtest SamplePostCenterPoint QCNodeTestUtils )
install(TARGETS gtest_PostCenterPoint DESTINATION bin)