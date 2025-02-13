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
#include "../include/proteus/include/proteus.hpp"
#include "../include/proteus/include/util.hpp"
#include "../include/proteus/include/modeling.hpp"
#include <algorithm>
#include <cstdint>
#include <limits>
#include <tmmintrin.h>

#define SUPPRESS_STDOUT
const double default_sample_rate = 0.2;

int range_size = -1;

template <typename t_itr, typename... Args>
inline proteus::Proteus init(const t_itr begin, const t_itr end, const double bpk, Args... args) {
    std::vector<typename t_itr::value_type> keys(begin, end);
    auto&& t = std::forward_as_tuple(args...);
    auto queries_temp = std::get<0>(t);
    auto sample_rate = std::get<1>(t);
    auto klen = 64;
    auto queries = std::vector<std::pair<uint64_t, uint64_t>>(queries_temp.size());
    if (range_size != -1) {
        const uint64_t seed = 10;
        std::mt19937_64 rng(seed);
        std::transform(queries_temp.begin(), queries_temp.end(), queries.begin(), [&](auto x) {
                const uint64_t left = rng();
                const uint64_t next_key_ind = std::upper_bound(keys.begin(), keys.end(), left) - keys.begin();
                const uint64_t length = std::min<uint64_t>(range_size, (next_key_ind < keys.size() ? keys[next_key_ind] 
                                                                                 : std::numeric_limits<uint64_t>::max())
                                                                        - left + 1);
                return std::make_pair(left, left + rng() % length);
            });
    }
    else {
        std::transform(queries_temp.begin(), queries_temp.end(), queries.begin(), [](auto x) {
                auto [left, right, result] = x;
                return std::make_pair(left, right);
            });
    }

    auto sample_queries = proteus::sampleQueries(queries, sample_rate);
    std::sort(sample_queries.begin(), sample_queries.end());
    time_points['m'] = timer::now();
    std::tuple<size_t, size_t, size_t> parameters = proteus::modeling(keys, sample_queries, bpk, klen);
    timer_results['m'] = std::chrono::duration_cast<std::chrono::milliseconds>(timer::now() - time_points['m']).count();
    time_points['c'] = timer::now();
    auto p = proteus::Proteus(keys,
                              std::get<0>(parameters),  // Trie Depth
                              std::get<1>(parameters),  // Sparse-Dense Cutoff
                              std::get<2>(parameters),  // Bloom Filter Prefix Length
                              bpk);
    return p;
}

inline void insert(proteus::Proteus& f, uint64_t key) {
    throw std::runtime_error("Fitler does not support inserts");
}

inline void del(proteus::Proteus& f, uint64_t key) {
    throw std::runtime_error("Fitler does not support deletes");
}

inline bool query(proteus::Proteus& f, uint64_t left, uint64_t right) {
    if (left == right)
        return f.Query(left);
    /* Query ranges are [x, y) for Proteus, so we increment the right endpoint */
    return f.Query(left, right + 1);
}

inline size_t size(proteus::Proteus& f) {
    auto ser = f.serialize();
    return ser.second;
}

int main(int argc, char const *argv[]) {
    auto parser = init_parser("bench-proteus");

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

    if (auto max_range_size = parser.present<int>("--range_size")) {
        range_size = *max_range_size;
    }

    auto queries = wio.GetIntQueries();
    experiment(pass_fun(init), pass_ref(insert), pass_ref(del), pass_ref(query), pass_ref(size),
               queries, default_sample_rate);

    return 0;
}
