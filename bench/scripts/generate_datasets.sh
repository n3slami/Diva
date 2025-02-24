#!/bin/bash

#
# This file is part of --- <>.
# Copyright (C) 2024 ---.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

if [ "$#" -ne 2 ]; then
    echo "Illegal number of parameters, usage: generate_datasets.sh <build_path> <real_datasets_path>"
fi

BUILD_PATH=$(realpath $1)
if [ ! -d "$BUILD_PATH" ]; then
    echo "Build path does not exist"
    exit 1
fi
REAL_DATASETS_PATH=$(realpath $2)
if [ ! -d "$REAL_DATASETS_PATH" ]; then
    echo "Real datasets path does not exist"
    exit 1
fi

WORKLOAD_GEN_PATH=$(realpath $BUILD_PATH/bench/workload_gen)
if [ ! -f "$WORKLOAD_GEN_PATH" ]; then
    echo "Workload generator does not exist"
    exit 1
fi

OUT_PATH=$(realpath ./workloads)
generate_corr_bench() {
    short=1024
    long=1048576
    i=0
    x=0.0
    while [ $i -le 5 ]
    do
        $WORKLOAD_GEN_PATH -t correlated --min-range-size 1 --max-range-size 1 --corr-degree ${x} -o unif_0_${i}
        $WORKLOAD_GEN_PATH -t correlated --max-range-size $short --corr-degree ${x} -o unif_10_${i}
        $WORKLOAD_GEN_PATH -t correlated --max-range-size $long  --corr-degree ${x} -o unif_20_${i}
        x=$(echo $x + 0.1 | bc)
        i=$(($i + 1))
    done
}

generate_fpr_bench() {
    i=0
    norm_mu=$(echo 2 ^ 63 | bc)
    norm_std=$(echo 2 ^ 50 | bc)
    norm_byte=2
    while [ $i -le 24 ]
    do
        range_size=$(echo 2 ^ $i | bc)
        $WORKLOAD_GEN_PATH -t standard-int --kdist unif --qdist unif --max-range-size $range_size -o unif_${i}
        $WORKLOAD_GEN_PATH -t standard-int --kdist norm $norm_mu $norm_std --qdist norm $norm_mu $norm_std --max-range-size $range_size -o norm_${i}
        $WORKLOAD_GEN_PATH -t standard-int --kdist real $REAL_DATASETS_PATH/books_200M_uint64 --qdist real --max-range-size $range_size -o books_${i}
        $WORKLOAD_GEN_PATH -t standard-int --kdist real $REAL_DATASETS_PATH/osm_cellids_200M_uint64 --qdist real --max-range-size $range_size -o osm_${i}
        i=$(($i + 4))
    done
    $WORKLOAD_GEN_PATH -t standard-string --kdist unif --max-range-size 1024 -o unif_string
    $WORKLOAD_GEN_PATH -t standard-string --kdist norm $norm_mu $norm_std $norm_byte --max-range-size 1024 -o norm_string
}

generate_true_bench() {
    short=1024
    $WORKLOAD_GEN_PATH -t true --max-range-size $short -o unif

    i=0
    while [ $i -le 24 ]
    do
        range_size=$(echo 2 ^ $i | bc)
        $WORKLOAD_GEN_PATH -t true --kdist unif --qdist unif --min-range-size $range_size --max-range-size $range_size -o unif_${i}
        i=$(($i + 4))
    done
}

generate_expansion_bench() {
    short=$(echo 2 ^ 10 | bc)
    long=$(echo 2 ^ 20 | bc)
    norm_mu=$(echo 2 ^ 63 | bc)
    norm_std=$(echo 2 ^ 50 | bc)
    n_expansions=6

    $WORKLOAD_GEN_PATH -t expansion -e $n_expansions --max-range-size $short -o unif_short
    $WORKLOAD_GEN_PATH -t expansion -e $n_expansions --max-range-size $long -o unif_long
}

generate_delete_bench() {
    short=$(echo 2 ^ 10 | bc)
    long=$(echo 2 ^ 20 | bc)
    norm_mu=$(echo 2 ^ 63 | bc)
    norm_std=$(echo 2 ^ 50 | bc)
    n_deletes=500000

    $WORKLOAD_GEN_PATH -t delete -d $n_deletes -o unif
}

generate_construction_bench() {
    i=5
    x=100000
    while [ $i -le 7 ]
    do
        $WORKLOAD_GEN_PATH -t construction --max-range-size 1024 -n ${x} -q $(echo "($x * 0.1)/1" | bc) -o unif_${i}
        x=$(echo "$x * 10" | bc)
        i=$(($i + 1))
    done
}

generate_wiredtiger_bench() {
    short=$(echo 2 ^ 10 | bc)
    n_expansions=6
    $WORKLOAD_GEN_PATH -t wiredtiger --qdist unif -e $n_expansions --max-range-size $short -o unif_short
}

mkdir -p $OUT_PATH/corr_bench && cd $OUT_PATH/corr_bench || exit 1
if ! generate_corr_bench ; then
    echo "[!!] generate_corr_bench generation failed"
    exit 1
fi
echo "[!!] corr_bench generated"

mkdir -p $OUT_PATH/fpr_bench && cd $OUT_PATH/fpr_bench || exit 1
if ! generate_fpr_bench ; then
    echo "[!!] fpr_bench generation failed"
    exit 1
fi
echo "[!!] fpr_bench generated"

mkdir -p $OUT_PATH/true_bench && cd $OUT_PATH/true_bench || exit 1
if ! generate_true_bench ; then
    echo "[!!] true_bench generation failed"
    exit 1
fi
echo "[!!] true_bench generated"

mkdir -p $OUT_PATH/expansion_bench && cd $OUT_PATH/expansion_bench || exit 1
if ! generate_expansion_bench ; then
    echo "[!!] expansion_bench generation failed"
    exit 1
fi
echo "[!!] expansion_bench generated"

mkdir -p $OUT_PATH/delete_bench && cd $OUT_PATH/delete_bench || exit 1
if ! generate_delete_bench ; then
    echo "[!!] delete_bench generation failed"
    exit 1
fi
echo "[!!] delete_bench generated"

mkdir -p $OUT_PATH/construction_bench && cd $OUT_PATH/construction_bench || exit 1
if ! generate_construction_bench ; then
    echo "[!!] construction_bench generation failed"
    exit 1
fi
echo "[!!] construction_bench generated"

mkdir -p $OUT_PATH/wiredtiger_bench && cd $OUT_PATH/wiredtiger_bench || exit 1
if ! generate_wiredtiger_bench ; then
    echo "[!!] wiredtiger_bench generation failed"
    exit 1
fi
echo "[!!] wiredtiger_bench generated"

echo "[!!] success, all workloads generated"
