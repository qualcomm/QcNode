#!/bin/sh
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause

set -xe

homedir=$(dirname $0)
homedir=$(realpath $homedir)

# Target to build
target=$1

# Toplevel build dir
topdir=$(realpath $2)

# Linux kernel variant: rh, oe
if [ $# -eq 3 ] ; then
variant=$3
else
variant=oe
fi

# Build directory
workdir=$topdir/bld-$target

# Install / runtime directory
destdir=$topdir/run-$target
export QCNODE_INSTALL_DIR=$destdir

# Package name
pkgname=$topdir/qcnode-$target.tar.gz

if ! [[ -v QC_TARGET_SOC ]] ; then
  export QC_TARGET_SOC=8797
fi

if ! [[ -v ENABLE_TINYVIZ ]] ; then
  export ENABLE_TINYVIZ=ON
fi

if ! [[ -v ENABLE_GCOV ]] ; then
  export ENABLE_GCOV=OFF
fi

if ! [[ -v ENABLE_C2D ]] ; then
  if [[ "${QC_TARGET_SOC}" == "8797" ]] ; then
    export ENABLE_C2D=OFF
  else
    export ENABLE_C2D=ON
  fi
fi

if ! [[ -v ENABLE_EVA ]] ; then
  if [[ "${QC_TARGET_SOC}" == "8797" ]] ; then
    export ENABLE_EVA=OFF
  else
    export ENABLE_EVA=ON
  fi
fi

if ! [[ -v ENABLE_DEMUXER ]] ; then
  export ENABLE_DEMUXER=OFF
fi

if ! [[ -v ENABLE_FADAS ]] ; then
  export ENABLE_FADAS=ON
fi

if ! [[ -v ENABLE_RSM_V2 ]] ; then
  if [[ "${QC_TARGET_SOC}" == "8797" ]] ; then
    export ENABLE_RSM_V2=OFF
  else
    export ENABLE_RSM_V2=ON
  fi
fi

if ! [[ -v ENABLE_C2C ]] ; then
  if [[ "${QC_TARGET_SOC}" == "8797" ]] ; then
    export ENABLE_C2C=OFF
  elif [[ "${target}" == "aarch64-qnx" ]]; then
    export ENABLE_C2C=ON
  else
    export ENABLE_C2C=OFF
  fi
fi

# Get dependent packages
export THIRD_PARTY_DIR=$topdir/third_party
sh $topdir/scripts/build/toolchain/get-3rd-party.sh

# Get QC Toolchain path
export QC_TOOLCHAIN_PATH=/opt/toolchain

setup_env_qnx() {
    if [[ -v BSP_ROOT ]]; then
        if [[ "${QC_TARGET_SOC}" == "8797" ]] ; then
            QNX_VERSION=qnx8.0.0
        else
            QNX_VERSION=qnx7.1.0
        fi
        echo build with QNX CRM
        echo BSP_ROOT: $BSP_ROOT
        echo QNX_ROOT: $QNX_ROOT
        echo QNX_TARGET: $QNX_TARGET
        echo QNX_HOST: $QNX_HOST
        export PATH=$QNX_HOST/usr/bin:$PATH
        export CC=aarch64-unknown-nto-${QNX_VERSION}-gcc
        export CXX=aarch64-unknown-nto-${QNX_VERSION}-g++
        export LD=aarch64-unknown-nto-${QNX_VERSION}-ld
        export AR=aarch64-unknown-nto-${QNX_VERSION}-ar
        export AS=aarch64-unknown-nto-${QNX_VERSION}-as
        export NM=aarch64-unknown-nto-${QNX_VERSION}-nm
        export RANLIB=aarch64-unknown-nto-${QNX_VERSION}-ranlib
        export STRIP=aarch64-unknown-nto-${QNX_VERSION}-strip
        export TOOLCHAIN_SYSROOT=$BSP_ROOT/install/aarch64le
    else
        if [ ! -d /opt/qnx ]; then
            if [ ! -d $QC_TOOLCHAIN_PATH/qnx ];then
                echo "qnx toolchain not found under $QC_TOOLCHAIN_PATH path"
                exit -1
            else
                ln -sf $QC_TOOLCHAIN_PATH/qnx /opt/qnx
            fi
        fi
        source /opt/qnx/env.sh
    fi
    if [[ "${QC_TARGET_SOC}" == "8797" ]] ; then
        export CMAKE_TOOLCHAIN_FILE=$homedir/toolchain/toolchain-aarch64-qnx8.cmake
    else
        export CMAKE_TOOLCHAIN_FILE=$homedir/toolchain/toolchain-aarch64-qnx7.cmake
    fi

    setup_qnn_sdk
    sh $homedir/toolchain/build-3rd-party-aarch64-qnx.sh $workdir $destdir
}

setup_env_linux() {
    if ! [[ -v LINUX_SDK_ROOT ]]; then
        if [ ! -d /opt/linux ]; then
            if [ ! -d $QC_TOOLCHAIN_PATH/linux ];then
                echo "hgy linux toolchain not found under $QC_TOOLCHAIN_PATH path"
                exit -1
            else
                ln -sf $QC_TOOLCHAIN_PATH/linux /opt/linux
            fi
        fi

        export LINUX_SDK_ROOT=/opt/linux
        if [ ! -d $LINUX_SDK_ROOT/sysroots ]; then
            tc_num=$(find -L $LINUX_SDK_ROOT -name "oecore-*-toolchain-*.sh" | wc -l)
            if [ $tc_num -gt 1 ]; then
                echo "LINUX SDK toolchain more than 1"
                exit -1
            fi
            if [ $tc_num -lt 1 ]; then
                echo "LINUX SDK toolchain does not exist"
                exit -1
            fi
            toolchain=$(find -L $LINUX_SDK_ROOT -name "oecore-*-toolchain-*.sh")
            export ARCH=`echo basename $toolchain | awk -F '-' '{print $3}'`
            if echo "$toolchain" | grep -q "image"; then
                export ARCH=`echo basename $toolchain | awk -F '-' '{print $5}'`
            fi
            echo "Toolchain target path not found, installing LINUX SDK"
            cd $LINUX_SDK_ROOT
            echo y | sh $toolchain -d .
        fi
    fi

    if [[ -v LINUX_SDK_ROOT ]]; then
        target_path=$(find -L $LINUX_SDK_ROOT/sysroots -maxdepth 1 -name "*-oe-linux" -type d)
        if [ -z $target_path ]; then
            echo "LINUX toolchain target path does not exist"
            exit -1
        fi
        export ARCH=$(basename $target_path | awk -F '-' '{print $1}')
        echo build with LINUX SDK
        echo LINUX_SDK_ROOT: $LINUX_SDK_ROOT
        echo LINUX_SDK_ARCH: $ARCH
        export LINUX_HOST=$LINUX_SDK_ROOT/sysroots/x86_64-oesdk-linux
        export LINUX_TARGET=$target_path
        export PATH=$LINUX_HOST/usr/bin/aarch64-oe-linux:$PATH
        export CC=aarch64-oe-linux-gcc
        export CXX=aarch64-oe-linux-g++
        export LD=aarch64-oe-linux-ld
        export AR=aarch64-oe-linux-ar
        export AS=aarch64-oe-linux-as
        export NM=aarch64-oe-linux-nm
        export RANLIB=aarch64-oe-linux-ranlib
        export STRIP=aarch64-oe-linux-strip

        export CMAKE_TOOLCHAIN_FILE=$homedir/toolchain/toolchain-aarch64-linux.cmake
        export TOOLCHAIN_SYSROOT=$LINUX_TARGET

        # For Nordy toolchain

        if [ -f $TOOLCHAIN_SYSROOT/usr/bin/aarch64-oe-linux-gcc ]; then
                rm $TOOLCHAIN_SYSROOT/usr/bin/aarch64-oe-linux-*
        fi
    else
        echo please specify the LINUX_SDK_ROOT path that contains the sysroots/$ARCH-oe-linux
        exit -1
    fi

    setup_qnn_sdk
    sh $homedir/toolchain/build-3rd-party-aarch64-linux.sh $workdir $destdir
}

setup_env_ubuntu() {
    if ! [[ -v UBUNTU_SDK_ROOT ]]; then
        if [ ! -d /opt/ubuntu ]; then
            if [ ! -d $QC_TOOLCHAIN_PATH/ubuntu ];then
                echo "hgy ubuntu toolchain not found under $QC_TOOLCHAIN_PATH path"
                exit -1
            else
                ln -sf $QC_TOOLCHAIN_PATH/ubuntu /opt/ubuntu
            fi
        fi

        export UBUNTU_SDK_ROOT=/opt/ubuntu
        if [ ! -d $UBUNTU_SDK_ROOT/sysroots ]; then
                tc_num=$(find -L $UBUNTU_SDK_ROOT -name "oecore-x86_64-*-ubuntu-toolchain-*.sh" | wc -l)
            if [ $tc_num -gt 1 ]; then
                echo "UBUNTU SDK toolchain more than 1"
                exit -1
            fi
            if [ $tc_num -lt 1 ]; then
                echo "UBUNTU SDK toolchain does not exist"
                exit -1
            fi
            toolchain=$(find -L $UBUNTU_SDK_ROOT -name "oecore-x86_64-*-ubuntu-toolchain-*.sh")
            export ARCH=`echo basename $toolchain | awk -F '-' '{print $3}'`
            echo "Toolchain target path not found, installing UBUNTU SDK"
            cd $UBUNTU_SDK_ROOT
            echo y | sh $toolchain -d .
        fi
    fi

    if [[ -v UBUNTU_SDK_ROOT ]]; then
        target_path=$(find -L $UBUNTU_SDK_ROOT/sysroots -maxdepth 1 -name "*-oe-linux" -type d)
        if [ -z $target_path ]; then
            echo "UBUNTU toolchain target path does not exist"
            exit -1
        fi
        export ARCH=$(basename $target_path | awk -F '-' '{print $1}')
        echo build with UBUNTU SDK
        echo UBUNTU_SDK_ROOT: $UBUNTU_SDK_ROOT
        echo UBUNTU_SDK_ARCH: $ARCH
        export UBUNTU_HOST=$UBUNTU_SDK_ROOT/sysroots/x86_64-oesdk-linux
        export UBUNTU_TARGET=$UBUNTU_SDK_ROOT/sysroots/$ARCH-oe-linux
        export CC=aarch64-linux-gnu-gcc
        export CXX=aarch64-linux-gnu-g++
        export LD=aarch64-linux-gnu-ld
        export AR=aarch64-linux-gnu-ar
        export AS=aarch64-linux-gnu-as
        export NM=aarch64-linux-gnu-nm
        export RANLIB=aarch64-linux-gnu-ranlib
        export STRIP=aarch64-linux-gnu-strip

        export CMAKE_TOOLCHAIN_FILE=$homedir/toolchain/toolchain-aarch64-ubuntu.cmake
        export TOOLCHAIN_SYSROOT=$UBUNTU_TARGET
    else
        echo please specify the UBUNTU_SDK_ROOT path that contains the sysroots/$ARCH-oe-linux
        exit -1
    fi

    setup_qnn_sdk
    sh $homedir/toolchain/build-3rd-party-aarch64-ubuntu.sh $workdir $destdir
}

setup_qnn_sdk() {
    if ! [[ -v QNN_SDK_ROOT ]]; then
        if [ ! -f /opt/qnn_sdk/bin/envsetup.sh ]; then
            if [ -f $QC_TOOLCHAIN_PATH/qnn_sdk/bin/envsetup.sh ]; then
                ln -sf $QC_TOOLCHAIN_PATH/qnn_sdk /opt/qnn_sdk
            else
                echo "qnn_sdk not fould under $QC_TOOLCHAIN_PATH"
                exit -1
            fi
        fi
        source /opt/qnn_sdk/bin/envsetup.sh
    fi
}

## Run tests on x86 builds
case $target in
aarch64-qnx)
    setup_env_qnx
    ;;
