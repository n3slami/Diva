#!/bin/bash
function download_strings {
    curl -O --location https://crackstation.net/files/crackstation-human-only.txt.gz
    gunzip crackstation-human-only.txt
    cat crackstation-human-only.txt | sed 's/ //g' | LC_ALL='C' sed 's/[\d128-\d255]//g' | LC_ALL='C' sed 's/[\d0-\d31]//g' | sed '/^$/d' | shuf | head -n 27549660 | LC_ALL='C' sort  > email_list.txt
}

(
    cd ../SuRF_standalone/SuRF/bench/workload_gen/
    [[ -d ../workloads ]] || (echo "Downloading YCSB..." && bash ycsb_download.sh)
    [[ -f email_list.txt ]] || (echo "Downloading strings..." && download_strings)
    echo "Using the following YCSB specification:"
    cat workload_spec/workloadc_email_uniform | tail -n 25
    python2 gen_load.py email uniform
    python2 gen_txn.py email uniform
    cd ../workloads
    cat txn_email_uniform | python -c "import sys; print(''.join([line[:-2]+ chr(ord(line[-2])+128)+'\n' for line in sys.stdin]), end='')" > upper_email_uniform
    echo "Files generated in $(pwd)"
)
