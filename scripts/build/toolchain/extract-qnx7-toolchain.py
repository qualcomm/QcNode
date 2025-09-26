#!/usr/bin/env python3
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause


import sys
import os
import argparse
import traceback
import tarfile, tempfile
import distutils.dir_util
from shutil import copytree, rmtree, move, copy2
from pathlib import Path
from string import Template

##################################
# Main

if sys.version_info[0] < 3:
    sys.exit( "This script requires Python 3!" )

parser = argparse.ArgumentParser( description="Creates QNX toolchain for docker image from locally built QNX perforce tree." )
parser.add_argument( "-i", "--input",   help="directory that contains the locally built QNX perforce tree (parent dir of qnx_ap)" )
parser.add_argument( "-v", "--version",   help="the QNX toolchain version", default="QOS222" )
parser.add_argument( "-o", "--output",  default="toolchain", help="basename of output file" )
args = parser.parse_args()

if not args.input:
    parser.print_help(sys.stdout)
    sys.exit( "\nNo input directory provided!" )

if not os.path.exists( args.input ) or not os.path.isdir( args.input ):
    sys.exit( "The provided input directory: " + args.input + " does not exist!" )

inputDir = args.input

tcRootDirName = "qnx"
sdpString     = args.version
sdpVersion    = "7.1"

outputDir = args.output

def print_traceback_and_exit():
    print( '-'*45 )
    print( "Encountered an unexpected error: " )
    traceback.print_exc(file=sys.stdout)
    print( '-'*45 )
    sys.exit( "Exiting!" )

def printHeader( message ):
    print( '-'*65 )
    print( message )
    print( '-'*65 )

def make_symlink( src, dst):
    try:
        os.symlink( src, dst )
    except:
        print_traceback_and_exit()

def make_directory( dir ):
    try:
        os.makedirs( dir, exist_ok = True )
    except:
        print_traceback_and_exit()

def remove_tree( dir ):
    try:
        rmtree( dir )
    except:
        print_traceback_and_exit()

def remove_file( filePath ):
    try:
        os.remove( filePath )
    except:
        print_traceback_and_exit()

def remove_file_type( dir, ext ):
    try:
        for currDir, dirs, files in os.walk( dir ):
            for currFile in files:
                if( currFile.endswith( ext ) ):
                    os.remove( os.path.join( currDir, currFile ) )
    except:
        print_traceback_and_exit()

def copy_file( srcFile, dstFile ):
    try:
        print( "Copying " + srcFile + " to " + dstFile )
        copy2( srcFile, dstFile, follow_symlinks=False)
    except FileExistsError:
        # Same library can be listed multiple times
        return
    except:
        print_traceback_and_exit()

def copy_directory( srcDir, dstDir ):
    try:
        copytree( srcDir, dstDir, symlinks=True, ignore=None, copy_function=copy2, ignore_dangling_symlinks=True )
    except:
        print_traceback_and_exit()

# with higher python version we could use copytree( ..., dirs_exist_ok=True)
def merge_directories( srcDir, dstDir ):
    try:
        for currSrcDir, dirs, files in os.walk( srcDir ):
            currDstDir = currSrcDir.replace( srcDir, dstDir, 1 )
            if not os.path.exists( currDstDir ):
                os.makedirs( currDstDir )
            for currFile in files:
                srcFile = os.path.join( currSrcDir, currFile )
                dstFile = os.path.join( currDstDir, currFile )
                if os.path.exists( dstFile ):
                    os.remove( dstFile )
                copy2( srcFile, dstFile )
    except:
        print_traceback_and_exit()

