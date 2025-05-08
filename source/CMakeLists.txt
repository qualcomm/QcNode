
add_subdirectory(common)
add_subdirectory(components)
add_subdirectory(libs)
add_subdirectory(nodes)

set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

file( WRITE ${CMAKE_BINARY_DIR}/null.cpp "" )

set( QCNODE_SOURCES ${CMAKE_BINARY_DIR}/null.cpp )

set( TARGET_LIBRARIES
    QCNodeCommon
    QCNodeCamera
    QCNodeVideoEncoder
    QCNodeVideoDecoder
    QCNodeCL2DFlex
    QCNodeGL2DFlex
    QCNodeNodeBase
    QCNodeNodeCL2DFlex
    QCNodeNodeCamera
)

if ( ENABLE_C2D )
  list( APPEND TARGET_LIBRARIES QCNodeC2D )
endif()

if ( ENABLE_EVA )
  list( APPEND TARGET_LIBRARIES
            QCNodeOpticalFlow
            QCNodeDepthFromStereo
  )
endif()

if ( ENABLE_FADAS )
  list( APPEND TARGET_LIBRARIES
            QCNodeRemap
            QCNodePostCenterPoint
            QCNodeVoxelization
  )
endif()

if( NOT TARGET QCNodeQnnRuntime )
  message(STATUS "build QCNode without QNN")
else()
  list( APPEND TARGET_LIBRARIES QCNodeQnnRuntime QCNodeNodeQNN )
endif()

add_library( QCNode SHARED ${QCNODE_SOURCES} )

set_property( TARGET QCNode PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET QCNode PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( QCNode PUBLIC ${HEADERS_DIR} )

target_link_libraries( QCNode PUBLIC ${TARGET_LIBRARIES} )

install( TARGETS QCNode DESTINATION lib )
