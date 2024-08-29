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
#include "../include/oasis/src/include/oasis_plus.h"
#include <cstdint>

const int block_size = 150;

template <typename t_itr>
inline oasis_plus::OasisPlus init(const t_itr begin, const t_itr end, const double bpk) {
    oasis_plus::OasisPlus f(bpk, block_size, {begin, end});
    return f;
}

inline void insert(oasis_plus::OasisPlus& f, uint64_t key) {
    throw std::runtime_error("Fitler does not support inserts");
}

inline void del(oasis_plus::OasisPlus& f, uint64_t key) {
    throw std::runtime_error("Fitler does not support deletes");
}

inline bool query(oasis_plus::OasisPlus& f, uint64_t left, uint64_t right) {
    return f.query(left, right);
}

inline size_t size(oasis_plus::OasisPlus& f) {
    return f.size();
}

int main(int argc, char const *argv[]) {
    auto parser = init_parser("bench-oasis");
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

    experiment(pass_fun(init), pass_ref(insert), pass_ref(del), pass_ref(query), pass_ref(size));
    print_test();

    return 0;
}
