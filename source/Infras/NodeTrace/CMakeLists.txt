
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( NODETRACE_SOURCES
    NodeTrace.cpp
)

add_library( QCNodeTrace OBJECT ${NODETRACE_SOURCES} )

set_property( TARGET QCNodeTrace PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET QCNodeTrace PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( QCNodeTrace PUBLIC ${HEADERS_DIR} )

target_link_libraries( QCNodeTrace PUBLIC ${TARGET_LIBRARIES} )
target_compile_definitions( QCNodeTrace PUBLIC QC_ENABLE_NODETRACE )

target_compile_options( QCNodeTrace PRIVATE -Wno-error=deprecated-declarations )

install( FILES ${HEADERS_DIR}/QC/Infras/NodeTrace/Ifs/QCNodeTraceIfs.hpp DESTINATION include/QC/Infras/NodeTrace/Ifs/ )
install( FILES ${HEADERS_DIR}/QC/Infras/NodeTrace/NodeTrace.hpp DESTINATION include/QC/Infras/NodeTrace/ )
