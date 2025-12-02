
set( TEST_UTILS_SOURCES
    md5_utils.cpp
)

set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/tests/utils )

set( TARGET_LIBRARIES ssl crypto )

add_library( QCNodeTestUtils STATIC ${TEST_UTILS_SOURCES} )

set_property( TARGET QCNodeTestUtils PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET QCNodeTestUtils PROPERTY POSITION_INDEPENDENT_CODE ON )

target_link_libraries( QCNodeTestUtils PUBLIC ${TARGET_LIBRARIES} )

target_include_directories( QCNodeTestUtils PUBLIC ${HEADERS_DIR})

add_executable( QCNodeMD5 md5_utils.cpp )
target_link_libraries( QCNodeMD5 ${TARGET_LIBRARIES} )
target_compile_definitions(QCNodeMD5 PUBLIC MD5_UTILS_TEST)
install(TARGETS QCNodeMD5 DESTINATION bin)

if( NOT ENABLE_CTC )
add_library( QCNodeTestMockCLib STATIC MockCLib.cpp )
target_include_directories( QCNodeTestMockCLib PUBLIC ${HEADERS_DIR})
endif()
