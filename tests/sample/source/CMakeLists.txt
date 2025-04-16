
set( HEADERS_DIR
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/source/libs/OpenclIface
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

set( QCNODE_SAMPLE_SOURCES
    SampleIF.cpp
    SampleC2D.cpp
    SamplePlrPost.cpp
    SamplePlrPre.cpp
    SamplePostProcCenternet.cpp
    SampleCamera.cpp
    SampleDataReader.cpp
    SampleRecorder.cpp
    SampleRemap.cpp
    SampleVideoDecoder.cpp
    SampleVideoEncoder.cpp
    SharedBufferPool.cpp
    SampleCL2DFlex.cpp
    SampleDataOnline.cpp
    SampleSharedRing.cpp
    SampleFpsAdapter.cpp
    SampleOpticalFlow.cpp
    SampleOpticalFlowViz.cpp
    SampleDepthFromStereo.cpp
    SampleDepthFromStereoViz.cpp
    SampleFrameSync.cpp
    SysTrace.cpp
)

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" AND "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "aarch64" )
    list( APPEND QCNODE_SAMPLE_SOURCES SampleGL2DFlex.cpp )
endif()

set( QCNODE_SAMPLE_DEFINITIONS )

if( NOT TARGET QCNode )
  find_package(QCNode)
  if( NOT TARGET QCNode )
    message(FATAL_ERROR "QCNode SDK not found")
  endif()
endif()

set( TARGET_LIBRARIES QCNode SharedRing )

if( NOT TARGET TinyViz )
  message(STATUS "build sample without TinyViz")
else()
  list( APPEND QCNODE_SAMPLE_SOURCES SampleTinyViz.cpp )
  list( APPEND TARGET_LIBRARIES TinyViz )
endif()

if( DEFINED ENV{QNN_SDK_ROOT} )
    set( QNN_SDK_ROOT $ENV{QNN_SDK_ROOT} )
endif()

if ( NOT DEFINED QNN_SDK_ROOT )
  message(STATUS "build sample without QnnRuntime")
else()
  list( APPEND QCNODE_SAMPLE_SOURCES SampleQnn.cpp )
endif()

if ( ENABLE_DEMUXER )
  list( APPEND QCNODE_SAMPLE_SOURCES SampleVideoDemuxer.cpp )
  list( APPEND TARGET_LIBRARIES VidcDemuxer )
else()
  message(STATUS "build sample without VideoDemuxer")
endif()

add_library( QCNodeSample SHARED ${QCNODE_SAMPLE_SOURCES} )

set_property( TARGET QCNodeSample PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET QCNodeSample PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( QCNodeSample PUBLIC ${HEADERS_DIR} )

target_link_libraries( QCNodeSample PUBLIC ${TARGET_LIBRARIES} OpenclIface )

target_compile_definitions( QCNodeSample PUBLIC ${QCNODE_SAMPLE_DEFINITIONS} )

install(TARGETS QCNodeSample DESTINATION lib)
