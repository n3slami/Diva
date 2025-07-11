import argparse, shutil, itertools, subprocess
from pathlib import Path
from datetime import datetime

global build_dir
global workload_dir
global benchmarks_dir
global output_prefix
RANGE_FIXED_FILTERS = ["memento", "memento_expandable", "rosetta", "proteus"]

def execute_benchmark(build_dir, output_base, workload_subdir, workload, filter, bpk, force_range_size=None, wiredtiger=False):
    file_to_execute = f"bench/bench_{filter}_wiredtiger" if wiredtiger else f"bench/bench_{filter}"
    range_size_option = f"--range-size {force_range_size}" if force_range_size else ""
    command = f"{build_dir}/{file_to_execute} {bpk} -w {workload} {range_size_option} | tee {output_base}/{filter}_{bpk}_{workload.name}.json"
    cli_message_command = f"<build_dir>/{file_to_execute} {bpk} -w <workload_dir>/{workload_subdir}/{workload.name} {range_size_option} | tee <output_dir>/{workload_subdir}/{filter}_{bpk}_{workload.name}.json"

    print(f"[ Executing: {cli_message_command} ]")
    subprocess.run(command, shell=True)
    print("[ Command finished ]")

def corr_bench():
    filters = ["diva", "diva_int", "memento", "grafite", "surf",
               "rosetta", "proteus", "rencoder", "snarf", "oasis"]
    memory_footprints = [16]
    workload_subdir = "corr_bench"
    output_base = Path(f"./{output_prefix}/{workload_subdir}/")
    output_base.mkdir(parents=True, exist_ok=True)

    workload_path = Path(f"{workload_dir}/{workload_subdir}")
    for workload in workload_path.iterdir():
        if workload.is_file():
            print(workload.name)
            for filter, bpk in itertools.product(filters, memory_footprints):
                execute_benchmark(build_dir, output_base, workload_subdir, workload, filter, bpk)

def fpr_bench():
    filters = ["diva", "diva_int", "memento", "grafite", "surf",
               "rosetta", "proteus", "rencoder", "snarf", "oasis"]
    memory_footprints = [10, 16]
    MEDIAN_RANGE_SIZE = 2 ** 10
    workload_subdir = "fpr_bench"
    output_base = Path(f"./{output_prefix}/{workload_subdir}/")
    output_base.mkdir(parents=True, exist_ok=True)

    workload_path = Path(f"{workload_dir}/{workload_subdir}")
    for workload in workload_path.iterdir():
        if workload.is_file() and "string" not in workload.name:
            for filter, bpk in itertools.product(filters, memory_footprints):
                if filter == "oasis" and "osm" in workload.name:
                    continue
                execute_benchmark(build_dir, output_base, workload_subdir, workload, filter, bpk,
                                  MEDIAN_RANGE_SIZE if filter in RANGE_FIXED_FILTERS else None)

    memory_footprints = [8, 10, 12, 14, 16, 18]
    for workload in workload_path.iterdir():
        if (workload.is_file() and "string" not in workload.name) and ("unif" in workload.name or "osm" in workload.name):
            for filter, bpk in itertools.product(filters, memory_footprints):
                if filter == "oasis" and "osm" in workload.name:
                    continue
                execute_benchmark(build_dir, output_base, workload_subdir, workload, filter, bpk,
                                  MEDIAN_RANGE_SIZE if filter in RANGE_FIXED_FILTERS else None)

def fpr_string_bench():
    filters = ["diva", "surf"]
    memory_footprints = [12, 14, 16, 18, 20]
    workload_subdir = "fpr_bench"
    output_base = Path(f"./{output_prefix}/{workload_subdir}/")
    output_base.mkdir(parents=True, exist_ok=True)

    workload_path = Path(f"{workload_dir}/{workload_subdir}")
    for workload in workload_path.iterdir():
        if workload.is_file() and workload.name.endswith("string"):
            for filter, bpk in itertools.product(filters, memory_footprints):
                execute_benchmark(build_dir, output_base, workload_subdir, workload, filter, bpk)

def true_bench():
    filters = ["diva", "diva_int", "memento", "grafite", "surf",
               "rosetta", "proteus", "rencoder", "snarf", "oasis"]
    memory_footprints = [10, 12, 14, 16, 18, 20]
    DEFAULT_MEMORY_FOOTPRINT = 16
    MEDIAN_RANGE_SIZE = 2 ** 7
    workload_subdir = "true_bench"
    output_base = Path(f"./{output_prefix}/{workload_subdir}/")
    output_base.mkdir(parents=True, exist_ok=True)

    workload_path = Path(f"{workload_dir}/{workload_subdir}")
    for workload in workload_path.iterdir():
        if workload.is_file():
            if workload.name == "unif":
                for filter, bpk in itertools.product(filters, memory_footprints):
                    execute_benchmark(build_dir, output_base, workload_subdir, workload, filter, bpk)
            else:
                for filter in filters:
                    execute_benchmark(build_dir, output_base, workload_subdir, workload, filter, DEFAULT_MEMORY_FOOTPRINT, 
                                      MEDIAN_RANGE_SIZE if filter in RANGE_FIXED_FILTERS else None)

