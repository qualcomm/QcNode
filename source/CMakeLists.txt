
add_subdirectory(common)
add_subdirectory(components)
add_subdirectory(libs)

set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

file( WRITE ${CMAKE_BINARY_DIR}/null.cpp "" )

set( QCNODE_SOURCES ${CMAKE_BINARY_DIR}/null.cpp )

set( TARGET_LIBRARIES
    QCNodeCommon
    QCNodeCamera
    QCNodeRemap
    QCNodeVideoEncoder
    QCNodeVideoDecoder
    QCNodeVoxelization
    QCNodePostCenterPoint
    QCNodeCL2DFlex
    QCNodeGL2DFlex
)

if ( ENABLE_C2D )
  list( APPEND TARGET_LIBRARIES QCNodeC2D )
endif()

if ( ENABLE_EVA )
  list( APPEND TARGET_LIBRARIES QCNodeOpticalFlow QCNodeDepthFromStereo )
endif()

if( NOT TARGET QCNodeQnnRuntime )
  message(STATUS "build QCNode without QNN")
else()
  list( APPEND TARGET_LIBRARIES QCNodeQnnRuntime )
endif()

add_library( QCNode SHARED ${QCNODE_SOURCES} )

set_property( TARGET QCNode PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET QCNode PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( QCNode PUBLIC ${HEADERS_DIR} )

target_link_libraries( QCNode PUBLIC ${TARGET_LIBRARIES} )

install( TARGETS QCNode DESTINATION lib )
