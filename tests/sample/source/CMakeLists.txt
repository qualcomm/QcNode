
set( HEADERS_DIR
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/source/Library/OpenclIface
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

set( QCNODE_SAMPLE_SOURCES
    SampleIF.cpp
    SamplePostProcCenternet.cpp
    SampleCamera.cpp
    SampleDataReader.cpp
    SampleRecorder.cpp
    SampleVideoDecoder.cpp
    SampleVideoEncoder.cpp
    SharedBufferPool.cpp
    SampleCL2DFlex.cpp
    SampleDataOnline.cpp
    SampleSharedRing.cpp
    SampleFpsAdapter.cpp
    SampleFrameSync.cpp
    SysTrace.cpp
    SampleGL2DFlex.cpp
    SampleTemporal.cpp
)

if ( ENABLE_EVA )
  list( APPEND QCNODE_SAMPLE_SOURCES
            SampleOpticalFlow.cpp
            SampleOpticalFlowViz.cpp
            SampleDepthFromStereo.cpp
            SampleDepthFromStereoViz.cpp )
endif()

if ( ENABLE_FADAS )
  list( APPEND QCNODE_SAMPLE_SOURCES
            SamplePlrPost.cpp
            SamplePlrPre.cpp
            SampleRemap.cpp )
endif()

if ( ENABLE_RADAR AND NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" )
  list( APPEND QCNODE_SAMPLE_SOURCES SampleRadar.cpp )
else()
  message(STATUS "build sample without Radar")
endif()

set( QCNODE_SAMPLE_DEFINITIONS )

if( NOT TARGET QCNode )
  find_package(QCNode)
  if( NOT TARGET QCNode )
    message(FATAL_ERROR "QCNode SDK not found")
  endif()
endif()

set( TARGET_LIBRARIES QCNode SharedRing BufferManager )

if( NOT TARGET TinyViz )
  message(STATUS "build sample without TinyViz")
else()
  list( APPEND QCNODE_SAMPLE_SOURCES SampleTinyViz.cpp )
  list( APPEND TARGET_LIBRARIES TinyViz )
endif()

if ( ENABLE_C2D )
  list( APPEND QCNODE_SAMPLE_SOURCES SampleC2D.cpp )
  list( APPEND TARGET_LIBRARIES SampleC2D )
endif()

list( APPEND TARGET_LIBRARIES SampleGL2DFlex )

if ( ENABLE_FADAS )
  list( APPEND TARGET_LIBRARIES SamplePostCenterPoint )
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

if ( ENABLE_RSM_V2 )
if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" )
  list( APPEND QCNODE_SAMPLE_DEFINITIONS WITH_RSM_V2 )
  list( APPEND TARGET_LIBRARIES rsm )
endif()
endif()

if ( ENABLE_C2C )
  list( APPEND QCNODE_SAMPLE_SOURCES SampleC2C.cpp )
  list( APPEND TARGET_LIBRARIES c2c )
endif()

add_library( QCNodeSample SHARED ${QCNODE_SAMPLE_SOURCES} )

set_property( TARGET QCNodeSample PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET QCNodeSample PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( QCNodeSample PUBLIC ${HEADERS_DIR} )

target_link_libraries( QCNodeSample PUBLIC ${TARGET_LIBRARIES} OpenclIface )

target_compile_definitions( QCNodeSample PUBLIC ${QCNODE_SAMPLE_DEFINITIONS} )

install(TARGETS QCNodeSample DESTINATION lib)
