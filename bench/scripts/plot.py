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
from statistics import median
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

RANGE_FILTERS_STYLE_KWARGS = {"steroids_int": {"marker": 'v', "color": "fuchsia", "zorder": 12, "label": "Steroids (Int)"},
                              "steroids": {"marker": 'v', "color": "fuchsia", "zorder": 12, "label": "Steroids", "linestyle": ":"},
                              "memento": {"marker": '4', "color": "C1", "zorder": 11, "label": "Memento"},
                              "memento_expandable": {"marker": '4', "color": "C1", "zorder": 11, "label": "Memento"},
                              "grafite": {"marker": 'o', "color": "teal", "label": "Grafite"},
                              "none": {"marker": 'x', "color": "dimgray", "zorder": 10, "label": "Baseline"},
                              "snarf": {"marker": '^', "color": "black", "label": "SNARF"},
                              "oasis": {"marker": '+', "color": "darkkhaki", "label": "Oasis+"},
                              "surf": {"marker": 's', "color": "C2", "label": "SuRF"},
                              "proteus": {"marker": 'X', "color": "dimgray", "label": "Proteus"},
                              "rosetta": {"marker": 'd', "color": "C4", "label": "Rosetta"},
                              "rencoder": {"marker": '>', "color": "C5", "label": "REncoder"}}
RANGE_FILTERS_HATCHES = {"steroids_int": '',
                         "steroids": '.',
                         "memento": '/',
                         "memento_expandable": '/',
                         "grafite": '\\',
                         "none": '|',
                         "snarf": '-',
                         "oasis": '+',
                         "surf": 'x',
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
                      "short": "Range ($R \\leq 2^{10}$)",
                      "long": "Range ($R \\leq 2^{20}$)"}


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
    HEIGHT = 7
    YTICKS = [1, 1e-01, 1e-02, 1e-03, 1e-04, 1e-05, 0]

    workloads = ["unif", "norm", "books", "osm"]
    filters = ["steroids", "steroids_int", "memento", "grafite", "surf",
               "rosetta", "proteus", "rencoder", "snarf", "oasis"]
    memory_footprints = [14, 16, 18, 20]
    range_sizes = [0, 4, 8, 12, 16, 20, 24]
    workload_subdir = Path("fpr_bench")

    fig, axes = plt.subplots(nrows=len(workloads), ncols=len(memory_footprints) + 1,
                             sharex=True, figsize=(WIDTH, HEIGHT))
    fpr_insert_sep = matplotlib.lines.Line2D((0.75, 0.75), (0.91, 0.07), linestyle="--", color="lightgrey",
                                         transform=fig.transFigure)
    fig.lines = fpr_insert_sep,
    for i, workload in enumerate(workloads):
        speed_plot_data = {filter: [[] for _ in range_sizes] for filter in filters}
        for j, memory_footprint in enumerate(memory_footprints):
            plot_data = {filter: [] for filter in filters}
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
                    if result[-1]["bpk"] < memory_footprint + 1:
                        plot_data[filter].append((2 ** range_size, result[-1]["fpr"]))
                        speed_plot_data[filter][range_sizes.index(range_size)].append(result[-1]["time_q"] * 1e6 / result[-1]["n_queries"])
            for filter in filters:
                axes[i][j].plot(*zip(*plot_data[filter]), **RANGE_FILTERS_STYLE_KWARGS[filter], **LINES_STYLE)
        for filter in filters:
            converted_speed_plot_data = []
            for j in range(len(speed_plot_data[filter])):
                if len(speed_plot_data[filter][j]) > 0:
                    converted_speed_plot_data.append((2 ** range_sizes[j], median(speed_plot_data[filter][j])))
            axes[i][-1].plot(*zip(*converted_speed_plot_data), **RANGE_FILTERS_STYLE_KWARGS[filter], **LINES_STYLE)
    
    for i, workload in enumerate(workloads):
        axes[i][0].set_ylabel(DATASET_NAMES[workload] + "\nFPR", fontsize=YLABEL_FONT_SIZE)
        axes[i][-1].yaxis.set_label_position("right")
        axes[i][-1].yaxis.tick_right()
        axes[i][-1].set_ylabel("Time [ns/query]", fontsize=YLABEL_FONT_SIZE)
    for i, memory_footprint in enumerate(memory_footprints):
        axes[-1][i].set_xlabel("Range Size", fontsize=XLABEL_FONT_SIZE)
        axes[0][i].set_title(f"{memory_footprint} BPK", fontsize=TITLE_FONT_SIZE)
    axes[-1][-1].set_xlabel("Range Size", fontsize=XLABEL_FONT_SIZE)
    axes[0][-1].set_title("Query Time", fontsize=TITLE_FONT_SIZE)
    for i in range(len(workloads)):
        for j in range(len(memory_footprints)):
            axes[i][j].set_xscale("log")
            axes[i][j].set_yscale("symlog", linthresh=(1e-05))
            axes[i][j].yaxis.set_minor_locator(matplotlib.ticker.LogLocator(numticks=10, subs='auto'))
            axes[i][j].set_xticks([2 ** range_size for range_size in range_sizes], ["$2^{" + str(range_size) + "}$" for range_size in range_sizes], fontsize=XTICK_FONT_SIZE)
            axes[i][j].set_yticks(YTICKS)
            axes[i][j].set_ylim(bottom=-0.0000003, top=1.9)
            axes[i][j].autoscale_view()
            axes[i][j].margins(0.04)
            if j > 0:
                axes[i][j].set_yticklabels([])
        axes[i][-1].set_xscale("log")
        axes[i][-1].set_yscale("log")
        axes[i][-1].yaxis.set_minor_locator(matplotlib.ticker.LogLocator(numticks=10, subs='auto'))
        axes[i][-1].set_xticks([2 ** range_size for range_size in range_sizes], ["$2^{" + str(range_size) + "}$" for range_size in range_sizes], fontsize=XTICK_FONT_SIZE)
        axes[i][-1].autoscale_view()
        axes[i][-1].margins(0.04)
    fig.subplots_adjust(wspace=0.1)

    legend_lines, legend_labels = axes[0][1].get_legend_handles_labels()
    axes[0][1].legend(legend_lines, legend_labels, loc='upper left', bbox_to_anchor=(-0.3, 1.55),
                  fancybox=True, shadow=False, ncol=5, fontsize=LEGEND_FONT_SIZE)
    fig.savefig(output_dir / "fpr.pdf", bbox_inches='tight', pad_inches=0.01)


