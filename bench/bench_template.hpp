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

#pragma once

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <argparse/argparse.hpp>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <vector>
#include "bench_utils.hpp"

#define pass_fun(f) ([](auto... args){ return f(args...); })
#define pass_ref(fun) ([](auto& f, auto... args){ return fun(f, args...); })

inline auto test_out = TestOutput();

inline std::string json_file = "";
inline double memory_budget = 10.0;
inline uint64_t kill_exec_time_threshold = 1ULL * 3600ULL * 1000000ULL;

inline WorkloadIO wio;
inline InputKeys<uint64_t> initial_int_keys;
inline InputKeys<std::string> initial_string_keys;
inline timer::time_point time_points[std::numeric_limits<uint8_t>::max()];
inline uint64_t timer_results[std::numeric_limits<uint8_t>::max()];

template <typename InitFun, typename InsertFun, typename DeleteFun, typename RangeFun, typename SizeFun, typename... Args>
void experiment(InitFun init_f, InsertFun insert_f, DeleteFun delete_f, RangeFun range_f, SizeFun size_f, Args... args) {
    uint16_t l_buf_len, r_buf_len;
    uint8_t l_buf[std::numeric_limits<uint16_t>::max()];
    uint8_t r_buf[std::numeric_limits<uint16_t>::max()];
    
    uint32_t n_keys = initial_int_keys.size(), n_queries = 0;
    uint32_t false_positives = 0, false_negatives = 0;
    time_points['c'] = timer::now();
    auto filter = init_f(initial_int_keys.begin(), initial_int_keys.end(), memory_budget, args...);
    timer_results['c'] = std::chrono::duration_cast<std::chrono::milliseconds>(timer::now() - time_points['c']).count();

    {
        test_out.AddMeasure("n_keys", n_keys);
        const size_t filter_size = size_f(filter);
        test_out.AddMeasure("size", filter_size);
        test_out.AddMeasure("bpk", static_cast<long double>(filter_size * 8) / n_keys);
        test_out.AddMeasure("construction_time", timer_results['c']);
        if (timer_results['m'] > 0)
            test_out.AddMeasure("modeling_time", timer_results['m']);
        std::cout << test_out.ToJson() << ',' << std::endl;
        timer_results['c'] = 0;
        timer_results['m'] = 0;
        test_out.Clear();
    }

    timer::time_point op_start_time = timer::now();
    while (!wio.Done()) {
        WorkloadIO::opcode opcode = wio.GetOpcode();
        switch (opcode) {
            case WorkloadIO::opcode::Bulk:
                throw std::runtime_error("Cannot bulk load in the middle of a workload");
            case WorkloadIO::opcode::Insert: {
                insert_f(filter, wio.ReadValue<uint64_t>());
                n_keys++;
                break;
            }
            case WorkloadIO::opcode::Delete: {
                const uint64_t value = wio.ReadValue<uint64_t>();
                delete_f(filter, value);
                n_keys--;
                break;
            }
            case WorkloadIO::opcode::Query: {
                auto [l, r, actual_res] = wio.GetIntQuery();
                bool filter_res = range_f(filter, l, r);
                false_positives += filter_res & (!actual_res);
                false_negatives += (!filter_res) & actual_res;
                n_queries++;
                break;
            }
            case WorkloadIO::opcode::Timer: {
                char timer_key = wio.ReadValue<char>();
                if (timer_results[timer_key] == 0) {
                    time_points[timer_key] = timer::now();
                    timer_results[timer_key] = -1;
                }
                else
                    timer_results[timer_key] = std::chrono::duration_cast<std::chrono::milliseconds>(timer::now() - time_points[timer_key]).count();
                break;
            }
            case WorkloadIO::opcode::Flush: {
                test_out.AddMeasure("n_keys", n_keys);
                test_out.AddMeasure("n_queries", n_queries);
                test_out.AddMeasure("false_positives", false_positives);
                test_out.AddMeasure("false_negatives", false_negatives);
                test_out.AddMeasure("fpr", static_cast<long double>(false_positives) / n_queries);
                const size_t filter_size = size_f(filter);
                test_out.AddMeasure("size", filter_size);
                test_out.AddMeasure("bpk", static_cast<long double>(filter_size * 8) / n_keys);
                for (int32_t i = 0; i < std::numeric_limits<uint8_t>::max(); i++) {
                    if (timer_results[i] > 0) {
                        std::string measure_name = "time_";
                        measure_name += static_cast<char>(i);
                        test_out.AddMeasure(measure_name, timer_results[i]);
                    }
                }

                std::cout << test_out.ToJson() << ',' << std::endl;

                n_queries = 0;
                false_positives = 0;
                false_negatives = 0;
                memset(timer_results, 0, sizeof(timer_results));
                test_out.Clear();

                if (std::chrono::duration_cast<std::chrono::milliseconds>(timer::now() - op_start_time).count() 
                            > kill_exec_time_threshold)
                    return;
                
                break;
            }
        }
    }
}

