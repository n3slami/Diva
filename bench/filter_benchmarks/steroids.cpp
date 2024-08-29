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
#include <cmath>
#include <cstdint>
#include "steroids.hpp"

template <typename t_itr>
inline Steroids<false> *init(const t_itr begin, const t_itr end, const double bpk) {
    const uint32_t rng_seed = 1024;
    const double load_factor = 0.95;
    const uint32_t infix_size = std::round(load_factor * (bpk - 1));
    
    Steroids<false> *filter = new Steroids<false>(infix_size, begin, end, rng_seed, load_factor);
    return filter;
}

inline void insert(Steroids<false> *filter, const uint8_t *key, uint32_t key_length) {
    filter->Insert(key, key_length);
}

inline void del(Steroids<false> *filter, const uint8_t *key, uint32_t key_length) {
    filter->Delete(key, key_length);
}

inline bool query(const Steroids<false> *filter, const uint8_t *l_key, uint16_t l_key_length,
                                                 const uint8_t *r_key, uint16_t r_key_length) {
    return filter->RangeQuery(l_key, l_key_length, r_key, r_key_length);
}

inline size_t size(const Steroids<false> *filter) {
    return filter->Size();
}


int main(int argc, char const *argv[]) {
    auto parser = init_parser("bench-steroids");

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

    experiment_string(pass_fun(init), pass_fun(insert), pass_fun(del), pass_fun(query), pass_fun(size));

    print_test();

    return 0;
}