def plot_fpr_string(result_dir, output_dir):
    TITLE_FONT_SIZE = 9.5
    LEGEND_FONT_SIZE = 7
    YLABEL_FONT_SIZE = 9.5
    XLABEL_FONT_SIZE = 9.5
    WIDTH = 3
    HEIGHT = 3
    YTICKS = [1, 1e-01, 1e-02, 1e-03, 1e-04, 1e-05, 0]
    YTICKS_QUERY = [1000, 100, 10, 1, 0]

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
        axes[i][1].set_ylabel("Time [ns/query]", fontsize=YLABEL_FONT_SIZE)
    for i in range(2):
        axes[-1][i].set_xlabel("Size [BPK]", fontsize=XLABEL_FONT_SIZE)
    for ax in list(axes.flatten()):
        ax.set_xticks(memory_footprints)
        ax.autoscale_view()
        ax.margins(0.04)
        ax.xaxis.set_minor_locator(matplotlib.ticker.MultipleLocator(1))
    axes[0][0].set_title("FPR", fontsize=TITLE_FONT_SIZE)
    axes[0][1].set_title("Query Time", fontsize=TITLE_FONT_SIZE)
    for i in range(len(workloads)):
        axes[i][0].set_yscale("symlog", linthresh=(1e-05))
        axes[i][0].yaxis.set_minor_locator(matplotlib.ticker.LogLocator(numticks=10, subs='auto'))
        axes[i][0].set_yticks(YTICKS)
        axes[i][0].set_ylim(bottom=-0.0000003, top=1.9)
        axes[i][1].set_yscale("symlog", linthresh=1)
        axes[i][1].yaxis.set_minor_locator(matplotlib.ticker.LogLocator(numticks=10, subs='auto'))
        axes[i][1].set_yticks(YTICKS_QUERY)
        axes[i][1].set_ylim(bottom=-0.0000003, top=1000)
    fig.subplots_adjust(wspace=0.1)

    legend_lines, legend_labels = axes[0][1].get_legend_handles_labels()
    axes[0][1].legend(legend_lines, legend_labels, loc='upper left', bbox_to_anchor=(-0.75, 1.425),
                      fancybox=True, shadow=False, ncol=5, fontsize=LEGEND_FONT_SIZE)
    fig.savefig(output_dir / "fpr_string.pdf", bbox_inches='tight', pad_inches=0.01)


