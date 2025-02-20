/*
 * This file is part of Grafite <https://github.com/marcocosta97/grafite>.
 * Copyright (C) 2023 Marco Costa.
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

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <functional>
#include <iostream>
#include <iterator>
#include <limits>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <system_error>
#include <unordered_map>
#include <vector>

#include "bench_utils.hpp"
#include <argparse/argparse.hpp>
#include <x86intrin.h>

static const std::vector<std::string> kdist_names = {"unif", "norm", "real"};
static const std::vector<std::string> kdist_default = {"unif"};
static const std::vector<std::string> qdist_names = {"unif", "norm", "real", "corr", "true"};
static const std::vector<std::string> qdist_default = {"unif"};

uint64_t default_n_keys = 200'000'000;
uint64_t default_n_deletes = 10'000'000;
uint64_t default_n_queries = 10'000'000;
std::vector<uint64_t> default_string_lens {8, 16, 32, 64, 128, 256};
uint64_t range_size_min = 1, range_size_max = std::numeric_limits<uint64_t>::max();
std::uniform_int_distribution<uint64_t> query_size_dist;

InputKeys<uint64_t> keys_from_file = InputKeys<uint64_t>();

const char pbstr[] = "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||";
const size_t pbwidth = 60;
uint32_t seed = 2024;


void print_progress(double percentage) {
    static int last_percentage = 0;
    int val = (int) (percentage * 100);
    if (last_percentage == val)
        return;

    int lpad = static_cast<int>(percentage * pbwidth);
    int rpad = pbwidth - lpad;
    last_percentage = val;
    printf("\r%3d%% [%.*s%*s]", val, lpad, pbstr, rpad, "");
    fflush(stdout);
}

bool create_dir_recursive(const std::string_view& dir_name) {
    std::error_code err;
    if (!std::filesystem::create_directories(dir_name, err)) {
        if (std::filesystem::exists(dir_name))
            return true; // the folder probably already existed
        std::cerr << "Failed to create [" << dir_name << "]" << std::endl;
        return false;
    }
    return true;
}

std::set<uint64_t> generate_int_keys_uniform(uint64_t n_keys, std::mt19937_64& rng) {
    std::set<uint64_t> keys;
    std::uniform_int_distribution<uint64_t> dist(0, std::numeric_limits<uint64_t>::max());
    std::cout << "Generating keys..." << std::endl;
    while (keys.size() < n_keys) {
        keys.insert(dist(rng));
        print_progress(1.0 * keys.size() / n_keys);
    }
    std::cout << std::endl;
    return keys;
}

std::set<uint64_t> generate_int_keys_normal(uint64_t n_keys, long double mu, long double std, std::mt19937_64& rng) {
    std::set<uint64_t> keys;
    std::normal_distribution<long double> dist(mu, std);
    std::cout << "Generating keys..." << std::endl;;
    while (keys.size() < n_keys) {
        keys.insert(static_cast<uint64_t>(dist(rng)));
        print_progress(1.0 * keys.size() / n_keys);
    }
    std::cout << std::endl;
    return keys;
}

ByteString generate_single_string_key_uniform(std::mt19937_64& rng,
                                              std::vector<uint64_t>& key_lens,
                                              std::uniform_int_distribution<uint64_t>& len_dist,
                                              std::uniform_int_distribution<uint64_t>& dist) {
    const uint32_t len = key_lens[len_dist(rng)];
    const uint32_t word_len = ((len + 7) / 8) * 8;
    uint64_t buf[word_len];
    for (int32_t i = 0; i < word_len; i++)
        buf[i] = dist(rng);
    return {reinterpret_cast<const uint8_t *>(buf), len};
}

std::set<ByteString> generate_string_keys_uniform(uint64_t n_keys, std::vector<uint64_t> key_lens, std::mt19937_64& rng) {
    std::set<ByteString> keys;
    std::uniform_int_distribution<uint64_t> len_dist(0, key_lens.size() - 1);
    std::uniform_int_distribution<uint64_t> dist(0, std::numeric_limits<uint64_t>::max());
    std::cout << "Generating keys..." << std::endl;
    while (keys.size() < n_keys) {
        keys.insert(generate_single_string_key_uniform(rng, key_lens, len_dist, dist));
        print_progress(1.0 * keys.size() / n_keys);
    }
    std::cout << std::endl;
    return keys;
}

ByteString global_norm_base_key {nullptr, 0};

ByteString generate_single_string_key_normal(std::mt19937_64& rng,
                                             std::vector<uint64_t>& key_lens,
                                             uint32_t norm_byte,
                                             std::uniform_int_distribution<uint64_t>& len_dist,
                                             std::normal_distribution<long double>& dist) {
    const uint32_t len = key_lens[len_dist(rng)];
    uint8_t buf[len + 8];
    memcpy(buf, global_norm_base_key.str, len);
    const uint64_t pert = __builtin_bswap64(static_cast<uint64_t>(dist(rng)));
    uint64_t payload;
    memcpy(&payload, buf + norm_byte, sizeof(payload));
    payload += pert;
    memcpy(buf + norm_byte, &payload, sizeof(payload));
    return {reinterpret_cast<const uint8_t *>(buf), len};
}

std::set<ByteString> generate_string_keys_normal(uint64_t n_keys, std::vector<uint64_t> key_lens, 
                                                     long double mu, long double std, uint32_t norm_byte,
                                                     std::mt19937_64& rng) {
    const uint32_t max_len = *std::max_element(key_lens.begin(), key_lens.end());
    const uint32_t max_word_len = ((max_len + 7) / 8) * 8;
    uint64_t base_key[max_word_len];
    for (int32_t i = 0; i < max_word_len; i++)
        base_key[i] = rng();
    global_norm_base_key = ByteString(reinterpret_cast<const uint8_t *>(base_key),
                                      max_word_len * sizeof(uint64_t));

    std::set<ByteString> keys;
    std::uniform_int_distribution<uint64_t> len_dist(0, key_lens.size() - 1);
    std::normal_distribution<long double> dist(mu, std);
    std::cout << "Generating keys..." << std::endl;
    while (keys.size() < n_keys) {
        keys.insert(generate_single_string_key_normal(rng, key_lens, norm_byte, len_dist, dist));
        print_progress(1.0 * keys.size() / n_keys);
    }
    std::cout << std::endl;
    return keys;
}


std::tuple<std::string, long double, long double, std::string> get_kdist(argparse::ArgumentParser& parser, uint32_t& pos) {
    std::string dist_name = parser.get<std::vector<std::string>>("--kdist")[pos++];
    std::string key_file;
    long double mu = 0.0, sigma = 0.0;
    if (std::find(kdist_names.begin(), kdist_names.end(), dist_name) == kdist_names.end()) {
        std::string msg = "Invalid key distribution name: ";
        msg += dist_name;
        throw std::runtime_error(msg);
    }

    if (dist_name == "norm") {
        mu = std::stod(parser.get<std::vector<std::string>>("--kdist")[pos++]);
        sigma = std::stod(parser.get<std::vector<std::string>>("--kdist")[pos++]);
    }
    else if (dist_name == "real")
        key_file = parser.get<std::vector<std::string>>("--kdist")[pos++];
    return {dist_name, mu, sigma, key_file};
}

std::tuple<std::string, long double, long double> get_qdist(argparse::ArgumentParser& parser, uint32_t& pos) {
    std::string dist_name = parser.get<std::vector<std::string>>("--qdist")[pos++];
    long double mu = 0.0, sigma = 0.0;
    if (std::find(qdist_names.begin(), qdist_names.end(), dist_name) == qdist_names.end()) {
        std::string msg = "Invalid query distribution name: ";
        msg += dist_name;
        throw std::runtime_error(msg);
    }

    if (dist_name == "norm") {
        mu = std::stod(parser.get<std::vector<std::string>>("--qdist")[pos++]);
        sigma = std::stod(parser.get<std::vector<std::string>>("--qdist")[pos++]);
    }
    return {dist_name, mu, sigma};
}


 ///////////////////////////////////////////////////////////////////////////// 
///////////////////////////////////////////////////////////////////////////////
////                        Benchmark Functions                            ////
///////////////////////////////////////////////////////////////////////////////
 ///////////////////////////////////////////////////////////////////////////// 


void correlated_bench(argparse::ArgumentParser& parser) {
    WorkloadIO wio(parser.get<std::string>("--output-file"), WorkloadIO::iomode::Write, false);
    uint32_t kdist_ind = 0;
    auto [key_dist, key_dist_mu, key_dist_std, key_file] = get_kdist(parser, kdist_ind);

    const uint32_t n_keys = parser.get<uint64_t>("--n-keys");
    const uint64_t seed = parser.get<uint64_t>("--seed");
    std::mt19937_64 rng(seed);

    std::set<uint64_t> keys;
    if (key_dist == "unif")
        keys = generate_int_keys_uniform(n_keys, rng);
    else if (key_dist == "norm")
        keys = generate_int_keys_normal(n_keys, key_dist_mu, key_dist_std, rng);
    else
        keys = read_data_binary<uint64_t>(key_file);
    std::vector<uint64_t> keys_vec {keys.begin(), keys.end()};
    wio.Bulk(keys_vec);

    wio.Timer('q');
    std::cout << "Generating queries..." << std::endl;
    const uint32_t n_queries = parser.get<uint64_t>("--n-queries");
    const double corr_deg = parser.get<double>("--corr-degree");
    const uint64_t corr_distance = static_cast<uint64_t>(std::min(std::pow(2.0, 64 * (1 - corr_deg)),
                                                                  std::pow(2.0, 64)));
    std::uniform_int_distribution<uint64_t> picker(0, n_keys - 1);
    std::uniform_int_distribution<uint64_t> corr_distance_dist(1, corr_distance);
    for (uint32_t i = 0; i < n_queries;) {
        const uint32_t picked = picker(rng);
        const uint64_t query_size = query_size_dist(rng);
        const uint64_t distance = corr_distance_dist(rng);
        if (keys_vec[picked] < distance + query_size - 1)
            continue;
        uint64_t r_key = keys_vec[picked] - distance;
        uint64_t l_key = r_key - query_size + 1;
        if (vector_range_query(keys_vec, l_key, r_key))
            continue;
        wio.Query(l_key, r_key, false);
        i++;
        print_progress(1.0 * i / n_queries);
    }
    wio.Timer('q');
    wio.Flush();
}


void standard_int_bench(argparse::ArgumentParser& parser) {
    WorkloadIO wio(parser.get<std::string>("--output-file"), WorkloadIO::iomode::Write, false);
    uint32_t kdist_ind = 0, qdist_ind = 0;
    auto [key_dist, key_dist_mu, key_dist_std, key_file] = get_kdist(parser, kdist_ind);
    auto [query_dist, query_dist_mu, query_dist_std] = get_qdist(parser, qdist_ind);

    const uint32_t n_keys = parser.get<uint64_t>("--n-keys");
    const uint64_t seed = parser.get<uint64_t>("--seed");
    std::mt19937_64 rng(seed);

    std::set<uint64_t> keys;
    if (key_dist == "unif")
        keys = generate_int_keys_uniform(n_keys, rng);
    else if (key_dist == "norm")
        keys = generate_int_keys_normal(n_keys, key_dist_mu, key_dist_std, rng);
    else
        keys = read_data_binary<uint64_t>(key_file);
    std::vector<uint64_t> keys_vec {keys.begin(), keys.end()};
    std::shuffle(keys_vec.begin(), keys_vec.end(), rng);
    keys_vec.resize(n_keys);
    std::sort(keys_vec.begin(), keys_vec.end());
    keys = std::set<uint64_t>(keys_vec.begin(), keys_vec.end());

    const uint32_t n_queries = parser.get<uint64_t>("--n-queries");
    std::cout << "Generating queries..." << std::endl;
    if (query_dist == "unif") {
        wio.Bulk(keys_vec);
        wio.Timer('q');
        std::uniform_int_distribution<uint64_t> dist(0, std::numeric_limits<uint64_t>::max());
        for (uint32_t i = 0; i < n_queries;) {
            const uint64_t query_size = query_size_dist(rng);
            uint64_t l_key = dist(rng);
            if (std::numeric_limits<uint64_t>::max() - l_key < query_size)
                continue;
            uint64_t r_key = l_key + query_size - 1;
            if (vector_range_query(keys_vec, l_key, r_key))
                continue;
            wio.Query(l_key, r_key, false);
            i++;
            print_progress(1.0 * i / n_queries);
        }
        wio.Timer('q');
    }
    else if (query_dist == "norm") {
        wio.Bulk(keys_vec);
        wio.Timer('q');
        std::normal_distribution<long double> dist(query_dist_mu, query_dist_std);
        for (uint32_t i = 0; i < n_queries;) {
            const uint64_t query_size = query_size_dist(rng);
            uint64_t l_key = static_cast<uint64_t>(dist(rng));
            if (std::numeric_limits<uint64_t>::max() - l_key < query_size)
                continue;
            uint64_t r_key = l_key + query_size - 1;
            if (vector_range_query(keys_vec, l_key, r_key))
                continue;
            wio.Query(l_key, r_key, false);
            i++;
            print_progress(1.0 * i / n_queries);
        }
        wio.Timer('q');
    }
    else if (query_dist == "real") {
        std::shuffle(keys_vec.begin(), keys_vec.end(), rng);
        std::vector<std::tuple<uint64_t, uint64_t>> queries;
        for (uint32_t i = 0; queries.size() < n_queries; i = (i + 1) % keys_vec.size()) {
            if (keys.find(keys_vec[i]) == keys.end())
                continue;
            const uint64_t query_size = query_size_dist(rng);
            const uint64_t l_key = keys_vec[i];
            if (std::numeric_limits<uint64_t>::max() - l_key < query_size)
                continue;
            const uint64_t r_key = l_key + query_size - 1;
            auto it = keys.upper_bound(l_key);
            if (*it <= r_key)
                continue;
            keys.erase(l_key);
            queries.emplace_back(l_key, r_key);
            print_progress(1.0 * queries.size() / n_queries);
        }
        std::vector<uint64_t> keys_vec {keys.begin(), keys.end()};
        wio.Bulk(keys_vec);
        wio.Timer('q');
        std::shuffle(queries.begin(), queries.end(), rng);
        for (auto [l, r] : queries)
            wio.Query(l, r, false);
        wio.Timer('q');
    }
    else 
        throw std::runtime_error("Invalid query distribution type for this benchmark");
    wio.Flush();
}


void standard_string_bench(argparse::ArgumentParser& parser) {
    WorkloadIO wio(parser.get<std::string>("--output-file"), WorkloadIO::iomode::Write, true);
    uint32_t kdist_ind = 0, qdist_ind = 0;
    auto [key_dist, key_dist_mu, key_dist_std, key_file] = get_kdist(parser, kdist_ind);
    uint32_t key_norm_byte = 0;
    if (key_dist == "norm")
        key_norm_byte = std::stoi(parser.get<std::vector<std::string>>("--kdist")[kdist_ind++]);

    const uint32_t n_keys = parser.get<uint64_t>("--n-keys");
    const uint64_t seed = parser.get<uint64_t>("--seed");
    std::mt19937_64 rng(seed);

    std::vector<uint64_t> key_lens = parser.get<std::vector<uint64_t>>("--string-lens");
    std::set<ByteString> keys;
    if (key_dist == "unif")
        keys = generate_string_keys_uniform(n_keys, key_lens, rng);
    else if (key_dist == "norm")
        keys = generate_string_keys_normal(n_keys, key_lens, key_dist_mu, key_dist_std,
                                           key_norm_byte, rng);
    else
        keys = read_data_binary<ByteString>(key_file);
    std::vector<ByteString> keys_vec {keys.begin(), keys.end()};

    const uint32_t n_queries = parser.get<uint64_t>("--n-queries");
    wio.Bulk(keys_vec);
    wio.Timer('q');
    std::cout << "Generating queries..." << std::endl;
    std::uniform_int_distribution<uint32_t> picker(0, n_keys - 2);
    std::uniform_int_distribution<uint64_t> length_picker(0, key_lens.size() - 1);
    std::uniform_int_distribution<uint8_t> byte_dist(0, std::numeric_limits<uint8_t>::max());
    std::uniform_int_distribution<uint64_t> unif_dist(0, std::numeric_limits<uint64_t>::max());
    std::normal_distribution<long double> norm_dist(key_dist_mu, key_dist_std);
    for (uint32_t i = 0; i < n_queries;) {
        uint32_t picked;
        if (key_dist == "unif") {
            ByteString sample = generate_single_string_key_uniform(rng, key_lens, length_picker, unif_dist);
            picked = std::upper_bound(keys_vec.begin(), keys_vec.end(), sample) - keys_vec.begin() - 1;
            picked = std::min(picked, n_keys - 2);
        }
        else if (key_dist == "norm") {
            ByteString sample = generate_single_string_key_normal(rng, key_lens, key_norm_byte, length_picker, norm_dist);
            picked = std::upper_bound(keys_vec.begin(), keys_vec.end(), sample) - keys_vec.begin() - 1;
            picked = std::min(picked, n_keys - 2);
        }
        else 
            picked = picker(rng);
        const uint32_t lcp = calculate_lcp(keys_vec[picked].str, keys_vec[picked].length,
                                           keys_vec[picked + 1].str, keys_vec[picked + 1].length);
        const uint8_t left_diff = lcp < keys_vec[picked].length ? keys_vec[picked].str[lcp] : 0x00;
        const uint8_t right_diff = lcp < keys_vec[picked + 1].length ? keys_vec[picked + 1].str[lcp] : 0xFF;
        const uint32_t left_len = std::max<uint32_t>(lcp + 1 + sizeof(uint64_t), key_lens[length_picker(rng)]);
        const uint32_t right_len = std::max<uint32_t>(lcp + 1 + sizeof(uint64_t), key_lens[length_picker(rng)]);
        uint8_t l_key[left_len], r_key[right_len];
        memcpy(l_key, keys_vec[picked].str, lcp);
        memcpy(r_key, keys_vec[picked + 1].str, lcp);
        std::uniform_int_distribution<uint8_t> diff_dist(left_diff, right_diff);
        l_key[lcp] = diff_dist(rng);
        r_key[lcp] = diff_dist(rng);
        for (uint32_t i = lcp + 1; i < left_len; i++)
            l_key[i] = byte_dist(rng);
        for (uint32_t i = lcp + 1; i < right_len; i++)
            r_key[i] = byte_dist(rng);

        ByteString l(l_key, left_len), r(r_key, right_len);
        if (l > r)
            std::swap(l, r);
        if (set_range_query(keys, l, r))
            continue;
        wio.Query(l, r, false);
        i++;
        print_progress(1.0 * i / n_queries);
    }
    wio.Timer('q');
    wio.Flush();
}


void true_bench(argparse::ArgumentParser& parser) {
    WorkloadIO wio(parser.get<std::string>("--output-file"), WorkloadIO::iomode::Write, false);
    uint32_t kdist_ind = 0, qdist_ind = 0;
    auto [key_dist, key_dist_mu, key_dist_std, key_file] = get_kdist(parser, kdist_ind);

    const uint32_t n_keys = parser.get<uint64_t>("--n-keys");
    const uint64_t seed = parser.get<uint64_t>("--seed");
    std::mt19937_64 rng(seed);

    std::set<uint64_t> keys;
    if (key_dist == "unif")
        keys = generate_int_keys_uniform(n_keys, rng);
    else if (key_dist == "norm")
        keys = generate_int_keys_normal(n_keys, key_dist_mu, key_dist_std, rng);
    else
        keys = read_data_binary<uint64_t>(key_file);
    std::vector<uint64_t> keys_vec {keys.begin(), keys.end()};
    wio.Bulk(keys_vec);

    wio.Timer('q');
    std::cout << "Generating queries..." << std::endl;
    const uint32_t n_queries = parser.get<uint64_t>("--n-queries");
    std::uniform_int_distribution<uint64_t> picker(0, n_keys - 1);
    for (uint32_t i = 0; i < n_queries;) {
        const uint32_t picked = picker(rng);
        const uint64_t query_size = query_size_dist(rng);
        const uint64_t offset = rng() % query_size;
        if (offset > keys_vec[picked])
            continue;
        const uint64_t l_key = keys_vec[picked] - offset;
        if (std::numeric_limits<uint64_t>::max() - l_key + 1 < query_size)
            continue;
        const uint64_t r_key = l_key + query_size - 1;
        wio.Query(l_key, r_key, true);
        i++;
        print_progress(1.0 * i / n_queries);
    }
    wio.Timer('q');
    wio.Flush();
}


void expansion_bench(argparse::ArgumentParser& parser) {
    WorkloadIO wio(parser.get<std::string>("--output-file"), WorkloadIO::iomode::Write, false);
    uint32_t kdist_ind = 0, qdist_ind = 0;
    auto [key_dist, key_dist_mu, key_dist_std, key_file] = get_kdist(parser, kdist_ind);
    auto [query_dist, query_dist_mu, query_dist_std] = get_qdist(parser, qdist_ind);

    const uint32_t n_keys = parser.get<uint64_t>("--n-keys");
    const uint32_t n_queries = parser.get<uint64_t>("--n-queries");
    const uint64_t seed = parser.get<uint64_t>("--seed");
    std::mt19937_64 rng(seed);

    std::set<uint64_t> keys;
    if (key_dist == "unif")
        keys = generate_int_keys_uniform(n_keys, rng);
    else if (key_dist == "norm")
        keys = generate_int_keys_normal(n_keys, key_dist_mu, key_dist_std, rng);
    else
        keys = read_data_binary<uint64_t>(key_file);
    std::vector<uint64_t> keys_vec {keys.begin(), keys.end()};

    const uint32_t n_expansions = parser.get<uint64_t>("--n-expansions");
    std::shuffle(keys_vec.begin(), keys_vec.end(), rng);
    uint32_t cur_n_keys = n_keys >> n_expansions;
    std::sort(keys_vec.begin(), keys_vec.begin() + cur_n_keys);
    keys = std::set<uint64_t>(keys_vec.begin(), keys_vec.begin() + cur_n_keys);

    {
        std::vector<uint64_t> init_keys_vec = {keys_vec.begin(), keys_vec.begin() + cur_n_keys};

        wio.Bulk(init_keys_vec);

        wio.Timer('q');
        std::cout << "Generating queries..." << std::endl;
        if (query_dist == "unif") {
            std::uniform_int_distribution<uint64_t> dist(0, std::numeric_limits<uint64_t>::max());
            for (uint32_t i = 0; i < n_queries;) {
                const uint64_t query_size = query_size_dist(rng);
                uint64_t l_key = dist(rng);
                if (std::numeric_limits<uint64_t>::max() - l_key < query_size)
                    continue;
                uint64_t r_key = l_key + query_size - 1;
                if (set_range_query(keys, l_key, r_key))
                    continue;
                wio.Query(l_key, r_key, false);
                i++;
                print_progress(1.0 * i / n_queries);
            }
        }
        else if (query_dist == "norm") {
            std::normal_distribution<long double> dist(query_dist_mu, query_dist_std);
            for (uint32_t i = 0; i < n_queries;) {
                const uint64_t query_size = query_size_dist(rng);
                uint64_t l_key = static_cast<uint64_t>(dist(rng));
                if (std::numeric_limits<uint64_t>::max() - l_key < query_size)
                    continue;
                uint64_t r_key = l_key + query_size - 1;
                if (set_range_query(keys, l_key, r_key))
                    continue;
                wio.Query(l_key, r_key, false);
                i++;
                print_progress(1.0 * i / n_queries);
            }
        }
        else if (query_dist == "corr") {
            const double corr_deg = parser.get<double>("--corr-degree");
            const uint64_t corr_distance = static_cast<uint64_t>(std::min(std::pow(2.0, 64 * (1 - corr_deg)),
                                                                 std::pow(2.0, 64)));
            std::uniform_int_distribution<uint64_t> picker(0, n_keys);
            std::uniform_int_distribution<uint64_t> corr_distance_dist(1, corr_distance);
            for (uint32_t i = 0; i < n_queries;) {
                const uint32_t picked = picker(rng);
                const uint64_t query_size = query_size_dist(rng);
                const uint64_t distance = corr_distance_dist(rng);
                if (keys_vec[picked] < distance + query_size - 1)
                    continue;
                uint64_t r_key = keys_vec[picked] - distance;
                uint64_t l_key = r_key - query_size + 1;
                if (set_range_query(keys, l_key, r_key))
                    continue;
                wio.Query(l_key, r_key, false);
                i++;
                print_progress(1.0 * i / n_queries);
            }
        }
        else 
            throw std::runtime_error("Invalid query distribution type for this benchmark");
        wio.Timer('q');
        wio.Flush();
    }

    for (int32_t expansion = 0; expansion < n_expansions * 4; expansion++) {
        const uint32_t n_inserts = cur_n_keys / 4;
        const uint32_t pos = cur_n_keys + (expansion % 4) * cur_n_keys / 4;

        wio.Timer('i');
        for (uint32_t i = pos; i < pos + n_inserts; i++) {
            wio.Insert(keys_vec[i]);
            keys.insert(keys_vec[i]);
        }
        wio.Timer('i');

        wio.Timer('q');
        std::cout << "Generating queries..." << std::endl;
        if (query_dist == "unif") {
            std::uniform_int_distribution<uint64_t> dist(0, std::numeric_limits<uint64_t>::max());
            for (uint32_t i = 0; i < n_queries;) {
                const uint64_t query_size = query_size_dist(rng);
                uint64_t l_key = dist(rng);
                if (std::numeric_limits<uint64_t>::max() - l_key < query_size)
                    continue;
                uint64_t r_key = l_key + query_size - 1;
                if (set_range_query(keys, l_key, r_key))
                    continue;
                wio.Query(l_key, r_key, false);
                i++;
                print_progress(1.0 * i / n_queries);
            }
        }
        else if (query_dist == "norm") {
            std::normal_distribution<long double> dist(query_dist_mu, query_dist_std);
            for (uint32_t i = 0; i < n_queries;) {
                const uint64_t query_size = query_size_dist(rng);
                uint64_t l_key = static_cast<uint64_t>(dist(rng));
                if (std::numeric_limits<uint64_t>::max() - l_key < query_size)
                    continue;
                uint64_t r_key = l_key + query_size - 1;
                if (set_range_query(keys, l_key, r_key))
                    continue;
                wio.Query(l_key, r_key, false);
                i++;
                print_progress(1.0 * i / n_queries);
            }
        }
        else if (query_dist == "corr") {
            const double corr_deg = parser.get<double>("--corr-degree");
            const uint64_t corr_distance = static_cast<uint64_t>(std::min(std::pow(2.0, 64 * (1 - corr_deg)),
                                                                 std::pow(2.0, 64)));
            std::uniform_int_distribution<uint64_t> picker(0, pos + n_inserts - 1);
            std::uniform_int_distribution<uint64_t> corr_distance_dist(1, corr_distance);
            for (uint32_t i = 0; i < n_queries;) {
                const uint32_t picked = picker(rng);
                const uint64_t query_size = query_size_dist(rng);
                const uint64_t distance = corr_distance_dist(rng);
                if (keys_vec[picked] < distance + query_size - 1)
                    continue;
                uint64_t r_key = keys_vec[picked] - distance;
                uint64_t l_key = r_key - query_size + 1;
                if (set_range_query(keys, l_key, r_key))
                    continue;
                wio.Query(l_key, r_key, false);
                i++;
                print_progress(1.0 * i / n_queries);
            }
        }
        else 
            throw std::runtime_error("Invalid query distribution type for this benchmark");
        wio.Timer('q');
        wio.Flush();
        std::cout << std::endl;

        if (expansion % 4 == 3)
            cur_n_keys *= 2;
    }
    wio.Flush();
}


void delete_bench(argparse::ArgumentParser& parser) {
    WorkloadIO wio(parser.get<std::string>("--output-file"), WorkloadIO::iomode::Write, false);
    uint32_t kdist_ind = 0, qdist_ind = 0;
    auto [key_dist, key_dist_mu, key_dist_std, key_file] = get_kdist(parser, kdist_ind);
    auto [query_dist, query_dist_mu, query_dist_std] = get_qdist(parser, qdist_ind);

    const uint32_t n_keys = parser.get<uint64_t>("--n-keys");
    const uint32_t n_queries = parser.get<uint64_t>("--n-queries");
    const uint64_t seed = parser.get<uint64_t>("--seed");
    std::mt19937_64 rng(seed);

    std::set<uint64_t> keys;
    if (key_dist == "unif")
        keys = generate_int_keys_uniform(n_keys, rng);
    else if (key_dist == "norm")
        keys = generate_int_keys_normal(n_keys, key_dist_mu, key_dist_std, rng);
    else
        keys = read_data_binary<uint64_t>(key_file);
    std::vector<uint64_t> keys_vec {keys.begin(), keys.end()};
    uint32_t perm[keys_vec.size()];
    for (uint32_t i = 0; i < keys_vec.size(); i++)
        perm[i] = i;

    const uint32_t n_deletes = parser.get<uint64_t>("--n-deletes");
    const uint32_t n_expansions = parser.get<uint64_t>("--n-expansions");
    std::shuffle(keys_vec.begin(), keys_vec.end(), rng);
    uint32_t cur_n_keys = n_keys >> n_expansions;
    std::sort(keys_vec.begin(), keys_vec.begin() + cur_n_keys);

    {
        std::vector<uint64_t> init_keys_vec = {keys_vec.begin(), keys_vec.begin() + cur_n_keys};

        wio.Bulk(init_keys_vec);

        wio.Timer('d');
        std::cout << "Generating deletes..." << std::endl;
        std::shuffle(perm, perm + cur_n_keys, rng);
        for (uint32_t i = 0; i < n_deletes; i++) {
            wio.Delete(keys_vec[perm[i]]);
            print_progress(1.0 * i / n_deletes);
        }
        wio.Timer('d');
        wio.Flush();
        std::cout << std::endl;

        std::cout << "Reinserting deleted keys..." << std::endl;
        for (uint32_t i = 0; i < n_deletes; i++) {
            wio.Insert(keys_vec[perm[i]]);
            print_progress(1.0 * i / n_deletes);
        }
        std::cout << std::endl;
    }

    for (int32_t expansion = 0; expansion < n_expansions; expansion++) {
        const uint32_t n_inserts = cur_n_keys;

        std::cout << "Inserting new keys..." << std::endl;
        for (uint32_t i = cur_n_keys; i < 2 * cur_n_keys; i++) {
            wio.Insert(keys_vec[i]);
            print_progress(1.0 * (i - cur_n_keys) / cur_n_keys);
        }
        std::cout << std::endl;

        wio.Timer('d');
        std::cout << "Generating deletes..." << std::endl;
        std::shuffle(perm, perm + cur_n_keys, rng);
        for (uint32_t i = 0; i < n_deletes; i++) {
            wio.Delete(keys_vec[perm[i]]);
            print_progress(1.0 * i / n_deletes);
        }
        wio.Timer('d');
        wio.Flush();
        std::cout << std::endl;

        std::cout << "Reinserting deleted keys..." << std::endl;
        for (uint32_t i = 0; i < n_deletes; i++) {
            wio.Insert(keys_vec[perm[i]]);
            print_progress(1.0 * i / n_deletes);
        }
        std::cout << std::endl;

        cur_n_keys *= 2;
    }
    wio.Flush();
}


void construction_bench(argparse::ArgumentParser& parser) {
    WorkloadIO wio(parser.get<std::string>("--output-file"), WorkloadIO::iomode::Write, false);
    uint32_t kdist_ind = 0;
    auto [key_dist, key_dist_mu, key_dist_std, key_file] = get_kdist(parser, kdist_ind);

    const uint32_t n_keys = parser.get<uint64_t>("--n-keys");
    const uint64_t seed = parser.get<uint64_t>("--seed");
    std::mt19937_64 rng(seed);

    std::set<uint64_t> keys;
    if (key_dist == "unif")
        keys = generate_int_keys_uniform(n_keys, rng);
    else if (key_dist == "norm")
        keys = generate_int_keys_normal(n_keys, key_dist_mu, key_dist_std, rng);
    else
        keys = read_data_binary<uint64_t>(key_file);
    std::vector<uint64_t> keys_vec {keys.begin(), keys.end()};
    wio.Bulk(keys_vec);
    wio.Flush();
}


template <typename T>
static uint64_t turn_zero_bytes_to_ones(T val) {
    for (uint32_t j = 0; j < sizeof(val); j++) {
        if (((val >> (8 * j)) & 0xFF) == 0)
            val |= 1ULL << (8 * j);
    }
    return val;
}


void wiredtiger_bench(argparse::ArgumentParser& parser) {
    WorkloadIO wio(parser.get<std::string>("--output-file"), WorkloadIO::iomode::Write, false);
    uint32_t kdist_ind = 0, qdist_ind = 0;
    auto [key_dist, key_dist_mu, key_dist_std, key_file] = get_kdist(parser, kdist_ind);
    auto [query_dist, query_dist_mu, query_dist_std] = get_qdist(parser, qdist_ind);

    const uint32_t n_keys = parser.get<uint64_t>("--n-keys");
    const uint32_t n_queries = parser.get<uint64_t>("--n-queries");
    const uint64_t seed = parser.get<uint64_t>("--seed");
    std::mt19937_64 rng(seed);

    std::set<uint64_t> keys;
    if (key_dist == "unif")
        keys = generate_int_keys_uniform(n_keys, rng);
    else if (key_dist == "norm")
        keys = generate_int_keys_normal(n_keys, key_dist_mu, key_dist_std, rng);
    else
        keys = read_data_binary<uint64_t>(key_file);
    std::vector<uint64_t> keys_vec {keys.begin(), keys.end()};

    for (uint32_t i = 0; i < keys_vec.size(); i++)
        keys_vec[i] = turn_zero_bytes_to_ones(keys_vec[i]);
    std::sort(keys_vec.begin(), keys_vec.end());

    const uint32_t n_expansions = parser.get<uint64_t>("--n-expansions");
    std::shuffle(keys_vec.begin(), keys_vec.end(), rng);
    uint32_t cur_n_keys = n_keys >> n_expansions;
    std::sort(keys_vec.begin(), keys_vec.begin() + cur_n_keys);
    keys = std::set<uint64_t>(keys_vec.begin(), keys_vec.begin() + cur_n_keys);

    {
        std::vector<uint64_t> init_keys_vec = {keys_vec.begin(), keys_vec.begin() + cur_n_keys};

        wio.Bulk(init_keys_vec);

        wio.Timer('q');
        std::cout << "Generating queries..." << std::endl;
        if (query_dist == "unif") {
            std::uniform_int_distribution<uint64_t> dist(0, std::numeric_limits<uint64_t>::max());
            for (uint32_t i = 0; i < n_queries;) {
                const uint64_t query_size = query_size_dist(rng);
                uint64_t l_key = turn_zero_bytes_to_ones(dist(rng));
                if (std::numeric_limits<uint64_t>::max() - l_key < query_size)
                    continue;
                uint64_t r_key = l_key + query_size - 1;
                if (set_range_query(keys, l_key, r_key))
                    continue;
                wio.Query(l_key, r_key, false);
                i++;
                print_progress(1.0 * i / n_queries);
            }
        }
        else if (query_dist == "norm") {
            std::normal_distribution<long double> dist(query_dist_mu, query_dist_std);
            for (uint32_t i = 0; i < n_queries;) {
                const uint64_t query_size = query_size_dist(rng);
                uint64_t l_key = turn_zero_bytes_to_ones(static_cast<uint64_t>(dist(rng)));
                if (std::numeric_limits<uint64_t>::max() - l_key < query_size)
                    continue;
                uint64_t r_key = l_key + query_size - 1;
                if (set_range_query(keys, l_key, r_key))
                    continue;
                wio.Query(l_key, r_key, false);
                i++;
                print_progress(1.0 * i / n_queries);
            }
        }
        else if (query_dist == "corr") {
            const double corr_deg = parser.get<double>("--corr-degree");
            const uint64_t corr_distance = static_cast<uint64_t>(std::min(std::pow(2.0, 64 * (1 - corr_deg)),
                                                                 std::pow(2.0, 64)));
            std::uniform_int_distribution<uint64_t> picker(0, n_keys);
            std::uniform_int_distribution<uint64_t> corr_distance_dist(1, corr_distance);
            for (uint32_t i = 0; i < n_queries;) {
                const uint32_t picked = picker(rng);
                const uint64_t query_size = query_size_dist(rng);
                const uint64_t distance = corr_distance_dist(rng);
                if (keys_vec[picked] < distance + query_size - 1)
                    continue;
                uint64_t r_key = keys_vec[picked] - distance;
                uint64_t l_key = r_key - query_size + 1;
                if (set_range_query(keys, l_key, r_key))
                    continue;
                wio.Query(l_key, r_key, false);
                i++;
                print_progress(1.0 * i / n_queries);
            }
        }
        else 
            throw std::runtime_error("Invalid query distribution type for this benchmark");
        wio.Timer('q');
        wio.Flush();
    }

    for (int32_t expansion = 0; expansion < n_expansions; expansion++) {
        const uint32_t n_inserts = cur_n_keys;
        const uint32_t pos = cur_n_keys;
        std::sort(keys_vec.begin() + pos, keys_vec.begin() + pos + n_inserts);

        wio.ResetDB();
        wio.Timer('i');
        for (uint32_t i = pos; i < pos + n_inserts; i++) {
            wio.Insert(keys_vec[i]);
            keys.insert(keys_vec[i]);
        }
        wio.Timer('i');

        wio.ResetDB();
        wio.Timer('q');
        std::cout << "Generating queries..." << std::endl;
        if (query_dist == "unif") {
            std::uniform_int_distribution<uint64_t> dist(0, std::numeric_limits<uint64_t>::max());
            for (uint32_t i = 0; i < n_queries;) {
                const uint64_t query_size = query_size_dist(rng);
                uint64_t l_key = turn_zero_bytes_to_ones(dist(rng));
                if (std::numeric_limits<uint64_t>::max() - l_key < query_size)
                    continue;
                uint64_t r_key = l_key + query_size - 1;
                if (set_range_query(keys, l_key, r_key))
                    continue;
                wio.Query(l_key, r_key, false);
                i++;
                print_progress(1.0 * i / n_queries);
            }
        }
        else if (query_dist == "norm") {
            std::normal_distribution<long double> dist(query_dist_mu, query_dist_std);
            for (uint32_t i = 0; i < n_queries;) {
                const uint64_t query_size = query_size_dist(rng);
                uint64_t l_key = turn_zero_bytes_to_ones(static_cast<uint64_t>(dist(rng)));
                if (std::numeric_limits<uint64_t>::max() - l_key < query_size)
                    continue;
                uint64_t r_key = l_key + query_size - 1;
                if (set_range_query(keys, l_key, r_key))
                    continue;
                wio.Query(l_key, r_key, false);
                i++;
                print_progress(1.0 * i / n_queries);
            }
        }
        else if (query_dist == "corr") {
            const double corr_deg = parser.get<double>("--corr-degree");
            const uint64_t corr_distance = static_cast<uint64_t>(std::min(std::pow(2.0, 64 * (1 - corr_deg)),
                                                                 std::pow(2.0, 64)));
            std::uniform_int_distribution<uint64_t> picker(0, pos + n_inserts - 1);
            std::uniform_int_distribution<uint64_t> corr_distance_dist(1, corr_distance);
            for (uint32_t i = 0; i < n_queries;) {
                const uint32_t picked = picker(rng);
                const uint64_t query_size = query_size_dist(rng);
                const uint64_t distance = corr_distance_dist(rng);
                if (keys_vec[picked] < distance + query_size - 1)
                    continue;
                uint64_t r_key = keys_vec[picked] - distance;
                uint64_t l_key = r_key - query_size + 1;
                if (set_range_query(keys, l_key, r_key))
                    continue;
                wio.Query(l_key, r_key, false);
                i++;
                print_progress(1.0 * i / n_queries);
            }
        }
        else 
            throw std::runtime_error("Invalid query distribution type for this benchmark");
        wio.Timer('q');
        wio.Flush();
        std::cout << std::endl;

        cur_n_keys *= 2;
    }
}


std::unordered_map<std::string, std::function<void(argparse::ArgumentParser&)>> benches = {
    {"correlated", correlated_bench},
    {"standard-int", standard_int_bench},
    {"standard-string", standard_string_bench},
    {"true", true_bench},
    {"expansion", expansion_bench},
    {"delete", delete_bench},
    {"construction", construction_bench},
    {"wiredtiger", wiredtiger_bench},
};

int main(int argc, char const *argv[]) {
    argparse::ArgumentParser parser("workload_gen");

    {
        std::string msg = "The benchmark type to create [";
        bool first_bench = true;
        for (auto bench : benches) {
            if (first_bench)
                msg += bench.first;
            else
                msg += " | " + bench.first;
            first_bench = false;
        }
        msg += "]";
        parser.add_argument("-t", "--type")
                .help(msg)
                .required()
                .nargs(1);
    }

    parser.add_argument("-o", "--output-file")
            .help("The path to the output file")
            .required()
            .nargs(1);

    parser.add_argument("--kdist")
            .help("The (possibly multiple, for different phases) key distributions")
            .nargs(argparse::nargs_pattern::at_least_one)
            .required()
            .default_value(kdist_default);

    parser.add_argument("--qdist")
            .help("The (possibly multiple, for different phases) query distributions")
            .nargs(argparse::nargs_pattern::at_least_one)
            .required()
            .default_value(qdist_default);

    parser.add_argument("--min-range-size")
            .help("The minimum accepted length of a range query")
            .nargs(1)
            .required()
            .scan<'u', uint64_t>()
            .default_value(range_size_min);

    parser.add_argument("--max-range-size")
            .help("The maximum accepted length of a range query")
            .nargs(1)
            .required()
            .scan<'u', uint64_t>()
            .default_value(range_size_max);

    parser.add_argument("--string-lens")
            .help("The lengths of the strings to be generated (chosen uniformly at random, can have duplicates)")
            .nargs(argparse::nargs_pattern::at_least_one)
            .default_value(default_string_lens)
            .required()
            .scan<'u', uint64_t>();

    parser.add_argument("-n", "--n-keys")
            .help("The number of input keys")
            .required()
            .default_value(static_cast<uint64_t>(default_n_keys))
            .scan<'u', uint64_t>()
            .nargs(1);

    parser.add_argument("-e", "--n-expansions")
            .help("The number of times that the key set doubles")
            .required()
            .default_value(static_cast<uint64_t>(6))
            .scan<'u', uint64_t>()
            .nargs(1);

    parser.add_argument("-d", "--n-deletes")
            .help("The number of delete operations")
            .required()
            .default_value(static_cast<uint64_t>(default_n_deletes))
            .scan<'u', uint64_t>()
            .nargs(1);

    parser.add_argument("-q", "--n-queries")
            .help("The number of queries")
            .nargs(1)
            .required()
            .default_value(static_cast<uint64_t>(default_n_queries))
            .scan<'u', uint64_t>();

    parser.add_argument("--mixed")
            .help("Generates mixed range queries in the range [0, largest_range_size]")
            .implicit_value(true)
            .default_value(false);

    parser.add_argument("--corr-degree")
            .help("Correlation degree for correlated workloads")
            .required()
            .default_value(0.1)
            .scan<'g', double>();

    parser.add_argument("--seed")
            .help("The seed used for random number generation")
            .required()
            .default_value(1380UL)
            .scan<'u', uint64_t>()
            .nargs(1);

    try {
        parser.parse_args(argc, argv);
    }
    catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << parser;
        std::exit(1);
    }

    range_size_min = parser.get<uint64_t>("--min-range-size");
    range_size_max = parser.get<uint64_t>("--max-range-size");
    if (range_size_min > range_size_max)
        std::runtime_error("Error: Invalid accepted range query size range");
    query_size_dist = std::uniform_int_distribution<uint64_t>(range_size_min, range_size_max);


    std::string bench_type = parser.get<std::string>("--type");
    if (benches.find(bench_type) != benches.end())
        benches[bench_type](parser);
    else {
        std::string msg = "Error: Invalid benchmark type. Valid benchmarks: ";
        for (auto bench : benches)
            msg += bench.first + " ";
        throw std::runtime_error(msg);
    }

    std::cout << "Done" << std::endl;

    return 0;
}
