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
#include "diva.hpp"

static const uint32_t max_thread_count = 1024;
static const uint32_t rng_seed = 1024;
static inline std::mt19937_64 rngs[max_thread_count];

template <typename t_itr>
inline diva::Diva<diva::DivaType::Int> *init(const t_itr begin, const t_itr end, const double bpk) {
    const uint32_t rng_seed = 1024;
    const double load_factor = 0.95;
    const uint32_t infix_size = std::round(load_factor * (bpk - 1));
    
    auto *filter = new diva::Diva<diva::DivaType::Int>(infix_size,
                                                       begin, end,
                                                       sizeof(uint64_t),
                                                       rng_seed,
                                                       load_factor);
    return filter;
}

inline void insert(diva::Diva<diva::DivaType::Int> *filter, uint64_t key) {
    filter->Insert(key);
}

inline void insert_concurrent(diva::Diva<diva::DivaType::Int> *filter,
                              uint64_t key,
                              uint32_t thread_id) {
    filter->Insert(key, nullptr, rngs[thread_id]());
}

inline void del(diva::Diva<diva::DivaType::Int> *filter, uint64_t key) {
    filter->Delete(key);
}

inline bool query(const diva::Diva<diva::DivaType::Int> *filter, uint64_t l_key, uint64_t r_key) {
    return filter->RangeQuery(l_key, r_key);
}

inline size_t size(const diva::Diva<diva::DivaType::Int> *filter) {
    return filter->Size();
}


int main(int argc, char const *argv[]) {
    auto parser = init_parser("bench-diva-int");

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
    const uint32_t num_threads = parser.get<int>("--num-threads");

    if (num_threads == 1)
        experiment(pass_fun(init), pass_fun(insert), pass_fun(del), pass_fun(query), pass_fun(size));
    else {
        if (num_threads > max_thread_count)
            throw std::runtime_error("Number of threads requested exceed the maximum of 1024");
        for (int32_t i = 0; i < num_threads; i++)
            rngs[i].seed(rng_seed + i);
        experiment_concurrency(num_threads, pass_fun(init), pass_fun(insert_concurrent), pass_fun(size));
    }


    return 0;
}
