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
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <vector>
#include "bench_utils.hpp"
#include <wiredtiger.h>
#include <x86intrin.h>

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

static const char *wt_home = "./wt_database_home";
const uint32_t max_schema_len = 128;
const uint32_t max_conn_config_len = 128;
const int default_key_len = 8, default_val_len = 504;
const double default_memory_to_disk_ratio = 0.01;
static double memory_to_disk_ratio = default_memory_to_disk_ratio;
static uint64_t key_len = default_key_len, val_len = default_val_len;
static uint64_t optimizer_hack = 0;


static uint64_t compute_buffer_pool_size_mb(uint32_t n_keys, uint64_t filter_size) {
    return std::max<uint64_t>((n_keys * (key_len + val_len) * memory_to_disk_ratio - filter_size) / 1024.0 / 1024.0, 1ULL);
}


static inline void error_check(int ret) {
    if (ret != 0) {
        std::cerr << "WiredTiger Error: " << wiredtiger_strerror(ret) << std::endl;
        exit(ret);
    }
}


static inline void insert_kv(WT_CURSOR *cursor, const uint8_t *key, const uint8_t *value) {
    cursor->set_key(cursor, key);
    cursor->set_value(cursor, value);
    error_check(cursor->insert(cursor));
}


static inline void delete_kv(WT_CURSOR *cursor, const uint8_t *key) {
    cursor->set_key(cursor, key);
    error_check(cursor->remove(cursor));
}


static inline void fetch_range_from_db(WT_CURSOR *cursor, const uint8_t *l, const uint8_t *r) {
    error_check(cursor->reset(cursor));
    cursor->set_key(cursor, l);
    error_check(cursor->bound(cursor, "action=set,bound=lower,inclusive=true"));
    cursor->set_key(cursor, r);
    error_check(cursor->bound(cursor, "action=set,bound=upper,inclusive=true"));

    uint32_t x = 1;
    while ((cursor->next(cursor)) == 0) {
        x ^= 1;
    }
    optimizer_hack += x;
}