def expansion_bench():
    filters = ["diva", "diva_int", "memento_expandable", "rosetta",
               "rencoder", "snarf"]
    memory_footprints = [16]
    workload_subdir = "expansion_bench"
    output_base = Path(f"./{output_prefix}/{workload_subdir}/")
    output_base.mkdir(parents=True, exist_ok=True)

    workload_path = Path(f"{workload_dir}/{workload_subdir}")
    for workload in workload_path.iterdir():
        if workload.is_file():
            for filter, bpk in itertools.product(filters, memory_footprints):
                execute_benchmark(build_dir, output_base, workload_subdir, workload,
                                  filter, bpk - (1 if "diva" in filter else 0) \
                                              + (1 if "memento" in filter else 0))

def delete_bench():
    filters = ["diva", "diva_int", "memento_expandable", "snarf"]
    memory_footprints = [16]
    workload_subdir = "delete_bench"
    output_base = Path(f"./{output_prefix}/{workload_subdir}/")
    output_base.mkdir(parents=True, exist_ok=True)
    MEMENTO_RANGE_SIZE = 2 ** 10

    workload_path = Path(f"{workload_dir}/{workload_subdir}")
    for workload in workload_path.iterdir():
        if workload.is_file():
            for filter, bpk in itertools.product(filters, memory_footprints):
                execute_benchmark(build_dir, output_base, workload_subdir, workload, filter, bpk,
                                  MEMENTO_RANGE_SIZE if filter in RANGE_FIXED_FILTERS else None)

def construction_bench():
    filters = ["surf", "rosetta", "proteus", "rencoder", "snarf", "oasis",
               "memento", "diva", "diva_int", "grafite"]
    memory_footprints = [16]
    workload_subdir = "construction_bench"
    output_base = Path(f"./{output_prefix}/{workload_subdir}/")
    output_base.mkdir(parents=True, exist_ok=True)

    workload_path = Path(f"{workload_dir}/{workload_subdir}")
    for workload in workload_path.iterdir():
        if workload.is_file():
            for filter, bpk in itertools.product(filters, memory_footprints):
                execute_benchmark(build_dir, output_base, workload_subdir, workload, filter, bpk)

def wiredtiger_bench():
    filters = ["diva", "diva_int", "memento_expandable", "base"]
    memory_footprints = [16]
    workload_subdir = "wiredtiger_bench"
    output_base = Path(f"./{output_prefix}/{workload_subdir}/")
    output_base.mkdir(parents=True, exist_ok=True)

    workload_path = Path(f"{workload_dir}/{workload_subdir}")
    for workload in workload_path.iterdir():
        if workload.is_file():
            for filter, bpk in itertools.product(filters, memory_footprints):
                remove_amount = 1 if "diva" in filter else 0
                execute_benchmark(build_dir, output_base, workload_subdir, workload,
                                  filter, bpk - remove_amount, wiredtiger=True)

def mixed_bench():
    filters = ["diva", "diva_int", "memento", "grafite", "surf",
               "rosetta", "proteus", "rencoder", "snarf", "oasis"]
    filters = ["rosetta"]
    memory_footprints = [10, 12, 14, 16, 18, 20]
    DEFAULT_MEMORY_FOOTPRINT = 16
    MEDIAN_RANGE_SIZE = 2 ** 7
    workload_subdir = "mixed_bench"
    output_base = Path(f"./{output_prefix}/{workload_subdir}/")
    output_base.mkdir(parents=True, exist_ok=True)

    workload_path = Path(f"{workload_dir}/{workload_subdir}")
    for workload in workload_path.iterdir():
        if workload.is_file():
            if workload.name.endswith("_12"):
                for filter, bpk in itertools.product(filters, memory_footprints):
                    execute_benchmark(build_dir, output_base, workload_subdir, workload, filter, bpk)
            else:
                for filter in filters:
                    execute_benchmark(build_dir, output_base, workload_subdir, workload, filter, DEFAULT_MEMORY_FOOTPRINT, 
                                      MEDIAN_RANGE_SIZE if filter in RANGE_FIXED_FILTERS else None)

RUNNERS = {"fpr": fpr_bench,
           "fpr_string": fpr_string_bench,
           "true": true_bench,
           "construction": construction_bench,
           "expansion": expansion_bench,
           "delete": delete_bench,
           "wiredtiger": wiredtiger_bench,
           "mixed": mixed_bench}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(prog="run_benchmarks")
    parser.add_argument("build_dir", type=Path, help="The directory containing the benchmark binaries")
    parser.add_argument("workload_dir", type=Path, help="The directory containing the generated workloads")
    parser.add_argument("-b", "--benchmarks", nargs="+", choices=["all",] + list(RUNNERS.keys()),
                        default=["all"], type=str, help="The benchmarks to run")

    args = parser.parse_args()
    build_dir = args.build_dir
    workload_dir = args.workload_dir

    if not workload_dir.exists():
        raise FileNotFoundError("The workload directory does not exist")
    if not build_dir.exists():
        raise FileNotFoundError("The build directory does not exist")

    benchmarks_dir = Path(f"{build_dir}/bench/")
    output_prefix = Path(f"results/{datetime.now().strftime('%Y-%m-%d.%H:%M:%S')}")

    try:
        for benchmark in (RUNNERS if "all" in args.benchmarks else args.benchmarks):
            RUNNERS[benchmark]()
    except Exception as e:
        print(f"Received exception: {str(e)}, cleaning up output and closing")
        shutil.rmtree(output_prefix)