aarch64-linux)
    setup_env_linux
    ;;
aarch64-ubuntu)
    setup_env_ubuntu
    ;;
esac

# build FastADAS interface library
if [[ -v HEXAGON_SDK_ROOT && -v BSP_ROOT ]] ; then
  if ! [ -f $topdir/source/libs/FadasIface/prebuilt/dsp/libFadasIface_skel.so ];  then
    cd $topdir/source/libs/FadasIface/bld
    rm hexagon_Release_toolv*_v68 -fr
    make tree V=hexagon_Release_dynamic_toolv86_v68 VERBOSE=1 V_dynamic=1 || make tree V=hexagon_Release_dynamic_toolv84_v68 VERBOSE=1 V_dynamic=1
    cp -fv hexagon_Release_toolv*_v68/ship/libFadasIface_skel.so ../prebuilt/dsp
    cp -fv hexagon_Release_toolv*_v68/FadasIface.h ../
    cp -fv hexagon_Release_toolv*_v68/FadasIface_stub.c ../FadasIface.c
    $HEXAGON_SDK_ROOT/tools/HEXAGON_Tools/*/Tools/bin/hexagon-strip ../prebuilt/dsp/libFadasIface_skel.so
    if [[ -v SWIV_BUILD_UTILITY ]] ; then
        python ${SWIV_BUILD_UTILITY}  -i ../prebuilt/dsp/libFadasIface_skel.so -o ../prebuilt/dsp/libFadasIface_skel.so.crc
        mv ../prebuilt/dsp/libFadasIface_skel.so.crc ../prebuilt/dsp/libFadasIface_skel.so
    fi
  fi
fi

mkdir -p $workdir && cd $workdir || exit -1
rm -frv $destdir/opt/qcnode/include/QC
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
    -DENABLE_C2C=${ENABLE_C2C} \
    -DQC_TARGET_SOC=${QC_TARGET_SOC} \
    .. || exit -1
make -j 16 || exit -1
# Install the QC SDK
make DESTDIR=$destdir install || exit -1

if [[ -v QNN_SDK_ROOT ]] ; then
# Install QNN Runtime dependencies
mkdir -p $destdir/opt/qcnode/lib/dsp
if [[ "${QC_TARGET_SOC}" == "8797" ]] ; then
  HEXAGON_VARIANT=hexagon-v81
elif [[ "${QC_TARGET_SOC}" == "8620" ]] ; then
  HEXAGON_VARIANT=hexagon-v75
else
  HEXAGON_VARIANT=hexagon-v73
fi

case $target in
aarch64-qnx)
    if [[ "${QC_TARGET_SOC}" == "8797" ]] ; then
      QNX_VARIANT=aarch64-qnx800
    else
      QNX_VARIANT=aarch64-qnx
    fi
    cp -vf $QNN_SDK_ROOT/bin/${QNX_VARIANT}/* $destdir/opt/qcnode/bin
    cp -vf $QNN_SDK_ROOT/lib/${QNX_VARIANT}/libQnn* $destdir/opt/qcnode/lib
    cp -vf $QNN_SDK_ROOT/lib/${HEXAGON_VARIANT}/unsigned/libQnn* $destdir/opt/qcnode/lib/dsp
    ;;
aarch64-linux)
    cp -vf $QNN_SDK_ROOT/bin/aarch64-${variant}-linux-gcc9.3/* $destdir/opt/qcnode/bin
    cp -vf $QNN_SDK_ROOT/lib/aarch64-${variant}-linux-gcc9.3/libQnn* $destdir/opt/qcnode/lib
    cp -vf $QNN_SDK_ROOT/lib/${HEXAGON_VARIANT}/unsigned/libQnn* $destdir/opt/qcnode/lib/dsp
    ;;
aarch64-ubuntu)
    cp -vf $QNN_SDK_ROOT/bin/aarch64-${variant}-linux-gcc9.3/* $destdir/opt/qcnode/bin
    cp -vf $QNN_SDK_ROOT/lib/aarch64-${variant}-linux-gcc9.3/libQnn* $destdir/opt/qcnode/lib
    cp -vf $QNN_SDK_ROOT/lib/${HEXAGON_VARIANT}/unsigned/libQnn* $destdir/opt/qcnode/lib/dsp
    ;;
esac
fi

mkdir -p $destdir/opt/qcnode/lib/runtime
if [ -f /opt/toolchain/LiberationSans-Regular.ttf ]; then
    cp -v /opt/toolchain/LiberationSans-Regular.ttf $destdir/opt/qcnode/lib/runtime
else
    if [ ! -f LiberationSans-Regular.ttf ]; then
        unzip $THIRD_PARTY_DIR/liberation_sans.zip
    fi
    cp -v LiberationSans-Regular.ttf $destdir/opt/qcnode/lib/runtime
fi

if [[ "${QC_TARGET_SOC}" == "8797" ]] && [[ "${target}" == "aarch64-qnx" ]] ; then
    SSL_LIB=/opt/qnx/target/qnx/aarch64le/usr/lib/libssl.so.3
    if [ ! -f $SSL_LIB ]; then
        SSL_LIB=$BSP_ROOT/qnx_bins/prebuilt_SDP800/target/qnx/aarch64le/usr/lib/libssl.so.3
    fi
    cp -v $SSL_LIB $destdir/opt/qcnode/lib/runtime
fi

if [ -d $QNN_SDK_ROOT/model ]; then
    if [ ! -d $destdir/opt/qcnode/data ]; then
        mkdir -p $destdir/opt/qcnode/data
    fi
    cp -r $QNN_SDK_ROOT/model/* $destdir/opt/qcnode/data
fi

# Add fadas libs
if [ -d /opt/toolchain/fadaslib/dsp ]; then
        cp -v /opt/toolchain/fadaslib/dsp/* $destdir/opt/qcnode/lib/dsp
fi

case $target in
aarch64-qnx)
    if [ -d /opt/toolchain/fadaslib/qnx ]; then
        cp -v /opt/toolchain/fadaslib/qnx/* $destdir/opt/qcnode/lib
    fi
    ;;
aarch64-linux)
    if [ -d /opt/toolchain/fadaslib/linux ]; then
        cp -v /opt/toolchain/fadaslib/linux/* $destdir/opt/qcnode/lib
    fi
    ;;
aarch64-ubuntu)
    if [ -d /opt/toolchain/fadaslib/ubuntu ]; then
        cp -v /opt/toolchain/fadaslib/ubuntu/* $destdir/opt/qcnode/lib
    fi
    ;;
esac

# Create run-time package
echo "Generating $pkgname"
tar -C $topdir --xform="s/run/pkg/" --exclude="*.a" \
    --exclude="*.la" --exclude="include" --exclude="share" \
    --exclude="cmake" \
    --use-compress-program=pigz -cf $pkgname run-$target