template <typename InitFun, typename InsertFun, typename DeleteFun, typename RangeFun, typename SizeFun, typename... Args>
void experiment(InitFun init_f, InsertFun insert_f, DeleteFun delete_f, RangeFun range_f, SizeFun size_f, Args... args) {
    WT_CONNECTION *conn;
    WT_SESSION *session;
    WT_CURSOR *cursor;
    char table_schema[max_schema_len];
    char connection_config[max_conn_config_len];

    uint32_t n_keys = initial_int_keys.size(), n_queries = 0;
    uint64_t current_buffer_pool_size_mb = compute_buffer_pool_size_mb(n_keys, 0);

    sprintf(table_schema, "key_format=%lds,value_format=%lds", key_len, val_len);
    sprintf(connection_config, "create,statistics=(all),direct_io=[data],cache_size=%ldMB", current_buffer_pool_size_mb);

    if (std::filesystem::exists(wt_home))
        std::filesystem::remove_all(wt_home);
    std::filesystem::create_directory(wt_home);
    std::cerr << "[+] Created WiredTiger directory at " << wt_home << std::endl;

    error_check(wiredtiger_open(wt_home, NULL, connection_config, &conn));
    error_check(conn->open_session(conn, NULL, NULL, &session));
    error_check(session->create(session, "table:access", table_schema));
    error_check(session->open_cursor(session, "table:access", NULL, NULL, &cursor));
    std::cerr << "[+] WiredTiger initialized" << std::endl;

    uint8_t key_buf[sizeof(uint64_t) + 1], val_buf[val_len + 1];
    memset(key_buf, 0, sizeof(uint64_t) + 1);
    memset(val_buf, 0, val_len + 1);

    for (uint64_t key : initial_int_keys) {
        const uint64_t key_swapped = __builtin_bswap64(key);
        memcpy(key_buf, &key_swapped, sizeof(key_swapped));
        generate_random_string(val_buf, val_len);
        insert_kv(cursor, reinterpret_cast<const uint8_t *>(key_buf), val_buf);
    }

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

    {
        error_check(conn->close(conn, NULL)); /* Close all handles. */
        current_buffer_pool_size_mb = compute_buffer_pool_size_mb(n_keys, size_f(filter));
        sprintf(connection_config, "statistics=(all),direct_io=[data],cache_size=%ldMB", current_buffer_pool_size_mb);
        error_check(wiredtiger_open(wt_home, NULL, connection_config, &conn));
        error_check(conn->open_session(conn, NULL, NULL, &session));
        error_check(session->open_cursor(session, "table:access", NULL, NULL, &cursor));
        error_check(cursor->reset(cursor));
        std::cerr << "[+] Set WiredTiger's initial buffer pool size to " << current_buffer_pool_size_mb << "MB" << std::endl;
    }


    timer::time_point op_start_time = timer::now();
    while (!wio.Done()) {
        WorkloadIO::opcode opcode = wio.GetOpcode();
        switch (opcode) {
            case WorkloadIO::opcode::Bulk:
                throw std::runtime_error("Cannot bulk load in the middle of a workload");
            case WorkloadIO::opcode::Insert: {
                const uint64_t new_key = wio.ReadValue<uint64_t>();
                insert_f(filter, new_key);
                const uint64_t key_swapped = __builtin_bswap64(new_key);
                memcpy(key_buf, &key_swapped, sizeof(key_swapped));
                generate_random_string(val_buf, val_len);
                insert_kv(cursor, reinterpret_cast<const uint8_t *>(key_buf), val_buf);
                n_keys++;
                break;
            }
            case WorkloadIO::opcode::Delete: {
                const uint64_t new_key = wio.ReadValue<uint64_t>();
                delete_f(filter, new_key);
                const uint64_t key_swapped = __builtin_bswap64(new_key);
                memcpy(key_buf, &key_swapped, sizeof(key_swapped));
                delete_kv(cursor, reinterpret_cast<const uint8_t *>(key_buf));
                n_keys--;
                break;
            }
            case WorkloadIO::opcode::Query: {
                auto [l, r, actual_res] = wio.GetIntQuery();
                bool filter_res = range_f(filter, l, r);
                if (filter_res) {
                    const uint64_t l_swapped = __builtin_bswap64(l);
                    const uint64_t r_swapped = __builtin_bswap64(r);
                    fetch_range_from_db(cursor, reinterpret_cast<const uint8_t *>(&l_swapped),
                                                reinterpret_cast<const uint8_t *>(&r_swapped));
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
                test_out.AddMeasure("buffer_pool_size_mb", current_buffer_pool_size_mb);

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
            case WorkloadIO::opcode::ResetDB: {
                error_check(conn->close(conn, NULL)); /* Close all handles. */
                current_buffer_pool_size_mb = compute_buffer_pool_size_mb(n_keys, size_f(filter));
                sprintf(connection_config, "statistics=(all),direct_io=[data],cache_size=%ldMB", current_buffer_pool_size_mb);
                error_check(wiredtiger_open(wt_home, NULL, connection_config, &conn));
                error_check(conn->open_session(conn, NULL, NULL, &session));
                error_check(session->open_cursor(session, "table:access", NULL, NULL, &cursor));
                error_check(cursor->reset(cursor));
                std::cerr << "[+] Set WiredTiger's buffer pool size to " << current_buffer_pool_size_mb << "MB" << std::endl;
            }
        }
    }

    error_check(conn->close(conn, NULL)); /* Close all handles. */
    std::cerr << "[+] optimizer_hack=" << optimizer_hack << std::endl;
}


template <typename InitFun, typename InsertFun, typename DeleteFun, typename RangeFun, typename SizeFun, typename... Args>
void experiment_string(InitFun init_f, InsertFun insert_f, DeleteFun delete_f, RangeFun range_f, SizeFun size_f, Args... args) {
    WT_CONNECTION *conn;
    WT_SESSION *session;
    WT_CURSOR *cursor;
    char table_schema[max_schema_len];
    char connection_config[max_conn_config_len];

    uint32_t n_keys = initial_string_keys.size(), n_queries = 0;
    if (n_keys == 0) {
        n_keys = initial_int_keys.size();
        initial_string_keys = std::vector<std::string>(n_keys);
        std::transform(initial_int_keys.begin(), initial_int_keys.end(), initial_string_keys.begin(), [&](uint64_t k) { return uint64ToString(k); });
    }
    uint64_t current_buffer_pool_size_mb = compute_buffer_pool_size_mb(n_keys, 0);

    sprintf(table_schema, "key_format=%lds,value_format=%lds", key_len, val_len);
    sprintf(connection_config, "create,statistics=(all),direct_io=[data],cache_size=%ldMB", current_buffer_pool_size_mb);

    if (std::filesystem::exists(wt_home))
        std::filesystem::remove_all(wt_home);
    std::filesystem::create_directory(wt_home);
    std::cerr << "[+] Created WiredTiger directory at " << wt_home << std::endl;

    error_check(wiredtiger_open(wt_home, NULL, connection_config, &conn));
    error_check(conn->open_session(conn, NULL, NULL, &session));
    error_check(session->create(session, "table:access", table_schema));
    error_check(session->open_cursor(session, "table:access", NULL, NULL, &cursor));
    std::cerr << "[+] WiredTiger initialized" << std::endl;

    uint16_t l_buf_len, r_buf_len;
    uint8_t l_buf[std::numeric_limits<uint16_t>::max()];
    uint8_t r_buf[std::numeric_limits<uint16_t>::max()];
    uint8_t val_buf[val_len];
    
    for (std::string key : initial_string_keys) {
        generate_random_string(val_buf, val_len);
        insert_kv(cursor, reinterpret_cast<const uint8_t *>(key.c_str()), val_buf);
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
                if (wio.StringKeys())
                    wio.GetStringKey(l_buf_len, l_buf);
                else {
                    const uint64_t key = __builtin_bswap64(wio.ReadValue<uint64_t>());
                    mempcpy(l_buf, reinterpret_cast<const uint8_t*>(&key), sizeof(key));
                    l_buf_len = sizeof(key);
                    l_buf[l_buf_len] = '\0';
                }
                insert_f(filter, l_buf, l_buf_len);
                generate_random_string(val_buf, val_len);
                insert_kv(cursor, l_buf, val_buf);
                n_keys++;
                break;
            }
            case WorkloadIO::opcode::Delete: {
                if (wio.StringKeys())
                    wio.GetStringKey(l_buf_len, l_buf);
                else {
                    const uint64_t key = __builtin_bswap64(wio.ReadValue<uint64_t>());
                    mempcpy(l_buf, reinterpret_cast<const uint8_t*>(&key), sizeof(key));
                    l_buf_len = sizeof(key);
                    l_buf[l_buf_len] = '\0';
                }
                delete_f(filter, l_buf, l_buf_len);
                delete_kv(cursor, l_buf);
                n_keys--;
                break;
            }
            case WorkloadIO::opcode::Query: {
                bool actual_res, filter_res;
                if (wio.StringKeys())
                    wio.GetStringQuery(l_buf_len, l_buf, r_buf_len, r_buf, actual_res);
                else {
                    auto [l, r, res] = wio.GetIntQuery();
                    l = __builtin_bswap64(l);
                    r = __builtin_bswap64(r);
                    actual_res = res;

                    mempcpy(l_buf, reinterpret_cast<const uint8_t*>(&l), sizeof(l));
                    l_buf_len = sizeof(l);
                    l_buf[l_buf_len] = '\0';
                    mempcpy(r_buf, reinterpret_cast<const uint8_t*>(&r), sizeof(r));
                    r_buf_len = sizeof(r);
                    r_buf[r_buf_len] = '\0';
                }

                filter_res = range_f(filter, l_buf, l_buf_len, r_buf, r_buf_len);
                if (filter_res)
                    fetch_range_from_db(cursor, l_buf, r_buf);
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
            case WorkloadIO::opcode::ResetDB: {
                error_check(conn->close(conn, NULL)); /* Close all handles. */
                current_buffer_pool_size_mb = compute_buffer_pool_size_mb(n_keys, size_f(filter));
                sprintf(connection_config, "statistics=(all),direct_io=[data],cache_size=%ldMB", current_buffer_pool_size_mb);
                error_check(wiredtiger_open(wt_home, NULL, connection_config, &conn));
                error_check(conn->open_session(conn, NULL, NULL, &session));
                error_check(session->open_cursor(session, "table:access", NULL, NULL, &cursor));
                error_check(cursor->reset(cursor));
                std::cerr << "[+] Set WiredTiger's buffer pool size to " << current_buffer_pool_size_mb << "MB" << std::endl;
            }
        }
    }

    error_check(conn->close(conn, NULL)); /* Close all handles. */
    std::cerr << "[+] optimizer_hack=" << optimizer_hack << std::endl;
}


inline argparse::ArgumentParser init_parser(const std::string& name) {
    argparse::ArgumentParser parser(name);

    parser.add_argument("arg")
            .help("The main parameters of the filter (e.g., the memory footprint in bits-per-key)")
            .nargs(argparse::nargs_pattern::at_least_one)
            .scan<'g', double>();

    parser.add_argument("-m", "--memory_to_disk_ratio")
        .help("The ratio expressing how much memory should be allocated to the buffer pool and "
              "the filter compared to the data size on disk")
        .nargs(1)
        .scan<'g', double>()
        .required()
        .default_value(default_memory_to_disk_ratio);

    parser.add_argument("-w", "--workload")
            .help("Pass the workload from file")
            .nargs(1);

    parser.add_argument("--val_len")
        .help("length of WiredTiger's values, in bytes")
        .nargs(1)
        .scan<'i', int>()
        .required()
        .default_value(default_val_len);

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
