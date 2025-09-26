set( CMAKE_SYSTEM_NAME Linux )
set( CMAKE_SYSTEM_PROCESSOR aarch64 )

set( arch gcc_hgyaarch64le )

set( CMAKE_C_COMPILER aarch64-linux-gnu-gcc )
set( CMAKE_C_COMPILER_TARGET ${arch} )
set( CMAKE_CXX_COMPILER aarch64-linux-gnu-g++ )
set( CMAKE_CXX_COMPILER_TARGET ${arch} )

set( CMAKE_SYSROOT $ENV{UBUNTU_TARGET} )

# workaround to fix link issue of standard libs
add_link_options( "-L$ENV{UBUNTU_TARGET}/usr/lib/aarch64-linux-gnu" )
# add_link_options( "-L$ENV{UBUNTU_TARGET}/lib/aarch64-linux-gnu" )
# add_link_options( "-L$ENV{UBUNTU_TARGET}/usr/lib/gcc-cross/aarch64-linux-gnu/10" )
# add_link_options( "-B$ENV{UBUNTU_TARGET}/usr/aarch64-linux-gnu/lib" )
# add_link_options( "-B$ENV{UBUNTU_TARGET}/usr/lib/gcc-cross/aarch64-linux-gnu/10" )

# workaround to fix include issues
# include_directories( $ENV{UBUNTU_TARGET}/usr/include/c++/10 )
# include_directories( $ENV{UBUNTU_TARGET}/usr/include/c++/10/aarch64-linux-gnu )
if(EXISTS $ENV{UBUNTU_TARGET}/usr/include/linux-ark)
include_directories( $ENV{UBUNTU_TARGET}/usr/include/linux-ark)
else()
include_directories( $ENV{UBUNTU_TARGET}/usr/include/linux-qcom-custom-rt)
endif()

include_directories( $ENV{QCNODE_INSTALL_DIR}/opt/qcnode/include )
add_link_options( "-L$ENV{QCNODE_INSTALL_DIR}/opt/qcnode/lib" )

