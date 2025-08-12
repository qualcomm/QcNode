
set( HEADERS_DIR 
    ${PROJECT_SOURCE_DIR}/include
    ${CMAKE_PREFIX_PATH}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include )

set ( VIDCDEMUX_SRC VidcDemuxer.cpp )

add_library( VidcDemuxer STATIC ${VIDCDEMUX_SRC} )

target_include_directories( VidcDemuxer PUBLIC  ${HEADERS_DIR} )

set( TARGET_LIBRARIES QCNode )

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" )
    list( APPEND TARGET_LIBRARIES filedemux )
endif()

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" )
    target_include_directories( VidcDemuxer PUBLIC ${CMAKE_SYSROOT}/usr/include/mm-osal/include )
    target_compile_definitions( VidcDemuxer PUBLIC _VIDC_LRH_LINUX_ )
endif()

if( DEFINED QC_VIDC_FILEDEMUX_LIBS )
    list( APPEND TARGET_LIBRARIES ${QC_VIDC_FILEDEMUX_LIBS} )
endif()

set_property( TARGET VidcDemuxer PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET VidcDemuxer PROPERTY POSITION_INDEPENDENT_CODE ON )

target_link_libraries( VidcDemuxer PUBLIC ${TARGET_LIBRARIES} )