template <typename InitFun, typename InsertFun, typename DeleteFun, typename RangeFun, typename SizeFun, typename... Args>
void experiment_string(InitFun init_f, InsertFun insert_f, DeleteFun delete_f, RangeFun range_f, SizeFun size_f, Args... args) {
    uint16_t l_buf_len, r_buf_len;
    uint8_t l_buf[std::numeric_limits<uint16_t>::max()];
    uint8_t r_buf[std::numeric_limits<uint16_t>::max()];
    
    uint32_t n_keys = initial_string_keys.size(), n_queries = 0;
    if (n_keys == 0) {
        n_keys = initial_int_keys.size();
        initial_string_keys = std::vector<std::string>(n_keys);
        std::transform(initial_int_keys.begin(), initial_int_keys.end(), initial_string_keys.begin(), [&](uint64_t k) { return uint64ToString(k); });
    }
    uint32_t false_positives = 0, false_negatives = 0;
    time_points['c'] = timer::now();
    auto filter = init_f(initial_string_keys.begin(), initial_string_keys.end(), memory_budget, args...);
    timer_results['c'] = std::chrono::duration_cast<std::chrono::milliseconds>(timer::now() - time_points['c']).count();

    {
        test_out.AddMeasure("n_keys", n_keys);
        const size_t filter_size = size_f(filter);
        test_out.AddMeasure("size", filter_size);
        test_out.AddMeasure("bpk", static_cast<long double>(filter_size * 8) / n_keys);
        test_out.AddMeasure("construction_time", timer_results['c']);
        if (timer_results['m'] > 0)
            test_out.AddMeasure("modeling_time", timer_results['m']);
        std::cout << test_out.ToJson() << ',' << std::endl;
        timer_results['c'] = 0;
        timer_results['m'] = 0;
        test_out.Clear();
    }

    while (!wio.Done()) {
        WorkloadIO::opcode opcode = wio.GetOpcode();
        switch (opcode) {
            case WorkloadIO::opcode::Bulk:
                throw std::runtime_error("Cannot bulk load in the middle of a workload");
            case WorkloadIO::opcode::Insert: {
                if (wio.StringKeys()) {
                    wio.GetStringKey(l_buf_len, l_buf);
                    insert_f(filter, l_buf, l_buf_len);
                }
                else {
                    uint64_t key = __builtin_bswap64(wio.ReadValue<uint64_t>());
                    insert_f(filter, reinterpret_cast<const uint8_t *>(&key),
                                     static_cast<uint16_t>(sizeof(key)));
                }
                n_keys++;
                break;
            }
            case WorkloadIO::opcode::Delete: {
                if (wio.StringKeys()) {
                    wio.GetStringKey(l_buf_len, l_buf);
                    delete_f(filter, l_buf, l_buf_len);
                }
                else {
                    uint64_t key = __builtin_bswap64(wio.ReadValue<uint64_t>());
                    delete_f(filter, reinterpret_cast<const uint8_t *>(&key),
                                     static_cast<uint16_t>(sizeof(key)));
                }
                n_keys--;
                break;
            }
            case WorkloadIO::opcode::Query: {
                bool actual_res, filter_res;
                if (wio.StringKeys()) {
                    wio.GetStringQuery(l_buf_len, l_buf, r_buf_len, r_buf, actual_res);
                    filter_res = range_f(filter, l_buf, l_buf_len, r_buf, r_buf_len);
                }
                else {
                    auto [l, r, res] = wio.GetIntQuery();
                    l = __builtin_bswap64(l);
                    r = __builtin_bswap64(r);
                    actual_res = res;
                    filter_res = range_f(filter, reinterpret_cast<const uint8_t *>(&l), static_cast<uint16_t>(sizeof(l)),
                                                 reinterpret_cast<const uint8_t *>(&r), static_cast<uint16_t>(sizeof(r)));
                }
                false_positives += filter_res & (!actual_res);
                false_negatives += (!filter_res) & actual_res;
                n_queries++;
                break;
            }
            case WorkloadIO::opcode::Timer: {
                char timer_key = wio.ReadValue<char>();
                if (timer_results[timer_key] == 0) {
                    time_points[timer_key] = timer::now();
                    timer_results[timer_key] = -1;
                }
                else
                    timer_results[timer_key] = std::chrono::duration_cast<std::chrono::milliseconds>(timer::now() - time_points[timer_key]).count();
                break;
            }
            case WorkloadIO::opcode::Flush: {
                test_out.AddMeasure("n_keys", n_keys);
                test_out.AddMeasure("n_queries", n_queries);
                test_out.AddMeasure("false_positives", false_positives);
                test_out.AddMeasure("false_negatives", false_negatives);
                test_out.AddMeasure("fpr", static_cast<long double>(false_positives) / n_queries);
                const size_t filter_size = size_f(filter);
                test_out.AddMeasure("size", filter_size);
                test_out.AddMeasure("bpk", static_cast<long double>(filter_size * 8) / n_keys);
                for (int32_t i = 0; i < std::numeric_limits<uint8_t>::max(); i++) {
                    if (timer_results[i] > 0) {
                        std::string measure_name = "time_";
                        measure_name += static_cast<char>(i);
                        test_out.AddMeasure(measure_name, timer_results[i]);
                    }
                }

                std::cout << test_out.ToJson() << ',' << std::endl;

                n_queries = 0;
                false_positives = 0;
                false_negatives = 0;
                memset(timer_results, 0, sizeof(timer_results));
                test_out.Clear();
                break;
            }
        }
    }
}


