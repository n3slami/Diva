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

#include <bits/stdc++.h>
#include <cstdint>
#include <immintrin.h>

#include "../include/rencoder/src/BOBHash32.h"
#include "../include/rencoder/src/RBF.h"
#include "../include/rencoder/src/REncoder.h"

#ifndef SET_VERSION
#define SET_VERSION "REncoder"
#endif

long long cache_hit = 0;
long long query_count = 0;

int HASH_NUM = 3;
int STORED_LEVELS;
int START_LEVEL = 1;
int END_LEVEL = -1;

std::string VERSION = SET_VERSION;

int LongestCommonPrefix(const uint64_t a, const uint64_t b, const size_t max_klen) {
    (void) max_klen;
    uint64_t xored = a ^ b;
    return xored == 0 ? 64 : __builtin_clzll(xored);
}

std::vector<std::pair<uint64_t, uint64_t>> SampleQueries(std::vector<uint64_t> &keys,
                                                         std::vector<std::pair<uint64_t, uint64_t>> &queries,
                                                         double sample_rate) {
    std::vector<std::pair<uint64_t, uint64_t>> sample_queries;
    std::default_random_engine generator;
    std::bernoulli_distribution distribution(sample_rate);
    for (auto const &q: queries) {
        if (distribution(generator))
            sample_queries.push_back(q);
    }

    return sample_queries;
}

std::pair<int, int> KQlcp(const std::vector<uint64_t> &keys,
                          const size_t max_klen,
                          std::vector<uint64_t>::const_iterator &kstart,
                          uint64_t qleft,
                          uint64_t qright) {
    kstart = lower_bound(kstart, keys.cend(), qleft);

    if (kstart != keys.cend() && *kstart <= qright)
        return std::make_pair(-1, -1);

    auto kleft = (kstart != keys.cbegin()) ? kstart - 1 : kstart;
    auto kright = kstart;

    if (qleft > *kleft && kright != keys.cend() && qright < *kright)
        return std::make_pair(LongestCommonPrefix(qleft, *kleft, max_klen),
                              LongestCommonPrefix(qright, *kright, max_klen));
    else if (qleft < *kleft && qright < *kleft)
        return std::make_pair(-1, LongestCommonPrefix(qright, *kleft, max_klen));
    else if (kright == keys.cend() && keys.back() < qleft)
        return std::make_pair(LongestCommonPrefix(qleft, keys.back(), max_klen), -1);


    throw std::runtime_error("Should not reach here!");
    return {};
}

void SetBeginEndLevel(std::vector<uint64_t> &keys, std::vector<std::pair<uint64_t, uint64_t>> &queries) {
    int max_kklcp = -1;
    int max_kqlcp = -1;
    for (size_t i = 1; i < keys.size(); i++) {
        max_kklcp = max(max_kklcp, LongestCommonPrefix(keys[i], keys[i - 1], 64));
    }
    if (VERSION == "REncoderSE") {
        auto sample_queries = SampleQueries(keys, queries, 0.001);
        std::vector<uint64_t>::const_iterator kstart = keys.cbegin();
        for (auto const &q: sample_queries) {
            std::pair<int, int> lcps = KQlcp(keys, 64, kstart, q.first, q.second);
            if (lcps.first < 0 && lcps.second < 0)
                continue;
            else {
                max_kqlcp = max(max_kqlcp, lcps.first);
                max_kqlcp = max(max_kqlcp, lcps.second);
            }
        }
    }
    if (VERSION == "REncoderSE" && max_kklcp < max_kqlcp) {
        END_LEVEL = 64 - max_kqlcp;
    }
    START_LEVEL = 66 - max_kklcp;
    return;
}

template<typename t_itr, typename... Args>
inline RENCODER init(const t_itr begin, const t_itr end, const double bpk, Args... args) {
    auto memory = bpk * std::distance(begin, end);
    auto keys = std::vector(begin, end);
    auto&& t = std::forward_as_tuple(args...);
    auto queries_temp = std::get<0>(t);
    auto klen = 64;
    auto queries = std::vector<std::pair<uint64_t, uint64_t>>(queries_temp.size());
    std::transform(queries_temp.begin(), queries_temp.end(), queries.begin(), [](auto x) {
        auto [left, right, result] = x;
        return std::make_pair(left, right);
    });

    if (VERSION == "REncoderSE" || VERSION == "REncoderSS") {
        time_points['m'] = timer::now();
        SetBeginEndLevel(keys, queries);
        timer_results['m'] = std::chrono::duration_cast<std::chrono::milliseconds>(timer::now() - time_points['m']).count();
    }
    time_points['c'] = timer::now();
    RENCODER f = RENCODER();
    f.init(memory, HASH_NUM, 64, STORED_LEVELS, START_LEVEL, END_LEVEL);
    f.Insert_SelfAdapt(keys, 1);
    return f;
}

inline void insert(RENCODER& f, uint64_t key) {
    f.Insert(key);
}

inline void del(RENCODER& f, uint64_t key) {

}

inline bool query(RENCODER& f, uint64_t left, uint64_t right) {
    return f.RangeQuery(left, right);
}

inline size_t size(RENCODER& f) {
    auto [_, size] = f.serialize();
    return size;
}


int main(int argc, char const *argv[]) {
    auto parser = init_parser("bench-rencoder");
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

    std::cout << "[INFO] " << "REncoder version: " << VERSION << std::endl;

    experiment(pass_fun(init), pass_ref(insert), pass_ref(del), pass_ref(query), pass_ref(size), 
               wio.GetIntQueries());

    return 0;
}
