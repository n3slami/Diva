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

#include <cassert>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <argparse/argparse.hpp>
#include <limits>
#include <ratio>
#include <stdexcept>
#include <sys/types.h>
#include "bench_utils.hpp"

#define pass_fun(f) ([](auto... args){ return f(args...); })
#define pass_ref(fun) ([](auto& f, auto... args){ return fun(f, args...); })

inline auto test_out = TestOutput();

inline auto test_verbose = true;
inline bool print_json = false;
inline std::string json_file = "";

inline WorkloadIO wio;
inline InputKeys<uint64_t> initial_int_keys;
inline InputKeys<ByteString> initial_string_keys;
inline timer::time_point time_points[std::numeric_limits<uint8_t>::max()];
inline uint64_t timer_results[std::numeric_limits<uint8_t>::max()];

template <typename InitFun, typename InsertFun, typename DeleteFun, typename RangeFun, typename SizeFun, typename... Args>
void experiment(InitFun init_f, InsertFun insert_f, DeleteFun delete_f, RangeFun range_f, SizeFun size_f, Args... args) {
    uint16_t l_buf_len, r_buf_len;
    uint8_t l_buf[std::numeric_limits<uint16_t>::max()];
    uint8_t r_buf[std::numeric_limits<uint16_t>::max()];
    
    uint32_t n_keys, n_queries;
    uint32_t false_positives = 0, false_negatives = 0;
    if (wio.StringKeys()) {
        n_keys = initial_string_keys.size();
        time_points['c'] = timer::now();
        auto filter = init_f(initial_string_keys.begin(), initial_string_keys.end(), args...);
        timer_results['c'] = std::chrono::duration_cast<std::chrono::milliseconds>(timer::now() - time_points['c']).count();

        {
            test_out.AddMeasure("n_keys", n_keys);
            const size_t filter_size = size_f(filter);
            test_out.AddMeasure("size", filter_size);
            test_out.AddMeasure("size", static_cast<long double>(filter_size * 8) / n_keys);
            test_out.AddMeasure("construction_time", timer_results);
            std::cout << test_out.ToJson() << std::endl;
            timer_results['c'] = 0;
            test_out.Clear();
        }

        while (!wio.Done()) {
            WorkloadIO::opcode opcode = wio.GetOpcode();
            switch (opcode) {
                case WorkloadIO::opcode::Bulk:
                    throw std::runtime_error("Cannot bulk load in the middle of a workload");
                case WorkloadIO::opcode::Insert: {
                    wio.GetStringKey(l_buf_len, l_buf);
                    insert_f(filter, l_buf, l_buf_len);
                    n_keys++;
                    break;
                }
                case WorkloadIO::opcode::Delete: {
                    wio.GetStringKey(l_buf_len, l_buf);
                    delete_f(filter, l_buf, l_buf_len);
                    n_keys--;
                    break;
                }
                case WorkloadIO::opcode::Query: {
                    bool actual_res;
                    wio.GetStringQuery(l_buf_len, l_buf, r_buf_len, r_buf, actual_res);
                    bool filter_res = range_f(filter, l_buf, l_buf_len, r_buf, r_buf_len);
                    false_positives += filter_res & (!actual_res);
                    false_negatives += (!filter_res) & actual_res;
                    n_queries++;
                    break;
                }
                case WorkloadIO::opcode::Timer: {
                    char timer_key = wio.ReadValue<char>();
                    if (timer_results[timer_key] == 0)
                        time_points[timer_key] = timer::now();
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
                    test_out.AddMeasure("bpk", static_cast<long double>(filter_size) / n_keys);
                    for (int32_t i = 0; i < std::numeric_limits<uint8_t>::max(); i++) {
                        if (timer_results[i] > 0) {
                            std::string measure_name = "time_";
                            measure_name += static_cast<char>(i);
                            test_out.AddMeasure(measure_name, timer_results[i]);
                        }
                    }

                    std::cout << test_out.ToJson() << std::endl;

                    n_queries = 0;
                    memset(timer_results, 0, sizeof(timer_results));
                    test_out.Clear();
                    break;
                }
            }
        }
    }
    else {
        n_keys = initial_int_keys.size();
        time_points['c'] = timer::now();
        auto filter = init_f(initial_int_keys.begin(), initial_int_keys.end(), args...);
        timer_results['c'] = std::chrono::duration_cast<std::chrono::milliseconds>(timer::now() - time_points['c']).count();

        {
            test_out.AddMeasure("n_keys", n_keys);
            const size_t filter_size = size_f(filter);
            test_out.AddMeasure("size", filter_size);
            test_out.AddMeasure("size", static_cast<long double>(filter_size * 8) / n_keys);
            test_out.AddMeasure("construction_time", timer_results);
            std::cout << test_out.ToJson() << std::endl;
            timer_results['c'] = 0;
            test_out.Clear();
        }

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
                    delete_f(filter, wio.ReadValue<uint64_t>());
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
                    if (timer_results[timer_key] == 0)
                        time_points[timer_key] = timer::now();
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
                    test_out.AddMeasure("bpk", static_cast<long double>(filter_size) / n_keys);
                    for (int32_t i = 0; i < std::numeric_limits<uint8_t>::max(); i++) {
                        if (timer_results[i] > 0) {
                            std::string measure_name = "time_";
                            measure_name += static_cast<char>(i);
                            test_out.AddMeasure(measure_name, timer_results[i]);
                        }
                    }

                    std::cout << test_out.ToJson() << std::endl;

                    n_queries = 0;
                    memset(timer_results, 0, sizeof(timer_results));
                    test_out.Clear();
                    break;
                }
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
            initial_string_keys.push_back({buf, buf_len});
        }
        else 
            initial_int_keys.push_back(wio.ReadValue<uint64_t>());
    }
}


inline void print_test() {
    if (test_verbose)
        std::cout << test_out.ToJson() << std::endl;

    if (print_json) {
        std::cout << "[+] writing results in " << json_file << std::endl;
        std::ofstream outFile(std::filesystem::path(json_file), std::ios::app);
        outFile << test_out.ToJson();
        outFile.close();
    }
}