def generate_env_file( envFilePath, sdpVersion ):
    with open( envFilePath, "wt") as textFile:
        textFile.write( '''#/bin/bash
#

QNX_HOME=/opt/qnx

export QNX_CONFIGURATION_EXCLUSIVE=$QNX_HOME
export QNX_HOST=$QNX_HOME/host/linux/x86_64
export QNX_TARGET=$QNX_HOME/target/qnx7

export PATH=$QNX_HOST/usr/bin:$PATH

export CC=aarch64-unknown-nto-qnx{0}.0-gcc
export CXX=aarch64-unknown-nto-qnx{0}.0-g++
export LD=aarch64-unknown-nto-qnx{0}.0-ld
export AR=aarch64-unknown-nto-qnx{0}.0-ar
export AS=aarch64-unknown-nto-qnx{0}.0-as
export NM=aarch64-unknown-nto-qnx{0}.0-nm
export RANLIB=aarch64-unknown-nto-qnx{0}.0-ranlib
export STRIP=aarch64-unknown-nto-qnx{0}.0-strip

SYSROOT=$QNX_TARGET
export PKG_CONFIG_SYSROOT_DIR=$SYSROOT
export PKG_CONFIG_LIBDIR=$SYSROOT/aarch64le/usr/lib/pkgconfig:$SYSROOT/usr/lib/pkgconfig:$SYSROOT/usr/share/pkgconfig
export CMAKE_TOOLCHAIN_FILE=$QNX_HOME/toolchain.cmake
export TOOLCHAIN_SYSROOT=$QNX_TARGET/aarch64le
'''.format(sdpVersion) )

def generate_toolchain_file( toolchainFilePath, sdpVersion ):
    with open( toolchainFilePath, "wt") as textFile:
        textFile.write( '''
set( CMAKE_SYSTEM_NAME QNX )
set( CMAKE_SYSTEM_PROCESSOR aarch64 )

set( arch gcc_ntoaarch64le )

set( CMAKE_C_COMPILER aarch64-unknown-nto-qnx{0}.0-gcc )
set( CMAKE_C_COMPILER_TARGET ${{arch}} )
set( CMAKE_CXX_COMPILER aarch64-unknown-nto-qnx{0}.0-g++ )
set( CMAKE_CXX_COMPILER_TARGET ${{arch}} )

set( CMAKE_SYSROOT $ENV{{QNX_TARGET}}/aarch64le/ )
'''.format(sdpVersion) )

print( "Using input directory: "  + inputDir )
print( "Using output directory: " + outputDir )
print( "Version: {} {}".format(sdpString, sdpVersion) )

tcRootPath = outputDir  + "/" + tcRootDirName
tcLibDir   = tcRootPath + "/target/qnx7/aarch64le/usr/lib"
tcIncDir   = tcRootPath + "/target/qnx7/aarch64le/usr/include"
srcLibDir  = inputDir   + "/qnx_ap/install/aarch64le/lib"
srcIncDir  = inputDir   + "/qnx_ap/install/usr/include"
srcPrebuiltIncDir = inputDir + "/qnx_ap/qnx_bins/prebuilt_" + sdpString + "/target/qnx7/usr/include"
srcPrebuiltLibDir = inputDir + "/qnx_ap/qnx_bins/prebuilt_" + sdpString + "/target/qnx7/aarch64le/usr/lib"
srcPatchIncDir = inputDir + "/qnx_ap/qnx_bins/prebuilt_" + sdpString + "_patches/target/qnx7/usr/include"

print( "Creating toolchain root directory: " + tcRootDirName + "\n" )
make_directory( tcRootPath )

print( "Updating environment file\n")
generate_env_file( tcRootPath + "/env.sh", sdpVersion )

print( "Updating toolchain file\n")
generate_toolchain_file( tcRootPath + "/toolchain.cmake", sdpVersion )

printHeader( "Copying essential toolchain files..." )
make_directory( tcRootPath + "/license" ) # workaround to prevent having to chmod (which needs sudo)
copyList = [ { "desc":"host files", "src":"qnx_ap/qnx_bins/prebuilt_" + sdpString + "/host", "dst":"host", "op":"copy" },
             { "desc":"target (aarch64le) files", "src":"qnx_ap/qnx_bins/prebuilt_" + sdpString + "/target/qnx7/aarch64le", "dst":"target/qnx7/aarch64le", "op":"copy" },
             { "desc":"target (etc) files", "src":"qnx_ap/qnx_bins/prebuilt_" + sdpString + "/target/qnx7/etc", "dst":"target/qnx7/etc", "op":"copy" },
             { "desc":"target (sbin) files", "src":"qnx_ap/qnx_bins/prebuilt_" + sdpString + "/target/qnx7/sbin", "dst":"target/qnx7/sbin", "op":"copy" },
             { "desc":"target (usr) files", "src":"qnx_ap/qnx_bins/prebuilt_" + sdpString + "/target/qnx7/usr", "dst":"target/qnx7/usr", "op":"copy" },
             { "desc":"license files", "src":"qnx_ap/qnx_bins/prebuilt_" + sdpString + "/target/qnx7/license", "dst":"license", "op":"merge" } ]

