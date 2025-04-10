# How to build RideHal with TinyViz for HGY Linux

## Set WORKSPACE path:
export WORKSPACE=$PWD (or other path.)

## Prepare host environment
On Ubuntu 20.04 host environment, install dependent packages:
- Install basic dependency:
```sh
apt install rsync wget curl vim \
    cpio unzip zip lsof bc pixz pigz tree gawk \
    build-essential pkg-config libtool autoconf automake debhelper \
    bison flex gdb gdb-multiarch strace ltrace tzdata locales \
    chrpath texinfo binutils ftp lftp telnet \
    apt-transport-https ca-certificates gnupg software-properties-common \
    openssh-client git git-lfs diffstat openssl libssl-dev
```

- Install SDL2 dependency:
```sh
apt install libwayland-dev libwayland-bin libegl-dev libxkbcommon-dev
```

- Install QNN Linux dependency:
```sh
apt install libncurses5 libgl1 libasound2-dev \
    libnss3 libgbm-dev desktop-file-utils \
    && add-apt-repository 'deb http://cz.archive.linux.com/linux focal main universe' \
    && apt-get update && apt install -y \
    flatbuffers-compiler libflatbuffers-dev rename
```

- Install cmake:
```sh
wget https://github.com/Kitware/CMake/releases/download/v3.20.0/cmake-3.20.0.tar.gz -P $WORKSPACE --no-check-certificate
cd $WORKSPACE && tar -zxf cmake-3.20.0.tar.gz
cd $WORKSPACE/cmake-3.20.0
./bootstrap
make -j8
make install
cd $WORKSPACE && rm -rf $WORKSPACE/cmake-3.20.0
```

## Setup HGY OELinux toolchain env
```sh
mkdir -p $WORKSPACE/linux
# copy oecore-x86_64-aarch64-toolchain-nodistro.0.sh from LR.AU.0.1.1.r2-16600-gen4meta.1-2\apps_proc\poky\build\tmp-glibc\deploy\sdk-sa8775 to $WORKSPACE/linux
cd $WORKSPACE/linux
chmod +x ./oecore-x86_64-aarch64-toolchain-nodistro.0.sh
./oecore-x86_64-aarch64-toolchain-nodistro.0.sh -y -d .
export LINUX_HOST=$WORKSPACE/linux/sysroots/x86_64-oesdk-linux
export LINUX_TARGET=$WORKSPACE/linux/sysroots/aarch64-oe-linux
export PATH=$LINUX_HOST/usr/bin/aarch64-oe-linux:$PATH
export TOOLCHAIN_SYSROOT=$LINUX_TARGET
```

## Build and install SDL libraries

- Set toolchain environment
```sh
export CC=aarch64-oe-linux-gcc
export CXX=aarch64-oe-linux-g++
export LD=aarch64-oe-linux-ld
export AR=aarch64-oe-linux-ar
export AS=aarch64-oe-linux-as
export NM=aarch64-oe-linux-nm
export RANLIB=aarch64-oe-linux-ranlib
export STRIP=aarch64-oe-linux-strip
```

- Set toolchain flags
```sh
export CFLAGS="-O3 -g --sysroot=$TOOLCHAIN_SYSROOT"
export CXXFLAGS="-O3 -g --sysroot=$TOOLCHAIN_SYSROOT"
export LDFLAGS="--sysroot=$TOOLCHAIN_SYSROOT"
```

- Set and create object file path
```sh
export TARGET=aarch64-linux
export PKG_NAME=ridehal-$TARGET.tar.gz
export INSTALL_PATH=/opt/ridehal
export DST_DIR=$WORKSPACE/opt/ridehal

mkdir -p $DST_DIR
```

- Set cmake environment:
Create a file named toolchain-aarch64-linux.cmake under $WORKSPACE/linux path.
Fill with these contents:
```
set( CMAKE_SYSTEM_NAME Linux )
set( CMAKE_SYSTEM_PROCESSOR aarch64 )

set( arch gcc_hgyaarch64le )

set( CMAKE_C_COMPILER aarch64-oe-linux-gcc )
set( CMAKE_C_COMPILER_TARGET ${arch} )
set( CMAKE_CXX_COMPILER aarch64-oe-linux-g++ )
set( CMAKE_CXX_COMPILER_TARGET ${arch} )

set( CMAKE_SYSROOT $ENV{LINUX_TARGET} )
```
Set the cmake toolchain env:
```sh
export CMAKE_TOOLCHAIN_FILE=$WORKSPACE/linux/toolchain-aarch64-linux.cmake
```

- Build and install SDL2 for TinyViz:
```sh
cd $WORKSPACE
wget https://www.libsdl.org/release/SDL2-2.0.14.tar.gz -P $WORKSPACE --no-check-certificate
tar -zxf SDL2-2.0.14.tar.gz
cd SDL2-2.0.14
./configure CXXFLAGS="$CXXFLAGS" CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" \
          --prefix=$DST_DIR --host=aarch64-oe-linux \
          --with-sysroot --enable-esd=no --enable-pulseaudio=no \
          --enable-dbus=no
make -j16
make install
```