def plot_true(result_dir, output_dir):
    TITLE_FONT_SIZE = 11.5
    LEGEND_FONT_SIZE = 8
    YLABEL_FONT_SIZE = 11.5
    XLABEL_FONT_SIZE = 11.5
    WIDTH = 6
    HEIGHT = 3
    YTICKS = [1e5, 1e4, 1e3, 1e2]

    workloads = ["unif", "real"]
    filters = ["steroids", "steroids_int", "memento", "grafite", "surf",
               "rosetta", "proteus", "rencoder", "snarf", "oasis"]
    memory_footprints = [10, 12, 14, 16, 18, 20]
    range_sizes = ["point", "short", "long"]
    workload_subdir = Path("true_bench")

    fig, axes = plt.subplots(nrows=len(workloads), ncols=len(range_sizes),
                             sharex=True, sharey='row', figsize=(WIDTH, HEIGHT))
    for i, workload in enumerate(workloads):
        for j, range_size in enumerate(range_sizes):
            plot_data = {filter: [] for filter in filters}
            for filter, memory_footprint in itertools.product(filters, memory_footprints):
                file_path = result_dir / workload_subdir / Path(f"{filter}_{memory_footprint}_{workload}_{range_size}.json")
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
                axes[i][j].plot(*zip(*plot_data[filter]), **RANGE_FILTERS_STYLE_KWARGS[filter], **LINES_STYLE)

    for i, workload in enumerate(workloads):
        title = DATASET_NAMES[workload if workload != "real" else "books"] \
                + "\nTime [ns/query]" if len(workloads) > 1 else "Time [ns/query]" 
        axes[i][0].set_ylabel(title, fontsize=YLABEL_FONT_SIZE)
    for i, range_size in enumerate(range_sizes):
        axes[-1][i].set_xlabel("Space [BPK]", fontsize=XLABEL_FONT_SIZE)
        axes[0][i].set_title(RANGE_LENGTH_NAMES[range_size], fontsize=TITLE_FONT_SIZE)
    for ax in axes.flatten():
        ax.set_xlim(left=min(memory_footprints) - 1, right=max(memory_footprints) + 1)
        ax.set_yscale('log')
        ax.xaxis.set_minor_locator(matplotlib.ticker.MultipleLocator(1))
        ax.yaxis.set_minor_locator(matplotlib.ticker.LogLocator(numticks=10, subs='auto'))
        ax.set_xticks(memory_footprints)
        ax.set_yticks(YTICKS)
    fig.subplots_adjust(wspace=0.1)

    legend_lines, legend_labels = axes[0][1].get_legend_handles_labels()
    axes[0][1].legend(legend_lines, legend_labels, loc='upper left', bbox_to_anchor=(-1.25, 1.65),
                  fancybox=True, shadow=False, ncol=5, fontsize=LEGEND_FONT_SIZE)
    fig.savefig(output_dir / "true.pdf", bbox_inches='tight', pad_inches=0.01)


