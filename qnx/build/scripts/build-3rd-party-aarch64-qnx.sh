#! /bin/bash
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# All rights reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.

set -xe

homedir=$(dirname $0)
homedir=$(realpath $homedir/..)

# Build directory
workdir=$1/third_party

# Install / runtime directory
destdir=$2/opt/qcnode

mkdir -p $destdir || exit 1
mkdir -p $workdir && cd $workdir || exit 1

if [ "$ENABLE_TINYVIZ" == "ON" ] ; then

# build and install SDL2 for TinyViz
cd $THIRD_PARTY_DIR
if [ ! -d $destdir/include/SDL2 ]; then
  if [ ! -f SDL2-2.0.14.tar.gz ]; then
    echo "Package SDL2-2.0.14.tar.gz not found under $THIRD_PARTY_DIR"
  else
    tar -zxf SDL2-2.0.14.tar.gz -C $workdir
    cd $workdir/SDL2-2.0.14
    ./configure CXXFLAGS="-O3 -g --sysroot=$TOOLCHAIN_SYSROOT" \
      CFLAGS="-O3 -g --sysroot=$TOOLCHAIN_SYSROOT" \
      --prefix=$destdir --host=aarch64-unknown-nto-qnx \
      --with-sysroot --enable-esd=no --enable-pulseaudio=no \
      --disable-video-wayland \
      --enable-dbus=no --enable-audio=no
    make -j16
    make install
  fi
fi

# build and install SDL2_gfx
cd $THIRD_PARTY_DIR
if [ ! -f $destdir/lib/libSDL2_gfx.so ]; then
  if [ ! -f SDL2_gfx-1.0.4.tar.gz ]; then
    echo "Package SDL2_gfx-1.0.4.tar.gz not found under $THIRD_PARTY_DIR"
  else
    tar -zxf SDL2_gfx-1.0.4.tar.gz -C $workdir
    cd $workdir/SDL2_gfx-1.0.4
    ./configure CXXFLAGS="-O3 -g" CFLAGS="-O3 -g" \
      --prefix=$destdir --host=aarch64-unknown-nto-qnx \
      --with-sysroot --with-sdl-prefix=$destdir --enable-mmx=no
    make -j16
    make install
  fi
fi

# build and install SDL2_ttf
cd $THIRD_PARTY_DIR
if ! [ -f $destdir/lib/libSDL2_ttf.so ]; then
  if ! [ -f SDL2_ttf-2.0.15.tar.gz ]; then
    echo "Package SDL2_ttf-2.0.15.tar.gz not found under $THIRD_PARTY_DIR"
  else
    tar -zxf SDL2_ttf-2.0.15.tar.gz -C $workdir
    cd $workdir/SDL2_ttf-2.0.15/external/freetype-2.9.1
    sh ./autogen.sh
    ./configure CXXFLAGS="-O3 -g --sysroot=$TOOLCHAIN_SYSROOT" \
      CFLAGS="-O3 -g --sysroot=$TOOLCHAIN_SYSROOT" \
      LDFLAGS="--sysroot=$TOOLCHAIN_SYSROOT"  \
      --prefix=$destdir --with-png=no --with-harfbuzz=no \
      --host=aarch64-unknown-nto-qnx --with-sysroot
    sed -i '86s/^/# /' ./builds/unix/unix-cc.mk
    make -j16
    make install
    cd $workdir/SDL2_ttf-2.0.15
    ./configure CXXFLAGS="-O3 -g --sysroot=$TOOLCHAIN_SYSROOT" \
      CFLAGS="-O3 -g --sysroot=$TOOLCHAIN_SYSROOT" \
      LDFLAGS="--sysroot=$TOOLCHAIN_SYSROOT" \
      --prefix=$destdir \
      --with-sdl-prefix=$destdir \
      --with-ft-prefix=$destdir \
      --host=aarch64-unknown-nto-qnx
    sed -i '264d' ./Makefile
    sed -i '264i CFLAGS = -O3 -g -I${destdir}/include/freetype2 -I${destdir}/include/SDL2 -D_REENTRANT' ./Makefile
    sed -i '279d' ./Makefile
    sed -i '279i FT2_CFLAGS = -I${destdir}/include/freetype2' ./Makefile
    sed -i '281d' ./Makefile
    sed -i '281i FT2_LIBS = -L${destdir}/lib -lfreetype' ./Makefile
    sed -i '293d' ./Makefile
    sed -i '293i LIBS =  -L${destdir}/lib -lfreetype -L${destdir}/lib -lSDL2' ./Makefile
    make -j16 destdir=${destdir}
    make install
  fi
fi

fi # ENABLE_TINYVIZ

cd $THIRD_PARTY_DIR
if [ ! -d $destdir/include/nlohmann ]; then
  if [ ! -f json.tar.xz ]; then
    echo "Package json.tar.xz not found under $THIRD_PARTY_DIR"
    exit -1
  else
    tar -xf json.tar.xz -C $workdir
    cd $workdir/json
    mkdir -p build
    cd build
    cmake \
    -DCMAKE_TOOLCHAIN_FILE=$CMAKE_TOOLCHAIN_FILE \
    -DCMAKE_INCLUDE_PATH=$TOOLCHAIN_SYSROOT/usr/include \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_INSTALL_PREFIX=$destdir \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DJSON_BuildTests=OFF \
    ..
    make
    make install
  fi
fi

cd $THIRD_PARTY_DIR
if [ ! -d $destdir/include/gtest ]; then
  if [ ! -f googletest-1.10.0.tar.gz ]; then
    echo "Package googletest-1.10.0.tar.gz not found under $THIRD_PARTY_DIR"
    exit -1
  else
    tar -xf googletest-1.10.0.tar.gz -C $workdir
    cd $workdir/googletest-release-1.10.0
    patch -p1 < $homedir/patches/gtest-qnx.patch
    mkdir -p build
    cd build
    cmake \
    -DCMAKE_TOOLCHAIN_FILE=$CMAKE_TOOLCHAIN_FILE \
    -DCMAKE_INCLUDE_PATH=$TOOLCHAIN_SYSROOT/usr/include \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_INSTALL_PREFIX=$destdir \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    ..
    make
    make install
  fi
fi
