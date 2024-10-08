# FAJITA Experiments

FAJITA is a system built on top of [FastClick][Fastclick] to facilitate deploying chains of stateful network functions on commodity servers. In a nutshell, FAJITA (I) minimizes memory access overheads by exploiting batching \& software prefetching machinery available in modern CPUs, and (II) alleviates the overheads of accessing shared data structures by introducing auxiliary hash tables. For more information check out our [paper][fajita-paper] at CoNEXT '24.

This repository contains instructions for using FAJITA and running experiments that are already done in the FAJITA's paper. You can find the source code for FAJITA from [here][fajita-repo]. We plan to merge the essential parts of FAJITA in the Fastclick soon.

## Repository Organization

- `experiments` direcotry contains configuration files and necessary information that you need to run all experiments discussed in the paper.

- `graphs` directory contains all csv results and graphs that already presented in the paper. You can use the output csv of your experiments and use the provided [Gnuplot][Gnuplot] scripts to generate your graphs in the same style as the paper.

## Testbed

**NOTE: Before running the experiments, you need to prepare your testbed according to the following guidelines.**

To reproduce results demonstrated in our [paper][fajita-paper] you need to have a two commidity servers that are connected through a Tofino switch. One server acts as a Traffic Generator (TG) and the other is the Device Under Test (DUT) which runs the chain of stateful NFs. 

Note that we use an extra forwarder switch in our testbed to support a 200G to the DUT. However it is not required and nor affect the provided scripts if your Tofino switch already supports 200G or you run the experiment on a 100G link. 

<p align="center">
<br>
<img src="fajita-testbed.png" alt="Reframer working diagram" width="65%"/>
<br>
</p>


Since the generated rate on the TG is limited and normally can not go beyond 20Mpps (depending on your hardware setup), we create multiple clones of packets on the switch. You can read more about it in our [paper][fajita-paper]. Also, a proper code compatible with Tofino2 switches for generating clones is available on our [Multicast][tofino-multicast] respository.

All the experiments require [FAJITA][fajita-repo] and [NPF][NPF] tool.
To have a fair comparison with existing packet processing frameworks, we have developed elements and plugins on [FastClick][FastClick], [Dyssect][Dyssect], and [VPP][VPP] with identical behavior among all of them. Hence, to compare FAJITA with any of these frameworks you need to have our customized version of these frameworks as well, which are listed in the testbed setup instructions.

**There is a bash script (`setup_repo.sh`) that could help you to clone/compile different repositories, but you should mainly rely on this README.md.**

Below contains the step-by-step guide for preparing your testbed:

### Network Performance Framework (NPF) Tool

We use NPF to run experiments with different setups and store results in csv files. 
You can install [NPF][NPF] via the following command:

```bash
git clone https://github.com/tbarbette/npf.git npf
cd npf
python3 setup.py build
chmod u+x ./build/lib/npf_compare.py
```

NPF will look for `cluster/` in your current working/testie directory. We have included sample `cluster` templates, available at `npf-configuration/cluster`. Please ensure that you add proper node file in the provided directory. For more information about how to setup your cluster please check the [NPF guidelines][NPF-cluster].


Additionally, the provided script needs to have the NPF sudo access and shared directories enabled. Please configure your testbed according to the [NPF run time dependencies][NPF-cluster].

### Data Plane Development Kit (DPDK)
We use DPDK to bypass kernel network stack in order to achieve line rate in our tests. To build DPDK, you can run the following commands:

```
git clone https://github.com/DPDK/dpdk.git
cd dpdk
git checkout v22.07
mkdir build
cd build
meson --prefix $(pwd)/../install .. .
ninja install -j 8
```
In case you want to use a newer (or different) version of DPDK, please check [DPDK documentation][dpdk-doc].

After building DPDK, you have to define `RTE_SDK` and `RTE_TARGET` by running the following commands:

```
export RTE_SDK=<your DPDK root directory>
export RTE_TARGET=x86_64-native-linux-gcc
```
Also, do not forget to setup hugepages. To do so, you can modify `GRUB_CMDLINE_LINUX` variable in `/etc/default/grub` file similar to the following configuration:

```
GRUB_CMDLINE_LINUX="isolcpus=0,1,2,3,4,5,6,7,8,9 iommu=pt intel_iommu=on default_hugepagesz=1G hugepagesz=1G hugepages=32 acpi=on selinux=0 audit=0 nosoftlockup processor.max_cstate=1 intel_idle.max_cstate=0 intel_pstate=on nopti nospec_store_bypass_disable nospectre_v2 nospectre_v1 nospec l1tf=off netcfg/do_not_use_netplan=true mitigations=off"
```


### FAJITA

After building DPDK, you can run the following commands to download and build FAJITA in a given directory:

