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

import argparse
import json
import numpy as np
from copy import deepcopy
import matplotlib
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import itertools, os
from pathlib import Path
import logging


rc_fonts = {
    "font.family": "serif",
    "font.size": 9.5,
    "text.usetex": True,
    'text.latex.preamble': r'\usepackage{mathpazo}'}
matplotlib.rcParams.update(rc_fonts)

logging.getLogger().setLevel(logging.INFO)

RANGE_FILTERS_STYLE_KWARGS = {"steroids_int": {"marker": 'v', "color": "fuchsia", "zorder": 12, "label": "Diva (Int)"},
                              "steroids": {"marker": 'v', "color": "fuchsia", "zorder": 12, "label": "Diva", "linestyle": ":"},
                              "memento": {"marker": '4', "color": "C1", "zorder": 11, "label": "Memento"},
                              "memento_expandable": {"marker": '4', "color": "C1", "zorder": 11, "label": "Memento"},
                              "grafite": {"marker": 'o', "color": "teal", "label": "Grafite"},
                              "base": {"marker": 'x', "color": "dimgray", "zorder": 10, "label": "Baseline"},
                              "snarf": {"marker": '^', "color": "black", "label": "SNARF"},
                              "oasis": {"marker": '+', "color": "darkkhaki", "label": "Oasis+"},
                              "surf": {"marker": 's', "color": "C2", "label": "SuRF"},
                              "proteus": {"marker": 'X', "color": "dimgray", "label": "Proteus"},
                              "proteus_mistuned": {"marker": 'X', "color": "dimgray", "label": "Proteus (Diff. Tune)", "linestyle": ":"},
                              "rosetta": {"marker": 'd', "color": "C4", "label": "Rosetta"},
                              "rencoder": {"marker": '>', "color": "C5", "label": "REncoder"}}
RANGE_FILTERS_HATCHES = {"steroids_int": '',
                         "steroids": '...',
                         "memento": '///',
                         "memento_expandable": '///',
                         "grafite": '\\\\\\',
                         "none": '|',
                         "snarf": '---',
                         "oasis": '++++',
                         "surf": 'xxx',
                         "proteus": 'o',
                         "rosetta": 'O',
                         "rencoder": '*'}
LINES_STYLE = {"markersize": 4, "linewidth": 0.7, "fillstyle": "none"}
DATASET_NAMES = {"unif": r"$\textsc{Uniform}$", 
                 "norm": r"$\textsc{Normal}$",
                 "corr": r"$\textsc{Correlated}$", 
                 "real": r"$\textsc{Real}$", 
                 "books": r"$\textsc{Books}$",
                 "osm": r"$\textsc{OSM}$",
                 "fb": r"$\textsc{FB}$"}
RANGE_LENGTH_NAMES = {"point": "Point",
                      "short": "Range ($R=2^{10}$)",
                      "long": "Range ($R=2^{20}$)"}
RANGE_LENGTH_TAGS = {"short": "$R=2^{10}$",
                     "long": "$R=2^{20}$"}


def fix_file_contents(contents):
    contents = contents[contents.find('{'):]
    contents = contents.replace("nan", "0")
    last_comma = -1
    commas_to_remove = []
    for i in range(len(contents)):
        if contents[i] == ',':
            last_comma = i
        elif contents[i] == '}':
            commas_to_remove.append(last_comma)
    for i in reversed(commas_to_remove):
        contents = contents[:i] + contents[i+1:]
    return contents


