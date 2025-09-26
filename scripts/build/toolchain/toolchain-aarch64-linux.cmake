set( CMAKE_SYSTEM_NAME Linux )
set( CMAKE_SYSTEM_PROCESSOR aarch64 )

set( arch gcc_hgyaarch64le )

set( CMAKE_C_COMPILER aarch64-oe-linux-gcc )
set( CMAKE_C_COMPILER_TARGET ${arch} )
set( CMAKE_CXX_COMPILER aarch64-oe-linux-g++ )
set( CMAKE_CXX_COMPILER_TARGET ${arch} )

set( CMAKE_SYSROOT $ENV{LINUX_TARGET} )

include_directories( $ENV{QCNODE_INSTALL_DIR}/opt/qcnode/include )
add_link_options( "-L$ENV{QCNODE_INSTALL_DIR}/opt/qcnode/lib" )
