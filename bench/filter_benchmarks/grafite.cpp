/*
 * This file is part of --- <>.
 * Copyright (C) 2024 ---.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "../bench_template.hpp"
#include <cstdint>
#include <stdexcept>
#include "grafite/grafite.hpp"

/**
 * This file contains the benchmark for the Grafite filter.
 */

std::string default_container = "sux";

template <typename REContainer, typename t_itr>
inline grafite::filter<REContainer> init(const t_itr begin, const t_itr end, const double bpk) {
    grafite::filter<REContainer> filter(begin, end, bpk);
    return filter;
}

template <typename REContainer>
inline void insert(grafite::filter<REContainer>& f, uint64_t key) {
    throw std::runtime_error("Fitler does not support inserts");
}

template <typename REContainer>
inline void del(grafite::filter<REContainer>& f, uint64_t key) {
    throw std::runtime_error("Fitler does not support deletes");
}

template <typename REContainer>
inline bool query(grafite::filter<REContainer>& f, uint64_t left, uint64_t right) {
    return f.query(left, right);
}

template <typename REContainer>
inline size_t size(const grafite::filter<REContainer>& f) {
    return f.size();
}

int main(int argc, char const *argv[]) {
    auto parser = init_parser("bench-grafite");
    parser.add_argument("--ds")
        .nargs(1)
        .default_value(default_container);

    try {
        parser.parse_args(argc, argv);
    }
    catch (const std::runtime_error &err) {
        std::cerr << err.what() << std::endl;
        std::cerr << parser;
        std::exit(1);
    }
    memory_budget = parser.get<double>("arg");
    read_workload(parser.get<std::string>("--workload"));
    std::string container = parser.get<std::string>("ds");

    std::cout << "[+] using container `" << container << "`" << std::endl;
    if (container == "sux")
        experiment(pass_fun(init<grafite::ef_sux_vector>), pass_ref(insert), 
                   pass_ref(del), pass_ref(query), pass_ref(size));
    else if (container == "sdsl")
        experiment(pass_fun(init<grafite::ef_sdsl_vector>), pass_ref(insert), 
                   pass_ref(del), pass_ref(query), pass_ref(size));
    else
        throw std::runtime_error("Unknown range emptiness data structure");

    return 0;
}
