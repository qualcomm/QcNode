#!/bin/bash
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause

if [ ! -d $THIRD_PARTY_DIR ]; then
  mkdir -p $THIRD_PARTY_DIR
fi
cd $THIRD_PARTY_DIR

LOCAL_CACHE_1=/prj/cv2x/sandiego/qride/release/qcnode/third_party

qcdownload() {
  url=$1
  file=$2
  md5=$3
  if [ ! -f $file ]; then
    counter=1
    while [ $counter -le 10 ]; do
      if [ -f $LOCAL_CACHE_1/$file ]; then
        cp $LOCAL_CACHE_1/$file .
      else
        wget $url -O $file --no-check-certificate
      fi
      md5_=($(md5sum $file))
      if [ "$md5_" = "$md5" ]; then
        echo "Package $file downloaded"
        break
      fi
      rm -f $file
      if [ $counter -eq 10 ]; then
        echo "Failed to download $file"
        exit -1
      fi
      ((counter++))
    done
  else
    echo "Found $file"
  fi
}

if [ "$ENABLE_TINYVIZ" == "ON" ] ; then
  qcdownload https://www.libsdl.org/release/SDL2-2.0.14.tar.gz SDL2-2.0.14.tar.gz 76ed4e6da9c07bd168b2acd9bfefab1b
  qcdownload https://versaweb.dl.sourceforge.net/project/sdl2gfx/SDL2_gfx-1.0.4.tar.gz SDL2_gfx-1.0.4.tar.gz 15f9866c6464ca298f28f62fe5b36d9f
  qcdownload https://www.libsdl.org/projects/SDL_ttf/release/SDL2_ttf-2.0.15.tar.gz SDL2_ttf-2.0.15.tar.gz 04fe06ff7623d7bdcb704e82f5f88391
  qcdownload https://dl.dafont.com/dl/?f=liberation_sans liberation_sans.zip c553a360214638956a561ce5538e49cc
fi # ENABLE_TINYVIZ

JSON_VERSION=3.11.3
qcdownload https://github.com/nlohmann/json/releases/download/v${JSON_VERSION}/json.tar.xz json-${JSON_VERSION}.tar.xz c23a33f04786d85c29fda8d16b5f0efd
ln -fs json-${JSON_VERSION}.tar.xz json.tar.xz
qcdownload https://github.com/google/googletest/archive/refs/tags/release-1.10.0.tar.gz googletest-1.10.0.tar.gz ecd1fa65e7de707cd5c00bdac56022cd