def plot_fpr(result_dir, output_dir):
    TITLE_FONT_SIZE = 9.5
    LEGEND_FONT_SIZE = 7
    YLABEL_FONT_SIZE = 9.5
    XLABEL_FONT_SIZE = 9.5
    XTICK_FONT_SIZE = 8
    WIDTH = 8
    HEIGHT = 5
    YTICKS_SMALL = [1, 1e-01, 1e-02, 1e-03]
    YTICKS = [1, 1e-01, 1e-02, 1e-03, 1e-04, 1e-05]

    workloads = ["unif", "norm", "books", "osm"]
    filters = ["steroids", "steroids_int", "memento", "grafite", "surf",
               "rosetta", "proteus", "proteus_mistuned", "rencoder", "snarf",
               "oasis"]
    memory_footprints = [10, 16]
    range_sizes = [0, 4, 8, 12, 16, 20, 24]
    workload_subdir = Path("fpr_bench")

    fig, axes = plt.subplots(nrows=len(memory_footprints) + 1, ncols=len(workloads),
                             sharex=True, figsize=(WIDTH, HEIGHT))
    for i, memory_footprint in enumerate(memory_footprints):
        for j, workload in enumerate(workloads):
            plot_data = {filter: [] for filter in filters}
            speed_plot_data = {filter: [] for filter in filters}
            for filter, range_size in itertools.product(filters, range_sizes):
                file_path = result_dir / workload_subdir / Path(f"{filter}_{memory_footprint}_{workload}_{range_size}.json")
                if not file_path.is_file():
                    continue
                with open(file_path, 'r') as result_file:
                    contents = result_file.read()
                    if len(contents) == 0:
                        continue
                    json_string = "[" + fix_file_contents(contents[:-2]) + "]"
                    result = json.loads(json_string)
                    if result[-1]["bpk"] - (1 if "steroids" in filter else 0) < memory_footprint + 1:
                        plot_data[filter].append((2 ** range_size, result[-1]["fpr"]))
                        if i == len(memory_footprints) - 1:
                            speed_plot_data[filter].append((2 ** range_size, result[-1]["time_q"] * 1e6 / result[-1]["n_queries"]))
            for filter in filters:
                axes[i][j].plot(*zip(*plot_data[filter]), **RANGE_FILTERS_STYLE_KWARGS[filter], **LINES_STYLE)
                axes[len(memory_footprints)][j].plot(*zip(*speed_plot_data[filter]), **RANGE_FILTERS_STYLE_KWARGS[filter], **LINES_STYLE)
    
    for i, workload in enumerate(workloads):
        axes[0][i].set_title(DATASET_NAMES[workload], fontsize=TITLE_FONT_SIZE)
        axes[-1][i].set_xlabel("Range Size", fontsize=XLABEL_FONT_SIZE)
    for i, memory_footprint in enumerate(memory_footprints):
        axes[i][0].set_ylabel(f"FPR", fontsize=YLABEL_FONT_SIZE)
        axes[i][-1].set_ylabel(f"{memory_footprint} BPK", fontsize=YLABEL_FONT_SIZE)
        axes[i][-1].yaxis.set_label_position("right")
    axes[-1][0].set_ylabel(f"$\\;\\;$Query\\\\Latency [ns/op]", fontsize=YLABEL_FONT_SIZE)
    axes[-1][-1].set_ylabel(f"{memory_footprints[-1]} BPK", fontsize=YLABEL_FONT_SIZE)
    axes[-1][-1].yaxis.set_label_position("right")
    for j in range(len(workloads)):
        for i in range(len(memory_footprints)):
            axes[i][j].set_xscale("log")
            axes[i][j].set_yscale("log")
            axes[i][j].yaxis.set_minor_locator(matplotlib.ticker.LogLocator(numticks=10, subs='auto'))
            axes[i][j].set_xticks([2 ** range_size for range_size in range_sizes], ["$2^{" + str(range_size) + "}$" for range_size in range_sizes], fontsize=XTICK_FONT_SIZE)
            axes[i][j].set_yticks(YTICKS if i > 0 else YTICKS_SMALL)
            axes[i][j].set_ylim(top=1.9)
            axes[i][j].autoscale_view()
            axes[i][j].margins(0.04)
            if j > 0:
                axes[i][j].set_yticklabels([])
        axes[-1][j].set_xscale("log")
        axes[-1][j].set_yscale("log")
        axes[-1][j].set_xticks([2 ** range_size for range_size in range_sizes], ["$2^{" + str(range_size) + "}$" for range_size in range_sizes], fontsize=XTICK_FONT_SIZE)
        axes[-1][j].autoscale_view()
        axes[-1][j].margins(0.04)
        if j > 0:
            axes[-1][j].set_yticklabels([])
    fig.subplots_adjust(hspace=0.1, wspace=0.1)

    legend_lines, legend_labels = axes[1][0].get_legend_handles_labels()
    axes[0][1].legend(legend_lines, legend_labels, loc='upper left', bbox_to_anchor=(-0.94, 1.55),
                  fancybox=True, shadow=False, ncol=6, fontsize=LEGEND_FONT_SIZE)
    fig.savefig(output_dir / "fpr.pdf", bbox_inches='tight', pad_inches=0.01)


