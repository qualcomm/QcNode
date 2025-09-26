# aarch64 QNX & HGY based toolchain

FROM ubuntu:20.04

ARG DEBIAN_FRONTEND=noninteractive

USER root

COPY sources_aliyun.list  /etc/apt
COPY requirements.txt /tmp

# Force bash for everything
ENV SHELL /bin/bash

# Update APT source
RUN ln -fs /bin/bash /bin/sh \
    && mv /etc/apt/sources.list /etc/apt/sources_bak.list \
    && mv /etc/apt/sources_aliyun.list /etc/apt/sources.list \
    && apt-get update

# Install basic packages
RUN apt install -y rsync wget curl vim \
    cpio unzip zip lsof bc pixz pigz tree gawk \
    build-essential pkg-config libtool autoconf automake debhelper \
    bison flex gdb gdb-multiarch strace ltrace tzdata locales \
    chrpath texinfo binutils ftp lftp telnet \
    apt-transport-https ca-certificates gnupg software-properties-common \
    openssh-client git git-lfs diffstat openssl libssl-dev

# Install SDL2 dependency
RUN apt install -y libwayland-dev libwayland-bin libegl-dev libxkbcommon-dev

# Install QNN Linux dependency
RUN apt install -y libncurses5 libgl1 libasound2-dev \
    libnss3 libgbm-dev desktop-file-utils \
    && add-apt-repository 'deb http://cz.archive.ubuntu.com/ubuntu focal main universe' \
    && apt-get update && apt install -y \
    flatbuffers-compiler libflatbuffers-dev rename

# Install HGY Ubuntu toolchain
RUN apt install -y gcc-aarch64-linux-gnu g++-aarch64-linux-gnu \
    && ln -sf /usr/aarch64-linux-gnu/lib/ld-linux-aarch64.so.1 /lib/ld-linux-aarch64.so.1

# Install cmake
RUN wget https://github.com/Kitware/CMake/releases/download/v3.20.0/cmake-3.20.0.tar.gz -P /tmp --no-check-certificate \
    && cd /tmp && tar -zxf cmake-3.20.0.tar.gz && cd /tmp/cmake-3.20.0 \
    && ./bootstrap && make -j8 && make install \
    && cd /tmp && rm -rf /tmp/cmake-3.20.0*

# Install lconv
RUN wget https://github.com/linux-test-project/lcov/releases/download/v1.16/lcov-1.16.tar.gz -P /tmp --no-check-certificate \
    && cd /tmp && tar -zxf lcov-1.16.tar.gz && cd lcov-1.16 \
    && make -j8 && make install \
    && cd /tmp && rm -rf /tmp/lcov-1.16*

# Install python packages
RUN apt-get update && add-apt-repository ppa:deadsnakes/ppa \
    &&apt-get install -y python3 python3-dev python3-pip \
    python3-distutils python3-setuptools python3-venv

# clean up
RUN rm -rf /var/cache/apk/* && rm -rf /var/lib/apt/lists/* \
    && rm -rf /var/cache/apt/archives/* \
    && rm -rf ~/.cache/pip \
    && rm -rf /tmp/*

# Final config
RUN dpkg-reconfigure locales && \
    locale-gen en_US.UTF-8 && \
    update-locale LC_ALL=en_US.UTF-8 LANG=en_US.UTF-8

ENV LC_ALL   en_US.UTF-8
ENV LANG     en_US.UTF-8
ENV LANGUAGE en_US.UTF-8
ENV TZ America/Los_Angeles

CMD /bin/bash