for item in copyList:
    srcDir = inputDir + '/' + item["src"]
    dstDir = tcRootPath + '/' + item["dst"]
    print( "Copying " + item["desc"] + "\n  source: " + srcDir + "\n  destination: " + dstDir + "\n" )
    if item["op"] == "copy":
        copy_directory( srcDir, dstDir )
    elif item["op"] == "merge":
        merge_directories( srcDir, dstDir )
    else:
        sys.exit( "Unsupported operation" )

remove_file_type( tcRootPath + "/host/linux/x86_64/usr/lib/python2.7", ".pyc" )

printHeader( "Creating symlinks and reorganizing structure..." )

make_directory( tcRootPath + "/target/qnx7/aarch64le/etc" )

moveList = [ { "desc":"etc files", "src":"target/qnx7/etc", "dst":"target/qnx7/aarch64le/etc", "symlink":"aarch64le/etc" },
             { "desc":"sbin files", "src":"target/qnx7/sbin", "dst":"target/qnx7/aarch64le/sbin", "symlink":"aarch64le/sbin" },
             { "desc":"usr files", "src":"target/qnx7/usr", "dst":"target/qnx7/aarch64le/usr", "symlink":"aarch64le/usr" } ]

for item in moveList:
    srcDir = tcRootPath + '/' + item["src"]
    dstDir = tcRootPath + '/' + item["dst"]
    print( "Moving " + item["desc"] + " from : " + srcDir + " to: " + dstDir )
    merge_directories( srcDir, dstDir )
    remove_tree( srcDir )
    print( "Creating symlink from: " + srcDir + " to: " + dstDir )
    make_symlink( item["symlink"], srcDir )

printHeader( "Copying additional libraries and headers..." )

make_directory( tcIncDir + "/amss" )
copy_file( srcIncDir + "/amss/pmem_id.h", tcIncDir + "/amss" )

make_directory( tcIncDir + "/fastcv" )
merge_directories( srcIncDir + "/amss/multimedia/fastcv", tcIncDir + "/fastcv" )
merge_directories( srcIncDir + "/amss/multimedia/openmax", tcIncDir )

print( "Creating FastCV -> fastcv symlink" )
make_symlink( "fastcv", tcIncDir + "/FastCV" )

merge_directories( srcIncDir + "/amss/multimedia/fadas", tcIncDir )

print( "Creating rt symlink (hack)\n" )
make_symlink( "libshutdown.a",  tcRootPath + "/target/qnx7/aarch64le/lib/librt.a" )

incList = [ "protected/pmem.h", "qcamera/camera_qcx/cdk_qcx/api/qcarcam/qcarcam.h", "qcamera/camera_qcx/cdk_qcx/api/qcarcam/qcarcam_types.h","qcamera/camera_qcx/cdk_qcx/api/qcarcam/qcarcam_diag_types.h" ]

for inc in incList:
    print( "Copying header file: " + inc + " to: " + tcIncDir )
    copied = False
    for srcFile in Path( inputDir ).rglob( inc ):
        copy_file( str( srcFile ), tcIncDir)
        copied = True
    if not copied:
        sys.exit("Failed to copy: " + inc )


