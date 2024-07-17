SHELL := /bin/bash

NPF_CLUSTER?=nf=nslrack35 client=nslrack16,nic=1 tofino=tofino2 
OUTPUT_COLUMNS?=--output-columns x min perc1 perc20 median perc80 perc99 max avg --no-graph-time

FAJITA_CLICK_DIR?=/home/hamid/workspace/prefetch/repo/rack35/fajita/fastclick/
FASTCLICK_CLICK_DIR?=/home/hamid/workspace/prefetch/repo/rack35/fastclick/
VPP_INSTALL_DIR?=/home/hamid/workspace/prefetch/repo/vpp-customized/vpp/
NPF_DIR?=/home/hamid/workspace/prefetch/repo/npf

various_offered_load:
	${NPF_DIR}/npf_compare.py \
	"local+test,freshfc,min_batch:fastclick" \
	"local+test:fajita" \
	"local+test,vpp:vpp" \
	--npf ../offered-load-var/compare-all.npf \
	--cluster ${NPF_CLUSTER} \
	--show-all --show-files --no-build --no-build-deps fastclick fastclick-light \
	--graph-filename temp-results/res.pdf \
	--output ${OUTPUT_COLUMNS} \
	--show-cmd --config "var_format={result:%.02f}" \
	--config n_runs=5 \
	--graph-size 5 2.5 \
	--force-retest \
	--tags promisc two \
	--variables FAJITA_CLICK_DIR=${FAJITA_CLICK_DIR} FASTCLICK_CLICK_DIR=${FASTCLICK_CLICK_DIR} VPP_INSTALL_DIR=${VPP_INSTALL_DIR}