inline argparse::ArgumentParser init_parser(const std::string& name) {
    argparse::ArgumentParser parser(name);

    parser.add_argument("arg")
            .help("The main parameters of the filter (e.g., the memory footprint in bits-per-key)")
            .nargs(argparse::nargs_pattern::at_least_one)
            .scan<'g', double>();

    parser.add_argument("-w", "--workload")
            .help("Pass the workload from file")
            .nargs(1);

    parser.add_argument("-R", "--range-size")
            .help("Maximum range query size")
            .nargs(1)
            .scan<'i', int>();

    return parser;
}

inline void read_workload(const std::string& workload_file) {
    wio = WorkloadIO(workload_file, WorkloadIO::iomode::Read);

    WorkloadIO::opcode opcode = wio.GetOpcode();
    assert(opcode == WorkloadIO::opcode::Bulk);
    uint32_t key_count = wio.ReadValue<uint32_t>();
    uint8_t buf[std::numeric_limits<uint16_t>::max()];
    uint16_t buf_len;
    for (uint32_t i = 0; i < key_count; i++) {
        if (wio.StringKeys()) {
            wio.GetStringKey(buf_len, buf);
            initial_string_keys.push_back({reinterpret_cast<const char *>(buf), buf_len});
        }
        else
            initial_int_keys.push_back(wio.ReadValue<uint64_t>());
    }
}


inline void print_test() {
    std::cout << test_out.ToJson() << std::endl;
}
