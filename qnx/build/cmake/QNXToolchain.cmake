set(CMAKE_SYSTEM_NAME QNX)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(arch gcc_ntoaarch64le)

set(CMAKE_C_COMPILER qcc)
set(CMAKE_C_COMPILER_TARGET ${arch})

set(CMAKE_CXX_COMPILER qcc)
set(CMAKE_CXX_COMPILER_TARGET ${arch})

set( CMAKE_SYSROOT $ENV{QNX_TARGET} )

# common header files and libraries
include_directories( $ENV{BSP_ROOT}/install/usr/include )
include_directories( $ENV{BSP_ROOT}/install/aarch64le/usr/include )
add_link_options( "-L$ENV{BSP_ROOT}/install/aarch64le/lib" )
add_link_options( "-L$ENV{BSP_ROOT}/install/aarch64le/usr/lib" )
link_libraries( libstd slog2 socket )

# pmem
include_directories( $ENV{BSP_ROOT}/AMSS/inc )
link_libraries( pmem_client pmemext fastrpc_pmem mmap_peer OSAbstraction )

# apdf
include_directories( $ENV{BSP_ROOT}/install/usr/include/amss/multimedia/apdf/ )
include_directories( $ENV{QSDP_FIXME_ROOT}/target/qnx/usr/include )
include_directories( $ENV{QSDP_FIXME_ROOT}/target/qnx/usr/include/WF )
link_libraries( apdf aosal )

# c2d
if( ENABLE_C2D )
include_directories( $ENV{BSP_ROOT}/AMSS/multimedia/graphics/include/private/C2D/ )
include_directories( $ENV{BSP_ROOT}/AMSS/inc/graphics/include/private/C2D/ )
add_link_options( "-L$ENV{BSP_ROOT}/install/aarch64le/usr/lib/graphics/qc/" )
add_link_options( "-L$ENV{BSP_ROOT}/install_remote/aarch64le/lib" )
set( QC_C2D_LIBS c2d30 OSUser GSLUser )
endif()

# vidc
include_directories( $ENV{BSP_ROOT}/AMSS/multimedia/inc )
include_directories( $ENV{BSP_ROOT}/AMSS/multimedia/video/vendor/qcom/proprietary/video-driver/drivers/inc )
include_directories( $ENV{BSP_ROOT}/AMSS/multimedia/video/vendor/qcom/proprietary/video-driver/test/source/filedemux/Api/inc/ )
include_directories( $ENV{BSP_ROOT}/AMSS/multimedia/video/vendor/qcom/proprietary/video-driver/test/source/filedemux/FileBaseLib/inc/ )
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
include_directories( $ENV{BSP_ROOT}/AMSS/multimedia/qcamera/camera_qcx/cdk_qcx/api/qcarcam/ )
add_link_options( "-L$ENV{BSP_ROOT}/install/aarch64le/lib/camera_qcx/" )
link_libraries( xml2 )

# fadas
include_directories( $ENV{BSP_ROOT}/install/usr/include/amss/multimedia/fadas/ )
include_directories( $ENV{BSP_ROOT}/prebuilt/usr/include/amss/multimedia/fadas/ )

# rsm_v2
include_directories( $ENV{BSP_ROOT}/install/usr/include/amss )
include_directories( $ENV{BSP_ROOT}/prebuilt/usr/include/amss )
if( ENABLE_RSM_V2 )
add_compile_definitions( WITH_RSM_V2 )
link_libraries( rsm_client )
endif()

# OpenCL
include_directories( $ENV{BSP_ROOT}/AMSS/inc/graphics-fusa/include/public )
include_directories( $ENV{BSP_ROOT}/AMSS/multimedia/graphics-fusa-binaries/include/public )
add_compile_definitions( CL_TARGET_OPENCL_VERSION=300 )

# eva
include_directories( $ENV{BSP_ROOT}/install/usr/include/amss/multimedia/eva )

# gtest
include_directories( $ENV{QCNODE_INSTALL_DIR}/opt/qcnode/include )
add_link_options( "-L$ENV{QCNODE_INSTALL_DIR}/opt/qcnode/lib" )
link_libraries( regex )
