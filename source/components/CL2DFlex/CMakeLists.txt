
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( CL2DFlEX_HEADERS
    ${HEADERS_DIR}/ridehal/component/CL2DFlex.hpp
)

set( CL2DFLEX_SOURCES 
    CL2DPipelineBase.cpp
    CL2DPipelineConvert.cpp
    CL2DPipelineResize.cpp
    CL2DPipelineRemap.cpp
    CL2DPipelineLetterbox.cpp
    CL2DPipelineConvertUBWC.cpp
    CL2DPipelineResizeMultiple.cpp
    CL2DPipelineLetterboxMultiple.cpp
    CL2DFlex.cpp 
)

list( APPEND TARGET_LIBRARIES RideHalComponentCommon OpenclIface)
list( APPEND HEADERS_DIR ${PROJECT_SOURCE_DIR}/source/libs/OpenclIface )
list( APPEND HEADERS_DIR ${PROJECT_SOURCE_DIR}/source/components/CL2DFlex )
add_library( RideHalCL2DFlex OBJECT ${CL2DFLEX_SOURCES} )

set_property( TARGET RideHalCL2DFlex PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET RideHalCL2DFlex PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( RideHalCL2DFlex PUBLIC ${HEADERS_DIR} )

target_link_libraries( RideHalCL2DFlex PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${CL2DFlEX_HEADERS} DESTINATION include/ridehal/component )