# copy specific headers (search in tree might find wrong version if multiple files are present)
incList = [ srcIncDir + "/amss/core/comdef.h",
            srcIncDir + "/amss/core/com_dtypes.h",
            srcIncDir + "/amss/core/alltypes.h",
            srcIncDir + "/amss/core/Base.h",
            srcIncDir + "/amss/core/ProcessorBind.h",
            srcIncDir + "/amss/core/qnx_err.h",
            srcIncDir + "/amss/core/logger_utils.h",
            srcIncDir + "/amss/core/logger_codes.h",
            srcIncDir + "/amss/rsm_client.h",
            srcIncDir + "/amss/rsm_client_v2.h",
            srcIncDir + "/amss/multimedia/apdf/apdf.h",
            srcPatchIncDir + "/WF/wfd.h",
            srcPatchIncDir + "/WF/wfdext.h",
            srcPatchIncDir + "/WF/wfdext2.h",
            srcPrebuiltIncDir + "/screen/screen.h",
            inputDir  + "/qnx_ap/AMSS/multimedia/inc/ioctlClient.h",
            inputDir  + "/qnx_ap/AMSS/multimedia/video/source/common/drivers/inc/vidc_types.h",
            inputDir  + "/qnx_ap/AMSS/multimedia/video/source/common/drivers/codec/vidc/inc/vidc_api.h",
            inputDir  + "/qnx_ap/AMSS/inc/graphics/include/private/C2D/c2d2.h",
            inputDir  + "/qnx_ap/AMSS/inc/qgptp_helper.hpp",
            inputDir  + "/qnx_ap/AMSS/inc/AEEstd.h",
            inputDir  + "/qnx_ap/AMSS/inc/AEEVaList.h",
            inputDir  + "/qnx_ap/AMSS/inc/AEEStdDef.h",
            inputDir  + "/qnx_ap/AMSS/inc/AEEStdErr.h",
            inputDir  + "/qnx_ap/AMSS/inc/AEEalign.h",
            inputDir  + "/qnx_ap/AMSS/inc/fastrpc_api.h",
            inputDir  + "/qnx_ap/AMSS/inc/MMFile.h",
            inputDir  + "/qnx_ap/AMSS/inc/MMDebugMsg.h",
            inputDir  + "/qnx_ap/AMSS/inc/MMThread.h",
            inputDir  + "/qnx_ap/AMSS/inc/MMSignal.h",
            inputDir  + "/qnx_ap/AMSS/inc/MMCriticalSection.h",
            inputDir  + "/qnx_ap/AMSS/inc/MMTime.h",
            inputDir  + "/qnx_ap/AMSS/inc/MMTimer.h",
            inputDir  + "/qnx_ap/AMSS/inc/aosal_debug_msg.h",
            inputDir  + "/qnx_ap/AMSS/inc/aosal_error.h",
            inputDir  + "/qnx_ap/AMSS/inc/remote.h",
            inputDir  + "/qnx_ap/AMSS/inc/rpcmem.h",
            inputDir  + "/qnx_ap/AMSS/multimedia/eva/epl/public/amss/multimedia/eva/eva_mem.h",
            inputDir  + "/qnx_ap/AMSS/multimedia/eva/epl/public/amss/multimedia/eva/eva_of.h",
            inputDir  + "/qnx_ap/AMSS/multimedia/eva/epl/public/amss/multimedia/eva/eva_session.h",
            inputDir  + "/qnx_ap/AMSS/multimedia/eva/epl/public/amss/multimedia/eva/eva_types.h",
            inputDir  + "/qnx_ap/AMSS/multimedia/eva/epl/public/amss/multimedia/eva/eva_utils.h",
            inputDir  + "/qnx_ap/AMSS/multimedia/eva/epl/public/amss/multimedia/eva/eva_dfs.h",
            inputDir  + "/qnx_ap/AMSS/multimedia/video/source/common/drivers/inc/vidc_ioctl.h", 
            inputDir  + "/qnx_ap/AMSS/multimedia/common/source/qnx/aosal/inc/aosal_qnx_utils.h",
            inputDir  + "/qnx_ap/AMSS/platform/utilities/sysprofiler/sysprofiler.h",
            inputDir  + "/qnx_ap/AMSS/inc/c2c.h",
          ]

for inc in incList:
    print( "Copying header file: " + inc + " to: " + tcIncDir )
    copy_file( inc, tcIncDir)

# copy qcom OpenCL extension
copy_file(inputDir + '/qnx_ap/AMSS/multimedia/graphics-fusa-binaries/include/public/CL/cl_ext_qcom.h', tcIncDir + '/CL')

