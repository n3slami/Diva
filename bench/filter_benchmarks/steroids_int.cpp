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
#include "steroids.hpp"

template <typename t_itr>
inline Steroids<true> *init(const t_itr begin, const t_itr end, const double bpk) {
    const uint32_t rng_seed = 1024;
    const double load_factor = 0.95;
    const uint32_t infix_size = std::round(load_factor * (bpk - 1));
    
    Steroids<true> *filter = new Steroids<true>(infix_size, begin, end, sizeof(uint64_t), rng_seed, load_factor);
    return filter;
}

inline void insert(Steroids<true> *filter, uint64_t key) {
    const uint64_t k = __builtin_bswap64(key);
    filter->Insert(reinterpret_cast<const uint8_t *>(&k), sizeof(k));
}

inline void del(Steroids<true> *filter, uint64_t key) {
    const uint64_t k = __builtin_bswap64(key);
    filter->Delete(reinterpret_cast<const uint8_t *>(&k), sizeof(k));
}

inline bool query(const Steroids<true> *filter, uint64_t l_key, uint64_t r_key) {
    const uint64_t l = __builtin_bswap64(l_key);
    const uint64_t r = __builtin_bswap64(r_key);
    return filter->RangeQuery(reinterpret_cast<const uint8_t *>(&l), sizeof(l), 
                              reinterpret_cast<const uint8_t *>(&r), sizeof(r));
}

inline size_t size(const Steroids<true> *filter) {
    return filter->Size();
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
    read_workload(parser.get<std::string>("--workload"));

    experiment(pass_fun(init), pass_fun(insert), pass_fun(del), pass_fun(query), pass_fun(size));

    return 0;
}
