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


#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <boost/sort/sort.hpp>

#include "../bench_template.hpp"
#include "memento.h"
#include "memento_int.h"

inline uint64_t MurmurHash64A(const void * key, int len, unsigned int seed) {
	const uint64_t m = 0xc6a4a7935bd1e995;
	const int r = 47;

	uint64_t h = seed ^ (len * m);

	const uint64_t * data = (const uint64_t *)key;
	const uint64_t * end = data + (len/8);

	while(data != end) {
		uint64_t k = *data++;

		k *= m;
		k ^= k >> r;
		k *= m;

		h ^= k;
		h *= m;
	}

	const unsigned char * data2 = (const unsigned char*)data;

	switch(len & 7) {
		case 7: h ^= (uint64_t)data2[6] << 48; do {} while (0);  /* fallthrough */
		case 6: h ^= (uint64_t)data2[5] << 40; do {} while (0);  /* fallthrough */
		case 5: h ^= (uint64_t)data2[4] << 32; do {} while (0);  /* fallthrough */
		case 4: h ^= (uint64_t)data2[3] << 24; do {} while (0);  /* fallthrough */
		case 3: h ^= (uint64_t)data2[2] << 16; do {} while (0);  /* fallthrough */
		case 2: h ^= (uint64_t)data2[1] << 8; do {} while (0); /* fallthrough */
		case 1: h ^= (uint64_t)data2[0];
						h *= m;
	};

	h ^= h >> r;
	h *= m;
	h ^= h >> r;

	return h;
}

__attribute__((always_inline))
static inline uint32_t fast_reduce(uint32_t hash, uint32_t n) {
    // http://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction/
    return (uint32_t) (((uint64_t) hash * n) >> 32);
}

int predef_memento_size = -1;

template <typename t_itr, typename... Args>
inline QF *init(const t_itr begin, const t_itr end, const double bpk, Args... args) {
    auto&& t = std::forward_as_tuple(args...);
    auto queries_temp = std::get<0>(t);
    auto query_lengths = std::vector<uint64_t>(queries_temp.size());
    std::transform(queries_temp.begin(), queries_temp.end(), query_lengths.begin(), [](auto x) {
        auto [left, right, result] = x;
        return right - left + 1;
    });
    const uint64_t n_items = std::distance(begin, end);
    const uint64_t seed = 1380;
    const uint64_t max_range_size = query_lengths.empty() ? 0 
                                        : *std::max_element(query_lengths.begin(), query_lengths.end());
    const double load_factor = 0.95;
    const uint64_t n_slots = n_items / load_factor + std::sqrt(n_items);
    uint32_t memento_bits = 1;
    if (predef_memento_size == -1) {
        while ((1ULL << memento_bits) < max_range_size)
            memento_bits++;
        memento_bits = memento_bits < 2 ? 2 : memento_bits;
    }
    else 
        memento_bits = predef_memento_size;
    const uint32_t fingerprint_size = round(bpk * load_factor - memento_bits - 2.125);
    uint32_t key_size = 0;
    while ((1ULL << key_size) <= n_slots)
        key_size++;
    key_size += fingerprint_size;

    QF *qf = (QF *) malloc(sizeof(QF));
    qf_malloc(qf, n_slots, key_size, memento_bits, QF_HASH_DEFAULT, seed);
    qf_set_auto_resize(qf, true);

    time_points['c'] = timer::now();

    auto key_hashes = std::vector<uint64_t>(n_items);
    const uint64_t address_size = key_size - fingerprint_size;
    const uint64_t address_mask = (1ULL << address_size) - 1;
    const uint64_t memento_mask = (1ULL << memento_bits) - 1;
    const uint64_t hash_mask = (1ULL << key_size) - 1;
    std::transform(begin, end, key_hashes.begin(), [&](auto x) {
            auto y = x >> memento_bits;
            uint64_t hash = MurmurHash64A(((void *)&y), sizeof(y), seed) & hash_mask;
            const uint64_t address = fast_reduce((hash & address_mask) << (32 - address_size),
                                                    n_slots);
            hash = (hash >> address_size) | (address << fingerprint_size);
            return (hash << memento_bits) | (x & memento_mask);
            });
    /*
     * The following code uses the Boost library to sort the elements in a single thread, via spreadsort function.
     * This function is faster than std::sort and exploits the fact that the size of the maximum hash is bounded
     * via hybrid radix sort.
     */
    boost::sort::spreadsort::spreadsort(key_hashes.begin(), key_hashes.end());

    qf_bulk_load(qf, &key_hashes[0], key_hashes.size(), QF_NO_LOCK | QF_KEY_IS_HASH);

    return qf;
}

inline void insert(QF *f, uint64_t key) {
    uint64_t prefix = key >> f->metadata->memento_bits;
    uint64_t memento = key & ((1ULL << f->metadata->memento_bits) - 1);
    const int res = qf_insert_single(f, prefix, memento, QF_NO_LOCK);
    assert(res);
}

inline void del(QF *f, uint64_t key) {
    uint64_t prefix = key >> f->metadata->memento_bits;
    uint64_t memento = key & ((1ULL << f->metadata->memento_bits) - 1);
    const int res = qf_delete_single(f, prefix, memento, QF_NO_LOCK);
    assert(res);
}

inline bool query(QF *f, uint64_t left, uint64_t right) {
    uint64_t l_key = left >> f->metadata->memento_bits;
    uint64_t l_memento = left & ((1ULL << f->metadata->memento_bits) - 1);
    if (left == right) {
        return qf_point_query(f, l_key, l_memento, QF_NO_LOCK);
    }
    uint64_t r_key = right >> f->metadata->memento_bits;
    uint64_t r_memento = right & ((1ULL << f->metadata->memento_bits) - 1);
    return qf_range_query(f, l_key, l_memento, r_key, r_memento, QF_NO_LOCK);
}

inline size_t size(QF *f) {
    return qf_get_total_size_in_bytes(f);
}

int main(int argc, char const *argv[]) {
    auto parser = init_parser("bench-memento");

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
        for (predef_memento_size = 0; (1 << predef_memento_size) < *max_range_size; predef_memento_size++);
    }

    experiment(pass_fun(init), pass_fun(insert), pass_fun(del), pass_ref(query), pass_ref(size),
               wio.GetIntQueries(), -1);

    return 0;
}
