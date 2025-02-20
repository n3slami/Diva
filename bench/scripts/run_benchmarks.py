import argparse, shutil, itertools, subprocess
from pathlib import Path
from datetime import datetime

global build_dir
global workload_dir
global benchmarks_dir
global output_prefix

def execute_benchmark(build_dir, output_base, workload_subdir, workload, filter, bpk, force_range_size=None, wiredtiger=False):
    file_to_execute = f"bench/bench_{filter}_wiredtiger" if wiredtiger else f"bench/bench_{filter}"
    range_size_option = f"--range_size {force_range_size}" if force_range_size else ""
    command = f"{build_dir}/{file_to_execute} {bpk} -w {workload} {range_size_option} | tee {output_base}/{filter}_{bpk}_{workload.name}.json"
    cli_message_command = f"<build_dir>/{file_to_execute} {bpk} -w <workload_dir>/{workload_subdir}/{workload.name} {range_size_option} | tee <output_dir>/{workload_subdir}/{filter}_{bpk}_{workload.name}.json"

    print(f"[ Executing: {cli_message_command} ]")
    subprocess.run(command, shell=True)
    print("[ Command finished ]")

def corr_bench():
    filters = ["steroids", "steroids_int", "memento", "grafite", "surf",
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
    filters = ["steroids", "steroids_int", "memento", "grafite", "surf",
               "rosetta", "proteus", "rencoder", "snarf", "oasis"]
    # filters = ["memento", "rosetta"]
    memory_footprints = [10, 16]
    MEDIAN_RANGE_SIZE = 2 ** 7
    workload_subdir = "fpr_bench"
    output_base = Path(f"./{output_prefix}/{workload_subdir}/")
    output_base.mkdir(parents=True, exist_ok=True)

    workload_path = Path(f"{workload_dir}/{workload_subdir}")
    for workload in workload_path.iterdir():
        if workload.is_file() and "string" not in workload.name:
            for filter, bpk in itertools.product(filters, memory_footprints):
                if filter == "oasis" and "osm" in workload.name:
                    continue
                if filter in ["memento", "rosetta", "proteus"]:
                    execute_benchmark(build_dir, output_base, workload_subdir, workload, filter, bpk, MEDIAN_RANGE_SIZE)
                else:
                    execute_benchmark(build_dir, output_base, workload_subdir, workload, filter, bpk)

    memory_footprints = [8, 10, 12, 14, 16, 18]
    for workload in workload_path.iterdir():
        if (workload.is_file() and "string" not in workload.name) and ("unif" in workload.name or "osm" in workload.name):
            for filter, bpk in itertools.product(filters, memory_footprints):
                if filter == "oasis" and "osm" in workload.name:
                    continue
                if filter in ["memento", "rosetta", "proteus"]:
                    execute_benchmark(build_dir, output_base, workload_subdir, workload, filter, bpk, MEDIAN_RANGE_SIZE)
                else:
                    execute_benchmark(build_dir, output_base, workload_subdir, workload, filter, bpk)

def fpr_string_bench():
    filters = ["steroids", "surf"]
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
    #filters = ["steroids", "steroids_int", "memento", "grafite", "surf",
    #           "rosetta", "proteus", "rencoder", "snarf", "oasis"]
    filters = ["steroids", "steroids_int"]
    memory_footprints = [10, 12, 14, 16, 18, 20]
    workload_subdir = "true_bench"
    output_base = Path(f"./{output_prefix}/{workload_subdir}/")
    output_base.mkdir(parents=True, exist_ok=True)

    workload_path = Path(f"{workload_dir}/{workload_subdir}")
    for workload in workload_path.iterdir():
        if workload.is_file():
            for filter, bpk in itertools.product(filters, memory_footprints):
                execute_benchmark(build_dir, output_base, workload_subdir, workload, filter, bpk)

def expansion_bench():
    #filters = ["steroids", "steroids_int", "memento_expandable", "rosetta",
    #           "rencoder", "snarf"]
    filters = ["memento_expandable"]
    memory_footprints = [16]
    workload_subdir = "expansion_bench"
    output_base = Path(f"./{output_prefix}/{workload_subdir}/")
    output_base.mkdir(parents=True, exist_ok=True)

    workload_path = Path(f"{workload_dir}/{workload_subdir}")
    for workload in workload_path.iterdir():
        if workload.is_file() and "unif" in workload.name:
            for filter, bpk in itertools.product(filters, memory_footprints):
                execute_benchmark(build_dir, output_base, workload_subdir, workload,
                                  filter, bpk - (2 if "steroids" in filter else 0))

def delete_bench():
    filters = ["steroids", "steroids_int", "memento_expandable", "snarf"]
    memory_footprints = [16]
    workload_subdir = "delete_bench"
    output_base = Path(f"./{output_prefix}/{workload_subdir}/")
    output_base.mkdir(parents=True, exist_ok=True)
    MEMENTO_RANGE_SIZE = 2 ** 10

    workload_path = Path(f"{workload_dir}/{workload_subdir}")
    for workload in workload_path.iterdir():
        if workload.is_file():
            for filter, bpk in itertools.product(filters, memory_footprints):
                if filter == "memento_expandable":
                    execute_benchmark(build_dir, output_base, workload_subdir, workload, filter, bpk, MEMENTO_RANGE_SIZE)
                else:
                    execute_benchmark(build_dir, output_base, workload_subdir, workload, filter, bpk)

def construction_bench():
    #filters = ["surf", "rosetta", "proteus", "rencoder", "snarf", "oasis",
    #           "memento", "steroids", "steroids_int", "grafite"]
    filters = ["steroids", "steroids_int"]
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
    filters = ["steroids", "steroids_int", "memento_expandable", "base"]
    memory_footprints = [16]
    workload_subdir = "wiredtiger_bench"
    output_base = Path(f"./{output_prefix}/{workload_subdir}/")
    output_base.mkdir(parents=True, exist_ok=True)

    workload_path = Path(f"{workload_dir}/{workload_subdir}")
    for workload in workload_path.iterdir():
        if workload.is_file():
            for filter, bpk in itertools.product(filters, memory_footprints):
                remove_amount = 1 if "steroids" in filter else 0
                execute_benchmark(build_dir, output_base, workload_subdir, workload,
                                  filter, bpk - remove_amount, wiredtiger=True)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(prog="run_benchmarks")
    parser.add_argument("build_dir", type=Path, help="The directory containing the benchmark binaries")
    parser.add_argument("workload_dir", type=Path, help="The directory containing the generated workloads")

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
        corr_bench()
        fpr_string_bench()
        fpr_bench()
        true_bench()
        construction_bench()
        expansion_bench()
        delete_bench()
        wiredtiger_bench()
    except Exception as e:
        print(f"Received exception: {str(e)}, cleaning up output and closing")
        shutil.rmtree(output_prefix)