def plot_fpr_string(result_dir, output_dir):
    TITLE_FONT_SIZE = 9.5
    LEGEND_FONT_SIZE = 7
    YLABEL_FONT_SIZE = 9.5
    XLABEL_FONT_SIZE = 9.5
    WIDTH = 3
    HEIGHT = 3
    YTICKS = [1, 1e-01, 1e-02, 1e-03, 1e-04, 1e-05]
    YTICKS_QUERY = [1000, 100, 10, 1]

    workloads = ["unif", "norm"]
    filters = ["steroids", "surf"]
    memory_footprints = [12, 14, 16, 18, 20]
    workload_subdir = Path("fpr_bench")

    fig, axes = plt.subplots(nrows=len(workloads), ncols=2,
                             sharex=True, figsize=(WIDTH, HEIGHT))
    for i, workload in enumerate(workloads):
        speed_plot_data = {filter: [] for filter in filters}
        plot_data = {filter: [] for filter in filters}
        for memory_footprint in memory_footprints:
            for filter in filters:
                file_path = result_dir / workload_subdir / Path(f"{filter}_{memory_footprint}_{workload}_string.json")
                if not file_path.is_file():
                    continue
                with open(file_path, 'r') as result_file:
                    contents = result_file.read()
                    if len(contents) == 0:
                        continue
                    json_string = "[" + fix_file_contents(contents[:-2]) + "]"
                    result = json.loads(json_string)
                    if result[-1]["bpk"] < memory_footprint + 1:
                        plot_data[filter].append((result[-1]["bpk"] - (1 if filter == "steroids" else 0), result[-1]["fpr"]))
                        speed_plot_data[filter].append((result[-1]["bpk"] - (1 if filter == "steroids" else 0), result[-1]["time_q"] * 1e6 / result[-1]["n_queries"]))
        for filter in filters:
            axes[i][0].plot(*zip(*plot_data[filter]), **RANGE_FILTERS_STYLE_KWARGS[filter], **LINES_STYLE)
            axes[i][1].plot(*zip(*speed_plot_data[filter]), **RANGE_FILTERS_STYLE_KWARGS[filter], **LINES_STYLE)
    
    for i, workload in enumerate(workloads):
        axes[i][0].set_ylabel(DATASET_NAMES[workload] + "\nFPR", fontsize=YLABEL_FONT_SIZE)
        axes[i][1].yaxis.set_label_position("right")
        axes[i][1].yaxis.tick_right()
        axes[i][1].set_ylabel("$\\;\\;$Query\\\\Latency [ns/op]", fontsize=YLABEL_FONT_SIZE)
    for i in range(2):
        axes[-1][i].set_xlabel("Space [BPK]", fontsize=XLABEL_FONT_SIZE)
    for ax in list(axes.flatten()):
        ax.set_xticks(memory_footprints)
        ax.autoscale_view()
        ax.margins(0.04)
        ax.xaxis.set_minor_locator(matplotlib.ticker.MultipleLocator(1))
    for i in range(len(workloads)):
        axes[i][0].set_yscale("log")
        axes[i][0].yaxis.set_minor_locator(matplotlib.ticker.LogLocator(numticks=10, subs='auto'))
        axes[i][0].set_yticks(YTICKS)
        axes[i][0].set_ylim(top=1.9)
        axes[i][1].set_yscale("log")
        axes[i][1].yaxis.set_minor_locator(matplotlib.ticker.LogLocator(numticks=10, subs='auto'))
        axes[i][1].set_yticks(YTICKS_QUERY)
        axes[i][1].set_ylim(top=1000)
    fig.subplots_adjust(wspace=0.1)

    legend_lines, legend_labels = axes[0][1].get_legend_handles_labels()
    axes[0][1].legend(legend_lines, legend_labels, loc='upper left', bbox_to_anchor=(-0.75, 1.275),
                      fancybox=True, shadow=False, ncol=5, fontsize=LEGEND_FONT_SIZE)
    fig.savefig(output_dir / "fpr_string.pdf", bbox_inches='tight', pad_inches=0.01)


