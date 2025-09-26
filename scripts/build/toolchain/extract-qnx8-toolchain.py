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
parser.add_argument( "-v", "--version",   help="the QNX toolchain version", default="SDP800" )
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
sdpVersion    = "8.0"

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
export QNX_TARGET=$QNX_HOME/target/qnx

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

print( "Using input directory: "  + inputDir )
print( "Using output directory: " + outputDir )
print( "Version: {} {}".format(sdpString, sdpVersion) )

tcRootPath = outputDir  + "/" + tcRootDirName
tcLibDir   = tcRootPath + "/target/qnx/aarch64le/usr/lib"
tcIncDir   = tcRootPath + "/target/qnx/usr/include"
srcLibDir  = inputDir   + "/qnx_ap/install/aarch64le/lib"
srcIncDir  = inputDir   + "/qnx_ap/install/usr/include"
srcPrebuiltIncDir = inputDir + "/qnx_ap/qnx_bins/prebuilt_" + sdpString + "/target/qnx/usr/include"
srcPrebuiltLibDir = inputDir + "/qnx_ap/qnx_bins/prebuilt_" + sdpString + "/target/qnx/aarch64le/usr/lib"
srcPatchIncDir = inputDir + "/qnx_ap/qnx_bins/prebuilt_" + sdpString + "_patches/target/qnx/usr/include"

print( "Creating toolchain root directory: " + tcRootDirName + "\n" )
make_directory( tcRootPath )

print( "Updating environment file\n")
generate_env_file( tcRootPath + "/env.sh", sdpVersion )

printHeader( "Copying essential toolchain files..." )
copyList = [ { "desc":"host files", "src":"qnx_ap/qnx_bins/prebuilt_" + sdpString + "/host", "dst":"host", "op":"copy" },
             { "desc":"target files", "src":"qnx_ap/qnx_bins/prebuilt_" + sdpString + "/target", "dst":"target", "op":"copy" },
             { "desc":"license files", "src":"qnx_ap/qnx_bins/prebuilt_" + sdpString + "_patches/license", "dst":"license", "op":"copy" } ]

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

printHeader( "Copying additional libraries and headers..." )

make_directory( tcIncDir + "/amss" )
copy_file( srcIncDir + "/amss/pmem_id.h", tcIncDir + "/amss" )

merge_directories( srcIncDir + "/amss/multimedia/fadas", tcIncDir )

# copy specific headers (search in tree might find wrong version if multiple files are present)
incList = [ inputDir  + "/qnx_ap/AMSS/inc/AEEStdDef.h",
            inputDir  + "/qnx_ap/AMSS/inc/pmem.h",
            inputDir  + "/qnx_ap/qnx_bins/prebuilt_SDP800_patches/target/qnx/usr/include/WF/wfd.h",
            inputDir  + "/qnx_ap/qnx_bins/prebuilt_SDP800_patches/target/qnx/usr/include/WF/wfdext2.h",
            inputDir  + "/qnx_ap/AMSS/inc/MMTimer.h",
            inputDir  + "/qnx_ap/AMSS/inc/MMTime.h",
            inputDir  + "/qnx_ap/AMSS/inc/AEEstd.h",
            inputDir  + "/qnx_ap/AMSS/inc/AEEVaList.h",
            inputDir  + "/qnx_ap/AMSS/multimedia/inc/ioctlClient.h",
            inputDir  + "/qnx_ap/AMSS/inc/fastrpc_api.h",
            inputDir  + "/qnx_ap/AMSS/inc/rpcmem.h",
            inputDir  + "/qnx_ap/AMSS/inc/remote.h",
            inputDir  + "/qnx_ap/AMSS/inc/AEEStdErr.h",
            inputDir  + "/qnx_ap/AMSS/pcie_c2c/vendor/qcom/proprietary/pcie-c2c/c2clib/public/c2c.h",
            inputDir  + "/qnx_ap/AMSS/inc/graphics-fusa/include/private/C2D/c2d2.h",
            srcIncDir + "/amss/multimedia/apdf/apdf.h",
            srcIncDir + "/amss/multimedia/camera_qcx/qcarcam.h",
            srcIncDir + "/amss/multimedia/camera_qcx/qcarcam_types.h",
            srcIncDir + "/amss/multimedia/video/vidc_ioctl.h",
            srcIncDir + "/amss/multimedia/video/vidc_types.h"
          ]

for inc in incList:
    print( "Copying header file: " + inc + " to: " + tcIncDir )
    copy_file( inc, tcIncDir)

# copy qcom OpenCL extension
copy_file(inputDir + '/qnx_ap/AMSS/multimedia/graphics-fusa-binaries/include/public/CL/cl_ext_qcom.h', tcIncDir + '/CL')

# multimedia header files
mm_video_path = inputDir + "/qnx_ap/AMSS/multimedia/video/vendor/qcom/proprietary/video-driver/test/"
copy_file( mm_video_path + "source/filedemux/Api/inc/filesource.h", tcIncDir )
copy_file( mm_video_path + "source/filedemux/Api/inc/filesourcetypes.h", tcIncDir )
copy_file( mm_video_path + "source/filedemux/FileBaseLib/inc/parserinternaldefs.h", tcIncDir )

merge_directories( inputDir + "/qnx_ap/install/aarch64le/usr/lib/graphics-fusa/qc", tcLibDir )

libListVidc  = [ "libioctlClient.so", "libOSAbstraction.so" ]
libListDemux = [ "libFileDemux_Common.so",
            "libFileBaseLib.so",
            "libFileSource.so",
            "libdal.so",
            "libdalconfig.so",
            "libdalconfig.so.1",
            "libAACParserLib.so",
            "libAC3ParserLib.so",
            "libAMRNBParserLib.so",
            "libAMRWBParserLib.so",
            "libASFParserLib.so",
            "libAVIParserLib.so",
            "libFileDemux_Common.so",
            "libEVRCBParserLib.so",
            "libEVRCWBParserLib.so",
            "libFLACParserLib.so",
            "libID3Lib.so",
            "libMP2ParserLib.so",
            "libMP3ParserLib.so",
            "libOGGParserLib.so",
            "libQCPParserLib.so",
            "libRawParserLib.so",
            "libSeekLib.so",
            "libSeekTableLib.so",
            "libVideoFMTReaderLib.so",
            "libWAVParserLib.so",
            "libISOBaseFileLib.so",
            "libMKAVParserLib.so",
            "libAIFFParserLib.so",
]
libListPmem  = [ "libpmem_client.so", "libpmemext.so" ]
libListFastADAS = [ "libfadas.so", "libfastrpc.so", "libfastrpc_pmem.so", "libfastrpc_pmem.so.1" ]
libListQcx = [ "libqcxclient.so", "libqcxosal.so" ]
libList = libListVidc + libListPmem + libListFastADAS + libListQcx + [
        "libplanedef.so", "libcdsprpc.so", "libapdf.so", "libaosal.so", "libfastrpc_pmem.so",
        "liblibstd.so", "libmmap_peer.so", "libOSAbstraction.so"
    ] + libListDemux
targetLibDirs = [
        inputDir + '/qnx_ap/install/aarch64le/lib',
        inputDir + '/qnx_ap/install/aarch64le/lib',
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

print( "\nFinished copying additional libraries and headers!\n" )

print( "Successfully created " + tcRootPath )
