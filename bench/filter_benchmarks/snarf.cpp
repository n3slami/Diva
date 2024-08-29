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
#include "../include/snarf/include/snarf.cpp"

template <typename t_itr>
inline snarf_updatable_gcs<typename t_itr::value_type> init(const t_itr begin, const t_itr end, const double bpk) {
    std::vector<typename t_itr::value_type> keys(begin, end);
    time_points['c'] = timer::now();
    snarf_updatable_gcs<uint64_t> f;
    f.snarf_init(keys, bpk, 100);
    return f;
}

inline void insert(snarf_updatable_gcs<uint64_t>& f, uint64_t key) {
    f.insert_key(key);
}

inline void del(snarf_updatable_gcs<uint64_t>& f, uint64_t key) {
    f.delete_key(key);
}


inline bool query(snarf_updatable_gcs<uint64_t>& f, uint64_t left, uint64_t right) {
    return f.range_query(left, right);
}

inline size_t size(snarf_updatable_gcs<uint64_t>& f) {
    return f.return_size();
}


int main(int argc, char const *argv[]) {
    auto parser = init_parser("bench-snarf");
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
