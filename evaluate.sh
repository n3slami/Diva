#!/bin/bash

project_root=$(pwd)

cd .. && mkdir paper_results && cd paper_results
bash ${project_root}/bench/scripts/download_datasets.sh
bash ${project_root}/bench/scripts/generate_datasets.sh ${project_root}/build real_datasets
python3 ${project_root}/bench/scripts/run_benchmarks.py ${project_root}/build workloads
python3 ${project_root}/bench/scripts/plot.py
