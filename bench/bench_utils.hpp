/*
 * This file is part of --- <url>.
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
#include <cstdint>
#include <cstdio>
#include <limits>
#include <ostream>
#include <set>
#include <vector>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <algorithm>
#include <fstream>
#include <chrono>
#include <cstring>
#include <string_view>
#include <tuple>

/**
 * This file contains some utility functions and data structures used in the benchmarks.
 */
#define TO_MB(x) (x / (1024.0 * 1024.0))
#define TO_BPK(x, n) ((double)(x * 8) / n)

inline std::string uint64ToString(uint64_t key) {
    uint64_t endian_swapped_key = __builtin_bswap64(key);
    return std::string(reinterpret_cast<const char *>(&endian_swapped_key), 8);
}

inline uint64_t stringToUint64(const std::string_view str_key) {
    uint64_t int_key = 0;
    memcpy(reinterpret_cast<char *>(&int_key), str_key.data(), 8);
    return __builtin_bswap64(int_key);
}

inline bool has_suffix(const std::string_view str, const std::string_view suffix) {
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

using timer = std::chrono::high_resolution_clock;


struct ByteString {
    uint8_t *str;
    uint32_t length;

    ByteString(const uint8_t *buf, uint32_t buf_len)
        : length(buf_len) {
        str = new uint8_t[buf_len];
        memcpy(str, buf, buf_len);
    }

    ByteString(const ByteString& other)
        : length(other.length) {
        str = new uint8_t[other.length];
        memcpy(str, other.str, other.length);
    }

    ByteString(ByteString&& other)
        : length(other.length) {
        str = other.str;
        other.str = nullptr;
    }

    ~ByteString() {
        delete[] str;
    }

    ByteString& operator=(const ByteString& other) {
        delete[] str;
        length = other.length;
        str = new uint8_t[other.length];
        memcpy(str, other.str, other.length);
        return *this;
    }

    bool operator<(const ByteString& other) const {
        uint32_t min_length = std::min(length, other.length);
        int32_t cmp = memcmp(str, other.str, min_length);
        if (cmp == 0)
            return length < other.length;
        return cmp < 0;
    }

    bool operator<=(const ByteString& other) const {
        uint32_t min_length = std::min(length, other.length);
        int32_t cmp = memcmp(str, other.str, min_length);
        if (cmp == 0)
            return length <= other.length;
        return cmp < 0;
    }

    bool operator>(const ByteString& other) const {
        return !(*this <= other);
    }

    bool operator>=(const ByteString& other) const {
        return !(*this < other);
    }
};

inline std::ostream& operator<<(std::ostream& os, const ByteString& s)
{
    for (int32_t i = 0; i < s.length; i++) {
        for (int32_t j = 7; j >= 0; j--)
            os << ((s.str[i] >> j) & 1);
        os << ' ';
    }
    return os;
}


inline uint32_t calculate_lcp(const uint8_t *a, uint32_t a_len, const uint8_t *b, uint32_t b_len) {
    uint32_t min_len = std::min(a_len, b_len);
    for (int32_t i = 0; i < min_len; i++)
        if (a[i] != b[i])
            return i;
    return min_len;
}


template<typename KeyType>
using InputKeys = std::vector<KeyType>;

template<typename KeyType>
bool set_range_query(const std::set<KeyType>& k, const KeyType left, const KeyType right) {
    auto it = k.lower_bound(left);
    // if true than the range is found in the original data
    bool is_found_query = !((it == k.end()) || (*it > right));
    return is_found_query;
}

template<typename KeyType>
bool set_point_query(const std::set<KeyType>& k, const KeyType x) {
    return k.find(x) != k.end();
}

template<typename KeyType>
bool vector_range_query(const InputKeys<KeyType>& k, const KeyType left, const KeyType right) {
    auto lower = std::lower_bound(k.begin(), k.end(), left);
    // if true than the range is found in the original data
    bool is_found_query = !((lower == k.end()) || (*lower > right));
    return is_found_query;
}

template <typename KeyType>
bool vector_point_query(const InputKeys<KeyType>& k, const KeyType x) {
    auto lower = std::lower_bound(k.begin(), k.end(), x);
    if (lower == k.end())
        return false;
    return *lower == x;
}

template <typename KeyType>
std::set<KeyType> read_data_binary(const std::string& filename) {
    std::vector<KeyType> data;
    std::fstream in(filename, std::ios::in | std::ios::binary);
    in.exceptions(std::ios::failbit | std::ios::badbit);
    KeyType size;
    in.read((char *) &size, sizeof(KeyType));
    data.resize(size);
    in.read((char *) data.data(), size * sizeof(KeyType));
    return {data.begin(), data.end()};
}

template <>
inline std::set<ByteString> read_data_binary(const std::string& filename) {
    std::set<ByteString> data;
    std::fstream in(filename, std::ios::in | std::ios::binary);
    in.exceptions(std::ios::failbit | std::ios::badbit);
    uint8_t buf[std::numeric_limits<uint16_t>::max()];
    uint16_t key_size;
    while (!in.eof()) {
        in.read(reinterpret_cast<char *>(&key_size), sizeof(key_size));
        in.read(reinterpret_cast<char *>(buf), key_size);
        data.insert(ByteString(buf, key_size));
    }
    return data;
}


class WorkloadIO {
public:
    enum class opcode {
        Bulk,
        Insert,
        Delete,
        Query,
        Timer,
        Flush
    };

    enum class iomode {
        Read,
        Write
    };


    WorkloadIO(std::string_view file_path, iomode mode, bool string_keys)
            : mode_(mode),
              string_keys_(string_keys) {
        if (mode == iomode::Read) {
            io_ = std::fstream(file_path.data(), std::ios::in | std::ios::binary);
            io_.read(reinterpret_cast<char *>(&string_keys_), sizeof(string_keys_));
        }
        else {
            io_ = std::fstream(file_path.data(), std::ios::out | std::ios::binary);
            io_.write(reinterpret_cast<const char *>(&string_keys_), sizeof(string_keys_));
        }
    }


    void Bulk(InputKeys<ByteString>& keys) {
        if (!std::is_sorted(keys.begin(), keys.end()))
            throw std::runtime_error("Keys are not sorted");
        const uint8_t opcode = static_cast<uint8_t>(opcode::Bulk);
        io_.write(reinterpret_cast<const char *>(&opcode), sizeof(opcode));
        uint32_t key_count = keys.size();
        io_.write(reinterpret_cast<const char *>(&key_count), sizeof(key_count));
        for (auto key : keys) {
            io_.write(reinterpret_cast<const char *>(&key.length), sizeof(key.length));
            io_.write(reinterpret_cast<const char *>(key.str), key.length);
        }
    }

    void Bulk(InputKeys<uint64_t>& keys) {
        if (!std::is_sorted(keys.begin(), keys.end()))
            throw std::runtime_error("Keys are not sorted");
        const uint8_t opcode = static_cast<uint8_t>(opcode::Bulk);
        io_.write(reinterpret_cast<const char *>(&opcode), sizeof(opcode));
        uint32_t key_count = keys.size();
        io_.write(reinterpret_cast<const char *>(&key_count), sizeof(key_count));
        for (auto key : keys) {
            const uint16_t key_len = sizeof(key);
            io_.write(reinterpret_cast<const char *>(&key_len), sizeof(key_len));
            io_.write(reinterpret_cast<const char *>(&key), key_len);
        }
    }

    void Insert(ByteString& key) {
        const uint8_t opcode = static_cast<uint8_t>(opcode::Insert);
        io_.write(reinterpret_cast<const char *>(&opcode), sizeof(opcode));
        io_.write(reinterpret_cast<const char *>(&key.length), sizeof(key.length));
        io_.write(reinterpret_cast<const char *>(key.str), key.length);
    }

    void Insert(uint64_t key) {
        const uint8_t opcode = static_cast<uint8_t>(opcode::Insert);
        io_.write(reinterpret_cast<const char *>(&opcode), sizeof(opcode));
        io_.write(reinterpret_cast<const char *>(&key), sizeof(key));
    }


    void Delete(ByteString& key) {
        const uint8_t opcode = static_cast<uint8_t>(opcode::Delete);
        io_.write(reinterpret_cast<const char *>(&opcode), sizeof(opcode));
        io_.write(reinterpret_cast<const char *>(&key.length), sizeof(key.length));
        io_.write(reinterpret_cast<const char *>(key.str), key.length);
    }

    void Delete(uint64_t key) {
        const uint8_t opcode = static_cast<uint8_t>(opcode::Delete);
        io_.write(reinterpret_cast<const char *>(&opcode), sizeof(opcode));
        io_.write(reinterpret_cast<const char *>(&key), sizeof(key));
    }


    void Query(ByteString& l_key, ByteString& r_key, bool res) {
        const uint8_t opcode = static_cast<uint8_t>(opcode::Query);
        io_.write(reinterpret_cast<const char *>(&opcode), sizeof(opcode));
        io_.write(reinterpret_cast<const char *>(&l_key.length), sizeof(l_key.length));
        io_.write(reinterpret_cast<const char *>(l_key.str), l_key.length);
        io_.write(reinterpret_cast<const char *>(&r_key.length), sizeof(r_key.length));
        io_.write(reinterpret_cast<const char *>(r_key.str), r_key.length);
        io_.write(reinterpret_cast<const char *>(&res), sizeof(res));
    }

    void Query(uint64_t l_key, uint64_t r_key, bool res) {
        const uint8_t opcode = static_cast<uint8_t>(opcode::Query);
        io_.write(reinterpret_cast<const char *>(&opcode), sizeof(opcode));
        io_.write(reinterpret_cast<const char *>(&l_key), sizeof(l_key));
        io_.write(reinterpret_cast<const char *>(&r_key), sizeof(r_key));
        io_.write(reinterpret_cast<const char *>(&res), sizeof(res));
    }


    void Timer(char timer_stamp) {
        const uint8_t opcode = static_cast<uint8_t>(opcode::Timer);
        io_.write(reinterpret_cast<const char *>(&opcode), sizeof(opcode));
        io_.write(&timer_stamp, sizeof(timer_stamp));
    }


    void Flush() {
        const uint8_t opcode = static_cast<uint8_t>(opcode::Flush);
        io_.write(reinterpret_cast<const char *>(&opcode), sizeof(opcode));
    }


    bool Done() const {
        return mode_ == iomode::Write ? false : io_.eof();
    }


    opcode GetOpcode() {
        uint8_t res;
        io_.read(reinterpret_cast<char *>(&res), sizeof(res));
        return static_cast<opcode>(res);
    }

    void GetStringKey(uint16_t& length, uint8_t *key) {
        io_.read(reinterpret_cast<char *>(&length), sizeof(length));
        io_.read(reinterpret_cast<char *>(key), length);
    }

    uint64_t GetIntKey() {
        uint64_t key;
        io_.read(reinterpret_cast<char *>(&key), sizeof(key));
        return key;
    }


    void GetStringQuery(uint16_t& l_length, uint8_t *l_key, uint16_t& r_length, uint8_t* r_key, bool& res) {
        io_.read(reinterpret_cast<char *>(&l_length), sizeof(l_length));
        io_.read(reinterpret_cast<char *>(l_key), l_length);
        io_.read(reinterpret_cast<char *>(&r_length), sizeof(r_length));
        io_.read(reinterpret_cast<char *>(r_key), r_length);
        io_.read(reinterpret_cast<char *>(&res), sizeof(res));
    }

    std::tuple<uint64_t, uint64_t, bool> GetIntQuery() {
        uint64_t l_key, r_key;
        bool res;
        io_.read(reinterpret_cast<char *>(&l_key), sizeof(l_key));
        io_.read(reinterpret_cast<char *>(&r_key), sizeof(r_key));
        io_.read(reinterpret_cast<char *>(&res), sizeof(res));
        return {l_key, r_key, res};
    }

private:
    std::fstream io_;
    iomode mode_;
    bool string_keys_;
};


class TestOutput {
    std::map<std::string, std::string> test_values;

public:
    template<typename TestValueType>
    void AddMeasure(const std::string& key, TestValueType value) {
        auto str = std::to_string(value);
        if (key == "fpr" && str == "0.000000")
            str = "0.000001";
        test_values[key] = str;
    }

    void AddMeasure(const std::string& key, const std::string& value) {
        test_values[key] = value;
    }

    void Clear() {
        test_values.clear();
    }

    void Print() const {
        for (auto t: test_values) {
            std::cout << t.first << ": " << t.second << std::endl;
        }
    }

    auto operator[](const std::string& key) {
        return test_values[key];
    }

    std::string ToJson(bool print_header=true) {
        std::string res = "{\n";
        for (auto it = test_values.begin(); it != test_values.end(); ++it) {
            res += "\t\"";
            res += (*it).first;
            res += "\": ";
            res += (*it).second;
            res += "\",\n";
        }
        res += "}\n";
        return res;
    }
};

