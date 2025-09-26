set( CMAKE_SYSTEM_NAME QNX )
set( CMAKE_SYSTEM_PROCESSOR aarch64 )

set( arch gcc_ntoaarch64le )

set( CMAKE_C_COMPILER aarch64-unknown-nto-qnx8.0.0-gcc )
set( CMAKE_C_COMPILER_TARGET ${arch} )
set( CMAKE_CXX_COMPILER aarch64-unknown-nto-qnx8.0.0-g++ )
set( CMAKE_CXX_COMPILER_TARGET ${arch} )

set( CMAKE_SYSROOT $ENV{QNX_TARGET}/aarch64le/ )

# common header files and libraries
link_libraries( libstd slog2 socket )

# pmem
link_libraries( pmem_client pmemext fastrpc_pmem mmap_peer OSAbstraction )

# apdf
include_directories( $ENV{BSP_ROOT}/install/usr/include/amss/multimedia/apdf/ )
link_libraries( apdf aosal )

# c2d
add_link_options( "-L$ENV{BSP_ROOT}/install/aarch64le/usr/lib/graphics-fusa/qc" )
set( QC_C2D_LIBS c2d30 OSUser GSLUser planedef )

# vidc
include_directories( $ENV{BSP_ROOT}/install/usr/include/amss/multimedia/video )
set( QC_VIDC_LIBS ioctlClient )
set( QC_VIDC_FILEDEMUX_LIBS
    FileDemux_Common
    FileBaseLib
    FileSource
    dal
    dalconfig
    AACParserLib
    AC3ParserLib
    AMRNBParserLib
    AMRWBParserLib
    ASFParserLib
    AVIParserLib
    FileDemux_Common
    EVRCBParserLib
    EVRCWBParserLib
    FLACParserLib
    ID3Lib
    MP2ParserLib
    MP3ParserLib
    OGGParserLib
    QCPParserLib
    RawParserLib
    SeekLib
    SeekTableLib
    VideoFMTReaderLib
    WAVParserLib
    ISOBaseFileLib
    MKAVParserLib
    AIFFParserLib
)

# qcarcam
include_directories( $ENV{BSP_ROOT}/install/usr/include/amss/multimedia/camera_qcx )
add_link_options( "-L$ENV{BSP_ROOT}/install/aarch64le/lib/camera_qcx" )
link_libraries( xml2 )

# gtest
include_directories( $ENV{QCNODE_INSTALL_DIR}/opt/qcnode/include )
include_directories( $ENV{BSP_ROOT}/install/aarch64le/include )
add_link_options( "-L$ENV{QCNODE_INSTALL_DIR}/opt/qcnode/lib" )
add_link_options( "-L$ENV{BSP_ROOT}/install/aarch64le/lib" )
link_libraries( regex )

# fadas
include_directories( $ENV{BSP_ROOT}/install/usr/include/amss/multimedia/fadas )

add_compile_definitions( CL_TARGET_OPENCL_VERSION=300 )