def plot_fpr_memory(result_dir, output_dir):
    TITLE_FONT_SIZE = 9.5
    LEGEND_FONT_SIZE = 7
    YLABEL_FONT_SIZE = 9.5
    XLABEL_FONT_SIZE = 9.5
    XTICK_FONT_SIZE = 8
    XPADDING = 1
    WIDTH = 3
    HEIGHT = 1.35
    YTICKS = [1, 1e-01, 1e-02, 1e-03, 1e-04, 1e-05]

    workloads = ["unif", "osm"]
    filters = ["steroids", "steroids_int", "memento", "grafite", "surf",
               "rosetta", "proteus", "proteus_mistuned", "rencoder", "snarf",
               "oasis"]
    memory_footprints = [8, 10, 12, 14, 16, 18]
    RANGE_SIZE = 8
    workload_subdir = Path("fpr_bench")

    fig, axes = plt.subplots(nrows=1, ncols=len(workloads),
                             sharex=True, figsize=(WIDTH, HEIGHT))
    for i, workload in enumerate(workloads):
        plot_data = {filter: [] for filter in filters}
        for memory_footprint, filter in itertools.product(memory_footprints, filters):
            file_path = result_dir / workload_subdir / Path(f"{filter}_{memory_footprint}_{workload}_{RANGE_SIZE}.json")
            if not file_path.is_file():
                continue
            with open(file_path, 'r') as result_file:
                contents = result_file.read()
                if len(contents) == 0:
                    continue
                json_string = "[" + fix_file_contents(contents[:-2]) + "]"
                result = json.loads(json_string)
                plot_data[filter].append((result[-1]["bpk"] - (1 if "steroids" in filter else 0), result[-1]["fpr"]))
        for filter in filters:
            axes[i].plot(*zip(*plot_data[filter]), **RANGE_FILTERS_STYLE_KWARGS[filter], **LINES_STYLE)
    
    for i, workload in enumerate(workloads):
        axes[i].set_title(DATASET_NAMES[workload], fontsize=TITLE_FONT_SIZE)
        axes[i].set_xlabel("Space [BPK]", fontsize=XLABEL_FONT_SIZE)
    axes[0].set_ylabel(f"FPR", fontsize=YLABEL_FONT_SIZE)
    for i in range(len(workloads)):
            axes[i].set_yscale("log")
            axes[i].yaxis.set_minor_locator(matplotlib.ticker.LogLocator(numticks=10, subs='auto'))
            axes[i].set_xticks(memory_footprints)
            axes[i].set_yticks(YTICKS)
            axes[i].set_ylim(top=1.9)
            axes[i].autoscale_view()
            axes[i].margins(0.04)
            if i > 0:
                axes[i].set_yticklabels([])
    plt.xlim(memory_footprints[0] - XPADDING, memory_footprints[-1] + XPADDING)
    fig.subplots_adjust(hspace=0.1, wspace=0.1)

    legend_lines, legend_labels = axes[0].get_legend_handles_labels()
    axes[0].legend(legend_lines, legend_labels, loc='upper left', bbox_to_anchor=(2.15, 1.25),
                   fancybox=True, shadow=False, ncol=1, fontsize=LEGEND_FONT_SIZE)
    fig.savefig(output_dir / "fpr_memory.pdf", bbox_inches='tight', pad_inches=0.01)


def plot_true(result_dir, output_dir):
    TITLE_FONT_SIZE = 11.5
    LEGEND_FONT_SIZE = 8
    YLABEL_FONT_SIZE = 11.5
    XLABEL_FONT_SIZE = 11.5
    WIDTH = 3.5
    HEIGHT = 1.5
    YTICKS = [1e6, 1e5, 1e4, 1e3, 1e2]

    WORKLOAD = "unif"
    filters = ["steroids", "steroids_int", "memento", "grafite", "surf",
               "rosetta", "proteus", "rencoder", "snarf", "oasis"]
    memory_footprints = [10, 12, 14, 16, 18, 20]
    range_sizes = ["short", "long"]
    workload_subdir = Path("true_bench")

    fig, axes = plt.subplots(nrows=1, ncols=len(range_sizes),
                             sharex=True, sharey='row', figsize=(WIDTH, HEIGHT))
    for i, range_size in enumerate(range_sizes):
        plot_data = {filter: [] for filter in filters}
        for filter, memory_footprint in itertools.product(filters, memory_footprints):
            file_path = result_dir / workload_subdir / Path(f"{filter}_{memory_footprint}_{WORKLOAD}_{range_size}.json")
            if not file_path.is_file():
                continue
            with open(file_path, 'r') as result_file:
                contents = result_file.read()
                if len(contents) == 0:
                    continue
                json_string = "[" + fix_file_contents(contents[:-2]) + "]"
                result = json.loads(json_string)
                plot_data[filter].append((result[-1]["bpk"] - (1 if "steroids" in filter else 0),
                                          result[-1]["time_q"] * 1e6 / result[-1]["n_queries"]))
        for filter in filters:
            axes[i].plot(*zip(*plot_data[filter]), **RANGE_FILTERS_STYLE_KWARGS[filter], **LINES_STYLE)

    title = DATASET_NAMES[WORKLOAD if WORKLOAD != "real" else "books"] + "\nQuery Latency [ns/op]" 
    axes[0].set_ylabel(title, fontsize=YLABEL_FONT_SIZE)
    for i, range_size in enumerate(range_sizes):
        axes[i].set_xlabel("Space [BPK]", fontsize=XLABEL_FONT_SIZE)
        axes[i].text(9.5, 4e5, RANGE_LENGTH_TAGS[range_size], fontsize=XLABEL_FONT_SIZE)
    for ax in axes.flatten():
        ax.set_xlim(left=min(memory_footprints) - 1, right=max(memory_footprints) + 1)
        ax.set_yscale('log')
        ax.xaxis.set_minor_locator(matplotlib.ticker.MultipleLocator(1))
        ax.yaxis.set_minor_locator(matplotlib.ticker.LogLocator(numticks=10, subs='auto'))
        ax.set_xticks(memory_footprints)
        ax.set_yticks(YTICKS)
    fig.subplots_adjust(wspace=0.1)

    legend_lines, legend_labels = axes[0].get_legend_handles_labels()
    axes[1].legend(legend_lines, legend_labels, loc='upper left', bbox_to_anchor=(1.025, 1.25),
                   fancybox=True, shadow=False, ncol=1, fontsize=LEGEND_FONT_SIZE)
    fig.savefig(output_dir / "true.pdf", bbox_inches='tight', pad_inches=0.01)


