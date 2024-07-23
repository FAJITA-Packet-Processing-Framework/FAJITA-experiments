# FAJITA Experiments

FAJITA is a system built on top of [FastClick][Fastclick] to facilitate deploying chains of stateful network functions on commodity servers. In a nutshell, FAJITA (I) minimizes memory access overheads by exploiting batching \& software prefetching machinery available in modern CPUs, and (II) alleviates the overheads of accessing shared data structures by introducing auxiliary hash tables. For more information check out our [paper][fajita-paper] at CoNEXT '24.

This repository contains instructions for using FAJITA and running experiments that are already done in the FAJITA's paper. You can find the source code for FAJITA from [here][fajita-repo]. We plan to merge the essential parts of FAJITA in the Fastclick soon.

## Repository Organization

- `experiments` direcotry contains configuration files and necessary information that you need to run all experiments discussed in the paper.

- `graphs` directory contains all csv results and graphs that already presented in the paper. You can use the output csv of your experiments and use the provided [Gnuplot][Gnuplot] scripts to generate your graphs in the same style as the paper.

## Testbed

**NOTE: Before running the experiments, you need to prepare your testbed according to the following guidelines.**

To reproduce results demonstrated in our [paper][fajita-paper] you need to have a two commidity servers that are connected through a switch. One server acts as a Traffic Generator (TG) and the other is the Device Under Test (DUT) which runs the chain of stateful NFs. 

Since the generated rate on the TG is limited and normally can not go beyond 20Mpps (depending on your hardware setup), we create multiple clones of packets on the switch. You can read more about it in our [paper][fajita-paper]. Also, a proper code compatible with Tofino2 switches for generating clones is available on our [Multicast][tofino-multicast] respository.

All the experiments require [FAJITA][fajita-repo] and [NPF][NPF] tool.
To have a fair comparison with existing packet processing frameworks, we have developed elements and plugins on [FastClick][FastClick], [Dyssect][Dyssect], and [VPP][VPP] with identical behavior among all of them. Hence, to compare FAJITA with any of these frameworks you need to have our customized version of these frameworks as well, which are listed in the testbed setup instructions.

**There is a bash script (`setup_repo.sh`) that could help you to clone/compile different repositories, but you should mainly rely on this README.md.**

Below contains the step-by-step guide for preparing your testbed:

### Network Performance Framework (NPF) Tool

We use NPF to run experiments with different setups and store results in csv files. 
You can install [NPF][NPF] via the following command:

```bash
python3 -m pip install --user npf
```

**Do not forget to add `export PATH=$PATH:~/.local/bin` to `~/.bashrc` or `~/.zshrc`. Otherwise, you cannot run `npf-compare` and `npf-run` commands.** 

NPF will look for `cluster/` and `repo/` in your current working/testie directory. We have included the required `repo` for our experiments and a sample `cluster` template, available at `experiment/`. For more information about how to setup your cluster please check the [NPF guidelines][NPF-cluster].

### Data Plane Development Kit (DPDK)
We use DPDK to bypass kernel network stack in order to achieve line rate in our tests. To build DPDK, you can run the following commands:

```
git clone https://github.com/DPDK/dpdk.git
cd dpdk
git checkout v20.02
make install T=x86_64-native-linux-gcc
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
cd fastclick
git checkout fajita

PKG_CONFIG_PATH={YOUR_DPDK_INSTALL_DIR}/lib/x86_64-linux-gnu/pkgconfig ./configure --enable-dpdk --enable-intel-cpu --verbose --enable-select=poll "CFLAGS=-O3" "CXXFLAGS=-std=c++17 -O3" --disable-dynamic-linking --enable-poll --enable-bound-port-transfer --enable-local --disable-task-stats --enable-cpu-load --enable-dpdk-packet --disable-clone --disable-dpdk-softqueue --enable-research --disable-sloppy --enable-user-timestamp --enable-flow

make clean
make
```

So far, your testbed is ready for running experiments only for FAJITA. To build other frameworks with our developed stateful NFs you can continue the following:

### FastClick

### VPP

### Dyssect


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
[NPF-cluster]: https://github.com/tbarbette/npf/blob/master/cluster/README.md