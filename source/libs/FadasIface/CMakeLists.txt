# FastADAS interface is supported only on aarch64 QNX targets

list( APPEND HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )
list( APPEND HEADERS_DIR ${PROJECT_SOURCE_DIR}/source/libs/FadasIface )
if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" )
    list( APPEND HEADERS_DIR  ${CMAKE_SYSROOT}/usr/include/amss/multimedia/fadas )
endif()

set( FADASIFACE_HEADERS
    FadasIface.h
    FadasSrv.hpp
    FadasRemap.hpp
    FadasPlr.hpp
)

add_library(FadasIfaceStub SHARED)
target_sources(FadasIfaceStub PRIVATE FadasIface.c FadasIface.h)
target_include_directories(FadasIfaceStub PUBLIC ${HEADERS_DIR})
target_compile_definitions(FadasIfaceStub PUBLIC VARIANT_le_ BUILDENV_qss)
target_link_libraries(FadasIfaceStub PUBLIC cdsprpc)
add_library(Ride::Hal::Libs::FadasIfaceStub ALIAS FadasIfaceStub)
install(TARGETS FadasIfaceStub DESTINATION lib)

add_library(FadasIface SHARED FadasSrv.cpp FadasRemap.cpp FadasPlr.cpp)
target_include_directories(FadasIface PUBLIC ${HEADERS_DIR})
target_compile_definitions(FadasIface PUBLIC USE_FADAS_DSP)
target_link_libraries(FadasIface PUBLIC FadasIfaceStub fadas)
add_library(Ride::Hal::Libs::FadasIface ALIAS FadasIface)
install(TARGETS FadasIface DESTINATION lib)

install(FILES prebuilt/dsp/v68/libFadasIface_skel.so DESTINATION lib/dsp/v68)
install(FILES prebuilt/dsp/v73/libFadasIface_skel.so DESTINATION lib/dsp/v73)
install(FILES prebuilt/dsp/v75/libFadasIface_skel.so DESTINATION lib/dsp/v75)
install(FILES prebuilt/dsp/v73/libFadasIface_skel.so DESTINATION lib/dsp)

install( FILES ${FADASIFACE_HEADERS} DESTINATION include/qcnode/libs/FadasIface )

