SHELL := /bin/bash

NPF_CLUSTER?=nf=nslrack35 client=nslrack16,nic=1 tofino=tofino2 
OUTPUT_COLUMNS?=--output-columns x min perc1 perc20 median perc80 perc99 max avg --no-graph-time

FAJITA_CLICK_DIR?=/home/hamid/workspace/prefetch/repo/rack35/fajita/fastclick/
FASTCLICK_CLICK_DIR?=/home/hamid/workspace/prefetch/repo/rack35/fastclick/
VPP_INSTALL_DIR?=/home/hamid/workspace/prefetch/repo/vpp-customized/vpp/
NPF_DIR?=/home/hamid/npf/npf
MAIN_DIR := $(dir $(CURDIR:%/=%))

CORES?=8
N_RUNS?=1


various_offered_load:
		python3 ${NPF_DIR}/npf_compare.py \
		"local+test:Fajita" \
		"local+test,freshfc,min_batch:FastClick" \
		"local+test,vpp:VPP" \
		--npf ${MAIN_DIR}experiments/offered-load-var/compare-all.npf \
		--cluster ${NPF_CLUSTER} \
		--show-all --show-files --no-build --no-build-deps fastclick fastclick-light \
		--graph-filename various-offered-load/various-offered-load.pdf \
		--output ${OUTPUT_COLUMNS} \
		--show-cmd --config "var_format={result:%.02f}" \
		--config n_runs=${N_RUNS} \
		--graph-size 5 2.5 \
		--force-retest \
		--tags promisc two ratevar \
		--variables FAJITA_CLICK_DIR=${FAJITA_CLICK_DIR} FASTCLICK_CLICK_DIR=${FASTCLICK_CLICK_DIR} VPP_INSTALL_DIR=${VPP_INSTALL_DIR} NF_CPU=${CORES}

various_packet_size:
		python3 ${NPF_DIR}/npf_compare.py \
		"local+test:Fajita" \
		"local+test,freshfc,min_batch:FastClick" \
		"local+test,vpp:VPP" \
		--npf ${MAIN_DIR}experiments/packet-size-var/packet-size-var.npf \
		--cluster ${NPF_CLUSTER} \
		--show-all --show-files --no-build --no-build-deps fastclick fastclick-light \
		--graph-filename packet-size-var/packet-size-var.pdf \
		--output ${OUTPUT_COLUMNS} \
		--show-cmd --config "var_format={result:%.02f}" \
		--config n_runs=1 \
		--graph-size 5 2.5 \
		--force-retest \
		--tags promisc three synthetic gen_nolat pktsizevar \
		--variables NF_CPU=4 MAXLENGTH=1500 GEN_RATE=12000000 FAJITA_CLICK_DIR=${FAJITA_CLICK_DIR} FASTCLICK_CLICK_DIR=${FASTCLICK_CLICK_DIR} VPP_INSTALL_DIR=${VPP_INSTALL_DIR}

scalability-1NF:
		python3 ${NPF_DIR}/npf_compare.py \
		"local+test:Fajita" \
		"local+test,freshfc,min_batch:FastClick" \
		"local+test,vpp:VPP" \
		--npf ${MAIN_DIR}experiments/scalability/scalability.npf \
		--cluster ${NPF_CLUSTER} \
		--show-all --show-files --no-build --no-build-deps fastclick fastclick-light \
		--graph-filename scalability-results/scalability-1NF.pdf \
		--output ${OUTPUT_COLUMNS} \
		--show-cmd --config "var_format={result:%.02f}" \
		--config n_runs=1 \
		--graph-size 5 2.5 \
		--force-retest \
		--tags promisc one gen_nolat coresvar \
		--variables GEN_RATE=25000000 FAJITA_CLICK_DIR=${FAJITA_CLICK_DIR} FASTCLICK_CLICK_DIR=${FASTCLICK_CLICK_DIR} VPP_INSTALL_DIR=${VPP_INSTALL_DIR}

scalability-2NFs:
		python3 ${NPF_DIR}/npf_compare.py \
		"local+test:Fajita" \
		"local+test,freshfc,min_batch:FastClick" \
		"local+test,vpp:VPP" \
		--npf ${MAIN_DIR}experiments/scalability/scalability.npf \
		--cluster ${NPF_CLUSTER} \
		--show-all --show-files --no-build --no-build-deps fastclick fastclick-light \
		--graph-filename scalability-results/scalability-2NFs.pdf \
		--output ${OUTPUT_COLUMNS} \
		--show-cmd --config "var_format={result:%.02f}" \
		--config n_runs=1 \
		--graph-size 5 2.5 \
		--force-retest \
		--tags promisc two gen_nolat coresvar \
		--variables GEN_RATE=25000000 FAJITA_CLICK_DIR=${FAJITA_CLICK_DIR} FASTCLICK_CLICK_DIR=${FASTCLICK_CLICK_DIR} VPP_INSTALL_DIR=${VPP_INSTALL_DIR}

scalability-3NFs:
		python3 ${NPF_DIR}/npf_compare.py \
		"local+test:Fajita" \
		"local+test,freshfc,min_batch:FastClick" \
		"local+test,vpp:VPP" \
		--npf ${MAIN_DIR}experiments/scalability/scalability.npf \
		--cluster ${NPF_CLUSTER} \
		--show-all --show-files --no-build --no-build-deps fastclick fastclick-light \
		--graph-filename scalability-results/scalability-3NFs.pdf \
		--output ${OUTPUT_COLUMNS} \
		--show-cmd --config "var_format={result:%.02f}" \
		--config n_runs=1 \
		--graph-size 5 2.5 \
		--force-retest \
		--tags promisc three gen_nolat coresvar \
		--variables GEN_RATE=25000000 FAJITA_CLICK_DIR=${FAJITA_CLICK_DIR} FASTCLICK_CLICK_DIR=${FASTCLICK_CLICK_DIR} VPP_INSTALL_DIR=${VPP_INSTALL_DIR}

