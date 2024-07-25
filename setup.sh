#!/bin/sh

cd ..

echo "Building NPF ..."
git clone https://github.com/tbarbette/npf.git npf
cd npf
python3 setup.py build
chmod u+x ./build/lib/npf_compare.py
echo "NPF build done ..."
cd ..

echo "Building DPDK v22.07..."
git clone https://github.com/DPDK/dpdk.git
cd dpdk
git checkout v22.07
mkdir build
cd build
meson --prefix $(pwd)/../install .. .
ninja install -j 16
cd ../..

export RTE_TARGET="x86_64-native-linuxapp-gcc"
export RTE_SDK="$(pwd)/dpdk/"

echo "RTE_SDK=$RTE_SDK"
echo "DPDK v22.07 build done, make sure no errors are popped up..." 

echo "Building FAJITA..."
git clone https://github.com/FAJITA-Packet-Processing-Framework/FAJITA.git
cd FAJITA
PKG_CONFIG_PATH=${PWD%/*}/dpdk/install/lib/x86_64-linux-gnu/pkgconfig ./configure --enable-dpdk --enable-intel-cpu --verbose --enable-select=poll "CFLAGS=-O3" "CXXFLAGS=-std=c++17 -O3" --disable-dynamic-linking --enable-poll --enable-bound-port-transfer --enable-local --disable-task-stats --enable-cpu-load --enable-dpdk-packet --disable-clone --disable-dpdk-softqueue --enable-research --disable-sloppy --enable-user-timestamp --enable-flow
make clean
PKG_CONFIG_PATH=${PWD%/*}/dpdk/install/lib/x86_64-linux-gnu/pkgconfig ./configure --enable-dpdk --enable-intel-cpu --verbose --enable-select=poll "CFLAGS=-O3" "CXXFLAGS=-std=c++17 -O3" --disable-dynamic-linking --enable-poll --enable-bound-port-transfer --enable-local --disable-task-stats --enable-cpu-load --enable-dpdk-packet --disable-clone --disable-dpdk-softqueue --enable-research --disable-sloppy --enable-user-timestamp --enable-flow
make -j 16
echo "FAJITA Build done..."
cd ..

echo "Building VPP with added plugins..."
git clone https://github.com/FAJITA-Packet-Processing-Framework/custom-vpp.git
cd custom-vpp
sudo make build-release
sudo make build-release
echo "VPP Build done..."
cd ..

echo "Building FastClick..."
git clone https://github.com/tbarbette/fastclick.git
cd fastclick
PKG_CONFIG_PATH=${PWD%/*}/dpdk/install/lib/x86_64-linux-gnu/pkgconfig ./configure --enable-dpdk --enable-intel-cpu --verbose --enable-select=poll "CFLAGS=-O3" "CXXFLAGS=-std=c++17 -O3" --disable-dynamic-linking --enable-poll --enable-bound-port-transfer --enable-local --disable-task-stats --enable-cpu-load --enable-dpdk-packet --disable-clone --disable-dpdk-softqueue --enable-research --disable-sloppy --enable-user-timestamp --enable-flow
make clean
PKG_CONFIG_PATH=${PWD%/*}/dpdk/install/lib/x86_64-linux-gnu/pkgconfig ./configure --enable-dpdk --enable-intel-cpu --verbose --enable-select=poll "CFLAGS=-O3" "CXXFLAGS=-std=c++17 -O3" --disable-dynamic-linking --enable-poll --enable-bound-port-transfer --enable-local --disable-task-stats --enable-cpu-load --enable-dpdk-packet --disable-clone --disable-dpdk-softqueue --enable-research --disable-sloppy --enable-user-timestamp --enable-flow
make -j 16
echo "FastClick Build done..."
cd ..

echo "Building Dyssect..."
git clone https://github.com/tbarbette/fastclick.git dyssect
cd dyssect
mkdir elements/dyssect
cp ../FAJITA-experiments/extra-elements/dyssect/solver* .
cp ../FAJITA-experiments/extra-elements/dyssect/elements/* elements/dyssect/
PKG_CONFIG_PATH=${PWD%/*}/dpdk/install/lib/x86_64-linux-gnu/pkgconfig ./configure --enable-dpdk --enable-intel-cpu --verbose --enable-select=poll "CFLAGS=-O3" "CXXFLAGS=-std=c++17 -O3" --disable-dynamic-linking --enable-poll --enable-bound-port-transfer --enable-local --disable-task-stats --enable-cpu-load --enable-dpdk-packet --disable-clone --disable-dpdk-softqueue --enable-research --disable-sloppy --enable-user-timestamp
make clean 
PKG_CONFIG_PATH=${PWD%/*}/dpdk/install/lib/x86_64-linux-gnu/pkgconfig ./configure --enable-dpdk --enable-intel-cpu --verbose --enable-select=poll "CFLAGS=-O3" "CXXFLAGS=-std=c++17 -O3" --disable-dynamic-linking --enable-poll --enable-bound-port-transfer --enable-local --disable-task-stats --enable-cpu-load --enable-dpdk-packet --disable-clone --disable-dpdk-softqueue --enable-research --disable-sloppy --enable-user-timestamp
make -j 16
echo "Dyssect Build done..."

echo "All systems are ready! You can now run the experiments if no error has popped up during the setup..."