def plot_construction(result_dir, output_dir):
    LEGEND_FONT_SIZE = 8
    YLABEL_FONT_SIZE = 10.0
    XLABEL_FONT_SIZE = 10.5
    WIDTH = 4 * 21 / 30
    HEIGHT = 3 * 21 / 30
    TICKW = 1.1
    BARW = 0.1
    ALPHA = 0.7
    ALPHA_MODELING = 0.25
    PATTERN_DENSITY = 2
    matplotlib.rcParams["hatch.linewidth"] = 0.3

    filters = ["steroids", "steroids_int", "memento", "grafite", "surf",
               "rosetta", "proteus", "rencoder", "snarf", "oasis"]
    log_number_of_keys = [6, 7, 8, 9]
    workload_subdir = Path("construction_bench")

    fig, ax = plt.subplots(figsize=(WIDTH, HEIGHT))
    ax.set_ylim(bottom=0, top=600)
    for i, logn in enumerate(log_number_of_keys):
        for j, filter in enumerate(filters):
            file_path = result_dir / workload_subdir / Path(f"{filter}_16_unif_{logn}.json")
            if not file_path.is_file():
                continue
            with open(file_path, 'r') as result_file:
                contents = result_file.read()
                if len(contents) == 0:
                    continue
                json_string = "[" + fix_file_contents(contents[:-2]) + "]"
                result = json.loads(json_string)
                build_time = result[0]["construction_time"] * 1e6 / result[0]["n_keys"]
                ax.bar(j * BARW + TICKW * i, build_time, BARW, color=RANGE_FILTERS_STYLE_KWARGS[filter]["color"],
                       hatch=RANGE_FILTERS_HATCHES[filter] * PATTERN_DENSITY, alpha=ALPHA)
                if "modeling_time" in result[0]:
                    modeling_time = result[0]["modeling_time"] * 1e6 / result[0]["n_keys"]
                    ax.bar(j * BARW + TICKW * i, modeling_time, BARW, bottom=build_time, label="_nolegend_",
                           color=RANGE_FILTERS_STYLE_KWARGS[filter]["color"],
                           hatch=RANGE_FILTERS_HATCHES[filter] * PATTERN_DENSITY,
                           alpha=ALPHA_MODELING)

    ax.set_ylabel("Construction Time [ns/key]", fontsize=YLABEL_FONT_SIZE)
    ax.set_xticks(np.arange(len(log_number_of_keys)) + 4.5 * BARW, [f"$10^{logn}$" for logn in log_number_of_keys])
    ax.set_xlabel('Number of Keys', fontsize=XLABEL_FONT_SIZE)
    ax.yaxis.set_minor_locator(matplotlib.ticker.MultipleLocator(100))

    ax.text(0.48, 565, "700.0", fontsize=0.7*XLABEL_FONT_SIZE)
    ax.text(1.575, 565, "2234.0", fontsize=0.7*XLABEL_FONT_SIZE)

    legend_handles = [mpatches.Patch(facecolor=RANGE_FILTERS_STYLE_KWARGS[filter]["color"], alpha=ALPHA,
                                     hatch=RANGE_FILTERS_HATCHES[filter] * 2 * PATTERN_DENSITY,
                                     label=RANGE_FILTERS_STYLE_KWARGS[filter]["label"]) for filter in filters]
    ax.legend(handles=legend_handles, loc="center left", bbox_to_anchor=(1, 0.5),
              fancybox=True, shadow=False, ncol=1, fontsize=LEGEND_FONT_SIZE)

    fig.savefig(output_dir / "construction.pdf", bbox_inches='tight', pad_inches=0.01)


