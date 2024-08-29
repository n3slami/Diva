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
#include "../include/surf/include/surf.hpp"

template <typename t_itr, typename... Args>
inline surf::SuRF init(const t_itr begin, const t_itr end, const int suffix_bits, Args... args) {
    std::vector<std::string> string_keys(std::distance(begin, end));
    std::transform(begin, end, string_keys.begin(), [&](auto k) { return uint64ToString(k); });
    time_points['c'] = timer::now();
    surf::SuRF s = surf::SuRF(string_keys, surf::kReal, 0, suffix_bits);
    return s;
}

template <typename t_itr, typename... Args>
inline surf::SuRF init_hash(const t_itr begin, const t_itr end, const int suffix_bits, Args... args) {
    std::vector<std::string> string_keys(std::distance(begin, end));
    std::transform(begin, end, string_keys.begin(), [&](auto k) { return uint64ToString(k); });
    time_points['c'] = timer::now();
    surf::SuRF s = surf::SuRF(string_keys, surf::kHash, suffix_bits, 0);
    return s;
}

template <typename T>
inline void insert(surf::SuRF& f, T key) {
    throw std::runtime_error("Fitler does not support inserts");
}

template <typename T>
inline void del(surf::SuRF& f, T key) {
    throw std::runtime_error("Fitler does not support deletes");
}

inline bool query(surf::SuRF& f, uint64_t left, uint64_t right) {
    if (left == right)
        return f.lookupKey(uint64ToString(left));
    return f.lookupRange(uint64ToString(left), true, uint64ToString(right), true);
}

inline bool query(surf::SuRF& f, std::string& left, std::string& right) {
    if (left == right)
        return f.lookupKey(left);
    return f.lookupRange(left, true, right, true);
}

inline size_t size(surf::SuRF& f) {
    return f.serializedSize();
}

int main(int argc, char const *argv[]) {
    auto parser = init_parser("bench-surf");

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

    // Check if all the queries are point queries, if so we use the hash version of SuRF, otherwise we use the real version.
    auto queries = wio.GetIntQueries();
    bool surf_hash = !queries.empty();
    for (auto & it : queries) {
        if (std::get<0>(it) != std::get<1>(it)) {
            surf_hash = false;
            break;
        }
    }

    if (surf_hash)
        experiment(pass_fun(init_hash), pass_ref(insert), pass_ref(del), pass_ref(query), pass_ref(size));
    else
        experiment(pass_fun(init), pass_ref(insert), pass_ref(del), pass_ref(query), pass_ref(size));
    print_test();

    return 0;
}