# multimedia header files
mm_video_path = inputDir + "/qnx_ap/test/multimedia/experimental/video/"
copy_file( mm_video_path + "source/filedemux/FileSource/inc/filesource.h", tcIncDir )
copy_file( mm_video_path + "source/filedemux/FileSource/inc/filesourcetypes.h", tcIncDir )
copy_file( mm_video_path + "source/filedemux/FileBaseLib/inc/parserinternaldefs.h", tcIncDir )

make_directory( tcIncDir + "/WF" )
copy_file( srcPatchIncDir + "/WF/wfdplatform.h", tcIncDir + "/WF" )

libListVidc  = [ "libvidc.so", "libFileSource.so", "libioctlClient.so", "libOSAbstraction.so", "libopenwfd.so", "libpool.so", "libss_drv_util.so", "libhwio.so.1", "libtzss_drv.so", "libssloader.so", "libioctlServer.so", "libpil_client.so" ]
libListC2d   = [ "libc2d30.so" ]
libListPmem  = [ "libpmem_client.so", "libpmemext.so" ]
libListQgptp = [ "libqgptp.so", "libdal.so", "libdalconfig.so.1" ]
libListFastADAS = [ "libfadas.so", "libfastrpc.so", "libfastrpc_pmem.so" ]
libListFastCV = [ "libfastcvopt.so", "libOSUser.so", "libGSLUser.so", "libOpenCL.so", "libcdsprpc.so", "libfastrpc.so", "libfastrpc_pmem.so", "libsmmu_client.so" ]
libListEva = [ "libclock_client.so", "libdevioClient.so", "libevaEpl.so", "libevaPlatform.so" ]
libListQcx = [ "libqcxclient.so", "libqcxosal.so" ]
libListRSM = [ "librsm_client.so" ]
libList = libListVidc + libListFastCV + libListC2d + libListPmem + libListQgptp + libListFastADAS + libListEva + libListQcx + [
        "libcommonUtils.so", "libioctlClient_shim.so", "libplanedef.so", "libfdt_utils.so.1", "libcdsprpcS.a", "libcdsprpc.so", "libapdf.so", "libaosal.so",
        "libfastrpc_pmem.so.1", "libfastcvopt.so.1",
        "libadreno_utils.so", "libESXGLESv2_Adreno.so", "libglnext-llvm.so", "libESXEGL_Adreno.so",
        "libsysprofiler.so", "libQProfilerInterface.so", "libfdt_utils.so", "libtzbsplib.so", "libtzbsplib.so.1",
        "libsmmu_clientS.a", "libfastcvopt.a", "libfastcvoptS.a",
        "liblibstd.so", "libmmap_peer.so", "libxml_config.so", "libOpenCL_Adreno.so",
        "libicb_client.so", "libnpa_client.so", "libc2c.so", "libep_client.so", "librc_client.so",
        "libsafe_xml.so", "libsafe_xml_c.so"
    ] + libListRSM

targetLibDirs = [
        inputDir + '/qnx_ap/install/aarch64le',
        inputDir + '/qnx_ap/AMSS/qaic/prebuilt/lib64'
    ]


for lib in libList:
    print( "Copying library/symbol file: " + lib + " to: " + tcLibDir )
    copied = False
    for libDir in targetLibDirs:
        if copied: break
        for srcFile in Path(libDir).rglob(lib):
            copy_file(str(srcFile), tcLibDir)
            copied = True
            break
    if not copied:
        sys.exit("Failed to copy: " + lib )

# multimedia dependent libraries
mm_build_path = mm_video_path + "build"
for root, dirs, files in os.walk( mm_build_path ):
    for name in files:
        file_item = os.path.join(root, name)
        ext_name = file_item.split('.')[-1]
        if ext_name == "so":
            copy_file( file_item, tcLibDir )

copy_file( srcPrebuiltLibDir + "/libWFD.so", tcLibDir )
copy_file( srcPrebuiltLibDir + "/libWFD.so.1", tcLibDir )

print( "\nFinished copying additional libraries and headers!\n" )

print( "Successfully created " + tcRootPath )