def plot_construction(result_dir, output_dir):
    LEGEND_FONT_SIZE = 8
    YLABEL_FONT_SIZE = 10.0
    XLABEL_FONT_SIZE = 10.5
    WIDTH = 4 * 21 / 30
    HEIGHT = 3 * 21 / 30
    BARW = 0.08
    ALPHA = 0.7
    ALPHA_MODELING = 0.25
    PATTERN_DENSITY = 4
    matplotlib.rcParams["hatch.linewidth"] = 0.2

    filters = ["steroids", "steroids_int", "memento", "grafite", "surf",
               "rosetta", "proteus", "rencoder", "snarf", "oasis"]
    log_number_of_keys = [5, 6, 7, 8]
    workload_subdir = Path("construction_bench")

    fig, ax = plt.subplots(figsize=(WIDTH, HEIGHT))
    ax.set_ylim(bottom=0, top=1500)
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
                ax.bar(j * BARW + i, build_time, BARW, color=RANGE_FILTERS_STYLE_KWARGS[filter]["color"],
                       hatch=RANGE_FILTERS_HATCHES[filter] * PATTERN_DENSITY, alpha=ALPHA)
                if "modeling_time" in result[0]:
                    modeling_time = result[0]["modeling_time"] * 1e6 / result[0]["n_keys"]
                    ax.bar(j * BARW + i, modeling_time, BARW, bottom=build_time, label="_nolegend_",
                           color=RANGE_FILTERS_STYLE_KWARGS[filter]["color"],
                           hatch=RANGE_FILTERS_HATCHES[filter] * PATTERN_DENSITY,
                           alpha=ALPHA_MODELING)

    ax.set_ylabel("Construction Time [ns/key]", fontsize=YLABEL_FONT_SIZE)
    ax.set_xticks(np.arange(len(log_number_of_keys)) + 4.5 * BARW, [f"$10^{logn}$" for logn in log_number_of_keys])
    ax.set_xlabel('Number of Keys', fontsize=XLABEL_FONT_SIZE)
    ax.yaxis.set_minor_locator(matplotlib.ticker.MultipleLocator(100))

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
    WIDTH = 4.5
    HEIGHT = 3.5
    FRAC_EXPANSION_COUNT = 4
    MEMORY_FOOTPRINT = 20
    XTICKS = list(range(7))

    filters = ["steroids", "steroids_int", "memento_expandable", "rosetta", "rencoder", "snarf"]
    range_sizes = ["short", "long"]
    workload_subdir = Path("expansion_bench")

    fig, axes = plt.subplots(nrows=2, ncols=2, sharex=True, figsize=(WIDTH, HEIGHT))
    plot_data = [{filter: [] for filter in filters} for _ in range(len(range_sizes) + 2)]
    for i, range_size in enumerate(range_sizes):
        for filter in filters:
            file_path = result_dir / workload_subdir / Path(f"{filter}_{MEMORY_FOOTPRINT}_unif_{range_size}.json")
            if not file_path.is_file():
                continue
            with open(file_path, 'r') as result_file:
                contents = result_file.read()
                if len(contents) == 0:
                    continue
                json_string = "[" + fix_file_contents(contents[:-2]) + "]"
                result = json.loads(json_string)
                for j, entry in enumerate(result[1:]):
                    plot_data[i][filter].append([j / FRAC_EXPANSION_COUNT, entry["fpr"]])
                    plot_data[2][filter].append([j / FRAC_EXPANSION_COUNT, entry["bpk"]])
                    if "time_i" in entry:
                        n_inserts = entry["n_keys"] - result[j]["n_keys"]
                        plot_data[3][filter].append([j / FRAC_EXPANSION_COUNT, entry["time_i"] * 1e6 / n_inserts])
    for filter in filters:
        plot_data[0][filter] = plot_data[0][filter][:-1]
        plot_data[1][filter] = plot_data[1][filter][:-1]
        for i in range(2, 4):
            for j in range(len(plot_data[i][filter]) // 2):
                plot_data[i][filter][j][1] += plot_data[i][filter][j + len(plot_data[i][filter]) // 2][1]
                plot_data[i][filter][j][1] /= 2
            plot_data[i][filter] = plot_data[i][filter][:len(plot_data[i][filter]) // 2]
    for i in range(len(range_sizes) + 2):
        for filter in filters:
            if i >= len(range_sizes):
                axes[1][i - 2].plot(*zip(*plot_data[i][filter]), **RANGE_FILTERS_STYLE_KWARGS[filter], **LINES_STYLE)
            else:
                axes[0][i].plot(*zip(*plot_data[i][filter]), **RANGE_FILTERS_STYLE_KWARGS[filter], **LINES_STYLE)

    for i in range(2):
        axes[1][i].set_xlabel("\\# of Expansions", fontsize=XLABEL_FONT_SIZE)
        axes[0][i].set_xticks(XTICKS)
        axes[1][i].set_xticks(XTICKS)
    for i, j in ((0, 0), (0, 1), (1, 1)):
        if i == 0:
            axes[i][j].set_yscale("symlog", linthresh=(1e-05))
            axes[i][j].set_ylim(bottom=-0.0000003, top=1.9)
        else:
            axes[i][j].set_yscale("log")
        axes[i][j].yaxis.set_minor_locator(matplotlib.ticker.LogLocator(numticks=10, subs="auto"))
    axes[1][0].yaxis.set_minor_locator(matplotlib.ticker.MultipleLocator(1))
    axes[1][0].set_ylim(bottom=15, top=32)

    axes[0][0].set_title(RANGE_LENGTH_NAMES[range_sizes[0]], fontsize=TITLE_FONT_SIZE)
    axes[0][0].set_ylabel("FPR", fontsize=YLABEL_FONT_SIZE)

    axes[0][1].set_title(RANGE_LENGTH_NAMES[range_sizes[1]], fontsize=TITLE_FONT_SIZE)
    axes[0][1].set_yticklabels([])

    axes[1][0].set_title("Size", fontsize=TITLE_FONT_SIZE)
    axes[1][0].set_ylabel("BPK", fontsize=TITLE_FONT_SIZE)

    axes[1][1].set_title("Inserts", fontsize=TITLE_FONT_SIZE)
    axes[1][1].yaxis.set_label_position("right")
    axes[1][1].yaxis.tick_right()
    axes[1][1].set_ylabel("Time [ns/insert]", fontsize=YLABEL_FONT_SIZE)

    legend_lines, legend_labels = axes[1][1].get_legend_handles_labels()
    axes[0][1].legend(legend_lines, legend_labels, loc="upper left", bbox_to_anchor=(1.0,0.975),
                  fancybox=True, shadow=False, ncol=1, fontsize=LEGEND_FONT_SIZE)
    fig.tight_layout()
    fig.savefig(output_dir / "expansion.pdf", bbox_inches='tight', pad_inches=0.01)


def plot_delete(result_dir, output_dir):
    LEGEND_FONT_SIZE = 7
    YLABEL_FONT_SIZE = 9.5
    XLABEL_FONT_SIZE = 9.5
    WIDTH = 1.6
    HEIGHT = 1.5

    filters = ["steroids", "steroids_int", "memento_expandable", "snarf"]
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

    ax.set_xlabel("Size [BPK]", fontsize=XLABEL_FONT_SIZE)
    ax.set_xticks(memory_footprints)
    ax.set_ylabel("Time [ns/delete]", fontsize=YLABEL_FONT_SIZE)
    ax.set_ylim(bottom=-0.0000003)
    ax.yaxis.set_minor_locator(matplotlib.ticker.MultipleLocator(1000))

    legend_lines, legend_labels = ax.get_legend_handles_labels()
    ax.legend(legend_lines, legend_labels, loc="upper left", bbox_to_anchor=(1.0,0.75),
              fancybox=True, shadow=False, ncol=1, fontsize=LEGEND_FONT_SIZE)
    fig.savefig(output_dir / "delete.pdf", bbox_inches='tight', pad_inches=0.01)


PLOTTERS = {"fpr": plot_fpr,
            "fpr_string": plot_fpr_string,
            "true": plot_true,
            "construction": plot_construction,
            "expansion": plot_expansion,
            "delete": plot_delete}

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