```
git clone https://github.com/hamidgh09/fastclick.git
cd FAJITA

PKG_CONFIG_PATH={YOUR_DPDK_INSTALL_DIR}/lib/x86_64-linux-gnu/pkgconfig ./configure --enable-dpdk --enable-intel-cpu --verbose --enable-select=poll "CFLAGS=-O3" "CXXFLAGS=-std=c++17 -O3" --disable-dynamic-linking --enable-poll --enable-bound-port-transfer --enable-local --disable-task-stats --enable-cpu-load --enable-dpdk-packet --disable-clone --disable-dpdk-softqueue --enable-research --disable-sloppy --enable-user-timestamp --enable-flow

make clean
make
```

So far, your testbed is ready for running experiments only for FAJITA. To build other frameworks with our developed stateful NFs you can continue the following:

### FastClick

```
git clone https://github.com/tbarbette/fastclick.git
cd fastclick
PKG_CONFIG_PATH={YOUR_DPDK_INSTALL_DIR}/lib/x86_64-linux-gnu/pkgconfig ./configure --enable-dpdk --enable-intel-cpu --verbose --enable-select=poll "CFLAGS=-O3" "CXXFLAGS=-std=c++17 -O3" --disable-dynamic-linking --enable-poll --enable-bound-port-transfer --enable-local --disable-task-stats --enable-cpu-load --enable-dpdk-packet --disable-clone --disable-dpdk-softqueue --enable-research --disable-sloppy --enable-user-timestamp --enable-flow

make clean
make -j 8
```

### VPP

```
git clone https://github.com/FAJITA-Packet-Processing-Framework/custom-vpp.git
cd custom-vpp
sudo make build-release
sudo make build-release
```

### Dyssect
```
git clone https://github.com/tbarbette/fastclick.git dyssect
cd dyssect
cp {FAJITA-EXPERIMETNS-DIR}/extra-elements/dyssect/solver* .
cp {FAJITA-EXPERIMETNS-DIR}/extra-elements/dyssect/elements/* elements/research/ 

PKG_CONFIG_PATH={YOUR_DPDK_INSTALL_DIR}/lib/x86_64-linux-gnu/pkgconfig ./configure --enable-dpdk --enable-intel-cpu --verbose --enable-select=poll "CFLAGS=-O3" "CXXFLAGS=-std=c++17 -O3" --disable-dynamic-linking --enable-poll --enable-bound-port-transfer --enable-local --disable-task-stats --enable-cpu-load --enable-dpdk-packet --disable-clone --disable-dpdk-softqueue --enable-research --disable-sloppy --enable-user-timestamp

make clean 
make -j 8
```
Note that you also need a a valid [Gurobi][gurobi] optimization account. for running Dyssect with the runtime load-balancer.

### Traffic Generator

To run the experiments you need to have a traffic generator on a separate server as explained above. The generator uses FastClick and either replays multiple CAIDA trace windows or uses a run-time synthetic traffic generator. To install the traffic generator you can use the following commands on the generator server:

```
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
make -j 16
```
Additionally you can use instructions provided in [RSS++ artifact source code][rss++] to generate the parrallel windows of CAIDA trace files.

### Updating the Makefile
We already provided a Makefile in `npf-configuration` directory that allows you to run reported experiments in the paper. Please ensure that after installing frameworks and dependencies you update this Makefile with the correct cluster configuration and frameworks directory in lines 3-14 of the Makefile.

## Citing our paper
If you use FAJITA, please cite our paper:

```bibtex
@article{fajita,
    author = {Ghasemirahni, Hamid and Farshin, Alireza and Scazzariello, Mariano and Maguire Jr., Gerald Q. and Kosti\'{c}, Dejan and Chiesa, Marco},
    title = {FAJITA: Stateful Packet Processing at 100 Million pps},
    year = {2024},
    issue_date = {September 2024},
    publisher = {Association for Computing Machinery},
    address = {New York, NY, USA},
    volume = {2},
    number = {CoNEXT3},
    url = {https://doi.org/10.1145/3676861},
    doi = {10.1145/3676861},
    journal = {Proc. ACM Netw.},
    month = {September},
    articleno = {14}
}
```

## Help
If you have any question regarding the code or the paper you can contact me (hamidgr [at] k t h [dot] s e).

[FastClick]: https://github.com/tbarbette/fastclick
[NPF]: https://github.com/tbarbette/npf
[Gnuplot]: http://www.gnuplot.info/
[fajita-paper]: https://google.com
[fajita-repo]: https://github.com/hamidgh09/fastclick
[Dyssect]: https://google.com
[VPP]: https://github.com/FDio/vpp
[tofino-multicast]: https://google.com
[NPF-cluster]: https://npf.readthedocs.io/en/latest/cluster.html
[NPF-setup]: https://npf.readthedocs.io/en/latest/usage.html#run-time-dependencies
[dpdk-doc]: https://doc.dpdk.org/guides/linux_gsg/index.html
[gurobi]: https://www.gurobi.com/
[rss++]: https://github.com/rsspp/experiments/tree/master/traces