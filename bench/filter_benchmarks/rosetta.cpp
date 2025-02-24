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

#include <boost/range/range_fwd.hpp>
#include <cmath>
#include <cstdint>
#include <utility>

#include "../bench_template.hpp"
#include "../include/rosetta/dst.h"

int range_size = -1;

template <typename t_itr, typename... Args>
inline DstFilter<BloomFilter<>> init(const t_itr begin, const t_itr end, const double bpk, Args... args) {
    auto sample_rate = 0.1;
    auto maxlen = 64, cutoff = 0, dfs_diff = 100, bfs_diff = 32;

    auto&& t = std::forward_as_tuple(args...);
    auto queries = std::get<0>(t);
    if (range_size != -1) {
        std::transform(queries.begin(), queries.end(), queries.begin(), [](auto x) {
                auto [left, right, result] = x;
                return std::tuple(left, left + range_size, result);
            });
    }
    auto model_queries = std::round(queries.size() * sample_rate);
    time_points['m'] = timer::now();
    DstFilter<BloomFilter<true>, true> dst_stat(dfs_diff, bfs_diff,
                                                [](vector<size_t> x) -> vector<size_t> {
                                                        for (size_t i=0; i<x.size(); ++i) {
                                                            x[i]*=1.44;
                                                        }
                                                        return x; 
                                                    });

    vector<Bitwise> tmp;
    tmp.emplace_back(false, maxlen); //maxlen = 64?
    dst_stat.AddKeys(tmp);

    for (auto it = queries.begin(); it < queries.begin() + model_queries; ++it) {
        auto [first, second, _] = *it;
        (void)dst_stat.Query(first, second);
    }
    auto qdist = std::vector<size_t>(dst_stat.qdist_.begin(), dst_stat.qdist_.end());
    timer_results['m'] = std::chrono::duration_cast<std::chrono::milliseconds>(timer::now() - time_points['m']).count();
    std::vector<Bitwise> string_keys(begin, end);
    time_points['c'] = timer::now();
    auto dst_inst = DstFilter<BloomFilter<>>(dfs_diff, bfs_diff,
                                       [&](vector<size_t> x) ->
                                       vector<size_t> { return calc_dst(std::move(x), bpk, qdist, cutoff); });
    dst_inst.AddKeys(string_keys);
    return dst_inst;
}

inline void insert(DstFilter<BloomFilter<>>& f, uint64_t key) {
    f.AddKey(key);
}

inline void del(DstFilter<BloomFilter<>>& f, uint64_t key) {
    throw std::runtime_error("Fitler does not support deletes");
}

inline bool query(DstFilter<BloomFilter<>>& f, uint64_t left, uint64_t right) {
    if (left == right)
        return f.Query(left);
    /* Query ranges are [x, y) for Rosetta, so we increment the right endpoint */
    return f.Query(left, right + 1);
}

inline size_t size(DstFilter<BloomFilter<>>& f) {
    auto ser = f.serialize();
    return ser.second;
}

int main(int argc, char const *argv[]) {
    auto parser = init_parser("bench-rosetta");

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

    if (auto max_range_size = parser.present<int>("--range-size")) {
        range_size = *max_range_size;
    }

    experiment(pass_fun(init), pass_ref(insert), pass_ref(del), pass_ref(query), pass_ref(size),
               wio.GetIntQueries());

    return 0;
}