workload-var:
		python3 ${NPF_DIR}/npf_compare.py \
		"local+test:Fajita-caida" \
		"local+test,freshfc,min_batch:FastClick-caida" \
		"local+test,vpp:VPP-caida" \
		"local+test,synthetic:Fajita-synthetic" \
		"local+test,freshfc,min_batch,synthetic:FastClick-synthetic" \
		"local+test,vpp,synthetic:VPP-synthetic" \
		--npf ${MAIN_DIR}experiments/scalability/scalability.npf \
		--cluster ${NPF_CLUSTER} \
		--show-all --show-files --no-build --no-build-deps fastclick fastclick-light \
		--graph-filename workload-var-results/workload-var.pdf \
		--output ${OUTPUT_COLUMNS} \
		--show-cmd --config "var_format={result:%.02f}" \
		--config n_runs=1 \
		--graph-size 5 2.5 \
		--force-retest \
		--tags promisc three gen_nolat \
		--variables NF_CPU=8 GEN_RATE=25000000 FAJITA_CLICK_DIR=${FAJITA_CLICK_DIR} FASTCLICK_CLICK_DIR=${FASTCLICK_CLICK_DIR} VPP_INSTALL_DIR=${VPP_INSTALL_DIR}

statefulness-impact:
		python3 ${NPF_DIR}/npf_compare.py \
		"local+test:Fajita" \
		"local+test,freshfc,min_batch:FastClick" \
		--npf ${MAIN_DIR}experiments/scalability/scalability.npf \
		--cluster ${NPF_CLUSTER} \
		--show-all --show-files --no-build --no-build-deps fastclick fastclick-light \
		--graph-filename statefulness/statefulness.pdf \
		--output ${OUTPUT_COLUMNS} \
		--show-cmd --config "var_format={result:%.02f}" \
		--config n_runs=1 \
		--graph-size 5 2.5 \
		--force-retest \
		--tags promisc one synthetic perf gen_nolat flowsvar\
		--variables NF_CPU=8 GEN_RATE=25000000 FAJITA_CLICK_DIR=${FAJITA_CLICK_DIR} FASTCLICK_CLICK_DIR=${FASTCLICK_CLICK_DIR} VPP_INSTALL_DIR=${VPP_INSTALL_DIR}

auxiliary-ht:
		python3 ${NPF_DIR}/npf_compare.py \
		"local+test,aux:Fajita-with-AUX-HT" \
		"local+test:Fajita-without-AUT-HT" \
		"local+test,freshfc,min_batch:FastClick" \
		"local+test,vpp:vpp" \
		--npf ${MAIN_DIR}experiments/scalability/scalability.npf \
		--cluster ${NPF_CLUSTER} \
		--show-all --show-files --no-build --no-build-deps fastclick fastclick-light \
		--graph-filename q/auxiliary.pdf \
		--output ${OUTPUT_COLUMNS} \
		--show-cmd --config "var_format={result:%.02f}" \
		--config n_runs=1 \
		--graph-size 5 2.5 \
		--force-retest \
		--tags promisc share synthetic gen_nolat coresvar \
		--variables GEN_RATE=25000000 FAJITA_CLICK_DIR=${FAJITA_CLICK_DIR} FASTCLICK_CLICK_DIR=${FASTCLICK_CLICK_DIR} VPP_INSTALL_DIR=${VPP_INSTALL_DIR}

dyssect-compare:
		python3 ${NPF_DIR}/npf_compare.py \
		"local+test,lb:Fajita" \
		"local+test,dyssect:Dyssect - w/o solver" \
		"local+test,dyssect,solver:Dyssect - w/ solver" \
		--npf ${MAIN_DIR}experiments/imbalance-impact/dyssect-compare.npf \
		--cluster ${NPF_CLUSTER} \
		--show-all --show-files --no-build --no-build-deps fastclick fastclick-light \
		--graph-filename imbalance/dyssect-compare.pdf \
		--output ${OUTPUT_COLUMNS} \
		--show-cmd --config "var_format={result:%.02f}" \
		--config n_runs=1 \
		--graph-size 5 2.5 \
		--force-retest \
		--tags promisc timing coresvar \
		--variables GEN_RATE=25000000 FAJITA_CLICK_DIR=${FAJITA_CLICK_DIR} FASTCLICK_CLICK_DIR=${FASTCLICK_CLICK_DIR} VPP_INSTALL_DIR=${VPP_INSTALL_DIR}

imbalance-impact:
		python3 ${NPF_DIR}/npf_compare.py \
		"local+test,lb:Fajita" \
		"local+test,dyssect,solver:Dyssect - w/ solver" \
		--npf ${MAIN_DIR}experiments/imbalance-impact/dyssect-compare.npf \
		--cluster ${NPF_CLUSTER} \
		--show-all --show-files --no-build --no-build-deps fastclick fastclick-light \
		--graph-filename imbalance/dyssect-compare.pdf \
		--output ${OUTPUT_COLUMNS} \
		--show-cmd --config "var_format={result:%.02f}" \
		--config n_runs=1 \
		--graph-size 5 2.5 \
		--force-retest \
		--tags promisc synthetic \
		--variables GEN_THREADS=4 NF_CPU=8 GEN_RATE=25000000 FAJITA_CLICK_DIR=${FAJITA_CLICK_DIR} FASTCLICK_CLICK_DIR=${FASTCLICK_CLICK_DIR} VPP_INSTALL_DIR=${VPP_INSTALL_DIR}
