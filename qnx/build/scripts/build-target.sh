#! /bin/bash
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# All rights reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.

set -xe

homedir=$(dirname $0)
homedir=$(realpath $homedir/..)

# Toplevel source dir
topdir=$(realpath $1)

# Target to build
target=aarch64-qnx

# Build directory
workdir=$homedir/tmp/bld-$target

# Install / runtime directory
destdir=$homedir/tmp/run-$target

export QCNODE_INSTALL_DIR=$destdir

# Package name
pkgname=$homedir/tmp/qcnode-$target.tar.gz

# set up env
if [ "$version" == "700" ] ; then
  export CC=aarch64-unknown-nto-qnx7.0.0-gcc
  export CXX=aarch64-unknown-nto-qnx7.0.0-g++
  export LD=aarch64-unknown-nto-qnx7.0.0-ld
  export AR=aarch64-unknown-nto-qnx7.0.0-ar
  export AS=aarch64-unknown-nto-qnx7.0.0-as
  export NM=aarch64-unknown-nto-qnx7.0.0-nm
  export RANLIB=aarch64-unknown-nto-qnx7.0.0-ranlib
  export STRIP=aarch64-unknown-nto-qnx7.0.0-strip
fi

if [ "$version" == "710" ] ; then
  export CC=aarch64-unknown-nto-qnx7.1.0-gcc
  export CXX=aarch64-unknown-nto-qnx7.1.0-g++
  export LD=aarch64-unknown-nto-qnx7.1.0-ld
  export AR=aarch64-unknown-nto-qnx7.1.0-ar
  export AS=aarch64-unknown-nto-qnx7.1.0-as
  export NM=aarch64-unknown-nto-qnx7.1.0-nm
  export RANLIB=aarch64-unknown-nto-qnx7.1.0-ranlib
  export STRIP=aarch64-unknown-nto-qnx7.1.0-strip
fi

if [ "$version" == "800" ] ; then
  export CC=aarch64-unknown-nto-qnx8.0.0-gcc
  export CXX=aarch64-unknown-nto-qnx8.0.0-g++
  export LD=aarch64-unknown-nto-qnx8.0.0-ld
  export AR=aarch64-unknown-nto-qnx8.0.0-ar
  export AS=aarch64-unknown-nto-qnx8.0.0-as
  export NM=aarch64-unknown-nto-qnx8.0.0-nm
  export RANLIB=aarch64-unknown-nto-qnx8.0.0-ranlib
  export STRIP=aarch64-unknown-nto-qnx8.0.0-strip
fi

export CMAKE_TOOLCHAIN_FILE=$homedir/cmake/QNXToolchain.cmake
export TOOLCHAIN_SYSROOT=$BSP_ROOT/install/aarch64le

if ! [[ -v QC_TARGET_SOC ]] ; then
  export QC_TARGET_SOC=8797
fi

if ! [[ -v ENABLE_TINYVIZ ]] ; then
  export ENABLE_TINYVIZ=OFF
fi

if ! [[ -v ENABLE_GCOV ]] ; then
  export ENABLE_GCOV=OFF
fi

if ! [[ -v ENABLE_C2D ]] ; then
  export ENABLE_C2D=OFF
fi

if ! [[ -v ENABLE_EVA ]] ; then
  export ENABLE_EVA=OFF
fi

if ! [[ -v ENABLE_DEMUXER ]] ; then
  export ENABLE_DEMUXER=OFF
fi

if ! [[ -v ENABLE_FADAS ]] ; then
  export ENABLE_FADAS=OFF
fi

if ! [[ -v ENABLE_RSM_V2 ]] ; then
  export ENABLE_RSM_V2=OFF
fi

# Get dependent packages and build it
export THIRD_PARTY_DIR=$homedir/tmp/third_party
if [ ! -d $THIRD_PARTY_DIR ] ; then
  mkdir -p $THIRD_PARTY_DIR
  if [ -d $homedir/third_party ] ; then
    cp -frv $homedir/third_party/* $homedir/tmp/third_party
  fi
fi
$homedir/scripts/get-3rd-party.sh
$homedir/scripts/build-3rd-party-$target.sh $workdir $destdir

mkdir -p $workdir && cd $workdir || exit -1
cmake \
  -DCMAKE_TOOLCHAIN_FILE=$CMAKE_TOOLCHAIN_FILE \
  -DCMAKE_INCLUDE_PATH=$TOOLCHAIN_SYSROOT/usr/include \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_INSTALL_PREFIX=/opt/qcnode \
  -DCMAKE_PREFIX_PATH=$destdir/opt/qcnode \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DENABLE_GCOV=${ENABLE_GCOV} \
  -DENABLE_C2D=${ENABLE_C2D} \
  -DENABLE_EVA=${ENABLE_EVA} \
  -DENABLE_DEMUXER=${ENABLE_DEMUXER} \
  -DENABLE_FADAS=${ENABLE_FADAS} \
  -DENABLE_RSM_V2=${ENABLE_RSM_V2} \
  -DQC_TARGET_SOC=${QC_TARGET_SOC} \
  $topdir || exit -1
make -j 16 || exit -1
# Install the QC SDK
make DESTDIR=$destdir install || exit -1

if [[ -v QNN_SDK_ROOT ]] ; then
# Install QNN Runtime dependencies
  cp -vf $QNN_SDK_ROOT/bin/aarch64-qnx/* $destdir/opt/qcnode/bin
  cp -vf $QNN_SDK_ROOT/lib/aarch64-qnx/libQnn* $destdir/opt/qcnode/lib
  mkdir -p $destdir/opt/qcnode/lib/dsp
  cp -vf $QNN_SDK_ROOT/lib/hexagon-v73/unsigned/libQnn* $destdir/opt/qcnode/lib/dsp
  cp -vf $QNN_SDK_ROOT/lib/hexagon-v75/unsigned/libQnn* $destdir/opt/qcnode/lib/dsp
fi

if [ "$ENABLE_TINYVIZ" == "ON" ] ; then
if [ ! -f LiberationSans-Regular.ttf ]; then
  unzip $THIRD_PARTY_DIR/liberation_sans.zip
fi
mkdir -p $destdir/opt/qcnode/lib/runtime
cp LiberationSans-Regular.ttf $destdir/opt/qcnode/lib/runtime
fi

# Create run-time package
echo "Generating $pkgname"
cd $homedir/tmp
tar --xform="s/run/pkg/" --exclude="*.a" \
  --exclude="*.la" --exclude="include" --exclude="share" \
  --exclude="cmake" --exclude="pkgconfig" --exclude="sdl2-config" \
  -cf $pkgname run-$target

# install to rootfs
QC_INSTALL_ROOT=$INSTALL_ROOT_nto/aarch64le/bin/qcnode

echo "Installing to $QC_INSTALL_ROOT"
mkdir -p $QC_INSTALL_ROOT
cp -frv run-$target/opt/qcnode/bin $QC_INSTALL_ROOT/
cp -frv run-$target/opt/qcnode/lib $QC_INSTALL_ROOT/