def plot_expansion(result_dir, output_dir):
    TITLE_FONT_SIZE = 9.5
    LEGEND_FONT_SIZE = 7
    YLABEL_FONT_SIZE = 9.5
    XLABEL_FONT_SIZE = 9.5
    WIDTH = 3.5
    HEIGHT = 3.5
    FRAC_EXPANSION_COUNT = 4
    MEMORY_FOOTPRINT = 16
    N_EXPANSIONS = 6
    XTICK_FONT_SIZE = 9
    XTICKS = [2 ** (i - N_EXPANSIONS) for i in range(N_EXPANSIONS + 1)]
    XTICK_LABELS = ["$\\frac{1}{" + str(2 ** (N_EXPANSIONS - i)) + "}$" for i in range(N_EXPANSIONS)] + ["$1$"]
    YTICKS = [1, 1e-01, 1e-02, 1e-03, 1e-04, 1e-05]
    YTICKS_MEMORY = [15, 20, 25, 30, 35]

    filters = ["steroids", "steroids_int", "memento_expandable", "rosetta", "rencoder", "snarf"]
    range_sizes = ["short", "long"]
    workload_subdir = Path("expansion_bench")

    fig, axes = plt.subplots(nrows=2, ncols=2, sharex=True, figsize=(WIDTH, HEIGHT))
    plot_data = [{filter: [] for filter in filters} for _ in range(len(range_sizes) + 2)]
    for i, range_size in enumerate(range_sizes):
        for filter in filters:
            file_path = result_dir / workload_subdir / Path(f"{filter}_{MEMORY_FOOTPRINT - (1 if 'steroids' in filter else 0)}_unif_{range_size}.json")
            if not file_path.is_file():
                continue
            with open(file_path, 'r') as result_file:
                contents = result_file.read()
                if len(contents) == 0:
                    continue
                json_string = "[" + fix_file_contents(contents[:-2]) + "]"
                result = json.loads(json_string)
                for j, entry in enumerate(result[1:]):
                    current_dataset_fraction = 2 ** (j // FRAC_EXPANSION_COUNT - N_EXPANSIONS) * (j % FRAC_EXPANSION_COUNT + 1)
                    plot_data[i][filter].append([current_dataset_fraction, entry["fpr"]])
                    plot_data[2][filter].append([current_dataset_fraction, entry["bpk"]])
                    if "time_i" in entry:
                        n_inserts = entry["n_keys"] - result[j]["n_keys"]
                        plot_data[3][filter].append([2 ** (j / FRAC_EXPANSION_COUNT - N_EXPANSIONS), entry["time_i"] * 1e6 / n_inserts])
    for filter in filters:
        plot_data[0][filter] = plot_data[0][filter][:-1]
        plot_data[1][filter] = plot_data[1][filter][:-1]
        if filter != "memento_expandable":
            for i in range(2, 4):
                for j in range(len(plot_data[i][filter]) // 2):
                    plot_data[i][filter][j][1] += plot_data[i][filter][j + len(plot_data[i][filter]) // 2][1]
                    plot_data[i][filter][j][1] /= 2
                plot_data[i][filter] = plot_data[i][filter][:len(plot_data[i][filter]) // 2]
    for i in range(len(range_sizes) + 2):
        for filter in filters:
            tmp_plot_data = list(zip(*plot_data[i][filter]))
            if i >= len(range_sizes):
                if filter == "memento_expandable" or (filter in ["rosetta", "rencoder"] and i == 2):
                    axes[1][i - 2].plot(*tmp_plot_data, **RANGE_FILTERS_STYLE_KWARGS[filter], **LINES_STYLE)
                else:
                    axes[1][i - 2].plot(tmp_plot_data[0][::2], tmp_plot_data[1][::2], **RANGE_FILTERS_STYLE_KWARGS[filter], **LINES_STYLE)
            elif len(tmp_plot_data) > 0:
                axes[0][i].plot(tmp_plot_data[0][::2], tmp_plot_data[1][::2], **RANGE_FILTERS_STYLE_KWARGS[filter], **LINES_STYLE)

    for ax in axes.flatten():
        ax.set_xscale("log")
        ax.minorticks_off()
        ax.set_xticks(XTICKS, XTICK_LABELS, fontsize=XTICK_FONT_SIZE)
    for i in range(2):
        axes[1][i].set_xlabel("Dataset Fraction", fontsize=XLABEL_FONT_SIZE)
    for i, j in ((0, 0), (0, 1), (1, 1)):
        if i == 0:
            axes[i][j].set_yscale("log")
            axes[i][j].set_ylim(top=1.9)
            axes[i][j].set_yticks(YTICKS)
        else:
            axes[i][j].set_yscale("log")
        axes[i][j].yaxis.set_minor_locator(matplotlib.ticker.LogLocator(numticks=10, subs="auto"))
    axes[1][0].yaxis.set_minor_locator(matplotlib.ticker.MultipleLocator(1))
    axes[1][0].set_ylim(bottom=13, top=35)
    axes[1][0].set_yticks(YTICKS_MEMORY)

    axes[0][0].text(0.25, 1.5 * YTICKS[-1], RANGE_LENGTH_TAGS[range_sizes[0]], fontsize=XLABEL_FONT_SIZE)
    axes[0][0].set_ylabel("FPR", fontsize=YLABEL_FONT_SIZE)

    axes[0][1].text(0.25, 1.5 * YTICKS[-1], RANGE_LENGTH_TAGS[range_sizes[1]], fontsize=XLABEL_FONT_SIZE)
    axes[0][1].set_yticklabels([])

    axes[1][0].set_ylabel("Space [BPK]", fontsize=YLABEL_FONT_SIZE)
    axes[1][1].yaxis.set_label_position("right")
    axes[1][1].yaxis.tick_right()
    axes[1][1].set_ylabel("Insert Latency [ns/op]", fontsize=YLABEL_FONT_SIZE)

    legend_lines, legend_labels = axes[1][1].get_legend_handles_labels()
    axes[0][1].legend(legend_lines, legend_labels, loc="upper left", bbox_to_anchor=(-1.075,1.35),
                      fancybox=True, shadow=False, ncol=(len(filters) + 1) // 2, fontsize=LEGEND_FONT_SIZE)
    fig.subplots_adjust(hspace=0.15, wspace=0.1)
    fig.savefig(output_dir / "expansion.pdf", bbox_inches='tight', pad_inches=0.01)


def plot_delete(result_dir, output_dir):
    LEGEND_FONT_SIZE = 7
    YLABEL_FONT_SIZE = 9.5
    XLABEL_FONT_SIZE = 9.5
    WIDTH = 1.8
    HEIGHT = 1.7

    filters = ["steroids", "steroids_int", "memento", "snarf"]
    range_sizes = ["short", "long"]
    memory_footprints = [10, 12, 14, 16, 18, 20]
    workload_subdir = Path("delete_bench")

    fig, ax = plt.subplots(nrows=1, ncols=1, figsize=(WIDTH, HEIGHT))
    plot_data = {filter: [] for filter in filters}
    for i, range_size in enumerate(range_sizes):
        for filter, memory_footprint in itertools.product(filters, memory_footprints):
            file_path = result_dir / workload_subdir / Path(f"{filter}_{memory_footprint}_unif_{range_size}.json")
            if not file_path.is_file():
                continue
            with open(file_path, 'r') as result_file:
                contents = result_file.read()
                if len(contents) == 0:
                    continue
                json_string = "[" + fix_file_contents(contents[:-2]) + "]"
                result = json.loads(json_string)
                n_deletes = result[0]["n_keys"] / 20
                plot_data[filter].append([result[-1]["bpk"] - (1 if "steroids" in filter else 0),
                                          result[-1]["time_d"] * 1e6 / n_deletes])
    for filter in filters:
        for i in range(len(plot_data[filter]) // 2):
            plot_data[filter][i][1] += plot_data[filter][i + len(plot_data[filter]) // 2][1]
            plot_data[filter][i][1] /= 2
        plot_data[filter] = plot_data[filter][:len(plot_data[filter]) // 2]
        ax.plot(*zip(*plot_data[filter]), **RANGE_FILTERS_STYLE_KWARGS[filter], **LINES_STYLE)

    ax.set_xlabel("Space [BPK]", fontsize=XLABEL_FONT_SIZE)
    ax.set_xticks(memory_footprints)
    ax.set_ylabel("Delete Latency [ns/op]", fontsize=YLABEL_FONT_SIZE)
    ax.set_ylim(bottom=-0.0000003)
    ax.yaxis.set_minor_locator(matplotlib.ticker.MultipleLocator(1000))

    legend_lines, legend_labels = ax.get_legend_handles_labels()
    ax.legend(legend_lines, legend_labels, loc="upper left", bbox_to_anchor=(1.0,0.8),
              fancybox=True, shadow=False, ncol=1, fontsize=LEGEND_FONT_SIZE)
    fig.savefig(output_dir / "delete.pdf", bbox_inches='tight', pad_inches=0.01)


def plot_wiredtiger(result_dir, output_dir):
    LEGEND_FONT_SIZE = 7
    TITLE_FONT_SIZE = 9.5
    YLABEL_FONT_SIZE = 9.5
    XLABEL_FONT_SIZE = 9.5
    WIDTH = 1.8
    HEIGHT = 1.7
    N_EXPANSIONS = 6
    XTICK_FONT_SIZE = 9
    XTICKS = [2 ** (i - N_EXPANSIONS) for i in range(N_EXPANSIONS + 1)]
    XTICK_LABELS = ["$\\frac{1}{" + str(2 ** (N_EXPANSIONS - i)) + "}$" for i in range(N_EXPANSIONS)] + ["$1$"]

    filters = ["steroids", "steroids_int", "memento_expandable", "base"]
    RANGE_SIZE = "short"
    MEMORY_FOOTPRINT = 16
    workload_subdir = Path("wiredtiger_bench")

    fig, ax = plt.subplots(nrows=1, ncols=1, figsize=(WIDTH, HEIGHT))
    plot_data = {filter: [] for filter in filters}
    for filter in filters:
        file_path = result_dir / workload_subdir / Path(f"{filter}_{MEMORY_FOOTPRINT - (1 if 'steroids' in filter else 0)}_unif_{RANGE_SIZE}.json")
        if not file_path.is_file():
            continue
        with open(file_path, 'r') as result_file:
            contents = result_file.read()
            if len(contents) == 0:
                continue
            json_string = "[" + fix_file_contents(contents[:-2]) + "]"
            result = json.loads(json_string)
            for i, entry in enumerate(result[1:]):
                plot_data[filter].append([2 ** (i - N_EXPANSIONS), entry["time_q"] * 1e6 / entry["n_queries"]])
    for filter in filters:
        ax.plot(*zip(*plot_data[filter]), **RANGE_FILTERS_STYLE_KWARGS[filter], **LINES_STYLE)

    ax.set_xlabel("Dataset Fraction", fontsize=XLABEL_FONT_SIZE)
    ax.set_xscale("log")
    ax.minorticks_off()
    ax.set_xticks(XTICKS, XTICK_LABELS, fontsize=XTICK_FONT_SIZE)
    ax.set_ylabel("$\\;\\;\\;$End-to-End\\\\Query Latency [ns/op]", fontsize=YLABEL_FONT_SIZE)
    ax.set_yscale("log")
    ax.text(0.25, 200, RANGE_LENGTH_TAGS[RANGE_SIZE], fontsize=XLABEL_FONT_SIZE)

    legend_lines, legend_labels = ax.get_legend_handles_labels()
    ax.legend(legend_lines, legend_labels, loc="upper left", bbox_to_anchor=(1.0,0.8),
              fancybox=True, shadow=False, ncol=1, fontsize=LEGEND_FONT_SIZE)
    fig.savefig(output_dir / "wiredtiger.pdf", bbox_inches='tight', pad_inches=0.01)


PLOTTERS = {"fpr": plot_fpr,
            "fpr_string": plot_fpr_string,
            "fpr_memory": plot_fpr_memory,
            "true": plot_true,
            "construction": plot_construction,
            "expansion": plot_expansion,
            "delete": plot_delete,
            "wiredtiger": plot_wiredtiger}

if __name__ == "__main__":
    parser = argparse.ArgumentParser(prog='BenchResultPlotter')

    parser.add_argument("-f", "--figures", nargs="+", choices=["all",] + list(PLOTTERS.keys()),
                        default=["all"], type=str, help="The figures to create")
    parser.add_argument("-t", "--timestamp", nargs="?", type=str, help="Result timestamp to process")
    parser.add_argument("--result_dir", default=Path("../../../paper_results/results/"),
                        type=Path, help="The directory containing benchmark results")
    parser.add_argument("--figure_dir", default=Path("../../../paper_results/figures/"),
                        type=Path, help="The output directory storing the figures")

    args = parser.parse_args()

    EXPERIMENT_TIMESTAMP = Path(sorted(os.listdir(args.result_dir), reverse=True)[0]) if args.timestamp is None \
                                                                                      else args.timestamp
    RESULT_DIR = args.result_dir / EXPERIMENT_TIMESTAMP
    FIGURE_DIR = args.figure_dir / EXPERIMENT_TIMESTAMP
    FIGURE_DIR.mkdir(parents=True, exist_ok=True)

    logging.info(f"Result Path: {RESULT_DIR}")
    logging.info(f"Ouput Figure Path: {FIGURE_DIR}")
    if "all" in args.figures:
        for figure in PLOTTERS:
            PLOTTERS[figure](RESULT_DIR, FIGURE_DIR)
    else:
        for figure in args.figures:
            PLOTTERS[figure](RESULT_DIR, FIGURE_DIR)
