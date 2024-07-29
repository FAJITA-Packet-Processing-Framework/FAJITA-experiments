#!/bin/sh

cd ..
mkdir generator
cd generator

echo "Building DPDK v22.07..."
git clone https://github.com/DPDK/dpdk.git
cd dpdk
git checkout v22.07
mkdir build
cd build
meson --prefix $(pwd)/../install .. .
ninja install -j 8
cd ../..

export RTE_TARGET="x86_64-native-linuxapp-gcc"
export RTE_SDK="$(pwd)/dpdk/"

echo "RTE_SDK=$RTE_SDK"
echo "DPDK v22.07 build done, make sure no errors are popped up..." 

echo "Building FastClick..."
git clone https://github.com/tbarbette/fastclick.git
cd fastclick
cp ../../FAJITA-experiments/extra-elements/generator/* elements/tcpudp/

PKG_CONFIG_PATH=${PWD%/*}/dpdk/install/lib/x86_64-linux-gnu/pkgconfig ./configure --enable-dpdk --enable-intel-cpu --verbose --enable-select=poll "CFLAGS=-O3" "CXXFLAGS=-std=c++17 -O3" --disable-dynamic-linking --enable-poll --enable-bound-port-transfer --enable-local --disable-task-stats --enable-cpu-load --enable-dpdk-packet --disable-clone --disable-dpdk-softqueue --enable-research --disable-sloppy --enable-user-timestamp --enable-flow
make clean
PKG_CONFIG_PATH=${PWD%/*}/dpdk/install/lib/x86_64-linux-gnu/pkgconfig ./configure --enable-dpdk --enable-intel-cpu --verbose --enable-select=poll "CFLAGS=-O3" "CXXFLAGS=-std=c++17 -O3" --disable-dynamic-linking --enable-poll --enable-bound-port-transfer --enable-local --disable-task-stats --enable-cpu-load --enable-dpdk-packet --disable-clone --disable-dpdk-softqueue --enable-research --disable-sloppy --enable-user-timestamp --enable-flow
make -j 16
echo "FastClick Build done..."
