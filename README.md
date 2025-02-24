# Diva
Diva is the first range filter to simultaneously support **d**ynam**i**c
operations, **va**riable-length keys, range queries of any length, and high
performance while providing a theoretical false positive rate guarantee. This
false positive guarantee holds when the input data comes from a "well-behaved"
distribution, which encompasses the common distributions seen in practice.

Our paper describes the inner workings of Diva in detail. It also presents
an in-depth theoretical analysis as well as empricial evaluations.

# Getting Started
To use Diva in your own project, simply add the files in the `include`
directory and include the `diva.hpp` header file.

The file `pashm` presents an example of how to use Diva's APIs.

# Building
The following software are prerequisites for building Diva and its evaluations
suite:
- gcc-11 (or later)
- Boost 1.67.0 (or later)
- Git 2.13 (or later)
- Python 3.8 (or later)
- Bash 4.4 (or later)
  - realpath 8.28 (or later)
  - wget 1.19.4 (or later)
  - zsdt 0.0.1 (or later)
  - md5sum 8.28 (or later)

# Running Tests
After building Diva, run the following command from the project's root director
to execute the unit tests:
```
ctest -VV
```

# Benchmarks
TLDR;

The following software is required to reproduce the experiments of the paper:

We advise to use a Linux machine with at least 64GB of RAM.

# Plotting
Once the results are gathered, run the script

The following is required for plotting the experiment results:
- Python 3.8 (or later)
  - Matplotlib 3.2.1 (or later)
- Latex full (for generating the plots)

# Citation
Please use the following BibTeX entry to cite our paper presenting Diva:
```{bibtex}
@article{}
```