- Build and install SDL2_gfx:
```sh
cd $WORKSPACE
wget https://versaweb.dl.sourceforge.net/project/sdl2gfx/SDL2_gfx-1.0.4.tar.gz -P $WORKSPACE --no-check-certificate
tar -zxf SDL2_gfx-1.0.4.tar.gz
cd SDL2_gfx-1.0.4
cp /usr/share/libtool/build-aux/config.sub .
cp /usr/share/libtool/build-aux/config.guess .
./configure CXXFLAGS="$CXXFLAGS" CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" \
          --prefix=$DST_DIR --host=aarch64-oe-linux \
          --with-sysroot --with-sdl-prefix=$DST_DIR --enable-mmx=no
make -j16
make install
```

- Build and install SDL2_ttf:
```sh
cd $WORKSPACE
export CC="$CC --sysroot=$TOOLCHAIN_SYSROOT"
wget https://www.libsdl.org/projects/SDL_ttf/release/SDL2_ttf-2.0.15.tar.gz -P $WORKSPACE --no-check-certificate
tar -zxf SDL2_ttf-2.0.15.tar.gz
cd SDL2_ttf-2.0.15/external/freetype-2.9.1
./autogen.sh
./configure CXXFLAGS="$CXXFLAGS" CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" \
          --prefix=$DST_DIR --with-png=no \
          --host=aarch64-oe-linux --with-sysroot
sed -i '86s/^/# /' ./builds/unix/unix-cc.mk
make -j16
make install
cd $WORKSPACE/SDL2_ttf-2.0.15
./configure CXXFLAGS="$CXXFLAGS" CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" \
          --prefix=$DST_DIR \
          --with-sdl-prefix=$DST_DIR \
          --with-ft-prefix=$DST_DIR \
          --host=aarch64-oe-linux --with-sysroot
sed -i '264d' ./Makefile
sed -i '264i CFLAGS = ${C_FLAGS} -I${DST_DIR}/include/freetype2 -I${DST_DIR}/include/SDL2 -D_REENTRANT' ./Makefile
sed -i '279d' ./Makefile
sed -i '279i FT2_CFLAGS = -I${DST_DIR}/include/freetype2' ./Makefile
sed -i '281d' ./Makefile
sed -i '281i FT2_LIBS = ${LD_FLAGS} -L${DST_DIR}/lib -lfreetype' ./Makefile
sed -i '293d' ./Makefile
sed -i '293i LIBS = ${LD_FLAGS} -L${DST_DIR}/lib -lfreetype -lSDL2' ./Makefile
make -j16 destdir=${DST_DIR} C_FLAGS="${CFLAGS}" LD_FLAGS="${LDFLAGS}"
make install
```

## Build RideHal package
- Setup QNN SDK env:
Unzip QNN SDK package to $WORKSPACE and rename as qnn_sdk.
```sh
source $WORKSPACE/qnn_sdk/bin/envsetup.sh
```
- Prepare multimedia dependency
    - Find parserinternaldefs.h file in chipcode
    - copy it to path: $WORKSPACE/linux/sysroots/aarch64-oe-linux/usr/include

- Build RideHal SDK:
Get source code and put at $WORKSPACE path, then use the following commands to build:
```sh
cd $WORKSPACE/ridehal
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=$CMAKE_TOOLCHAIN_FILE \
    -DCMAKE_INCLUDE_PATH=$TOOLCHAIN_SYSROOT/usr/include \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH \
    -DCMAKE_PREFIX_PATH=$DST_DIR \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DENABLE_GCOV=OFF ..
make -j16
make DESTDIR=$WORKSPACE install
```

- copy QNN library to RideHal
```sh
cp $QNN_SDK_ROOT/bin/aarch64-oe-linux-gcc9.3/* $DST_DIR/bin
cp $QNN_SDK_ROOT/lib/aarch64-oe-linux-gcc9.3/* $DST_DIR/lib
cp $QNN_SDK_ROOT/lib/hexagon-v73/unsigned/libQnn* $DST_DIR/lib/dsp
```

- copy font file to RideHal
```sh
cd $WORKSPACE
mkdir font
cd font
wget https://dl.dafont.com/dl/?f=liberation_sans -O liberation_sans.zip --no-check-certificate
unzip liberation_sans.zip
mkdir $DST_DIR/lib/runtime
cp LiberationSans-Regular.ttf $DST_DIR/lib/runtime
```

- copy dependent libraries to RideHal
```sh
cd $WORKSPACE
readelf -d $DST_DIR/lib/libSDL2.so | \
    grep Shared | awk -F \[ '{print $2}' | \
    sed 's/\]//' | \
    xargs -i find $LINUX_TARGET -name {} | \
    xargs -i cp {} $DST_DIR/lib
```

- generate RideHal package
```sh
cd $WORKSPACE
tar -C $WORKSPACE --exclude="*.a" \
    --exclude="*.la" --exclude="include" \
    --exclude="share" --exclude="cmake" \
    --use-compress-program=pigz -cf $PKG_NAME opt/ridehal
```

