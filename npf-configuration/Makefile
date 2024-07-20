SHELL := /bin/bash

NPF_CLUSTER?=nf=nslrack35 client=nslrack16,nic=1 tofino=tofino2 
OUTPUT_COLUMNS?=--output-columns x min perc1 perc20 median perc80 perc99 max avg --no-graph-time

FAJITA_CLICK_DIR?=/home/hamid/workspace/prefetch/repo/rack35/fajita/fastclick/
FASTCLICK_CLICK_DIR?=/home/hamid/workspace/prefetch/repo/rack35/fastclick/
VPP_INSTALL_DIR?=/home/hamid/workspace/prefetch/repo/vpp-customized/vpp/
NPF_DIR?=/home/hamid/npf/npf
MAIN_DIR := $(dir $(CURDIR:%/=%))

CORES?=8


various_offered_load:
		python3 ${NPF_DIR}/npf_compare.py \
		"local+test,freshfc,min_batch:fastclick" \
		"local+test:fajita" \
		"local+test,vpp:vpp" \
		--npf ${MAIN_DIR}experiments/offered-load-var/compare-all.npf \
		--cluster ${NPF_CLUSTER} \
		--show-all --show-files --no-build --no-build-deps fastclick fastclick-light \
		--graph-filename various-offered-load/various-offered-load.pdf \
		--output ${OUTPUT_COLUMNS} \
		--show-cmd --config "var_format={result:%.02f}" \
		--config n_runs=1 \
		--graph-size 5 2.5 \
		--force-retest \
		--tags promisc two ratevar \
		--variables FAJITA_CLICK_DIR=${FAJITA_CLICK_DIR} FASTCLICK_CLICK_DIR=${FASTCLICK_CLICK_DIR} VPP_INSTALL_DIR=${VPP_INSTALL_DIR} NF_CPU=${CORES}

various_packet_size:
		python3 ${NPF_DIR}/npf_compare.py \
		"local+test:fajita" \
		"local+test,freshfc,min_batch:fastclick" \
		"local+test,vpp:vpp" \
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
		"local+test:fajita" \
		"local+test,freshfc,min_batch:fastclick" \
		"local+test,vpp:vpp" \
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
		"local+test:fajita" \
		"local+test,freshfc,min_batch:fastclick" \
		"local+test,vpp:vpp" \
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
		"local+test:fajita" \
		"local+test,freshfc,min_batch:fastclick" \
		"local+test,vpp:vpp" \
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
		"local+test:fajita-caida" \
		"local+test,freshfc,min_batch:fastclick-caida" \
		"local+test,vpp:vpp-caida" \
		"local+test,synthetic:fajita-synthetic" \
		"local+test,freshfc,min_batch,synthetic:fastclick-synthetic" \
		"local+test,vpp,synthetic:vpp-synthetic" \
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
		"local+test:fajita" \
		"local+test,freshfc,min_batch:fastclick" \
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
