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

#include "../bench_template_wiredtiger.hpp"
#include <cstdint>

template <typename t_itr>
inline void *init(const t_itr begin, const t_itr end, const double bpk) {
    return nullptr;
}

inline void insert(void *filter, uint64_t key) {
    return;
}

inline void del(void *filter, uint64_t key) {
    return;
}

inline bool query(const void *filter, uint64_t l_key, uint64_t r_key) {
    return true;
}

inline size_t size(const void *filter) {
    return 0;
}


int main(int argc, char const *argv[]) {
    auto parser = init_parser("bench-steroids-int");

    try {
        parser.parse_args(argc, argv);
    }
    catch (const std::runtime_error &err) {
        std::cerr << err.what() << std::endl;
        std::cerr << parser;
        std::exit(1);
    }
    memory_budget = parser.get<double>("arg");
    memory_to_disk_ratio = parser.get<double>("--memory_to_disk_ratio ");
    val_len = parser.get<int>("--val_len");
    read_workload(parser.get<std::string>("--workload"));

    experiment(pass_fun(init), pass_fun(insert), pass_fun(del), pass_fun(query), pass_fun(size));

    return 0;
}
