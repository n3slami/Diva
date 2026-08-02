#pragma once

#include <algorithm>
#include <atomic>
#include <bitset>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <endian.h>
#include <exception>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <random>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <vector>
#include <x86intrin.h>

#include "wormhole/wh.h"
#include "util.hpp"
#include "wormhole/wh_int.h"

// 0: Allows for non-existent actual suffixes. For unit tests.
// 1: Forces a suffix length of at least half of the slot width. For the EnWiki dataset.
// 2: Forces a suffix length slightly smaller than half of the slot width. For the Emails dataset.
// 3: Forces a suffix length smaller than half of the slot width. For the Quotes dataset.
//#ifndef ACTUAL_SUFFIX_LEN_MODE 
#define ACTUAL_SUFFIX_LEN_MODE 0
//#endif


namespace diva {

static void print_key(const uint8_t *key, const uint32_t key_len, const bool binary=true) {
    for (int32_t i = 0; i < key_len; i++) {
        if (binary) {
            for (int32_t j = 7; j >= 0; j--)
                std::cerr << +((key[i] >> j) & 1);
            std::cerr << ' ';
        }
        else
            std::cerr << std::setfill(' ') << std::setw(3) << +key[i] << ' ';
    }
    std::cerr << std::endl;
}


static void print_key(const char *key, const uint32_t key_len, const bool binary=true) {
    print_key(reinterpret_cast<const uint8_t *>(key), key_len, binary);
}


static void validate_infixes_and_bounds(uint32_t infix_count, uint64_t *infix_list,
                                        uint32_t infix_size_,
                                        uint64_t prev_extraction, uint64_t next_extraction) {
    const uint64_t prev_implicit = prev_extraction >> infix_size_;
    const uint64_t prev_infix = prev_extraction & BITMASK(infix_size_);
    const uint64_t next_implicit = next_extraction >> infix_size_;
    const uint64_t next_infix = next_extraction & BITMASK(infix_size_);
    for (uint32_t i = 0; i < infix_count; i++) {
        const uint64_t shamt = lowbit_pos(infix_list[i]) + 1;
        const uint64_t infix_raw = infix_list[i] >> shamt;
        const uint64_t min_check = prev_infix >> shamt;
        const uint64_t max_check = (((next_implicit - prev_implicit) << infix_size_) | next_infix) >> shamt;
        assert(min_check <= infix_raw);
        assert(infix_raw <= max_check);
    }
}

enum class DivaType {
    Standard,
    Int,
    BinaryTrie  // TODO: Payloads don't play well with this mode of Diva. 
};

enum class PayloadType {
    None,
    FixedLength,
    VarLength
};

template <DivaType diva_type=DivaType::Standard, PayloadType payload_type=PayloadType::None>
class Diva {
    friend class Iterator;
    friend class DivaTests;
    friend class InfixStoreTests;
    friend class InfixTests;

public:
    Diva(const uint32_t infix_size, const uint32_t rng_seed, const float load_factor,
         const uint32_t payload_size=0, const bool setup_start_end_samples=false);

    template <class t_itr>
    Diva(const uint32_t infix_size, const t_itr begin, const t_itr end, const uint32_t key_len,
         const uint32_t rng_seed, const float load_factor,
         const uint32_t payload_size=0, const uint64_t **payload_list=nullptr);

    template <class t_itr>
    Diva(const uint32_t infix_size, const t_itr begin, const t_itr end, 
         const uint32_t rng_seed, const float load_factor,
         const uint32_t payload_size=0, const uint64_t **payload_list=nullptr);

    Diva(const char *deser_buf);

    ~Diva();

    void Insert(uint64_t key, const void *payload=nullptr, uint32_t random_number=0);
    void Insert(std::string_view key, const void *payload=nullptr, uint32_t random_number=0);
    void Insert(const uint8_t *key, const uint32_t key_len, const void *payload=nullptr, uint32_t random_number=0);
    void Delete(uint64_t key, std::function<bool(const uint64_t *)> should_remove=nullptr);
    void Delete(std::string_view input_key, std::function<bool(const uint64_t *)> should_remove=nullptr);
    void Delete(const uint8_t *input_key, const uint32_t input_key_len, std::function<bool(const uint64_t *)> should_remove=nullptr);
    void DeleteRange(uint64_t l, uint64_t r,
                     std::function<bool(const uint64_t *)> should_remove=nullptr);
    void DeleteRange(std::string_view input_l, std::string_view input_r,
                     std::function<bool(const uint64_t *)> should_remove=nullptr);
    void DeleteRange(const uint8_t *input_l, const uint32_t input_l_len,
                     const uint8_t *input_r, const uint32_t input_r_len,
                     std::function<bool(const uint64_t *)> should_remove=nullptr);
    bool RangeQuery(uint64_t l, uint64_t r) const;
    bool RangeQuery(std::string_view input_l, std::string_view input_r) const;
    bool RangeQuery(const uint8_t *input_l, const uint32_t input_l_len,
                    const uint8_t *input_r, const uint32_t input_r_len) const;
    bool PointQuery(uint64_t key) const;
    bool PointQuery(std::string_view key) const;
    bool PointQuery(const uint8_t *key, const uint32_t key_len) const;
    void Adapt(uint64_t key, const uint32_t new_prefix_len);
    void Adapt(std::string_view key, const uint32_t new_prefix_len);
    void Adapt(const uint8_t *input_key, const uint32_t input_key_len, const uint32_t new_prefix_len);
    void ShrinkInfixSize(const uint32_t new_infix_size);
    uint64_t Size() const;
    uint32_t Serialize(char *out) const;
    void BulkLoadStreaming(uint64_t key, const uint64_t *payload=nullptr);
    void BulkLoadStreaming(std::string_view key, const uint64_t *payload=nullptr);
    void BulkLoadStreaming(const uint8_t *key, const uint32_t key_len, const uint64_t *payload=nullptr);
    void BulkLoadStreamingFinish();
    uint64_t GetNumKeys() const;

    struct InfiniteByteString {
        const uint8_t *str;
        uint32_t length;

        InfiniteByteString(): str(nullptr), length(0) {};
        InfiniteByteString(const uint8_t *str, uint32_t length): str(str), length(length) {};
        InfiniteByteString(const InfiniteByteString& other): str(other.str), length(other.length) {};

        InfiniteByteString& operator=(const InfiniteByteString& other) {
            str = other.str;
            length = other.length;
            return *this;
        }

        //__attribute__((always_inline))
        uint64_t WordAt(const uint32_t byte_pos) const {
            if (byte_pos >= length)
                return 0;
            uint64_t res = 0;
            memcpy(&res, str + byte_pos, std::min<uint32_t>(sizeof(res), length - byte_pos));
            return __builtin_bswap64(res);
        };

        //__attribute__((always_inline))
        uint64_t BitsAt(const uint32_t bit_pos, const uint32_t res_width) const {
#ifdef DEBUG
            assert(bit_pos % 8 + res_width <= 64);
#endif // DEBUG
            if (bit_pos / 8 >= length)
                return 0;
            uint64_t res = 0;
            memcpy(&res, str + bit_pos / 8, std::min<uint32_t>(sizeof(res), length - bit_pos / 8));
            res = __builtin_bswap64(res) >> (8 * sizeof(res) - res_width - bit_pos % 8);
            return res & BITMASK(res_width);
        };

        //__attribute__((always_inline))
        uint64_t BitsAtBitLength(const uint32_t bit_pos, const uint32_t res_width) const {
#ifdef DEBUG
            assert(bit_pos % 8 + res_width <= 64);
#endif // DEBUG
            if (bit_pos >= length)
                return 0;
            uint64_t res = 0;
            memcpy(&res, str + bit_pos / 8, std::min<uint32_t>(sizeof(res), (length + 7) / 8 - bit_pos / 8));
            res = __builtin_bswap64(res) >> (8 * sizeof(res) - res_width - bit_pos % 8);
            return res & BITMASK(res_width);
        };

        //__attribute__((always_inline))
        uint32_t GetBit(const uint32_t pos) const {
            return (pos / 8 < length ? (str[pos / 8] >> (7 - pos % 8)) & 1 : 0);
        };

        //__attribute__((always_inline))
        uint32_t GetBitBitLength(const uint32_t pos) const {
            return (pos < length ? (str[pos / 8] >> (7 - pos % 8)) & 1 : 0);
        };

        //__attribute__((always_inline))
        bool IsPrefixOf(const InfiniteByteString& other, const uint32_t bits_to_ignore=0) const {
            if (length <= other.length && memcmp(str, other.str, length - 1) == 0)
                return (str[length - 1] | BITMASK(bits_to_ignore)) == (other.str[length - 1] | BITMASK(bits_to_ignore));
            return false;
        }

        bool operator<(const InfiniteByteString& rhs) const {
            int32_t cmp_result = memcmp(str, rhs.str, std::min(length, rhs.length));
            return cmp_result < 0 || (cmp_result == 0 && length < rhs.length);
        }

        bool operator<=(const InfiniteByteString& rhs) const {
            int32_t cmp_result = memcmp(str, rhs.str, std::min(length, rhs.length));
            return cmp_result < 0 || (cmp_result == 0 && length <= rhs.length);
        }

        bool operator>(const InfiniteByteString& rhs) const {
            return !(*this <= rhs);
        }

        bool operator==(const InfiniteByteString& rhs) const {
            return length == rhs.length && memcmp(str, rhs.str, std::min(length, rhs.length)) == 0;
        }
    };

    class Iterator {
        friend class Diva<diva_type, payload_type>;
        friend class DivaTests;

        using KeyType = std::conditional_t<diva_type == DivaType::Int, uint64_t, std::string>;

    public:
        Iterator(Diva<diva_type, payload_type> *parent):
            filter_(parent) { }
        ~Iterator();

        Iterator(const Iterator& other);
        Iterator& operator=(const Iterator& other);
        std::pair<KeyType, uint32_t> operator*();
        Iterator& operator++();
        Iterator operator++(int);
        bool operator==(const Iterator& rhs) const;
        bool operator!=(const Iterator& rhs) const;

        void GetPayload(uint64_t *out) const;
        void SetDeleteFunction(std::function<bool(const uint64_t *)> should_remove);
        bool IsValid() const;

    private:
        static constexpr uint32_t iterator_local_buf_len = 1 << 10;
        Diva<diva_type, payload_type> *filter_;
        InfiniteByteString next_to_fetch_, end_key_;
        uint32_t shared_, ignore_, implicit_;
        uint8_t next_to_fetch_contents_[iterator_local_buf_len], end_key_contents_[iterator_local_buf_len];
        uint8_t shared_prefix_[iterator_local_buf_len], current_key_contents_[iterator_local_buf_len];
        std::vector<uint64_t> infixes_, bit_counts_, payloads_;
        uint32_t ind_ = 0;
        std::function<bool(const uint64_t *)> should_remove_;
        bool first_store_to_fetched_and_delete_ = true;

        Iterator(Diva<diva_type, payload_type> *parent,
                 std::string_view start, std::string_view end,
                 std::function<bool(const uint64_t *)> should_remove=nullptr);
        Iterator(Diva<diva_type, payload_type> *parent, 
                 const uint8_t *start, uint32_t start_len, 
                 const uint8_t *end, uint32_t end_len, 
                 std::function<bool(const uint64_t *)> should_remove=nullptr);
        Iterator(Diva<diva_type, payload_type> *parent,
                 uint64_t start, uint64_t end,
                 std::function<bool(const uint64_t *)> should_remove=nullptr);

        void Fetch();
        void FetchDelete();

        void SetSharedPrefix(InfiniteByteString prefix, uint32_t shared, uint32_t ignore, uint32_t implicit) {
            memcpy(shared_prefix_, prefix.str, prefix.length);
            shared_ = shared;
            ignore_ = ignore;
            implicit_ = implicit;
        }

        void SetNextToFetch(const uint8_t *str, const uint32_t len) {
            if (str != nullptr && len > 0) {
                memcpy(next_to_fetch_contents_, str, len);
                next_to_fetch_ = {next_to_fetch_contents_, len};
            }
            else 
                next_to_fetch_ = {nullptr, 0};
        }

        void SetNextToFetchFromExtraction(const InfiniteByteString next, uint64_t extraction);

        void SetEnd(const uint8_t *str, const uint32_t len) {
            if (str != nullptr && len > 0) {
                memcpy(end_key_contents_, str, len);
                end_key_ = {end_key_contents_, len};
            }
            else 
                end_key_ = {nullptr, 0};
        }
    };

private:
    static constexpr uint32_t num_metadata_offset_words = payload_type == PayloadType::FixedLength ? 2 : 1;
#if ACTUAL_SUFFIX_LEN_MODE == 0
    static constexpr uint32_t infix_store_target_size = 1024;
#else
    static constexpr uint32_t infix_store_target_size = 256;
#endif // DEBUG
    static_assert(infix_store_target_size % 64 == 0);
    static constexpr uint32_t base_implicit_size = __builtin_ctz(infix_store_target_size);
    static constexpr uint32_t scale_shift = 15;
    static constexpr uint32_t scale_implicit_shift = 15;
    static constexpr uint32_t size_scalar_count = 500;
    static constexpr uint32_t heap_alloc_threshold = 20000U;
    static constexpr uint64_t max_exp_backoff = BITMASK(14);

    struct InfixStore {
        static const uint32_t size_grade_bit_count = 12;
        static const uint32_t full_slot_count_bit_count = 48;

        uint64_t status = 0;
        uint16_t num_sample_payloads = 0;
        std::atomic<lock_t> rwlock {0};
        uint64_t *ptr = nullptr;

        InfixStore(const uint32_t slot_count, const uint32_t slot_size,
                   const uint32_t size_grade, const uint32_t payload_size=0) {
            SetSizeGrade(size_grade);
            const uint64_t word_count = GetPtrWordCount(slot_count, slot_size, payload_size);
            rwlock.store(0, std::memory_order_release);
            ptr = new uint64_t[word_count];
            memset(ptr, 0, sizeof(uint64_t) * word_count);
        }
        InfixStore(uint64_t *ptr): status(0), ptr(ptr) {};
        InfixStore() = default;
        InfixStore(const InfixStore &other):
                    status(other.status),
                    num_sample_payloads(other.num_sample_payloads),
                    rwlock(0),
                    ptr(other.ptr) { 
            rwlock.store(0, std::memory_order_release);
        }
        InfixStore(InfixStore &&other) = default;
        InfixStore &operator=(const InfixStore &other) = default;

        void Reset(const uint32_t slot_count, const uint32_t slot_size, const uint32_t payload_size=0) {
#ifdef DEBUG
            assert(ptr);
#endif // DEBUG
            memset(ptr, 0, GetPtrWordCount(slot_count, slot_size, payload_size) * sizeof(uint64_t));
        }

        static uint64_t GetPtrWordCount(const uint32_t slot_count, const uint32_t slot_size, const uint32_t payload_size=0) {
            if constexpr (payload_type == PayloadType::FixedLength)
                return num_metadata_offset_words + (Diva::infix_store_target_size + slot_count * (slot_size + 1 + payload_size) + 63) / 64 + 1;
            return num_metadata_offset_words + (Diva::infix_store_target_size + slot_count * (slot_size + 1) + 63) / 64 + 1;
        }

        uint64_t GetFullSlotCount() const {
            return status & BITMASK(full_slot_count_bit_count);
        }

        void SetFullSlotCount(const int64_t elem_count) {
            status &= ~BITMASK(full_slot_count_bit_count);
            status |= elem_count;
        }

        void UpdateFullSlotCount(const int32_t delta) {
            const uint64_t updated_elem_count = GetFullSlotCount() + delta;
            status &= ~BITMASK(full_slot_count_bit_count);
            status |= updated_elem_count;
        }

        uint32_t GetSizeGrade() const {
            return (status >> full_slot_count_bit_count) & BITMASK(size_grade_bit_count);
        }

        void SetSizeGrade(const uint64_t size_grade) {
            status &= ~(BITMASK(size_grade_bit_count) << full_slot_count_bit_count);
            status |= size_grade << full_slot_count_bit_count;
        }
    };

    class Infix {
        friend class InfixTests;
        friend class InfixStoreTests;

    public: 
        class TrieIterator {
            friend class Diva<diva_type, payload_type>;
            friend class InfixTests;
            friend class InfixStoreTests;

        public:
            const uint64_t *buf_;
            uint32_t num_prefix_keys_read_ = 0;
            uint32_t num_keys_read_ = 0;
            uint32_t bit_pos_ = 0;
            std::vector<std::pair<int32_t, int32_t>> depth_branch_ = {{-1, 0b11}};

            TrieIterator(const void *buf);
            TrieIterator(const Infix *infix);
            TrieIterator(const Infix& infix);
            TrieIterator(const TrieIterator& other);
            TrieIterator& operator=(const TrieIterator& other);

            void SkipSubtree(bool has_prefix_keys);
            void Advance(bool has_prefix_keys);
            bool AtLeaf();
            bool AtPrefixKey(bool has_prefix_keys);

        private:
            static constexpr uint32_t depth_reserve_size = 100;
        };

        uint64_t infix_;
        uint32_t num_suffixes_ = 0;
        uint32_t num_suffix_bits_ = 0;
        uint32_t num_prefix_keys_ = 0;
        uint32_t num_trie_bits_ = 0;
        std::vector<uint64_t> trie_ = {};
        std::vector<uint64_t> trie_suffixes_ = {};

        Infix(const uint64_t infix=0):
                infix_(infix) { }
        Infix(uint64_t *ptr, uint32_t bit_pos, uint32_t slot_size);
        Infix(const Infix& other);
        ~Infix() = default;

        Infix& operator=(const Infix& other);
        bool operator<(const Infix& rhs) const {
            return CompareInfixes(infix_, rhs.infix_);
        }
        bool operator<=(const Infix& rhs) const {
            return CompareInfixes(infix_, rhs.infix_) || infix_ == rhs.infix_;
        }
        bool operator==(const Infix& rhs) const;
        bool operator!=(const Infix& rhs) const {
            return !(*this == rhs);
        }

        // The lengths in the InfiniteByteStrings are in bits here.
        void BuildTrieAndSuffixes(const InfiniteByteString *keys, uint32_t key_count,
                       uint32_t key_start_bit, uint32_t slot_size,
                       bool force_prefix_keys=false,
                       bool store_full_keys=false, bool pad_short_keys=false);
        void InsertTrie(const InfiniteByteString key, uint32_t key_start_bit,
                        uint32_t slot_size);
        void AdaptTrie(const InfiniteByteString key, uint32_t key_start_bit,
                       uint32_t adapt_length, uint32_t slot_size);
        void DeleteTrie(const InfiniteByteString key, uint32_t key_start_bit,
                        uint32_t slot_size);
        // The lengths in the InfiniteByteStrings are in bits here.
        std::pair<std::vector<InfiniteByteString>, std::vector<uint8_t>> GetStrings(uint32_t slot_size) const;
        bool QueryTrie(const InfiniteByteString l_key, const InfiniteByteString r_key, 
                       uint32_t key_start_bit, uint32_t slot_size) const;
        int32_t GetLongestMatch(const InfiniteByteString key,
                                uint32_t key_start_bit, 
                                uint32_t slot_size) const;
        void SerializeToInfixStore(InfixStore& store, uint32_t pos, uint32_t store_size, uint32_t slot_size) const;
        void SerializeToPtr(void *ptr, uint32_t bit_pos, uint32_t slot_size) const;
        void DeserializeFromPtr(void *ptr, uint32_t bit_pos, uint32_t slot_size);
        uint32_t GetNumTrieBitsFromInfixStore(const InfixStore& infix_store,
                                              uint32_t slot_pos, uint32_t slot_size) const;
        uint32_t GetNumTrieSlotsFromInfixStore(const InfixStore& infix_store,
                                               uint32_t slot_pos, uint32_t slot_size) const {
            int32_t num_bits = GetNumTrieBitsFromInfixStore(infix_store, slot_pos, slot_size);
            return (num_bits + slot_size - 1) / slot_size;
        }
        uint32_t GetActualSuffixLen(uint32_t slot_size) const;
        uint32_t GetNumSlots(uint32_t slot_size) const;

        std::vector<Infix> SplitPrefixBits(uint32_t num_bits, uint32_t slot_size) const;
        void Merge(const Infix& other, uint32_t slot_size);
        void PrependPrefix(const InfiniteByteString prefix, uint32_t prefix_offset, uint32_t prefix_len,
                           uint32_t infix_len, uint32_t slot_size);

        uint32_t GetSharedPrefixLen(const InfiniteByteString key_1,
                                    uint32_t start_bit_1,
                                    const uint8_t *key_2,
                                    uint32_t bit_count_2) const;
        uint32_t GetNumPrefixKeys() const;

    private:
        static constexpr uint32_t has_prefix_keys_bit_pos = 63;
        static constexpr uint32_t n_prefix_keys_bit_pos = 48;
        static constexpr uint32_t n_suffixes_bit_pos = 32;
        static constexpr uint32_t varlen_counter_encoding_fragment_length = 3;

        bool HasPrefixKeys() const;
        void SetHasPrefixKeys(bool has_prefix_keys);
        void UpdateNumPrefixKeys(int64_t delta);
        
        // Assumes word-aligned buffers
        // Returns the position of the first differring bit +1, multiplied by the usual
        // comparison sign.
        int32_t CompareStringToBitmap(const void *bitmap, uint32_t bitmap_pos,
                                      const InfiniteByteString str, uint32_t str_pos,
                                      uint32_t num_bits_to_compare) const;

        void AddBitsToTrie(uint64_t bits, uint32_t bit_count);
        void AddBitsToTrie(const void *bits, uint32_t bit_count, uint32_t bit_offset);
        void AddBitsFromBitmapToTrie(const void *bits, uint32_t bit_count, uint32_t bit_offset);
        void AddCounterToTrie(uint64_t counter);
        uint32_t GetCounterDigitCount(uint64_t counter) const;
        void AddSuffixToTrie(const InfiniteByteString suffix, uint32_t suffix_bit_count, uint32_t suffix_bit_pos,
                             uint32_t slot_size, uint32_t actual_suffix_len);
        uint32_t WriteSuffixToSuffixes(const InfiniteByteString suffix, uint32_t suffix_len, uint32_t suffix_offset,
                                       uint32_t write_pos, uint32_t actual_suffix_len, uint32_t slot_size);
        uint32_t GetSuffixBitPos(uint32_t suffix_rank, uint32_t slot_size,
                                 int32_t actual_suffix_len_=-1, uint32_t prev_suffix_bit_pos=0) const;
        uint32_t GetSuffixBitPos(const uint64_t *ptr, uint32_t suffix_rank, uint32_t slot_size,
                                 int32_t actual_suffix_len_, uint32_t prev_suffix_bit_pos) const;
        std::pair<uint32_t, uint32_t> GetSuffixLength(uint32_t suffix_bit_pos,
                                                      uint32_t slot_size,
                                                      int32_t actual_suffix_len_=-1) const;
        uint32_t GetSuffixString(uint32_t suffix_bit_pos, uint32_t slot_size,
                                 uint8_t *res, uint32_t res_bit_pos,
                                 int32_t actual_suffix_len_=-1) const;
        uint32_t GetSharedPrefixLen(const uint8_t *key_1,
                                    uint32_t bit_length_1,
                                    const uint8_t *key_2,
                                    uint32_t bit_length_2,
                                    uint32_t key_start_bit) const;
        void SwitchTrieEncoding(bool has_duplicates, uint32_t slot_size,
                                int32_t old_actual_suffix_len=-1);
        void AdjustActualSuffixLen(uint32_t old_actual_suffix_len,
                                   uint32_t new_actual_suffix_len,
                                   uint32_t slot_size);

        void BuildTrieRecurse(const InfiniteByteString *keys, uint32_t key_count,
                              uint32_t key_start_bit, uint32_t slot_size,
                              bool store_full_keys,
                              bool pad_short_keys,
                              std::vector<uint32_t>& suffix_offsets);
        void PrependPrefixToTrie(const InfiniteByteString prefix, uint32_t prefix_offset, uint32_t prefix_len);
    };

    uint32_t infix_size_;
    uint32_t payload_size_;
    wormhole *wh_;
    wormref *better_tree_;
    wormhole_int *wh_int_;
    wormref_int *better_tree_int_;
    std::mt19937 rng_;
    uint32_t rng_seed_;
    const float load_factor_ = 0.95;
    const float load_factor_alt_ = 0.95;
    const uint32_t size_scalar_shrink_grow_sep = std::log(infix_store_target_size / 64) / std::log(1 / load_factor_) + 1;
    uint64_t size_scalars_[size_scalar_count], scaled_sizes_[size_scalar_count], exception_scaled_size_;
    uint64_t implicit_scalars_[infix_store_target_size / 2 + 1];
    std::atomic<uint64_t> n_keys_ = 0;

    uint32_t bulk_load_streaming_ind_, bulk_load_streaming_max_len_;
    InfiniteByteString bulk_load_left_key_, bulk_load_key_list_[infix_store_target_size];
    uint64_t *bulk_load_left_payload_ = nullptr, *bulk_load_payload_list_ = nullptr;

    void AddTreeKey(const uint8_t *key, const uint32_t key_len, const uint64_t *payload=nullptr);
    void InsertSimple(const InfiniteByteString key, const void *payload=nullptr);
    uint32_t InsertSplit(const InfiniteByteString key, const void *payload=nullptr);
    void DeleteMerge(InfiniteByteString key);
    template <class t_itr>
    void BulkLoadFixedLength(t_itr begin, t_itr end, const uint32_t key_len, const uint64_t **payloads=nullptr);
    template <class t_itr>
    void BulkLoad(t_itr begin, t_itr end, const uint64_t **payloads=nullptr);
    void SetupScaleFactors();
    std::tuple<uint32_t, uint32_t, uint32_t> 
        GetSharedIgnoreImplicitLengths(const InfiniteByteString key_1,
                                       const InfiniteByteString key_2) const;
    uint64_t ExtractPartialKey(const InfiniteByteString key,
                               const uint32_t shared, const uint32_t ignore,
                               const uint32_t implicit_size, const uint64_t msb) const;
    void GetLowerUpperBounds(const InfiniteByteString& key, bool write, void *leaves[3],
                             wormhole_iter& it, wormhole_int_iter& it_int,
                             InfiniteByteString& prev_key, InfiniteByteString& next_key,
                             Diva<diva_type, payload_type>::InfixStore *& infix_store_ptr) const;
    void DeleteGetLowerMiddleUpperBounds(const InfiniteByteString& key, void *leaves[3],
                                   wormhole_iter& it, wormhole_int_iter& it_int,
                                   InfiniteByteString& left_key, InfiniteByteString& middle_key, InfiniteByteString& right_key,
                                   Diva<diva_type, payload_type>::InfixStore *& left_store_ptr,
                                   Diva<diva_type, payload_type>::InfixStore *& right_store_ptr) const;
    void OrderLeaves(void *leaves[3], uint32_t l, uint32_t r) const;
    void UnlockLeaves(void *leaves[3], bool write) const;

    uint32_t RankOccupieds(const InfixStore &store, const uint32_t pos) const;
    uint32_t SelectRunends(const InfixStore &store, const uint32_t rank) const;
    // Returns the total number of possible implicits if no next occupied
    int32_t NextOccupied(const InfixStore &store, const uint32_t pos) const;
    // Returns -1 if no previous occupied
    int32_t PreviousOccupied(const InfixStore &store, const uint32_t pos) const;
    // Returns the size of `store` if no next runend
    int32_t NextRunend(const InfixStore &store, const uint32_t pos) const;
    // Returns -1 if no previous runend
    int32_t PreviousRunend(const InfixStore &store, const uint32_t pos) const;

    int32_t GetMappedPos(const uint32_t implicit_part, const uint32_t size_grade, const uint64_t implicit_scalar) const;
    uint64_t GetSlot(const InfixStore &store, const uint32_t pos) const;
    void SetSlot(InfixStore &store, const uint32_t pos, const uint64_t value);
    void SetSlot(InfixStore &store, const uint32_t pos, const uint64_t value, const uint32_t width);
    void GetPayload(const InfixStore &store, const uint32_t pos, uint64_t *payload, const uint32_t payload_offset=0) const;
    void GetSamplePayload(const InfixStore &store, const uint32_t pos, uint64_t *payload, const uint32_t payload_offset=0) const;
    void SetPayload(InfixStore &store, const uint32_t pos, const uint64_t *payload, const uint32_t payload_offset=0);
    void AddSamplePayload(InfixStore &store, const void *payload, const uint32_t payload_offset=0);
    void RemoveSamplePayload(InfixStore &store, const uint32_t pos);

    bool GetOccupiedBit(const InfixStore &store, const uint32_t pos) const;
    void SetOccupiedBit(InfixStore &store, const uint32_t pos);
    void ResetOccupiedBit(InfixStore &store, const uint32_t pos);
    bool GetRunendBit(const InfixStore &store, const uint32_t pos) const;
    void SetRunendBit(InfixStore &store, const uint32_t pos);
    void ResetRunendBit(InfixStore &store, const uint32_t pos);

    void UpdatePopcnts(InfixStore& store);

    // Works with the inclusive-exclusive range [`l`, `r`).
    void ShiftSlotsRight(InfixStore &store, const uint32_t l, const uint32_t r, const uint32_t shamt);
    // Works with the inclusive-exclusive range [`l`, `r`).
    void ShiftSlotsLeft(InfixStore &store, const uint32_t l, const uint32_t r, const uint32_t shamt);
    // Works with the inclusive-exclusive range [`l`, `r`).
    void ShiftRunendsRight(InfixStore &store, const uint32_t l, const uint32_t r, const uint32_t shamt);
    // Works with the inclusive-exclusive range [`l`, `r`).
    void ShiftRunendsLeft(InfixStore &store, const uint32_t l, const uint32_t r, const uint32_t shamt);
    // Works with the inclusive-exclusive range [`l`, `r`).
    void ShiftPayloadsRight(InfixStore &store, const uint32_t l, const uint32_t r, const uint32_t shamt);
    // Works with the inclusive-exclusive range [`l`, `r`).
    void ShiftPayloadsLeft(InfixStore &store, const uint32_t l, const uint32_t r, const uint32_t shamt);

    // Works with the inclusive-exclusive range [`l`, `r`).
    void MoveSlotsRight(InfixStore &store, const uint32_t l, const uint32_t r, const uint32_t shamt);
    // Works with the inclusive-exclusive range [`l`, `r`).
    void MoveSlotsLeft(InfixStore &store, const uint32_t l, const uint32_t r, const uint32_t shamt);
    // Works with the inclusive-exclusive range [`l`, `r`).
    void MovePayloadsRight(InfixStore &store, const uint32_t l, const uint32_t r, const uint32_t shamt);
    // Works with the inclusive-exclusive range [`l`, `r`).
    void MovePayloadsLeft(InfixStore &store, const uint32_t l, const uint32_t r, const uint32_t shamt);

    // Works with the inclusive-exclusive range [`l`, `r`).
    void ZeroOutSlots(InfixStore &store, const uint32_t l, const uint32_t r);
    // Works with the inclusive-exclusive range [`l`, `r`).
    void ZeroOutPayloads(InfixStore &store, const uint32_t l, const uint32_t r);

    uint32_t MakeRoomFromSlot(InfixStore &store, const uint32_t pos, const uint32_t runend_pos,
                              const uint32_t num_slots, bool in_run=true);
    bool SlotHasTrie(const InfixStore &store, const uint32_t pos, const uint32_t runend_pos) const;
    std::pair<bool, uint32_t> FindInfixInRun(const InfixStore &store,
                                             const int32_t runstart_pos, const int32_t runend_pos, 
                                             const uint64_t explicit_part);

    int32_t FindEmptySlotAfter(const InfixStore &store, const uint32_t runend_pos) const;
    int32_t FindEmptySlotBefore(const InfixStore &store, const uint32_t runend_pos) const;
    void InsertRawIntoInfixStore(InfixStore &store, const uint64_t key,
                                 const uint32_t total_implicit=infix_store_target_size,
                                 const uint64_t *payload=nullptr,
                                 const InfiniteByteString original_key={nullptr, 0},
                                 const uint32_t original_key_start_bit=0);
    void AdaptRawInInfixStore(InfixStore &store, const uint64_t key,
                              const InfiniteByteString original_key,
                              const uint32_t original_key_start_bit,
                              const uint32_t adapt_length,
                              const uint32_t total_implicit=infix_store_target_size,
                              const uint64_t *payload=nullptr);
    // Assumes that `key` is a full length infix.
    void DeleteRawFromInfixStore(InfixStore &store, const uint64_t key,
                                 const uint32_t total_implicit=infix_store_target_size,
                                 std::function<bool(const uint64_t *)> should_remove=nullptr,
                                 const InfiniteByteString original_key={nullptr, 0},
                                 const uint32_t original_key_start_bit=0,
                                 bool take_first_match=false);
    // Returns the "next implicit" that should then be removed in the deletion
    // iterator, as well as the number of slots that were ultimately deleted.
    // Assumes that `l_key` and `r_key` are full length infixes. Removes
    // matching infixes that are guaranteed to lie within the range [`l_key`,
    // `r_key`].
    std::pair<uint64_t, uint32_t> DeleteRawRangeFromInfixStore(InfixStore &store,
                                                               const uint64_t l_key,
                                                               const uint64_t r_key, 
                                                               const uint32_t total_implicit=infix_store_target_size, 
                                                               std::function<bool(const uint64_t *)> should_remove=nullptr);
    // Assumes that `key` is a full length infix.
    int32_t GetLongestMatchingInfixSize(const InfixStore &store, const uint64_t key,
                                        const uint32_t total_implicit=infix_store_target_size,
                                        std::function<bool(const uint64_t*)> should_consider=nullptr,
                                        const InfiniteByteString original_key={nullptr, 0},
                                        const uint32_t original_key_start_bit=0) const;
    // Assumes that `l_key` and `r_key` are full length infixes. Returnes true
    // if there is an infix that may lie within the range [`l_key`, `r_key`].
    bool RangeQueryInfixStore(InfixStore &store, const uint64_t l_key, const uint64_t r_key,
                              const uint32_t total_implicit=infix_store_target_size,
                              const InfiniteByteString original_l_key={nullptr, 0},
                              const InfiniteByteString original_r_key={nullptr, 0},
                              const uint32_t original_key_start_bit=0) const;
    // Assumes that `key` is a full length infix. Returns true if there is an
    // infix that may match `key`.
    bool PointQueryInfixStore(InfixStore &store, const uint64_t key,
                              const uint32_t total_implicit=infix_store_target_size,
                              const InfiniteByteString original_key={nullptr, 0},
                              const uint32_t original_key_start_bit=0) const;
    // Resizes the infix store to the appropriate size based on its number of
    // elements.
    void ResizeInfixStore(InfixStore &store, const uint32_t total_implicit=infix_store_target_size);
    void LoadListToInfixStore(InfixStore &store, const uint64_t *list, const uint32_t list_len,
                              const uint32_t total_implicit=infix_store_target_size, const bool zero_out=false,
                              const uint64_t *payload_list=nullptr);
    void LoadVectorToInfixStore(InfixStore &store, const std::vector<Infix>& vec,
                                const uint32_t total_implicit=infix_store_target_size, const bool zero_out=false,
                                const uint64_t *payload_list=nullptr);
    InfixStore AllocateInfixStoreWithList(const uint64_t *list, const uint32_t list_len,
                                          const uint32_t total_implicit=infix_store_target_size,
                                          const uint64_t *payload_list=nullptr);
    InfixStore AllocateInfixStoreWithVector(const std::vector<Infix>& vec,
                                            const uint32_t total_implicit=infix_store_target_size,
                                            const uint64_t *payload_list=nullptr);
    uint32_t GetInfixList(const InfixStore &store, uint64_t *res, uint64_t *res_payload=nullptr) const;
    std::vector<Infix> GetInfixVector(const InfixStore &store, uint64_t *res_payload=nullptr) const;
    std::tuple<uint32_t, bool> GetExpandedInfixListLength(const uint64_t *list, const uint32_t list_len,
                                                          const uint32_t implicit_size, const uint32_t shamt,
                                                          const uint64_t lower_lim, const uint64_t upper_lim);
    void UpdateInfixList(const uint64_t *list, const uint32_t list_len, const uint32_t shamt, 
                         const uint64_t lower_lim, const uint64_t upper_lim,
                         uint64_t *res, const uint32_t res_len, 
                         const bool expanded,
                         const uint64_t *payload_list=nullptr, const uint32_t payload_list_offset=0,
                         uint64_t *res_payload=nullptr) const;
    std::vector<Infix> UpdateInfixVector(const std::vector<Infix>& infix_vec, const uint32_t shamt,
                                         const uint64_t lower_lim, const uint64_t upper_lim,
                                         const uint32_t implicit_size) const;
    void UpdateInfixListDelete(const uint32_t shared, const uint32_t ignore, const uint32_t implicit_size,
                               const InfiniteByteString left_key, const InfiniteByteString right_key,
                               uint64_t *infix_list, const uint32_t infix_list_len) const;
    std::vector<typename Diva<diva_type, payload_type>::Infix> UpdateInfixVectorDelete(const uint32_t shared,
                                                                                       const uint32_t ignore,
                                                                                       const uint32_t implicit_size,
                                                                                       const InfiniteByteString left_key,
                                                                                       const InfiniteByteString right_key, 
                                                                                       std::vector<Infix>& infix_vec) const;

    static bool CompareInfixes(uint64_t a, uint64_t b);

    uint32_t SerializeMetadata(char *out) const;
    uint64_t SerializeInfixStore(char *out, const InfixStore& store) const;
    uint32_t DeserializeMetadata(const char *deser_buf);
    uint32_t DeserializeInfixStore(const char *deser_buf, InfixStore& store) const;

public:
    Iterator GetIterator(std::string_view start="", std::string_view end="",
                         std::function<bool(const uint64_t *)> should_remove=nullptr);
    Iterator GetIterator(const uint8_t *start, const uint32_t start_len,
                         const uint8_t *end=nullptr, const uint32_t end_len=0,
                         std::function<bool(const uint64_t *)> should_remove=nullptr);
    Iterator GetIterator(uint64_t start,
                         uint64_t end=std::numeric_limits<uint64_t>::max(),
                         std::function<bool(const uint64_t *)> should_remove=nullptr);
};


template <DivaType diva_type, PayloadType payload_type>
inline Diva<diva_type, payload_type>::Diva(const uint32_t infix_size, const uint32_t rng_seed,
                                           const float load_factor, const uint32_t payload_size,
                                           const bool setup_start_end_samples):
            wh_(nullptr),
            better_tree_(nullptr),
            wh_int_(nullptr),
            better_tree_int_(nullptr),
            infix_size_(infix_size),
            payload_size_(0),
            rng_seed_(rng_seed),
            load_factor_(load_factor),
            load_factor_alt_(load_factor),
            size_scalar_shrink_grow_sep(std::log(infix_store_target_size / 64) / std::log(1 / load_factor) + 1),
            bulk_load_streaming_ind_(0) {
    if constexpr (diva_type == DivaType::Int) {
        wh_int_ = wh_int_create();
        better_tree_int_ = wh_int_ref(wh_int_);
    }
    else {
        wh_ = wh_create();
        better_tree_ = wh_ref(wh_);
    }
    if constexpr (payload_type == PayloadType::FixedLength) {
        payload_size_ = payload_size;
        bulk_load_left_payload_ = new uint64_t[(payload_size_ + 63) / 64 + 1];
        bulk_load_payload_list_ = new uint64_t[infix_store_target_size * ((payload_size_ + 63) / 64) + 1];
    }
    else if constexpr (payload_type == PayloadType::None)
        assert(payload_size == 0);

    rng_.seed(rng_seed_);
    SetupScaleFactors();

    if (setup_start_end_samples) {
        uint32_t key_len;
        if constexpr (diva_type == DivaType::Int)
            key_len = sizeof(uint64_t);
        else 
            key_len = 100;
        uint8_t key[key_len];
        memset(key, 0x00, key_len);
        AddTreeKey(key, key_len);
        memset(key, 0xFF, key_len);
        AddTreeKey(key, key_len);
    }
}


template <DivaType diva_type, PayloadType payload_type>
template <class t_itr>
Diva<diva_type, payload_type>::Diva(const uint32_t infix_size, const t_itr begin, const t_itr end, const uint32_t key_len,
                                    const uint32_t rng_seed, const float load_factor,
                                    const uint32_t payload_size, const uint64_t **payload_list):
        wh_(nullptr),
        better_tree_(nullptr),
        wh_int_(nullptr),
        better_tree_int_(nullptr),
        infix_size_(infix_size),
        payload_size_(0),
        rng_seed_(rng_seed),
        load_factor_(load_factor), 
        load_factor_alt_(load_factor),
        size_scalar_shrink_grow_sep(std::log(infix_store_target_size / 64) / std::log(1 / load_factor) + 1),
        bulk_load_streaming_ind_(0) {
    if constexpr (diva_type == DivaType::Int) {
        wh_int_ = wh_int_create();
        better_tree_int_ = wh_int_ref(wh_int_);
    }
    else {
        wh_ = wh_create();
        better_tree_ = wh_ref(wh_);
    }
    if constexpr (payload_type == PayloadType::FixedLength) {
        payload_size_ = payload_size;
        bulk_load_left_payload_ = new uint64_t[(payload_size_ + 63) / 64 + 1];
        bulk_load_payload_list_ = new uint64_t[infix_store_target_size * ((payload_size_ + 63) / 64) + 1];
    }
    else if constexpr (payload_type == PayloadType::None)
        assert(payload_size == 0);

    rng_.seed(rng_seed_);
    SetupScaleFactors();

    uint8_t key[key_len];
    memset(key, 0x00, key_len);
    AddTreeKey(key, key_len);
    memset(key, 0xFF, key_len);
    AddTreeKey(key, key_len);

    BulkLoadFixedLength(begin, end, key_len, payload_list);
}


template <DivaType diva_type, PayloadType payload_type>
template <class t_itr>
Diva<diva_type, payload_type>::Diva(const uint32_t infix_size, const t_itr begin, const t_itr end, 
                                    const uint32_t rng_seed, const float load_factor,
                                    const uint32_t payload_size, const uint64_t **payload_list):
        wh_(nullptr),
        better_tree_(nullptr),
        wh_int_(nullptr),
        better_tree_int_(nullptr),
        infix_size_(infix_size),
        payload_size_(0),
        rng_seed_(rng_seed),
        load_factor_(load_factor),
        load_factor_alt_(load_factor),
        size_scalar_shrink_grow_sep(std::log(infix_store_target_size / 64) / std::log(1 / load_factor) + 1),
        bulk_load_streaming_ind_(0) {
    if constexpr (diva_type == DivaType::Int) {
        wh_int_ = wh_int_create();
        better_tree_int_ = wh_int_ref(wh_int_);
    }
    else {
        wh_ = wh_create();
        better_tree_ = wh_ref(wh_);
    }
    if constexpr (payload_type == PayloadType::FixedLength)
        payload_size_ = payload_size;
    else if constexpr (payload_type == PayloadType::None)
        assert(payload_size == 0);

    rng_.seed(rng_seed_);
    SetupScaleFactors();

    uint8_t key[8];
    memset(key, 0x00, 8);
    AddTreeKey(key, 8);

    BulkLoad(begin, end, payload_list);
}



template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::SetupScaleFactors() {
    double pw = 1.0;
    for (int32_t i = size_scalar_shrink_grow_sep - 1; i >= 0; i--) {
        size_scalars_[i] = static_cast<uint64_t>(pw * (1ULL << scale_shift));
        scaled_sizes_[i] = infix_store_target_size * size_scalars_[i] >> scale_shift;
        pw *= load_factor_alt_;
    }
    exception_scaled_size_ = static_cast<uint64_t>(scaled_sizes_[0] * load_factor_alt_);
    pw = 1.0 / load_factor_;
    const int32_t loop_end_i = std::min<int32_t>(size_scalar_count,
                                                 std::log(std::numeric_limits<uint32_t>::max()) / std::log(1 / load_factor_));
    for (int32_t i = size_scalar_shrink_grow_sep; i < loop_end_i; i++) {
        size_scalars_[i] = static_cast<uint64_t>(pw * (1ULL << scale_shift));
        scaled_sizes_[i] = infix_store_target_size * size_scalars_[i] >> scale_shift;
        pw /= load_factor_;
    }
    for (int32_t i = loop_end_i; i < size_scalar_count; i++) {
        size_scalars_[i] = std::numeric_limits<uint64_t>::max();
        scaled_sizes_[i] = std::numeric_limits<uint64_t>::max();
    }
    
    for (int32_t i = 0; i < infix_store_target_size / 2; i++) {
        const double ratio = static_cast<double>(infix_store_target_size) 
                                / static_cast<double>(i + static_cast<double>(infix_store_target_size) / 2);
        implicit_scalars_[i] = static_cast<uint64_t>(ratio * (1ULL << scale_implicit_shift));
    }
    implicit_scalars_[infix_store_target_size / 2] = 1ULL << scale_implicit_shift;
}


template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::GetLowerUpperBounds(const InfiniteByteString& key, bool write, void *leaves[3],
                                                               wormhole_iter& it, wormhole_int_iter& it_int,
                                                               InfiniteByteString& prev_key, InfiniteByteString& next_key,
                                                               Diva<diva_type, payload_type>::InfixStore *& infix_store_ptr) const {
    const bool unlock = false;
GetLowerUpperBoundsRetry:
    uint32_t l_ind = 0, r_ind = 0;
    InfixStore *dummy_infix_store_ptr;
    uint32_t dummy_val;
    if constexpr (diva_type == DivaType::Int) {
        uint32_t exp_backoff = 2;
        it_int.ref = better_tree_int_;
        it_int.map = better_tree_int_->map;
        it_int.leaf = nullptr;
        it_int.is = 0;
        wh_int_iter_seek(&it_int, key.str, key.length, write);
        wh_int_iter_peek_ref(&it_int, reinterpret_cast<const void **>(&next_key.str), &next_key.length,
                                      reinterpret_cast<void **>(&dummy_infix_store_ptr), &dummy_val);
        leaves[r_ind++] = it_int.leaf;

        if (next_key == key) {
            infix_store_ptr = dummy_infix_store_ptr;
            prev_key = next_key;
            wh_int_iter_skip1(&it_int, write, unlock);
            if (wh_int_iter_valid(&it_int)) {
                wh_int_iter_peek_ref(&it_int, reinterpret_cast<const void **>(&next_key.str), &next_key.length,
                                              reinterpret_cast<void **>(&dummy_infix_store_ptr), &dummy_val);
                if (leaves[r_ind - 1] != it_int.leaf) {
                    leaves[r_ind] = it_int.leaf;
                    std::swap(leaves[r_ind - 1], leaves[r_ind]);
                    r_ind++;
                }
            }
            else 
                next_key = {nullptr, 0};
        }
        else {
            do {
                if (!wh_int_iter_skip1_rev(&it_int, write, unlock)) {
                    OrderLeaves(leaves, l_ind, r_ind);
                    UnlockLeaves(leaves, write);
                    for (uint32_t i = 0; i < exp_backoff; i++)
                        cpu_pause();
                    exp_backoff = (exp_backoff + (exp_backoff >> 1)) & max_exp_backoff;
                    goto GetLowerUpperBoundsRetry;
                }
                wh_int_iter_peek_ref(&it_int, reinterpret_cast<const void **>(&prev_key.str), &prev_key.length,
                                              reinterpret_cast<void **>(&infix_store_ptr), &dummy_val);

                const uint32_t prev_r_ind = (r_ind == 0 ? 2 : r_ind - 1);
                if (it_int.leaf != leaves[prev_r_ind]) {
                    leaves[r_ind++] = it_int.leaf;
                    r_ind = (r_ind >= 3 ? 0 : r_ind);
                    if (!unlock && l_ind == r_ind) {
                        if (write)
                            wormleaf_int_unlock_write(reinterpret_cast<struct wormleaf_int *>(leaves[l_ind]));
                        else
                            wormleaf_int_unlock_read(reinterpret_cast<struct wormleaf_int *>(leaves[l_ind]));
                        leaves[l_ind++] = nullptr;
                        l_ind = (l_ind >= 3 ? 0 : l_ind);
                    }
                }

                if (prev_key > key)
                    next_key = prev_key;
            } while (prev_key > key);
        }
    }
    else {
        it.ref = better_tree_;
        it.map = better_tree_->map;
        it.leaf = nullptr;
        it.is = 0;
        wh_iter_seek_pred(&it, key.str, key.length, write);
        while (true) {
            if (!unlock) {
                const uint32_t prev_l_ind = l_ind == 2 ? 0 : l_ind + 1;
                if (it.leaf != leaves[prev_l_ind]) {
                    leaves[l_ind] = it.leaf;
                    l_ind = l_ind == 0 ? 2 : l_ind - 1;
                    if (r_ind == l_ind) {
                        if (write)
                            wormleaf_unlock_write(reinterpret_cast<struct wormleaf *>(leaves[r_ind]));
                        else 
                            wormleaf_unlock_read(reinterpret_cast<struct wormleaf *>(leaves[r_ind]));
                        leaves[r_ind] = nullptr;
                        r_ind = r_ind == 0 ? 2 : r_ind - 1;
                    }
                }
                else if ((l_ind == 0 ? 2 : l_ind - 1) == r_ind) {
                    if (write)
                        wormleaf_unlock_write(reinterpret_cast<struct wormleaf *>(leaves[r_ind]));
                    else 
                        wormleaf_unlock_read(reinterpret_cast<struct wormleaf *>(leaves[r_ind]));
                    leaves[r_ind] = nullptr;
                    r_ind = r_ind == 0 ? 2 : r_ind - 1;
                }
            }

            if (wh_iter_valid(&it)) {
                wh_iter_peek_ref(&it, reinterpret_cast<const void **>(&next_key.str), &next_key.length,
                                      reinterpret_cast<void **>(&dummy_infix_store_ptr), &dummy_val);
            }
            else {
                next_key = {nullptr, 0};
                break;
            }

            if (key < next_key)
                break;
            prev_key = next_key;
            infix_store_ptr = dummy_infix_store_ptr;
            wh_iter_skip1(&it, write, unlock);
        }
        // Increment to make sure `l_ind` points to the first pointer
        l_ind = l_ind == 2 ? 0 : l_ind + 1;
        r_ind = r_ind == 2 ? 0 : r_ind + 1;
    }

    if (!unlock)
        OrderLeaves(leaves, l_ind, r_ind);

#ifdef DEBUG
    assert(prev_key <= key);
    assert(next_key.str == nullptr || (key < next_key || prev_key == next_key));
#endif // DEBUG
}


template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::DeleteGetLowerMiddleUpperBounds(const InfiniteByteString& key, void *leaves[3],
                                                                           wormhole_iter& it, wormhole_int_iter& it_int,
                                                                           InfiniteByteString& left_key,
                                                                           InfiniteByteString& middle_key,
                                                                           InfiniteByteString& right_key,
                                                                           Diva<diva_type, payload_type>::InfixStore *& left_store_ptr,
                                                                           Diva<diva_type, payload_type>::InfixStore *& right_store_ptr) const {
    const bool write = true, unlock = false;
    uint32_t exp_backoff = 1;

GetLowerMiddleUpperBoundsRetry:
    uint32_t l_ind = 0, r_ind = 0;
    InfixStore *dummy_infix_store_ptr;
    uint32_t dummy_val;
    if constexpr (diva_type == DivaType::Int) {
        it_int.ref = better_tree_int_;
        it_int.map = better_tree_int_->map;
        it_int.leaf = nullptr;
        it_int.is = 0;
        wh_int_iter_seek(&it_int, key.str, key.length, write);
        wh_int_iter_peek_ref(&it_int, reinterpret_cast<const void **>(&middle_key.str), &middle_key.length,
                                      reinterpret_cast<void **>(&right_store_ptr), &dummy_val);
        leaves[r_ind++] = it_int.leaf;

        assert(middle_key.length == key.length && memcmp(middle_key.str, key.str, key.length) == 0);
        struct wormleaf_int * const middle_key_leaf = it_int.leaf;
        const int middle_key_is = it_int.is;

        if (!wh_int_iter_skip1_rev(&it_int, write, unlock)) {
            UnlockLeaves(leaves, write);
            for (uint32_t i = 0; i < exp_backoff; i++)
                cpu_pause();
            exp_backoff = ((exp_backoff << 1) | 1) & BITMASK(8 * sizeof(exp_backoff) - 1);
            goto GetLowerMiddleUpperBoundsRetry;
        }
        wh_int_iter_peek_ref(&it_int, reinterpret_cast<const void **>(&left_key.str), &left_key.length,
                                      reinterpret_cast<void **>(&left_store_ptr), &dummy_val);

        if (it_int.leaf != leaves[0])
            leaves[r_ind++] = it_int.leaf;

        it_int.leaf = middle_key_leaf;
        it_int.is = middle_key_is;
        wh_int_iter_skip1(&it_int, write, unlock);
        wh_int_iter_peek_ref(&it_int, reinterpret_cast<const void **>(&right_key.str), &right_key.length,
                                      reinterpret_cast<void **>(&dummy_infix_store_ptr), &dummy_val);
        if (it_int.leaf != leaves[0]) {
            for (int32_t i = r_ind - 1; i >= 0; i--)
                leaves[i + 1] = leaves[i];
            leaves[0] = it_int.leaf;
            r_ind++;
        }
    }
    else {
        it.ref = better_tree_;
        it.map = better_tree_->map;
        it.leaf = nullptr;
        it.is = 0;
        wh_iter_seek_pred_strict(&it, key.str, key.length, write);
        while (true) {
            if (!unlock) {
                const uint32_t prev_l_ind = l_ind == 2 ? 0 : l_ind + 1;
                if (it.leaf != leaves[prev_l_ind]) {
                    leaves[l_ind] = it.leaf;
                    l_ind = l_ind == 0 ? 2 : l_ind - 1;
                    if (r_ind == l_ind) {
                        if (write)
                            wormleaf_unlock_write(reinterpret_cast<struct wormleaf *>(leaves[r_ind]));
                        else 
                            wormleaf_unlock_read(reinterpret_cast<struct wormleaf *>(leaves[r_ind]));
                        leaves[r_ind] = nullptr;
                        r_ind = r_ind == 0 ? 2 : r_ind - 1;
                    }
                }
                else if ((l_ind == 0 ? 2 : l_ind - 1) == r_ind) {
                    if (write)
                        wormleaf_unlock_write(reinterpret_cast<struct wormleaf *>(leaves[r_ind]));
                    else 
                        wormleaf_unlock_read(reinterpret_cast<struct wormleaf *>(leaves[r_ind]));
                    leaves[r_ind] = nullptr;
                    r_ind = r_ind == 0 ? 2 : r_ind - 1;
                }
            }

            if (wh_iter_valid(&it)) {
                wh_iter_peek_ref(&it, reinterpret_cast<const void **>(&middle_key.str), &middle_key.length,
                                      reinterpret_cast<void **>(&right_store_ptr), &dummy_val);
            }
            else {
                middle_key = {nullptr, 0};
                break;
            }

            if (key <= middle_key) {
                assert(key == middle_key);
                break;
            }
            left_key = middle_key;
            left_store_ptr = right_store_ptr;
            wh_iter_skip1(&it, write, unlock);
        }
        wh_iter_skip1(&it, write, unlock);
        wh_iter_peek_ref(&it, reinterpret_cast<const void **>(&right_key.str), &right_key.length,
                              reinterpret_cast<void **>(&dummy_infix_store_ptr), &dummy_val);
        if (!unlock) {
            const uint32_t prev_l_ind = l_ind == 2 ? 0 : l_ind + 1;
            if (it.leaf != leaves[prev_l_ind]) {
                leaves[l_ind] = it.leaf;
                l_ind = l_ind == 0 ? 2 : l_ind - 1;
                if (r_ind == l_ind) {
                    if (write)
                        wormleaf_unlock_write(reinterpret_cast<struct wormleaf *>(leaves[r_ind]));
                    else 
                        wormleaf_unlock_read(reinterpret_cast<struct wormleaf *>(leaves[r_ind]));
                    leaves[r_ind] = nullptr;
                    r_ind = r_ind == 0 ? 2 : r_ind - 1;
                }
            }
            else if ((l_ind == 0 ? 2 : l_ind - 1) == r_ind) {
                if (write)
                    wormleaf_unlock_write(reinterpret_cast<struct wormleaf *>(leaves[r_ind]));
                else 
                    wormleaf_unlock_read(reinterpret_cast<struct wormleaf *>(leaves[r_ind]));
                leaves[r_ind] = nullptr;
                r_ind = r_ind == 0 ? 2 : r_ind - 1;
            }
        }
        // Increment to make sure `l_ind` points to the first pointer
        l_ind = l_ind == 2 ? 0 : l_ind + 1;
        r_ind = r_ind == 2 ? 0 : r_ind + 1;
        OrderLeaves(leaves, l_ind, r_ind);
    }
}


template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::OrderLeaves(void *leaves[3], uint32_t l, uint32_t r) const {
    const uint32_t l_inc = l == 2 ? 0 : l + 1;
    if (l_inc == r) {
        leaves[0] = leaves[l];
        leaves[1] = leaves[2] = nullptr;
    }
    else {
        void *tmp[2];
        tmp[0] = leaves[l];
        tmp[1] = leaves[l_inc];
        leaves[0] = tmp[0];
        leaves[1] = tmp[1];
        leaves[2] = nullptr;
    }
}

template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::UnlockLeaves(void *leaves[3], bool write) const {
    for (uint32_t i = 0; i < 3; i++) {
        if (leaves[i] == nullptr)
            continue;
        if (write) {
            if constexpr (diva_type == DivaType::Int)
                wormleaf_int_unlock_write(reinterpret_cast<struct wormleaf_int *>(leaves[i]));
            else
                wormleaf_unlock_write(reinterpret_cast<struct wormleaf *>(leaves[i]));
        }
        else {
            if constexpr (diva_type == DivaType::Int)
                wormleaf_int_unlock_read(reinterpret_cast<struct wormleaf_int *>(leaves[i]));
            else
                wormleaf_unlock_read(reinterpret_cast<struct wormleaf *>(leaves[i]));
        }
        leaves[i] = nullptr;
    }
}


template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::Insert(uint64_t key,
                                                  const void *payload,
                                                  uint32_t random_number) {
    key = __builtin_bswap64(key);
    Insert(reinterpret_cast<const uint8_t *>(&key), sizeof(key), payload, random_number);
}


template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::Insert(std::string_view key,
                                                  const void *payload,
                                                  uint32_t random_number) {
    Insert(reinterpret_cast<const uint8_t *>(key.data()), key.size(), payload, random_number);
}


template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::Insert(const uint8_t *key, const uint32_t key_len,
                                                  const void *payload,
                                                  uint32_t random_number) {
    const InfiniteByteString converted_key {key, static_cast<uint32_t>(key_len)};
    random_number = random_number == 0 ? rng_() : random_number;
    uint32_t num_keys_added = 1;
    if (random_number % infix_store_target_size == 0)
        num_keys_added += InsertSplit(converted_key, payload);
    else 
        InsertSimple(converted_key, payload);
    n_keys_.fetch_add(num_keys_added, std::memory_order_release);
}

template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::InsertSimple(const InfiniteByteString key,
                                                        const void *payload) {
    payload = payload_type != PayloadType::FixedLength ? nullptr : payload;
    const bool it_write_lock = false;
    InfixStore *infix_store_ptr;
    void *leaves_to_unlock[3] = {};

    InfiniteByteString next_key {};
    InfiniteByteString prev_key {};

    wormhole_int_iter it_int;
    wormhole_iter it;
    GetLowerUpperBounds(key, it_write_lock, leaves_to_unlock, it, it_int,
                        prev_key, next_key, infix_store_ptr);
    uint64_t prev_key_word, next_key_word;
    if constexpr (diva_type == DivaType::Int) {
        prev_key_word = *reinterpret_cast<const uint64_t *>(prev_key.str);
        prev_key.str = reinterpret_cast<const uint8_t *>(&prev_key_word);
        next_key_word = *reinterpret_cast<const uint64_t *>(next_key.str);
        next_key.str = reinterpret_cast<const uint8_t *>(&next_key_word);
    }

    InfixStore& infix_store = *infix_store_ptr;
    rwlock_lock_write(infix_store.rwlock);
    UnlockLeaves(leaves_to_unlock, it_write_lock);

    if constexpr (payload_type == PayloadType::FixedLength) {
        if (prev_key == key) {
            // Add new sample payload
            AddSamplePayload(infix_store, payload);
            rwlock_unlock_write(infix_store.rwlock);
            return;
        }
    }

    auto [shared, ignore, implicit_size] = GetSharedIgnoreImplicitLengths(prev_key, next_key);
    // To insert into the binary tries
    const uint32_t key_start_bit = shared + ignore + implicit_size;

    const uint64_t extraction = ExtractPartialKey(key, shared, ignore, implicit_size, key.GetBit(shared));
    const uint64_t next_implicit = ExtractPartialKey(next_key, shared, ignore, implicit_size, 1) >> infix_size_;
    const uint64_t prev_implicit = ExtractPartialKey(prev_key, shared, ignore, implicit_size, 0) >> infix_size_;
    const uint32_t total_implicit = next_implicit - prev_implicit + 1;
    const uint64_t insertee = ((extraction | 1ULL) - (prev_implicit << infix_size_));
    InsertRawIntoInfixStore(infix_store, insertee, total_implicit, 
                            reinterpret_cast<const uint64_t *>(payload),
                            {key.str, 8 * key.length},
                            key_start_bit);
    rwlock_unlock_write(infix_store.rwlock);
}


template <DivaType diva_type, PayloadType payload_type>
inline bool Diva<diva_type, payload_type>::RangeQuery(uint64_t l, uint64_t r) const {
    l = __builtin_bswap64(l);
    r = __builtin_bswap64(r);
    return RangeQuery(reinterpret_cast<const uint8_t *>(&l), sizeof(l),
                      reinterpret_cast<const uint8_t *>(&r), sizeof(r));
}


template <DivaType diva_type, PayloadType payload_type>
inline bool Diva<diva_type, payload_type>::RangeQuery(std::string_view input_l, std::string_view input_r) const {
    return RangeQuery(reinterpret_cast<const uint8_t *>(input_l.data()), input_l.size(),
                      reinterpret_cast<const uint8_t *>(input_r.data()), input_r.size());
}


template <DivaType diva_type, PayloadType payload_type>
inline bool Diva<diva_type, payload_type>::RangeQuery(const uint8_t *input_l, const uint32_t input_l_len,
                                                      const uint8_t *input_r, const uint32_t input_r_len) const {
    const bool it_write_lock = false;
    const InfiniteByteString l_key {input_l, static_cast<uint32_t>(input_l_len)};
    const InfiniteByteString r_key {input_r, static_cast<uint32_t>(input_r_len)};

    InfixStore *infix_store_ptr;
    void *leaves_to_unlock[3] = {};
    InfiniteByteString next_key {};
    InfiniteByteString prev_key {};

    wormhole_int_iter it_int;
    wormhole_iter it;
    GetLowerUpperBounds(l_key, it_write_lock, leaves_to_unlock, it, it_int,
                        prev_key, next_key, infix_store_ptr);
    uint64_t prev_key_word, next_key_word;
    if constexpr (diva_type == DivaType::Int) {
        prev_key_word = *reinterpret_cast<const uint64_t *>(prev_key.str);
        prev_key.str = reinterpret_cast<const uint8_t *>(&prev_key_word);
        if (next_key.str != nullptr) {
            next_key_word = *reinterpret_cast<const uint64_t *>(next_key.str);
            next_key.str = reinterpret_cast<const uint8_t *>(&next_key_word);
        }
    }

    if (prev_key == l_key || (next_key.str != nullptr && next_key <= r_key)) {
        UnlockLeaves(leaves_to_unlock, it_write_lock);
        return true;
    }
    else if (next_key.str == nullptr) {
        UnlockLeaves(leaves_to_unlock, it_write_lock);
        return false;
    }
    
    InfixStore& infix_store = *infix_store_ptr;
    rwlock_lock_read(infix_store.rwlock);
    UnlockLeaves(leaves_to_unlock, it_write_lock);

    auto [shared, ignore, implicit_size] = GetSharedIgnoreImplicitLengths(prev_key, next_key);
    // To query the binary tries
    const uint32_t key_start_bit = shared + ignore + implicit_size;

    if constexpr (diva_type == DivaType::Int) {
        const uint64_t l_key_int = __builtin_bswap64(*((uint64_t *) l_key.str));
        const uint64_t r_key_int = __builtin_bswap64(*((uint64_t *) r_key.str));
        const uint64_t prev_key_int = __builtin_bswap64(*((uint64_t *) prev_key.str));
        const uint64_t next_key_int = __builtin_bswap64(*((uint64_t *) next_key.str));

        const uint32_t shamt_left = std::max<int>(0, shared + ignore + implicit_size + infix_size_ - 64);
        const uint32_t shamt_right = std::max<int>(0, 64 - shared - ignore - implicit_size - infix_size_);
        const uint32_t shamt_left_impl = std::max<int>(0, shared + ignore + implicit_size - 64);
        const uint32_t shamt_right_impl = std::max<int>(0, 64 - shared - ignore - implicit_size);

        const uint64_t l_extraction = (((l_key_int >> (63 - shared)) & 1) << (implicit_size - 1 + infix_size_)) 
                                    | (((l_key_int << shamt_left) >> shamt_right) & BITMASK(implicit_size - 1 + infix_size_));
        const uint64_t r_extraction = (((r_key_int >> (63 - shared)) & 1) << (implicit_size - 1 + infix_size_)) 
                                    | (((r_key_int << shamt_left) >> shamt_right) & BITMASK(implicit_size - 1 + infix_size_));

        const uint64_t prev_implicit = ((prev_key_int << shamt_left_impl) >> shamt_right_impl) & BITMASK(implicit_size - 1);
        const uint64_t next_implicit = (1ULL << (implicit_size - 1)) 
                                    | (((next_key_int << shamt_left_impl) >> shamt_right_impl) & BITMASK(implicit_size - 1));
        const uint32_t total_implicit = next_implicit - prev_implicit + 1;
        const uint64_t l_val = (l_extraction | 1ULL) - (prev_implicit << infix_size_);
        const uint64_t r_val = (r_extraction | 1ULL) - (prev_implicit << infix_size_);
        const bool res = RangeQueryInfixStore(infix_store, l_val, r_val, total_implicit,
                                              {l_key.str, 8 * l_key.length},
                                              {r_key.str, 8 * r_key.length},
                                              key_start_bit);

        rwlock_unlock_read(infix_store.rwlock);
        return res;
    }
    else {
        const uint64_t l_extraction = ExtractPartialKey(l_key, shared, ignore, implicit_size, l_key.GetBit(shared));
        const uint64_t r_extraction = ExtractPartialKey(r_key, shared, ignore, implicit_size, r_key.GetBit(shared));
        const uint64_t prev_implicit = ExtractPartialKey(prev_key, shared, ignore, implicit_size, 0) >> infix_size_;
        const uint64_t next_implicit = ExtractPartialKey(next_key, shared, ignore, implicit_size, 1) >> infix_size_;
        const uint32_t total_implicit = next_implicit - prev_implicit + 1;
        const uint64_t l_val = (l_extraction | 1ULL) - (prev_implicit << infix_size_);
        const uint64_t r_val = (r_extraction | 1ULL) - (prev_implicit << infix_size_);
        const bool res = RangeQueryInfixStore(infix_store, l_val, r_val, total_implicit,
                                              {l_key.str, 8 * l_key.length},
                                              {r_key.str, 8 * r_key.length},
                                              key_start_bit);

        rwlock_unlock_read(infix_store.rwlock);
        return res;
    }
}


template <DivaType diva_type, PayloadType payload_type>
inline bool Diva<diva_type, payload_type>::PointQuery(uint64_t key) const {
    key = __builtin_bswap64(key);
    return PointQuery(reinterpret_cast<const uint8_t *>(&key), sizeof(key));
}


template <DivaType diva_type, PayloadType payload_type>
inline bool Diva<diva_type, payload_type>::PointQuery(std::string_view key) const {
    return PointQuery(reinterpret_cast<const uint8_t *>(key.data()), key.size());
}


template <DivaType diva_type, PayloadType payload_type>
inline bool Diva<diva_type, payload_type>::PointQuery(const uint8_t *input_key, const uint32_t key_len) const {
    const bool it_write_lock = false;
    const InfiniteByteString key {input_key, static_cast<uint32_t>(key_len)};
    
    InfixStore *infix_store_ptr;
    void *leaves_to_unlock[3] = {};

    InfiniteByteString next_key {};
    InfiniteByteString prev_key {};

    wormhole_int_iter it_int;
    wormhole_iter it;
    GetLowerUpperBounds(key, it_write_lock, leaves_to_unlock, it, it_int,
                        prev_key, next_key, infix_store_ptr);
    uint64_t prev_key_word, next_key_word;
    if constexpr (diva_type == DivaType::Int) {
        prev_key_word = *reinterpret_cast<const uint64_t *>(prev_key.str);
        prev_key.str = reinterpret_cast<const uint8_t *>(&prev_key_word);
        if (next_key.str != nullptr) {
            next_key_word = *reinterpret_cast<const uint64_t *>(next_key.str);
            next_key.str = reinterpret_cast<const uint8_t *>(&next_key_word);
        }
    }

    InfixStore& infix_store = *infix_store_ptr;
    rwlock_lock_read(infix_store.rwlock);
    UnlockLeaves(leaves_to_unlock, it_write_lock);

    if (prev_key == key) {
        // Previous key matches the query key
        rwlock_unlock_read(infix_store.rwlock);
        return true;
    }
    else if (next_key.str == nullptr) {
        rwlock_unlock_read(infix_store.rwlock);
        return false;
    }

    auto [shared, ignore, implicit_size] = GetSharedIgnoreImplicitLengths(prev_key, next_key);
    // To query the binary tries
    const uint32_t key_start_bit = shared + ignore + implicit_size;

    const uint64_t extraction = ExtractPartialKey(key, shared, ignore, implicit_size, key.GetBit(shared));
    const uint64_t prev_implicit = ExtractPartialKey(prev_key, shared, ignore, implicit_size, 0) >> infix_size_;
    const uint64_t next_implicit = ExtractPartialKey(next_key, shared, ignore, implicit_size, 1) >> infix_size_;
    const uint32_t total_implicit = next_implicit - prev_implicit + 1;
    const uint64_t query_key = extraction - (prev_implicit << infix_size_);
    const bool res = PointQueryInfixStore(infix_store, query_key, total_implicit,
                                          {key.str, 8 * key.length},
                                          key_start_bit);

    rwlock_unlock_read(infix_store.rwlock);
    return res;
}


template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::AddTreeKey(const uint8_t *key, const uint32_t key_len, const uint64_t *payload) {
    InfixStore infix_store(scaled_sizes_[size_scalar_shrink_grow_sep], infix_size_,
                           size_scalar_shrink_grow_sep, payload_size_);
    if constexpr (payload_type == PayloadType::FixedLength) {
        if (payload)
            AddSamplePayload(infix_store, payload);
    }
    void *dummy_locked_leaf_addrs[3] = {nullptr, nullptr, nullptr};
    if constexpr (diva_type == DivaType::Int)
        wh_int_put(better_tree_int_, key, key_len, &infix_store, sizeof(infix_store), dummy_locked_leaf_addrs);
    else
        wh_put(better_tree_, key, key_len, &infix_store, sizeof(infix_store), dummy_locked_leaf_addrs);
}


template <DivaType diva_type, PayloadType payload_type>
inline uint32_t Diva<diva_type, payload_type>::InsertSplit(const InfiniteByteString key,
                                                           const void *payload) {
    const bool it_write_lock = true;
    InfixStore *infix_store_ptr;
    void *leaves_to_unlock[3] = {};

#ifdef DEBUG
    if (key.str[0] == 0b11001110 && key.str[1] == 0b10100111 && key.str[2] == 0b11110101 && key.str[3] == 0b10100111 && key.str[4] == 0b11010110 && key.str[5] == 0b00011101 && key.str[6] == 0b01011000 && key.str[7] == 0b00001111)
        std::cerr << "welp, here we are" << std::endl;
#endif // DEBUG


    InfiniteByteString next_key {};
    InfiniteByteString prev_key {};

    wormhole_int_iter it_int;
    wormhole_iter it;
    GetLowerUpperBounds(key, it_write_lock, leaves_to_unlock, it, it_int,
                        prev_key, next_key, infix_store_ptr);
    uint64_t prev_key_word, next_key_word;
    if constexpr (diva_type == DivaType::Int) {
        prev_key_word = *reinterpret_cast<const uint64_t *>(prev_key.str);
        prev_key.str = reinterpret_cast<const uint8_t *>(&prev_key_word);
        next_key_word = *reinterpret_cast<const uint64_t *>(next_key.str);
        next_key.str = reinterpret_cast<const uint8_t *>(&next_key_word);
    }

    rwlock_lock_write(infix_store_ptr->rwlock);
    InfixStore& infix_store = *infix_store_ptr;

    if (prev_key == key) {
        if constexpr (payload_type == PayloadType::FixedLength) {
            // Add new sample payload
            AddSamplePayload(infix_store, payload);
            rwlock_unlock_write(infix_store.rwlock);
            UnlockLeaves(leaves_to_unlock, it_write_lock);
            return 0;
        }
        else {
            rwlock_unlock_write(infix_store.rwlock);
            UnlockLeaves(leaves_to_unlock, it_write_lock);
            InsertSimple(key);
            return 0;
        }
    }

    auto [shared, ignore, implicit_size] = GetSharedIgnoreImplicitLengths(prev_key, next_key);
    uint64_t extraction = ExtractPartialKey(key, shared, ignore, implicit_size, key.GetBit(shared));
    uint64_t prev_extraction = ExtractPartialKey(prev_key, shared, ignore, implicit_size, 0);
    uint64_t next_extraction = ExtractPartialKey(next_key, shared, ignore, implicit_size, 1);
    const uint64_t separator = (extraction | 1ULL) - (prev_extraction & (BITMASK(implicit_size) << infix_size_));

    if constexpr (diva_type == DivaType::BinaryTrie) {  // Handle the binary trie case
        auto infix_vec = GetInfixVector(infix_store);

        // Get the contents of the left and right Infix Stores
        std::vector<Infix> left_infix_vec_init, right_infix_vec_init;
        uint32_t num_duplicates = 0;
        for (auto& infix : infix_vec) {
            const uint64_t infix_val = infix.infix_ & (infix.infix_ - 1);
            const uint64_t mask = ((infix.infix_ & (-infix.infix_)) << 1) - 1;
            if ((infix_val | mask) == (separator | mask)) {
                if (infix.num_trie_bits_ > 0) {     // Check and split trie
                    const InfiniteByteString key_trie = {key.str, 8 * key.length};
                    const auto [trie_keys, trie_key_contents] = infix.GetStrings(infix_size_);
                    std::vector<InfiniteByteString> left_trie_keys, right_trie_keys;
                    for (auto& trie_key : trie_keys) {
                        const uint32_t shared_prefix_len = infix.GetSharedPrefixLen(key_trie,
                                shared + ignore + implicit_size + infix_size_ - 1,
                                reinterpret_cast<const uint8_t*>(trie_key.str),
                                trie_key.length);
                        if (shared_prefix_len >= trie_key.length) {
                            left_trie_keys.push_back(trie_key);
                            right_trie_keys.push_back(trie_key);
                            num_duplicates++;
                        }
                        else if (trie_key.GetBitBitLength(shared_prefix_len) == 0)
                            left_trie_keys.push_back(trie_key);
                        else 
                            right_trie_keys.push_back(trie_key);
                    }
                    if (!left_trie_keys.empty()) {
                        left_infix_vec_init.emplace_back(infix.infix_);
                        left_infix_vec_init.back().BuildTrieAndSuffixes(left_trie_keys.data(), left_trie_keys.size(),
                                0, infix_size_, false, true, false);
                    }
                    if (!right_trie_keys.empty()) {
                        right_infix_vec_init.emplace_back(infix.infix_);
                        right_infix_vec_init.back().BuildTrieAndSuffixes(right_trie_keys.data(), right_trie_keys.size(),
                                0, infix_size_, false, true, false);
                    }
                }
                else {
                    left_infix_vec_init.push_back(infix);
                    right_infix_vec_init.push_back(infix);
                    num_duplicates++;
                }
            }
            else if (infix_val < separator - 1)
                left_infix_vec_init.push_back(infix);
            else 
                right_infix_vec_init.push_back(infix);
        }

        std::stable_sort(left_infix_vec_init.begin(), left_infix_vec_init.end());
        std::stable_sort(right_infix_vec_init.begin(), right_infix_vec_init.end());

        // Split the infixes as needed
        const uint32_t shared_word_byte = (shared / 64) * 8;
        auto [shared_lt, ignore_lt, implicit_size_lt] = GetSharedIgnoreImplicitLengths(
                {prev_key.str + shared_word_byte, prev_key.length < shared_word_byte ? 0 : prev_key.length - shared_word_byte},
                {key.str + shared_word_byte, key.length < shared_word_byte ? 0 : key.length - shared_word_byte});
        shared_lt += shared_word_byte * 8;
        const int32_t shamt_lt = shared_lt + ignore_lt + implicit_size_lt - shared - ignore - implicit_size;
        const uint64_t prev_extraction_lt = ExtractPartialKey(prev_key, shared_lt, ignore_lt, implicit_size_lt, 0);
        const uint64_t extraction_lt = ExtractPartialKey(key, shared_lt, ignore_lt, implicit_size_lt, 1);
        const uint64_t left_start = prev_key.BitsAt(shared + ignore + implicit_size + std::max(0, shamt_lt - 56), std::min(shamt_lt, 56)) << infix_size_;
        const uint64_t left_end = (((extraction >> infix_size_) - (prev_extraction >> infix_size_)) << (infix_size_ + shamt_lt))
                                | (key.BitsAt(shared + ignore + implicit_size + std::max(0, shamt_lt - 56), std::min(shamt_lt, 56)) << infix_size_);
        const uint32_t total_implicit_lt = ((extraction_lt >> infix_size_) - (prev_extraction_lt >> infix_size_)) + 1;
        std::vector<Infix> left_infix_vec = UpdateInfixVector(left_infix_vec_init,
                shamt_lt, left_start, left_end, implicit_size_lt);

        auto [shared_gt, ignore_gt, implicit_size_gt] = GetSharedIgnoreImplicitLengths(
                {key.str + shared_word_byte, key.length < shared_word_byte ? 0 : key.length - shared_word_byte},
                {next_key.str + shared_word_byte, next_key.length < shared_word_byte ? 0 : next_key.length - shared_word_byte});
        shared_gt += shared_word_byte * 8;
        const int32_t shamt_gt = shared_gt + ignore_gt + implicit_size_gt - shared - ignore - implicit_size;
        const uint64_t extraction_gt = ExtractPartialKey(key, shared_gt, ignore_gt, implicit_size_gt, 0);
        const uint64_t next_extraction_gt = ExtractPartialKey(next_key, shared_gt, ignore_gt, implicit_size_gt, 1);
        const uint64_t right_start = (((extraction >> infix_size_) - (prev_extraction >> infix_size_)) << (infix_size_ + shamt_gt))
                                    | (key.BitsAt(shared + ignore + implicit_size + std::max(0, shamt_gt - 56), std::min(shamt_gt, 56)) << infix_size_);
        const uint64_t right_end = (((next_extraction >> infix_size_) - (prev_extraction >> infix_size_)) << (infix_size_ + shamt_gt))
                                    | (next_key.BitsAt(shared + ignore + implicit_size + std::max(0, shamt_gt - 56), std::min(shamt_gt, 56)) << infix_size_);
        const uint32_t total_implicit_gt = ((next_extraction_gt >> infix_size_) - (extraction_gt >> infix_size_)) + 1;
        std::vector<Infix> right_infix_vec = UpdateInfixVector(right_infix_vec_init,
                shamt_gt, right_start, right_end, implicit_size_gt);

        InfixStore store_lt = AllocateInfixStoreWithVector(left_infix_vec, total_implicit_lt);
        InfixStore store_gt = AllocateInfixStoreWithVector(right_infix_vec, total_implicit_gt);

#ifdef DEBUG
        GetInfixVector(store_lt);
        GetInfixVector(store_gt);
#endif // DEBUG

        auto *ptr_to_free = infix_store.ptr;
        infix_store.status = store_lt.status;
        infix_store.ptr = store_lt.ptr;
        infix_store.rwlock.store(store_lt.rwlock.load(std::memory_order_acquire), std::memory_order_release);
        if constexpr (diva_type == DivaType::Int) {
            wh_int_put(better_tree_int_, key.str, key.length,
                    &store_gt, sizeof(InfixStore),
                    leaves_to_unlock);
        }
        else {
            wh_put(better_tree_, key.str, key.length,
                    &store_gt, sizeof(InfixStore),
                    leaves_to_unlock);
        }

        UnlockLeaves(leaves_to_unlock, it_write_lock);

        // No memory leaks!
        delete[] ptr_to_free;

        return num_duplicates;
    }

    // Handle the normal case with the payloads and everything
    const uint64_t infix_list_len = infix_store.GetFullSlotCount();
    uint64_t infix_list_contents[infix_list_len > heap_alloc_threshold ? 1 : (infix_list_len + 1)];
    const uint32_t payload_list_size = (infix_list_len + 1) * payload_size_ / 64 + 2;
    uint64_t payload_list_contents[infix_list_len > heap_alloc_threshold ? 1 : payload_list_size];
    uint64_t *infix_list = infix_list_contents;
    uint64_t *payload_list = payload_list_contents;
    if (infix_list_len > heap_alloc_threshold) {
        infix_list = new uint64_t[infix_list_len + 1];
        if constexpr (payload_type == PayloadType::FixedLength)
            payload_list = new uint64_t[payload_list_size];
    }
    uint32_t infix_count;
    if constexpr (payload_type == PayloadType::FixedLength)
        infix_count = GetInfixList(infix_store, infix_list, payload_list);
    else 
        infix_count = GetInfixList(infix_store, infix_list);

    int32_t sep_l = -1, sep_r = infix_count, sep_mid;
    while (sep_r - sep_l > 1) {
        sep_mid = (sep_l + sep_r) / 2;
        const uint64_t val = infix_list[sep_mid] & (infix_list[sep_mid] - 1);
        const bool cond = val <= separator - 1;
        sep_l = cond ? sep_mid : sep_l;
        sep_r = cond ? sep_r : sep_mid;
    }
    uint64_t *infix_list_right_half_ptr = infix_list + sep_r;
    uint64_t *payload_list_right_half_ptr = payload_list;
    uint32_t payload_list_right_half_offset = sep_r * payload_size_;
    uint32_t split_pos = sep_r, num_partial_matches = 0;
    for (int32_t i = sep_l; i >= 0 && (infix_list[i] >> infix_size_) == (separator >> infix_size_); i--) {
        const uint64_t mask = ((infix_list[i] & -infix_list[i]) << 1) - 1;
        if ((infix_list[i] | mask) == (separator | mask)) {
            split_pos = i;
            num_partial_matches++;
        }
    }
    const uint32_t left_half_len = sep_r;
    const uint32_t right_half_len = infix_list_len - sep_r + num_partial_matches;
    if (num_partial_matches > 0) {
        infix_list_right_half_ptr = new uint64_t[right_half_len];
        payload_list_right_half_ptr = new uint64_t[(right_half_len + 1) * payload_size_ / 64 + 2];
        payload_list_right_half_offset = 0;
        uint64_t ind = 0;
        for (uint32_t i = split_pos; i < sep_r; i++) {
            const uint64_t mask = ((infix_list[i] & -infix_list[i]) << 1) - 1;
            if ((infix_list[i] | mask) == (separator | mask)) {
                infix_list_right_half_ptr[ind] = infix_list[i];
                if constexpr (payload_type == PayloadType::FixedLength) {
                    copy_bitmap_to_bitmap(payload_list, i * payload_size_,
                                          payload_list_right_half_ptr, ind * payload_size_, 
                                          payload_size_);
                }
                ind++;
            }
        }
        memcpy(infix_list_right_half_ptr + ind, infix_list + sep_r,
               (right_half_len - ind) * sizeof(infix_list[0]));
        if constexpr (payload_type == PayloadType::FixedLength) {
            copy_bitmap_to_bitmap(payload_list, sep_r * payload_size_,
                                  payload_list_right_half_ptr, ind * payload_size_, 
                                  (right_half_len - ind) * payload_size_);
        }
    }

    const uint32_t shared_word_byte = (shared / 64) * 8;
    auto [shared_lt, ignore_lt, implicit_size_lt] = GetSharedIgnoreImplicitLengths(
            {prev_key.str + shared_word_byte, prev_key.length < shared_word_byte ? 0 : prev_key.length - shared_word_byte},
            {key.str + shared_word_byte, key.length < shared_word_byte ? 0 : key.length - shared_word_byte});
    shared_lt += shared_word_byte * 8;
    const int32_t shamt_lt = shared_lt + ignore_lt + implicit_size_lt - shared - ignore - implicit_size;
    const uint64_t prev_extraction_lt = ExtractPartialKey(prev_key, shared_lt, ignore_lt, implicit_size_lt, 0);
    const uint64_t extraction_lt = ExtractPartialKey(key, shared_lt, ignore_lt, implicit_size_lt, 1);
    const uint64_t left_start = prev_key.BitsAt(shared + ignore + implicit_size + std::max(0, shamt_lt - 56), std::min(shamt_lt, 56)) << infix_size_;
    const uint64_t left_end = (((extraction >> infix_size_) - (prev_extraction >> infix_size_)) << (infix_size_ + shamt_lt))
                              | (key.BitsAt(shared + ignore + implicit_size + std::max(0, shamt_lt - 56), std::min(shamt_lt, 56)) << infix_size_);
    const uint32_t total_implicit_lt = ((extraction_lt >> infix_size_) - (prev_extraction_lt >> infix_size_)) + 1;

    auto [shared_gt, ignore_gt, implicit_size_gt] = GetSharedIgnoreImplicitLengths(
            {key.str + shared_word_byte, key.length < shared_word_byte ? 0 : key.length - shared_word_byte},
            {next_key.str + shared_word_byte, next_key.length < shared_word_byte ? 0 : next_key.length - shared_word_byte});
    shared_gt += shared_word_byte * 8;
    const int32_t shamt_gt = shared_gt + ignore_gt + implicit_size_gt - shared - ignore - implicit_size;
    const uint64_t extraction_gt = ExtractPartialKey(key, shared_gt, ignore_gt, implicit_size_gt, 0);
    const uint64_t next_extraction_gt = ExtractPartialKey(next_key, shared_gt, ignore_gt, implicit_size_gt, 1);
    const uint64_t right_start = (((extraction >> infix_size_) - (prev_extraction >> infix_size_)) << (infix_size_ + shamt_gt))
                                | (key.BitsAt(shared + ignore + implicit_size + std::max(0, shamt_gt - 56), std::min(shamt_gt, 56)) << infix_size_);
    const uint64_t right_end = (((next_extraction >> infix_size_) - (prev_extraction >> infix_size_)) << (infix_size_ + shamt_gt))
                                | (next_key.BitsAt(shared + ignore + implicit_size + std::max(0, shamt_gt - 56), std::min(shamt_gt, 56)) << infix_size_);
    const uint32_t total_implicit_gt = ((next_extraction_gt >> infix_size_) - (extraction_gt >> infix_size_)) + 1;

    const auto [left_list_len, left_exp] = GetExpandedInfixListLength(infix_list,
                                                                      left_half_len,
                                                                      implicit_size,
                                                                      shamt_lt,
                                                                      left_start, left_end);
    uint64_t left_infix_list_contents[left_list_len > heap_alloc_threshold ? 1 : left_list_len];
    const uint32_t left_payload_list_size = (left_list_len + 1) * payload_size_ / 64 + 2;
    uint64_t left_payload_list_contents[left_list_len > heap_alloc_threshold ? 1 : left_payload_list_size];
    uint64_t *left_infix_list = left_infix_list_contents;
    uint64_t *left_payload_list = left_payload_list_contents;
    if (left_list_len > heap_alloc_threshold) {
        left_infix_list = new uint64_t[left_list_len];
        if constexpr (payload_type == PayloadType::FixedLength)
            left_payload_list = new uint64_t[left_payload_list_size];
    }
    if constexpr (payload_type == PayloadType::FixedLength) {
        const uint32_t payload_list_offset = 0;
        UpdateInfixList(infix_list, left_half_len, shamt_lt,
                        left_start, left_end,
                        left_infix_list, left_list_len,
                        left_exp,
                        payload_list, payload_list_offset,
                        left_payload_list);
    }
    else {
        UpdateInfixList(infix_list, left_half_len, shamt_lt,
                        left_start, left_end,
                        left_infix_list, left_list_len,
                        left_exp);
    }

    const auto [right_list_len, right_exp] = GetExpandedInfixListLength(infix_list_right_half_ptr,
                                                                        right_half_len,
                                                                        implicit_size,
                                                                        shamt_gt,
                                                                        right_start, right_end);
    uint64_t right_infix_list_contents[right_list_len > heap_alloc_threshold ? 1 : right_list_len];
    const uint32_t right_payload_list_size = (right_list_len + 1) * payload_size_ / 64 + 2;
    uint64_t right_payload_list_contents[right_list_len > heap_alloc_threshold ? 1 : right_payload_list_size];
    uint64_t *right_infix_list = right_infix_list_contents;
    uint64_t *right_payload_list = right_payload_list_contents;
    if (right_list_len > heap_alloc_threshold) {
        right_infix_list = new uint64_t[right_list_len];
        if constexpr (payload_type == PayloadType::FixedLength)
            right_payload_list = new uint64_t[right_payload_list_size];
    }
    if constexpr (payload_type == PayloadType::FixedLength) {
        UpdateInfixList(infix_list_right_half_ptr, right_half_len, shamt_gt,
                        right_start, right_end,
                        right_infix_list, right_list_len,
                        right_exp,
                        payload_list_right_half_ptr, payload_list_right_half_offset,
                        right_payload_list);
    }
    else {
        UpdateInfixList(infix_list_right_half_ptr, right_half_len, shamt_gt,
                        right_start, right_end,
                        right_infix_list, right_list_len,
                        right_exp);
    }

    InfixStore store_lt = AllocateInfixStoreWithList(left_infix_list,
                                                     left_list_len,
                                                     total_implicit_lt,
                                                     left_payload_list);
    if constexpr (payload_type == PayloadType::FixedLength) {
        // Set the sample's payload
        store_lt.ptr[1] = infix_store.ptr[1];
    }
    InfixStore store_gt = AllocateInfixStoreWithList(right_infix_list,
                                                     right_list_len,
                                                     total_implicit_gt,
                                                     right_payload_list);
    if constexpr (payload_type == PayloadType::FixedLength) {
        // Set the sample's payload
        AddSamplePayload(store_gt, reinterpret_cast<const uint64_t *>(payload));
    }

    auto *ptr_to_free = infix_store.ptr;
    infix_store.status = store_lt.status;
    infix_store.ptr = store_lt.ptr;
    infix_store.rwlock.store(store_lt.rwlock.load(std::memory_order_acquire), std::memory_order_release);
    if constexpr (diva_type == DivaType::Int) {
        wh_int_put(better_tree_int_, key.str, key.length,
                &store_gt, sizeof(InfixStore),
                leaves_to_unlock);
    }
    else {
        wh_put(better_tree_, key.str, key.length,
                &store_gt, sizeof(InfixStore),
                leaves_to_unlock);
    }

    UnlockLeaves(leaves_to_unlock, it_write_lock);
    // No memory leaks!
    if (infix_list_len > heap_alloc_threshold) {
        delete[] infix_list;
        if constexpr (payload_type == PayloadType::FixedLength)
            delete[] payload_list;
    }
    if (left_list_len > heap_alloc_threshold) {
        delete[] left_infix_list;
        if constexpr (payload_type == PayloadType::FixedLength)
            delete[] left_payload_list;
    }
    if (right_list_len > heap_alloc_threshold) {
        delete[] right_infix_list;
        if constexpr (payload_type == PayloadType::FixedLength)
            delete[] right_payload_list;
    }
    if (num_partial_matches > 0) {
        delete[] infix_list_right_half_ptr;
        delete[] payload_list_right_half_ptr;
    }
    delete[] ptr_to_free;

    return left_list_len + right_list_len - infix_count;
}


template <DivaType diva_type, PayloadType payload_type>
//__attribute__((always_inline))
inline std::tuple<uint32_t, bool> Diva<diva_type, payload_type>::GetExpandedInfixListLength(const uint64_t *list, const uint32_t list_len,
                                                                                            const uint32_t implicit_size, const uint32_t shamt,
                                                                                            const uint64_t lower_lim, const uint64_t upper_lim) {
    uint32_t actual_list_len = list_len;
    bool expanded = false;
    const uint64_t lower_implicit_lim = lower_lim >> infix_size_;
    const uint64_t upper_implicit_lim = upper_lim >> infix_size_;
    for (int32_t i = 0; i < list_len; i++) {
        const int32_t new_lowbit_position = lowbit_pos(list[i]) + shamt;
        if (infix_size_ <= new_lowbit_position && new_lowbit_position < 64) {
            const uint64_t implicit_part = (list[i] << shamt) >> infix_size_;
            const uint64_t start = std::max(lower_implicit_lim, implicit_part - (implicit_part & (-implicit_part)));
            const uint64_t end = std::min(upper_implicit_lim, implicit_part | (implicit_part - 1));
            actual_list_len += end - start;
            expanded = true;
        }
        else if (new_lowbit_position >= 64) {
            actual_list_len += upper_implicit_lim - lower_implicit_lim;
            expanded = true;
        }
    }
    return {actual_list_len, expanded};
}


template <DivaType diva_type, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<diva_type, payload_type>::UpdateInfixList(const uint64_t *list, const uint32_t list_len, const uint32_t shamt,
                                                           const uint64_t lower_lim, const uint64_t upper_lim,
                                                           uint64_t *res, const uint32_t res_len, const bool expanded,
                                                           const uint64_t *payload_list, const uint32_t payload_list_offset,
                                                           uint64_t *res_payload) const {
#ifdef DEBUG
    if constexpr (payload_type == PayloadType::FixedLength) {
        assert(payload_list != nullptr);
        assert(res_payload != nullptr);
    }
#endif // DEBUG
    if (!expanded) {
        for (int32_t i = 0; i < list_len; i++) {
            res[i] = (list[i] << shamt) - lower_lim;
#ifdef DEBUG
            assert(res[i] > 0);
#endif
        }
        if constexpr (payload_type == PayloadType::FixedLength) {
            const uint32_t total_payload_bits = list_len * payload_size_;
            copy_bitmap_to_bitmap(payload_list, payload_list_offset, res_payload, 0, total_payload_bits);
        }
        return;
    }

    const bool should_allocate_on_heap = res_len > heap_alloc_threshold;

    uint32_t res_ind = 0;
    uint32_t payload_ind_contents[should_allocate_on_heap ? 1 : res_len];
    uint32_t *payload_ind = payload_ind_contents;
    if (should_allocate_on_heap)
        payload_ind = new uint32_t[res_len];
    const uint64_t lower_implicit_lim = lower_lim >> infix_size_;
    const uint64_t upper_implicit_lim = upper_lim >> infix_size_;
    for (int32_t i = 0; i < list_len; i++) {
        const uint64_t val = shamt < 64 ? list[i] << shamt : 0UL;
        const uint64_t implicit_part = val >> infix_size_;
        const uint64_t explicit_part = val & BITMASK(infix_size_);
        if (explicit_part == 0) {
            const uint64_t start = implicit_part != 0 ? implicit_part - (implicit_part & (-implicit_part))
                                                      : lower_implicit_lim;
            const uint64_t end = implicit_part != 0 ? implicit_part | (implicit_part - 1)
                                                    : upper_implicit_lim;
            for (uint64_t j = std::max(start, lower_implicit_lim); j <= std::min(end, upper_implicit_lim); j++) {
                res[res_ind] = ((j - lower_implicit_lim) << infix_size_) | (1ULL << (infix_size_ - 1));
                if constexpr (payload_type == PayloadType::FixedLength)
                    payload_ind[res_ind] = i;
                res_ind++;
            }
        }
        else {
            res[res_ind] = val - lower_lim;
            if constexpr (payload_type == PayloadType::FixedLength)
                payload_ind[res_ind] = i;
            res_ind++;
        }
    }
#ifdef DEBUG
    assert(res_ind == res_len);
#endif
    
    if constexpr (payload_type == PayloadType::FixedLength) {
        std::pair<uint64_t, uint32_t> sorter_contents[should_allocate_on_heap ? 1 : res_len];
        std::pair<uint64_t, uint32_t> *sorter = sorter_contents;
        if (should_allocate_on_heap)
            sorter = new std::pair<uint64_t, uint32_t>[res_len];

        for (uint32_t i = 0; i < res_ind; i++)
            sorter[i] = {res[i], payload_ind[i]};
        auto comp = [&](std::pair<uint64_t, uint32_t> a, std::pair<uint64_t, uint32_t> b) {
                        return CompareInfixes(a.first, b.first);
                    };
        std::stable_sort(sorter, sorter + res_ind, comp);
        for (uint32_t i = 0; i < res_ind; i++) {
            res[i] = sorter[i].first;
            const uint32_t pos_in = payload_size_ * sorter[i].second + payload_list_offset;
            const uint32_t pos_out = payload_size_ * i;
            copy_bitmap_to_bitmap(payload_list, pos_in, res_payload, pos_out, payload_size_);
        }

        if (should_allocate_on_heap)
            delete[] sorter;
    } 
    else {
        auto comp = [&](uint64_t a, uint64_t b) {
                        return CompareInfixes(a, b);
                    };
        std::stable_sort(res, res + res_ind, comp);
    }

    if (res_len > heap_alloc_threshold)
        delete[] payload_ind;

#ifdef DEBUG
    for (int32_t i = 0; i < res_ind; i++)
        assert(res[i] > 0);
#endif
}


template <DivaType diva_type, PayloadType payload_type>
//__attribute__((always_inline))
inline std::vector<typename Diva<diva_type, payload_type>::Infix>
Diva<diva_type, payload_type>::UpdateInfixVector(const std::vector<Infix>& infix_vec, const uint32_t shamt,
                                                 const uint64_t lower_lim, const uint64_t upper_lim,
                                                 const uint32_t implicit_size) const {
    std::vector<Infix> res;
    for (auto& infix_to_split : infix_vec) {
        for (auto infix : infix_to_split.SplitPrefixBits(shamt, infix_size_)) {
            if (lowbit_pos(infix.infix_) >= infix_size_) {
                const uint64_t lower_implicit_lim = lower_lim >> infix_size_;
                const uint64_t upper_implicit_lim = upper_lim >> infix_size_;
                const uint64_t implicit_part = infix.infix_ >> infix_size_;
                const uint64_t start = implicit_part != 0 ? implicit_part & (implicit_part - 1)
                                                        : lower_implicit_lim;
                const uint64_t end = implicit_part != 0 ? implicit_part | (implicit_part - 1)
                                                        : upper_implicit_lim;
                for (uint64_t j = std::max(start, lower_implicit_lim); j <= std::min(end, upper_implicit_lim); j++)
                    res.emplace_back(((j - lower_implicit_lim) << infix_size_) | (1ULL << (infix_size_ - 1)));
            }
            else {
                infix.infix_ -= lower_lim;
                infix.infix_ &= BITMASK(implicit_size + infix_size_);
                res.push_back(infix);
            }
        }
    }
    std::sort(res.begin(), res.end());
    return std::move(res);
}


template <DivaType diva_type, PayloadType payload_type>
inline std::tuple<uint32_t, uint32_t, uint32_t> 
Diva<diva_type, payload_type>::GetSharedIgnoreImplicitLengths(const InfiniteByteString key_1,
                                                              const InfiniteByteString key_2) const {
    uint32_t share = 0, ignore = 0;

    uint32_t ind = 0, delta;
    do {
        const uint64_t read_1 = key_1.WordAt(ind * sizeof(uint64_t));
        const uint64_t read_2 = key_2.WordAt(ind * sizeof(uint64_t));
        delta = __builtin_ia32_lzcnt_u64(read_1 ^ read_2);
        share += delta;
        ind++;
    } while (delta == 64);

    ind--;
    do {
        const uint64_t read_1 = key_1.WordAt(ind * sizeof(uint64_t));
        const uint64_t read_2 = key_2.WordAt(ind * sizeof(uint64_t));
        const uint32_t offset = (ind > share / 64 ? 0 : share % 64 + 1);
        delta = __builtin_ia32_lzcnt_u64(((~read_1) | read_2) & BITMASK(64 - offset));
        ignore += delta - offset;
        ind++;
    } while (delta == 64);

    uint64_t implicit_size = base_implicit_size;
    const uint64_t implicit_1 = key_1.BitsAt(share + ignore + 1, base_implicit_size - 1);
    const uint64_t implicit_2 = (1ULL << (base_implicit_size - 1)) | key_2.BitsAt(share + ignore + 1, base_implicit_size - 1);
    implicit_size += (2 * (implicit_2 - implicit_1 + 1) < (1ULL << base_implicit_size));

    return {share, ignore, implicit_size};
}


template <DivaType diva_type, PayloadType payload_type>
inline uint64_t Diva<diva_type, payload_type>::Size() const {
    uint64_t res = sizeof(bool) + sizeof(infix_store_target_size) 
                 + sizeof(base_implicit_size) + sizeof(scale_shift)
                 + sizeof(scale_implicit_shift) + sizeof(size_scalar_count)
                 + sizeof(size_scalar_shrink_grow_sep) + sizeof(load_factor_)
                 + sizeof(load_factor_alt_) + sizeof(infix_size_) 
                 + sizeof(rng_seed_) + sizeof(n_keys_) 
                 + sizeof(InfixStore::size_grade_bit_count)
                 + sizeof(InfixStore::full_slot_count_bit_count);

    if constexpr (payload_type == PayloadType::FixedLength)
        res += sizeof(payload_size_);

    const uint8_t *tree_key, *last_tree_key = nullptr;
    uint32_t tree_key_len, last_tree_key_len = 0, dummy;
    InfixStore *store;
    const bool write = false, unlock = true;

    if constexpr (diva_type == DivaType::Int) {
        wormhole_int_iter it_int;
        it_int.ref = better_tree_int_;
        it_int.map = better_tree_int_->map;
        it_int.leaf = nullptr;
        it_int.is = 0;
        for (wh_int_iter_seek(&it_int, nullptr, 0, write); wh_int_iter_valid(&it_int); wh_int_iter_skip1(&it_int, write, unlock)) {
            wh_int_iter_peek_ref(&it_int, reinterpret_cast<const void **>(&tree_key), &tree_key_len, 
                                          reinterpret_cast<void **>(&store), &dummy);
            res += sizeof(tree_key_len) + tree_key_len;
            res += sizeof(store->status);
            if constexpr (payload_type == PayloadType::FixedLength) {
                res += sizeof(store->num_sample_payloads);
                res += (store->num_sample_payloads * payload_size_ + 7) / 8;
            }
            if (store->ptr != nullptr) {
                const uint64_t word_count = store->GetPtrWordCount(scaled_sizes_[store->GetSizeGrade()], infix_size_, payload_size_);
                res += word_count * sizeof(uint64_t);
            }
        }
        if (it_int.leaf)
            wormleaf_int_unlock_read(it_int.leaf);
    }
    else {
        wormhole_iter it;
        it.ref = better_tree_;
        it.map = better_tree_->map;
        it.leaf = nullptr;
        it.is = 0;
        for (wh_iter_seek(&it, nullptr, 0, write); wh_iter_valid(&it); wh_iter_skip1(&it, write, unlock)) {
            wh_iter_peek_ref(&it, reinterpret_cast<const void **>(&tree_key), &tree_key_len, 
                                  reinterpret_cast<void **>(&store), &dummy);
            res += sizeof(tree_key_len) + tree_key_len;
            res += sizeof(store->status); // + sizeof(store->ptr);
            if constexpr (payload_type == PayloadType::FixedLength) {
                res += sizeof(store->num_sample_payloads);
                res += (store->num_sample_payloads * payload_size_ + 7) / 8;
            }
            if (store->ptr != nullptr) {
                const uint64_t word_count = store->GetPtrWordCount(scaled_sizes_[store->GetSizeGrade()], infix_size_, payload_size_);
                res += word_count * sizeof(uint64_t);
            }
            last_tree_key = tree_key;
            last_tree_key_len = tree_key_len;
        }
        if (it.leaf)
            wormleaf_unlock_read(it.leaf);
    }
    res += sizeof(tree_key_len);
    return res;
}


template <DivaType diva_type, PayloadType payload_type>
inline uint32_t Diva<diva_type, payload_type>::Serialize(char *out) const {
    uint64_t res = SerializeMetadata(out);

    const uint8_t *tree_key;
    uint32_t tree_key_len, dummy;
    InfixStore *store;
    const bool write = false, unlock = true;

    if constexpr (diva_type == DivaType::Int) {
        wormhole_int_iter it_int;
        it_int.ref = better_tree_int_;
        it_int.map = better_tree_int_->map;
        it_int.leaf = nullptr;
        it_int.is = 0;
        for (wh_int_iter_seek(&it_int, nullptr, 0, write); wh_int_iter_valid(&it_int); wh_int_iter_skip1(&it_int, write, unlock)) {
            wh_int_iter_peek_ref(&it_int, reinterpret_cast<const void **>(&tree_key), &tree_key_len, 
                                          reinterpret_cast<void **>(&store), &dummy);
            memcpy(out + res, &tree_key_len, sizeof(tree_key_len));
            res += sizeof(tree_key_len);
            memcpy(out + res, tree_key, tree_key_len);
            res += tree_key_len;
            res += SerializeInfixStore(out + res, *store);
        }
        if (it_int.leaf)
            wormleaf_int_unlock_read(it_int.leaf);
    }
    else {
        wormhole_iter it;
        it.ref = better_tree_;
        it.map = better_tree_->map;
        it.leaf = nullptr;
        it.is = 0;
        for (wh_iter_seek(&it, nullptr, 0, write); wh_iter_valid(&it); wh_iter_skip1(&it, write, unlock)) {
            wh_iter_peek_ref(&it, reinterpret_cast<const void **>(&tree_key), &tree_key_len, 
                                  reinterpret_cast<void **>(&store), &dummy);
            memcpy(out + res, &tree_key_len, sizeof(tree_key_len));
            res += sizeof(tree_key_len);
            memcpy(out + res, tree_key, tree_key_len);
            res += tree_key_len;
            res += SerializeInfixStore(out + res, *store);
        }
        if (it.leaf)
            wormleaf_unlock_read(it.leaf);
    }
    tree_key_len = std::numeric_limits<uint32_t>::max();
    memcpy(out + res, &tree_key_len, sizeof(tree_key_len));
    res += sizeof(tree_key_len);
    return res;
}


template <DivaType diva_type, PayloadType payload_type>
inline uint32_t Diva<diva_type, payload_type>::SerializeMetadata(char *out) const {
    uint32_t res = 0;
    // Diva Version
    out[res++] = static_cast<char>(diva_type);

    // Global Metadata
    memcpy(out + res, &infix_store_target_size, sizeof(infix_store_target_size));
    res += sizeof(infix_store_target_size);

    memcpy(out + res, &base_implicit_size, sizeof(base_implicit_size));
    res += sizeof(base_implicit_size);

    memcpy(out + res, &scale_shift, sizeof(scale_shift));
    res += sizeof(scale_shift);

    memcpy(out + res, &scale_implicit_shift, sizeof(scale_implicit_shift));
    res += sizeof(scale_implicit_shift);

    memcpy(out + res, &size_scalar_count, sizeof(size_scalar_count));
    res += sizeof(size_scalar_count);

    memcpy(out + res, &size_scalar_shrink_grow_sep, sizeof(size_scalar_shrink_grow_sep));
    res += sizeof(size_scalar_shrink_grow_sep);

    memcpy(out + res, &load_factor_, sizeof(load_factor_));
    res += sizeof(load_factor_);

    memcpy(out + res, &load_factor_alt_, sizeof(load_factor_alt_));
    res += sizeof(load_factor_alt_);

    // Infix Size, Payload Size, and Random Seed
    memcpy(out + res, &infix_size_, sizeof(infix_size_));
    res += sizeof(infix_size_);

    if constexpr (payload_type == PayloadType::FixedLength) {
        memcpy(out + res, &payload_size_, sizeof(payload_size_));
        res += sizeof(payload_size_);
    }

    memcpy(out + res, &rng_seed_, sizeof(rng_seed_));
    res += sizeof(rng_seed_);

    const uint64_t n_keys_val = n_keys_.load(std::memory_order_acquire);
    memcpy(out + res, &n_keys_val, sizeof(n_keys_val));
    res += sizeof(n_keys_val);

    // Infix Store Metadata
    memcpy(out + res, &InfixStore::size_grade_bit_count, sizeof(InfixStore::size_grade_bit_count));
    res += sizeof(InfixStore::size_grade_bit_count);

    memcpy(out + res, &InfixStore::full_slot_count_bit_count, sizeof(InfixStore::full_slot_count_bit_count));
    res += sizeof(InfixStore::full_slot_count_bit_count);

    return res;
}


template <DivaType diva_type, PayloadType payload_type>
inline uint64_t Diva<diva_type, payload_type>::SerializeInfixStore(char *out,
                                                                   const Diva<diva_type, payload_type>::InfixStore& store) const {
    uint64_t offset = 0;
    memcpy(out + offset, &store.status, sizeof(store.status));
    offset += sizeof(store.status);

    if constexpr (payload_type == PayloadType::FixedLength) {
        memcpy(out + offset, &store.num_sample_payloads, sizeof(store.num_sample_payloads));
        offset += sizeof(store.num_sample_payloads);
    }

    const uint64_t word_count = store.GetPtrWordCount(scaled_sizes_[store.GetSizeGrade()], infix_size_, payload_size_);
    memcpy(out + offset, store.ptr, word_count * sizeof(uint64_t));
    offset += word_count * sizeof(uint64_t);

    if constexpr (payload_type == PayloadType::FixedLength) {
        if (store.num_sample_payloads > 0) {
            const uint32_t sample_payload_byte_count = (store.num_sample_payloads * payload_size_ + 7) / 8;
            uint8_t *sample_payloads = reinterpret_cast<uint8_t *>(store.ptr[1]);
            memcpy(out + offset, sample_payloads, sample_payload_byte_count);
            offset += sample_payload_byte_count;
        }
    }

    return offset;
}


template <DivaType diva_type, PayloadType payload_type>
inline Diva<diva_type, payload_type>::~Diva() {
    const bool write = true;
    const bool unlock = true;
    const uint8_t *tree_key;
    uint32_t tree_key_len, dummy;
    InfixStore *store;

    if constexpr (diva_type == DivaType::Int) {
        wormhole_int_iter it_int;
        it_int.ref = better_tree_int_;
        it_int.map = better_tree_int_->map;
        it_int.leaf = nullptr;
        it_int.is = 0;
        for (wh_int_iter_seek(&it_int, nullptr, 0, write); wh_int_iter_valid(&it_int); wh_int_iter_skip1(&it_int, write, unlock)) {
            wh_int_iter_peek_ref(&it_int, reinterpret_cast<const void **>(&tree_key), &tree_key_len, 
                                          reinterpret_cast<void **>(&store), &dummy);
            if constexpr (payload_type == PayloadType::FixedLength)
                free(reinterpret_cast<void *>(store->ptr[1]));
            delete[] store->ptr;
        }
        if (it_int.leaf)
            wormleaf_int_unlock_write(it_int.leaf);
        wh_int_destroy(wh_int_);
    }
    else {
        wormhole_iter it;
        it.ref = better_tree_;
        it.map = better_tree_->map;
        it.leaf = nullptr;
        it.is = 0;
        for (wh_iter_seek(&it, nullptr, 0, write); wh_iter_valid(&it); wh_iter_skip1(&it, write, unlock)) {
            wh_iter_peek_ref(&it, reinterpret_cast<const void **>(&tree_key), &tree_key_len, 
                                  reinterpret_cast<void **>(&store), &dummy);
            if constexpr (payload_type == PayloadType::FixedLength)
                free(reinterpret_cast<void *>(store->ptr[1]));
            delete[] store->ptr;
        }
        if (it.leaf)
            wormleaf_unlock_write(it.leaf);
        wh_destroy(wh_);
    }
}


template <DivaType diva_type, PayloadType payload_type>
inline Diva<diva_type, payload_type>::Diva(const char *deser_buf):
        bulk_load_streaming_ind_(0) {
    uint32_t ind = DeserializeMetadata(deser_buf);
    if constexpr (diva_type == DivaType::Int) {
        wh_int_ = wh_int_create();
        better_tree_int_ = wh_int_ref(wh_int_);
    }
    else {
        wh_ = wh_create();
        better_tree_ = wh_ref(wh_);
    }
    SetupScaleFactors();

    const uint32_t max_key_length = 20000;
    char key[max_key_length];
    uint32_t key_length;
    InfixStore store;

    memcpy(&key_length, deser_buf + ind, sizeof(key_length));
    ind += sizeof(key_length);
    while (key_length != std::numeric_limits<uint32_t>::max()) {
#ifdef DEBUG
        assert(key_length < max_key_length);
#endif
        memcpy(key, deser_buf + ind, key_length);
        ind += key_length;
        ind += DeserializeInfixStore(deser_buf + ind, store);

        void *dummy_locked_leaf_addrs[3] = {nullptr, nullptr, nullptr};
        if constexpr (diva_type == DivaType::Int) {
#ifdef DEBUG
            assert(1 <= key_length && key_length <= sizeof(uint64_t));
#endif
            wh_int_put(better_tree_int_, key, key_length, &store, sizeof(store), dummy_locked_leaf_addrs);
        }
        else
            wh_put(better_tree_, key, key_length, &store, sizeof(store), dummy_locked_leaf_addrs);

        memcpy(&key_length, deser_buf + ind, sizeof(key_length));
        ind += sizeof(key_length);
    }
}


template <DivaType diva_type, PayloadType payload_type>
inline uint32_t Diva<diva_type, payload_type>::DeserializeMetadata(const char *deser_buf) {
    uint32_t res = 0;
    uint32_t buf32;

    // Diva Version
    assert(static_cast<DivaType>(deser_buf[res]) == diva_type && "Mismatched Diva version");
    res++;

    // Global Metadata
    memcpy(&buf32, deser_buf + res, sizeof(infix_store_target_size));
    assert(buf32 == infix_store_target_size && "Mismatched Diva version");
    res += sizeof(infix_store_target_size);

    memcpy(&buf32, deser_buf + res, sizeof(base_implicit_size));
    assert(buf32 == base_implicit_size && "Mismatched Diva version");
    res += sizeof(base_implicit_size);

    memcpy(&buf32, deser_buf + res, sizeof(scale_shift));
    assert(buf32 == scale_shift && "Mismatched Diva version");
    res += sizeof(scale_shift);

    memcpy(&buf32, deser_buf + res, sizeof(scale_implicit_shift));
    assert(buf32 == scale_implicit_shift && "Mismatched Diva version");
    res += sizeof(scale_implicit_shift);

    memcpy(&buf32, deser_buf + res, sizeof(size_scalar_count));
    assert(buf32 == size_scalar_count && "Mismatched Diva version");
    res += sizeof(size_scalar_count);

    memcpy((void *) &size_scalar_shrink_grow_sep, deser_buf + res, sizeof(size_scalar_shrink_grow_sep));
    res += sizeof(size_scalar_shrink_grow_sep);

    memcpy((void *) &load_factor_, deser_buf + res, sizeof(load_factor_));
    res += sizeof(load_factor_);

    assert(size_scalar_shrink_grow_sep == static_cast<uint32_t>(std::log(infix_store_target_size / 64) 
                                                                / std::log(1 / load_factor_) + 1)
            && "Corrupted Diva version");

    memcpy((void *) &load_factor_alt_, deser_buf + res, sizeof(load_factor_alt_));
    res += sizeof(load_factor_alt_);

    // Infix Size, Payload Size, and Random Seed
    memcpy(&infix_size_, deser_buf + res, sizeof(infix_size_));
    res += sizeof(infix_size_);

    if constexpr (payload_type == PayloadType::FixedLength) {
        memcpy(&payload_size_, deser_buf + res, sizeof(payload_size_));
        res += sizeof(payload_size_);
    }

    memcpy(&rng_seed_, deser_buf + res, sizeof(rng_seed_));
    res += sizeof(rng_seed_);
    rng_.seed(rng_seed_);

    uint64_t n_keys_val;
    memcpy(&n_keys_val, deser_buf + res, sizeof(rng_seed_));
    res += sizeof(n_keys_val);
    n_keys_.store(n_keys_val, std::memory_order_release);

    // Infix Store Metadata
    memcpy(&buf32, deser_buf + res, sizeof(InfixStore::size_grade_bit_count));
    assert(buf32 == InfixStore::size_grade_bit_count && "Mismatched Diva version");
    res += sizeof(InfixStore::size_grade_bit_count);

    memcpy(&buf32, deser_buf + res, sizeof(InfixStore::full_slot_count_bit_count));
    assert(buf32 == InfixStore::full_slot_count_bit_count && "Mismatched Diva version");
    res += sizeof(InfixStore::full_slot_count_bit_count);

    return res;
}


template <DivaType diva_type, PayloadType payload_type>
inline uint32_t Diva<diva_type, payload_type>::DeserializeInfixStore(const char *deser_buf,
                                                                     Diva<diva_type, payload_type>::InfixStore& store) const {
    uint32_t offset = 0;
    memcpy(&store.status, deser_buf, sizeof(store.status));
    offset += sizeof(store.status);

    if constexpr (payload_type == PayloadType::FixedLength) {
        memcpy(&store.num_sample_payloads, deser_buf + offset, sizeof(store.num_sample_payloads));
        offset += sizeof(store.num_sample_payloads);
    }

    const uint64_t word_count = store.GetPtrWordCount(scaled_sizes_[store.GetSizeGrade()], infix_size_, payload_size_);
    store.ptr = new uint64_t[word_count];
    memcpy(store.ptr, deser_buf + offset, word_count * sizeof(uint64_t));
    offset += word_count * sizeof(uint64_t);

    if constexpr (payload_type == PayloadType::FixedLength) {
        if (store.num_sample_payloads > 0) {
            const uint32_t sample_payload_byte_count = (store.num_sample_payloads * payload_size_ + 7) / 8;
            uint8_t *sample_payloads = reinterpret_cast<uint8_t *>(malloc(sample_payload_byte_count));
            store.ptr[1] = reinterpret_cast<uint64_t>(sample_payloads);
            memcpy(sample_payloads, deser_buf + offset, sample_payload_byte_count);
            offset += sample_payload_byte_count;
        }
    }

    return offset;
}


template <DivaType diva_type, PayloadType payload_type>
//__attribute__((always_inline))
inline uint64_t Diva<diva_type, payload_type>::ExtractPartialKey(const InfiniteByteString key,
                                                                 const uint32_t shared, const uint32_t ignore,
                                                                 const uint32_t implicit_size, const uint64_t msb) const {
    const uint32_t real_diff_pos = shared + ignore;
    uint64_t res = key.WordAt(real_diff_pos / 8);
    res >>= (63 - (implicit_size - 1) - infix_size_ - real_diff_pos % 8);
    res &= BITMASK(implicit_size - 1 + infix_size_);
    res |= msb << (implicit_size - 1 + infix_size_);
    return res;
}


template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::Delete(uint64_t key,
                                                  std::function<bool(const uint64_t *)> should_remove) {
    key = __builtin_bswap64(key);
    Delete(reinterpret_cast<const uint8_t *>(&key), sizeof(key), should_remove);
}


template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::Delete(std::string_view input_key,
                                                  std::function<bool(const uint64_t *)> should_remove) {
    Delete(reinterpret_cast<const uint8_t *>(input_key.data()), input_key.size(), should_remove);
}

template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::Delete(const uint8_t *input_key, const uint32_t input_key_len,
                                                  std::function<bool(const uint64_t *)> should_remove) {
    const bool it_write_lock = false;
    InfiniteByteString key {input_key, input_key_len};

    InfixStore *infix_store_ptr;
    void *leaves_to_unlock[3] = {};

    InfiniteByteString next_key {};
    InfiniteByteString prev_key {};

    wormhole_int_iter it_int;
    wormhole_iter it;
    GetLowerUpperBounds(key, it_write_lock, leaves_to_unlock, it, it_int,
                        prev_key, next_key, infix_store_ptr);
    uint64_t prev_key_word, next_key_word;
    if constexpr (diva_type == DivaType::Int) {
        prev_key_word = *reinterpret_cast<const uint64_t *>(prev_key.str);
        prev_key.str = reinterpret_cast<const uint8_t *>(&prev_key_word);
        next_key_word = *reinterpret_cast<const uint64_t *>(next_key.str);
        next_key.str = reinterpret_cast<const uint8_t *>(&next_key_word);
    }

    rwlock_lock_write(infix_store_ptr->rwlock);
    InfixStore& infix_store = *infix_store_ptr;
    UnlockLeaves(leaves_to_unlock, it_write_lock);

    if (prev_key == key) {
        if constexpr (payload_type == PayloadType::FixedLength) {
            uint64_t payload[(payload_size_ + 63) / 64 + 1];
            bool removed = false;
            for (uint32_t i = 0; i < infix_store.num_sample_payloads; i++) {
                GetSamplePayload(infix_store, i, payload);
                if (should_remove(payload)) {
                    RemoveSamplePayload(infix_store, i);
                    removed = true;
                    break;
                }
            }
            if (removed || infix_store.num_sample_payloads == 0) {
                rwlock_unlock_write(infix_store.rwlock);
                if (infix_store.num_sample_payloads == 0)
                    DeleteMerge(key);
                else 
                    n_keys_.fetch_sub(1, std::memory_order_release);
                return;
            }
        }
        else {
            rwlock_unlock_write(infix_store.rwlock);
            DeleteMerge(key);
            return;
        }
    }

    auto [shared, ignore, implicit_size] = GetSharedIgnoreImplicitLengths(prev_key, next_key);
    // To delete from the binary tries
    const uint32_t key_start_bit = shared + ignore + implicit_size;

    const uint64_t extraction = ExtractPartialKey(key, shared, ignore, implicit_size, key.GetBit(shared));
    const uint64_t next_implicit = ExtractPartialKey(next_key, shared, ignore, implicit_size, 1) >> infix_size_;
    const uint64_t prev_implicit = ExtractPartialKey(prev_key, shared, ignore, implicit_size, 0) >> infix_size_;
    const uint32_t total_implicit = next_implicit - prev_implicit + 1;
    const uint64_t deletee = ((extraction | 1ULL) - (prev_implicit << infix_size_));

    DeleteRawFromInfixStore(infix_store, deletee, total_implicit, should_remove,
            {key.str, 8 * key.length}, key_start_bit);
    rwlock_unlock_write(infix_store.rwlock);
    n_keys_.fetch_sub(1, std::memory_order_release);
}


template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::DeleteRange(uint64_t l, uint64_t r,
                                                       std::function<bool(const uint64_t *)> should_remove) {
    l = __builtin_bswap64(l);
    r = __builtin_bswap64(r);
    DeleteRange(reinterpret_cast<const uint8_t *>(&l), sizeof(l),
                reinterpret_cast<const uint8_t *>(&r), sizeof(r),
                should_remove);
}


template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::DeleteRange(std::string_view input_l,
                                                       std::string_view input_r,
                                                       std::function<bool(const uint64_t *)> should_remove) {
    Delete(reinterpret_cast<const uint8_t *>(input_l.data()), input_l.size(),
           reinterpret_cast<const uint8_t *>(input_r.data()), input_r.size(),
           should_remove);
}

template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::DeleteRange(const uint8_t *input_l, const uint32_t input_l_len,
                                                       const uint8_t *input_r, const uint32_t input_r_len,
                                                       std::function<bool(const uint64_t *)> should_remove) {
    static_assert(diva_type != DivaType::BinaryTrie);   // TODO: Add binary trie support
    auto it = GetIterator(input_l, input_l_len, input_r, input_r_len,
                          should_remove ? should_remove 
                                        : [](const uint64_t *payload) { return true; });
    while (it.IsValid())
        it++;
}


template <DivaType diva_type, PayloadType payload_type>
inline bool Diva<diva_type, payload_type>::CompareInfixes(uint64_t a, uint64_t b) {
    const uint64_t a_lb = a & -a;
    a -= a_lb;
    const uint64_t b_lb = b & -b;
    b -= b_lb;
    return (a == b ? a_lb > b_lb : a < b);
}


template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::DeleteMerge(InfiniteByteString key) {
    const bool it_write_lock = true;

    InfiniteByteString middle_key {};
    InfiniteByteString left_key {};
    InfiniteByteString right_key {};
    InfixStore *store_l, *store_r;
    void *leaves_to_unlock[3] = {};

    wormhole_int_iter it_int;
    wormhole_iter it;
    DeleteGetLowerMiddleUpperBounds(key, leaves_to_unlock, it, it_int, 
                              left_key, middle_key, right_key, store_l, store_r);
    uint64_t left_key_word, middle_key_word, right_key_word;
    if constexpr (diva_type == DivaType::Int) {
        left_key_word = *reinterpret_cast<const uint64_t *>(left_key.str);
        left_key.str = reinterpret_cast<const uint8_t *>(&left_key_word);
        middle_key_word = *reinterpret_cast<const uint64_t *>(middle_key.str);
        middle_key.str = reinterpret_cast<const uint8_t *>(&middle_key_word);
        right_key_word = *reinterpret_cast<const uint64_t *>(right_key.str);
        right_key.str = reinterpret_cast<const uint8_t *>(&right_key_word);
    }

    rwlock_lock_write(store_l->rwlock);
    rwlock_lock_write(store_r->rwlock);

    auto [shared, ignore, implicit_size] = GetSharedIgnoreImplicitLengths(left_key, right_key);
    const uint64_t implicit = ExtractPartialKey(left_key, shared, ignore, implicit_size, 0) >> infix_size_;

    if constexpr (diva_type == DivaType::BinaryTrie) {  // Handle the binary trie case
        auto infix_vec = GetInfixVector(*store_l);
        infix_vec = UpdateInfixVectorDelete(shared, ignore, implicit_size, 
                left_key, middle_key, infix_vec);
        auto infix_vec_right = GetInfixVector(*store_r);
        infix_vec_right = UpdateInfixVectorDelete(shared, ignore, implicit_size, 
                middle_key, right_key, infix_vec_right);
        for (const auto& infix : infix_vec_right)
            infix_vec.push_back(infix);
        std::stable_sort(infix_vec.begin(), infix_vec.end());   // Stable sort to maintain suffix ordering
#ifdef DEBUG
        for (int32_t i = 0; i < infix_vec.size(); i++)
            assert((infix_vec[i].infix_ >> infix_size_) >= implicit);
        for (int32_t i = 1; i < infix_vec.size(); i++)
            assert((infix_vec[i - 1].infix_ >> infix_size_) <= (infix_vec[i].infix_ >> infix_size_));
#endif // DEBUG

        // Merge equal infixes to clean things up
        std::vector<Infix> merged_infix_vec = {infix_vec[0]};
        uint32_t merge_count = 1;
        for (int32_t i = 1; i < infix_vec.size(); i++) {
            const bool left_prefix_trie = merged_infix_vec.back().num_trie_bits_ == 0 
                || ((merged_infix_vec.back().trie_[0] & BITMASK(10)) == 0b0001000110)
                || (merged_infix_vec.back().trie_[0] & 1);
            const bool right_prefix_trie = infix_vec[i].num_trie_bits_ == 0 
                || ((infix_vec[i].trie_[0] & BITMASK(10)) == 0b0001000110)
                || (infix_vec[i].trie_[0] & 1);
            if (((merged_infix_vec.back().infix_ & 1) 
                    && merged_infix_vec.back().infix_ == infix_vec[i].infix_)
                    && (!left_prefix_trie || !right_prefix_trie)) {
                merged_infix_vec.back().Merge(infix_vec[i], infix_size_);
                merge_count++;
            }
            else {
                // Remove trie if possible
                if (merge_count == 1 && infix_vec[i - 1].num_trie_bits_ == 0) {
                    merged_infix_vec.back().num_suffixes_ = 0;
                    merged_infix_vec.back().num_suffix_bits_ = 0;
                    merged_infix_vec.back().num_trie_bits_ = 0;
                    merged_infix_vec.back().trie_.clear();
                    merged_infix_vec.back().trie_suffixes_.clear();
                }

                merged_infix_vec.push_back(infix_vec[i]);
                merge_count = 1;
            }
        }
        
        // Subtract out the implicit offset
        const uint64_t left_extraction = ExtractPartialKey(left_key, shared, ignore, implicit_size, 0);
        const uint64_t right_extraction = ExtractPartialKey(right_key, shared, ignore, implicit_size, 1);
        for (auto& infix : merged_infix_vec) {
#ifdef DEBUG
            assert((infix.infix_ >> infix_size_) >= (left_extraction >> infix_size_));
            assert((infix.infix_ >> infix_size_) <= (right_extraction >> infix_size_));
#endif // DEBUG
            infix.infix_ -= implicit << infix_size_;
        }

        // Setup the merged Infix Store
        const uint64_t *old_store_l_ptr = store_l->ptr;
        const uint32_t total_implicit = ((right_extraction >> infix_size_) - (left_extraction >> infix_size_)) + 1;
        InfixStore store = AllocateInfixStoreWithVector(merged_infix_vec, total_implicit); // TODO: Handle payloads
        if constexpr (payload_type == PayloadType::FixedLength) {
            // Make sure the sample payload list is moved
            store.ptr[1] = store_l->ptr[1];
        }

#ifdef DEBUG
        GetInfixVector(store);
#endif // DEBUG

        store_l->status = store.status;
        store_l->ptr = store.ptr;
        store_l->rwlock.store(store.rwlock.load(std::memory_order_acquire), std::memory_order_release);
        delete[] old_store_l_ptr;
        if constexpr (payload_type == PayloadType::FixedLength) {
            // Free the sample payload list for the deleted key
            free(reinterpret_cast<void *>(store_r->ptr[1]));
        }
        delete[] store_r->ptr;

        if constexpr (diva_type == DivaType::Int) {
            wh_int_del(better_tree_int_, middle_key.str,
                    middle_key.length,
                    leaves_to_unlock);
        }
        else {
            wh_del(better_tree_, middle_key.str,
                    middle_key.length,
                    leaves_to_unlock);
        }

        UnlockLeaves(leaves_to_unlock, it_write_lock);
        n_keys_.fetch_sub(1, std::memory_order_release);

        return;
    }

    uint64_t total_elem_count = store_l->GetFullSlotCount() + store_r->GetFullSlotCount();
    const bool should_allocate_on_heap = total_elem_count > heap_alloc_threshold;
    uint64_t infix_list_contents[should_allocate_on_heap ? 1 : total_elem_count + 1];
    uint64_t *infix_list = infix_list_contents;
    uint32_t payload_list_size = 1, right_payload_list_size = 1;
    if constexpr (payload_type == PayloadType::FixedLength) {
        payload_list_size = (total_elem_count + 2) * ((payload_size_ + 63) / 64);
        right_payload_list_size = (store_r->GetFullSlotCount() + 2) * ((payload_size_ + 63) / 64);
    }
    uint64_t payload_list_contents[should_allocate_on_heap ? 1 : payload_list_size + 1];
    uint64_t right_payload_list_contents[should_allocate_on_heap ? 1 : right_payload_list_size + 1];
    uint64_t *payload_list = payload_list_contents;
    uint64_t *right_payload_list = right_payload_list_contents;
    if (should_allocate_on_heap) {
        infix_list = new uint64_t[total_elem_count + 1];
        if constexpr (payload_type == PayloadType::FixedLength) {
            payload_list = new uint64_t[payload_list_size + 1];
            right_payload_list = new uint64_t[payload_list_size + 1];
        }
    }
    GetInfixList(*store_l, infix_list, payload_list);
    GetInfixList(*store_r, infix_list + store_l->GetFullSlotCount(), right_payload_list);
    if constexpr (payload_type == PayloadType::FixedLength) {
        copy_bitmap_to_bitmap(right_payload_list, 0,
                              payload_list, payload_size_ * store_l->GetFullSlotCount(),
                              payload_size_ * store_r->GetFullSlotCount());
    }

    UpdateInfixListDelete(shared, ignore, implicit_size, left_key, middle_key,
                          infix_list, store_l->GetFullSlotCount());
    UpdateInfixListDelete(shared, ignore, implicit_size, middle_key, right_key,
                          infix_list + store_l->GetFullSlotCount(), store_r->GetFullSlotCount());
    for (int32_t i = 0; i < total_elem_count; i++)
        infix_list[i] -= implicit << infix_size_;

    // Make sure the merged list is sorted by the infix starts
    if (0 < store_l->GetFullSlotCount() && store_l->GetFullSlotCount() < total_elem_count) {
        const uint64_t last_infix_l = infix_list[store_l->GetFullSlotCount() - 1];
        const uint64_t first_infix_r = infix_list[store_l->GetFullSlotCount()];
        if (!CompareInfixes(last_infix_l, first_infix_r)) {
            // Need to sort merge
            if constexpr (payload_type == PayloadType::FixedLength) {
                std::pair<uint64_t, uint32_t> pair_list_contents[should_allocate_on_heap ? 1 : total_elem_count + 1];
                std::pair<uint64_t, uint32_t> tmp_contents[should_allocate_on_heap ? 1 : total_elem_count + 1];
                std::pair<uint64_t, uint32_t> *pair_list = pair_list_contents;
                std::pair<uint64_t, uint32_t> *tmp = tmp_contents;
                if (should_allocate_on_heap) {
                    pair_list = new std::pair<uint64_t, uint32_t>[total_elem_count + 1];
                    tmp = new std::pair<uint64_t, uint32_t>[total_elem_count + 1];
                }

                for (uint32_t i = 0; i < total_elem_count; i++)
                    pair_list[i] = {infix_list[i], i};
                std::merge(pair_list, pair_list + store_l->GetFullSlotCount(),
                           pair_list + store_l->GetFullSlotCount(), pair_list + total_elem_count,
                           tmp, [&] (std::pair<uint64_t, uint32_t> a, std::pair<uint64_t, uint32_t> b) {
                                return CompareInfixes(a.first, b.first);
                           });
                uint64_t payload_list_copy_contents[should_allocate_on_heap ? 1 : payload_list_size + 1];
                uint64_t *payload_list_copy = payload_list_copy_contents;
                if (should_allocate_on_heap)
                    payload_list_copy = new uint64_t[payload_list_size + 1];
                memcpy(payload_list_copy, payload_list, (payload_list_size + 1) * sizeof(uint64_t));
                for (uint32_t i = 0; i < total_elem_count; i++) {
                    infix_list[i] = tmp[i].first;
                    const uint32_t pos_in = payload_size_ * tmp[i].second;
                    const uint32_t pos_out = payload_size_ * i;
                    copy_bitmap_to_bitmap(payload_list_copy, pos_in, payload_list, pos_out, payload_size_);
                }
                
                if (should_allocate_on_heap) {
                    delete[] pair_list;
                    delete[] tmp;
                    delete[] payload_list_copy;
                }
            }
            else {
                uint64_t tmp_contents[should_allocate_on_heap ? 1 : total_elem_count + 1];
                uint64_t *tmp = tmp_contents;
                if (should_allocate_on_heap)
                    tmp = new uint64_t[total_elem_count + 1];

                std::merge(infix_list, infix_list + store_l->GetFullSlotCount(),
                           infix_list + store_l->GetFullSlotCount(), infix_list + total_elem_count,
                           tmp, [&] (uint64_t a, uint64_t b) {
                                return CompareInfixes(a, b);
                           });
                memcpy(infix_list, tmp, sizeof(uint64_t) * total_elem_count);

                if (should_allocate_on_heap)
                    delete[] tmp;
            }
        }
    }

#ifdef DEBUG
    for (int32_t i = 1; i < total_elem_count; i++)
        assert(infix_list[i - 1] == infix_list[i] || CompareInfixes(infix_list[i - 1], infix_list[i]));
#endif // DEBUG

    const uint64_t *old_store_l_ptr = store_l->ptr;
    const uint64_t left_extraction = ExtractPartialKey(left_key, shared, ignore, implicit_size, 0);
    const uint64_t right_extraction = ExtractPartialKey(right_key, shared, ignore, implicit_size, 1);
    const uint32_t total_implicit = ((right_extraction >> infix_size_) - (left_extraction >> infix_size_)) + 1;

    InfixStore store = AllocateInfixStoreWithList(infix_list, total_elem_count, total_implicit, payload_list);
    if constexpr (payload_type == PayloadType::FixedLength) {
        // Make sure the sample payload list is moved
        store.ptr[1] = store_l->ptr[1];
    }

    store_l->status = store.status;
    store_l->ptr = store.ptr;
    store_l->rwlock.store(store.rwlock.load(std::memory_order_acquire), std::memory_order_release);
    delete[] old_store_l_ptr;
    if constexpr (payload_type == PayloadType::FixedLength) {
        // Free the sample payload list for the deleted key
        free(reinterpret_cast<void *>(store_r->ptr[1]));
    }
    delete[] store_r->ptr;
    if (should_allocate_on_heap) {
        delete[] infix_list;
        if constexpr (payload_type == PayloadType::FixedLength) {
            delete[] payload_list;
            delete[] right_payload_list;
        }
    }

    if constexpr (diva_type == DivaType::Int) {
        wh_int_del(better_tree_int_, middle_key.str,
                middle_key.length, leaves_to_unlock);
    }
    else {
        wh_del(better_tree_, middle_key.str,
                middle_key.length, leaves_to_unlock);
    }

    UnlockLeaves(leaves_to_unlock, it_write_lock);
    n_keys_.fetch_sub(1, std::memory_order_release);
}

template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::UpdateInfixListDelete(const uint32_t shared, const uint32_t ignore, const uint32_t implicit_size,
                                                                 const InfiniteByteString left_key, const InfiniteByteString right_key,
                                                                 uint64_t *infix_list, const uint32_t infix_list_len) const {
    const uint32_t shared_word_byte = (shared / 64) * 8;

    auto [old_shared, old_ignore, old_implicit_size] = GetSharedIgnoreImplicitLengths(
            {left_key.str + shared_word_byte, left_key.length < shared_word_byte ? 0 : left_key.length - shared_word_byte},
            {right_key.str + shared_word_byte, right_key.length < shared_word_byte ? 0 : right_key.length - shared_word_byte});
    old_shared += shared_word_byte * 8;

    const uint64_t old_left_implicit = ExtractPartialKey(left_key, old_shared, old_ignore,
                                                         old_implicit_size, 0) >> infix_size_;
    const uint32_t old_infix_size = old_implicit_size + infix_size_;
    const uint32_t new_infix_size = implicit_size + infix_size_;
    if (old_shared == shared) {
        for (int32_t i = 0; i < infix_list_len; i++) {
            infix_list[i] += old_left_implicit << infix_size_;
            const uint64_t old_diff_bit = infix_list[i] >> (old_infix_size - 1);
            infix_list[i] &= BITMASK(old_infix_size - 1);
            infix_list[i] = (new_infix_size > old_infix_size ? infix_list[i] << (new_infix_size - old_infix_size)
                                                             : (infix_list[i] >> (old_infix_size - new_infix_size))
                                                                    | (infix_list[i] & 1ULL));
            uint32_t recovered_bit_cnt = 0, recovery_bits = 1;
            recovered_bit_cnt += recovery_bits;
            uint64_t recovered_infix = old_diff_bit << (new_infix_size - recovered_bit_cnt);
            recovery_bits = std::min(old_ignore - ignore, new_infix_size - recovered_bit_cnt);
            recovered_bit_cnt += recovery_bits;
            recovered_infix |= (((1ULL << recovery_bits) - (1ULL ^ old_diff_bit)) & BITMASK(recovery_bits))
                                    << (new_infix_size - recovered_bit_cnt);
            if (recovered_bit_cnt < new_infix_size) {
                recovered_infix |= infix_list[i] >> (recovered_bit_cnt - 1);
                recovered_infix |= (lowbit_pos(infix_list[i]) < recovered_bit_cnt - 1 ? 1ULL : 0ULL);
            }
            else 
                recovered_infix |= 1ULL;
#ifdef DEBUG
            assert(recovered_bit_cnt <= new_infix_size);
#endif // DEBUG
            infix_list[i] = recovered_infix;
        }
    }
    else {
        for (int32_t i = 0; i < infix_list_len; i++) {
            infix_list[i] += old_left_implicit << infix_size_;
            const uint64_t old_diff_bit = infix_list[i] >> (old_infix_size - 1);
            infix_list[i] &= BITMASK(old_infix_size - 1);
            infix_list[i] = (new_infix_size > old_infix_size ? infix_list[i] << (new_infix_size - old_infix_size)
                                                             : (infix_list[i] >> (old_infix_size - new_infix_size))
                                                                    | (infix_list[i] & 1ULL));
            uint32_t recovered_bit_cnt = 1, recovery_bits = 1;
            uint64_t recovered_infix = left_key.GetBit(shared) << (new_infix_size - recovered_bit_cnt);
            
            recovery_bits = std::min(old_shared - shared - ignore - 1, new_infix_size - recovered_bit_cnt);
            recovered_bit_cnt += recovery_bits;
            recovered_infix |= left_key.BitsAt(shared + ignore + 1, recovery_bits)
                                    << (new_infix_size - recovered_bit_cnt);
            if (recovered_bit_cnt < new_infix_size) {
                recovered_infix |= old_diff_bit << (new_infix_size - recovered_bit_cnt - 1);
                recovery_bits = std::min(old_ignore + 1, new_infix_size - recovered_bit_cnt);
                recovered_bit_cnt += recovery_bits;
                recovered_infix |= (recovery_bits > 1 ? (((1ULL << (recovery_bits - 1)) - (1ULL ^ old_diff_bit)) 
                                                            & BITMASK(recovery_bits - 1)) 
                                                                << (new_infix_size - recovered_bit_cnt)
                                                      : 0ULL);
            }
            if (recovered_bit_cnt < new_infix_size) {
                recovered_infix |= infix_list[i] >> (recovered_bit_cnt - 1);
                recovered_infix |= (lowbit_pos(infix_list[i]) < recovered_bit_cnt - 1 ? 1ULL : 0ULL);
            }
            else 
                recovered_infix |= 1ULL;
#ifdef DEBUG
            assert(recovered_bit_cnt <= new_infix_size);
#endif // DEBUG
            infix_list[i] = recovered_infix;
        }
    }
}


template <DivaType diva_type, PayloadType payload_type>
inline std::vector<typename Diva<diva_type, payload_type>::Infix>
Diva<diva_type, payload_type>::UpdateInfixVectorDelete(const uint32_t shared, const uint32_t ignore, const uint32_t implicit_size,
                                                       const InfiniteByteString left_key, const InfiniteByteString right_key,
                                                       std::vector<Infix>& infix_vec) const {
    std::vector<Infix> res;
    if (infix_vec.empty())
        return std::move(res);
    const uint32_t shared_word_byte = (shared / 64) * 8;

    auto [old_shared, old_ignore, old_implicit_size] = GetSharedIgnoreImplicitLengths(
            {left_key.str + shared_word_byte, left_key.length < shared_word_byte ? 0 : left_key.length - shared_word_byte},
            {right_key.str + shared_word_byte, right_key.length < shared_word_byte ? 0 : right_key.length - shared_word_byte});
    old_shared += shared_word_byte * 8;
    const uint64_t old_left_implicit = ExtractPartialKey(left_key, old_shared, old_ignore,
                                                         old_implicit_size, 0) >> infix_size_;
    const uint32_t old_infix_size = old_implicit_size + infix_size_;
    const uint32_t new_infix_size = implicit_size + infix_size_;

    uint32_t prefix_shared_bits_len, prefix_ignore_bits_len, prefix_offset;
    const bool only_ignore_bits_grew = old_shared == shared;
    if (only_ignore_bits_grew) {
        prefix_shared_bits_len = 0;
        prefix_ignore_bits_len = old_ignore - ignore;
        prefix_offset = 0;
    }
    else {
        prefix_shared_bits_len = old_shared - shared - ignore - 1;
        prefix_ignore_bits_len = old_shared + old_ignore - shared - ignore
                                  - prefix_shared_bits_len - 1;
        prefix_offset = (shared + ignore) % 8;
    }

    uint8_t prefix_contents[(prefix_shared_bits_len + prefix_ignore_bits_len) / 8 + 4];
    memset(prefix_contents, 0, (prefix_shared_bits_len + prefix_ignore_bits_len) / 8 + 4);

    if (!only_ignore_bits_grew) {   // Set the shared bits up
        memcpy(prefix_contents + (prefix_offset == 7),
               left_key.str + (shared + ignore + 1) / 8,
               (prefix_shared_bits_len + prefix_offset + 1 + 7) / 8); // +1 to account for the new diff bit
        prefix_contents[0] &= ~(1UL << (7 - prefix_offset));
        prefix_contents[0] |= left_key.GetBit(shared) << (7 - prefix_offset);
    }

    uint64_t last_old_diff_bit = 2;     // To track if prefix needs updating
    for (auto infix : infix_vec) {
        // Reconstruct the old infix
        infix.infix_ += old_left_implicit << infix_size_;
        const uint64_t old_diff_bit = infix.infix_ >> (old_infix_size - 1);
        infix.infix_ &= BITMASK(old_infix_size - 1);

        // Update the prefix if needed
        if (last_old_diff_bit != old_diff_bit) {
            if (only_ignore_bits_grew) {
                memset(prefix_contents,
                    old_diff_bit ? 0x00 : 0xFF,
                    (prefix_ignore_bits_len + 8) / 8);
                prefix_contents[0] &= BITMASK(7);
                prefix_contents[0] |= old_diff_bit << 7;
            }
            else {
                uint32_t bit_pos = prefix_offset + prefix_shared_bits_len + 2;
#ifdef DEBUG
                assert(static_cast<int32_t>(prefix_ignore_bits_len - (8 - bit_pos % 8) + 7) / 8 < 100);
#endif // DEBUG
                memset(prefix_contents + bit_pos / 8 + 1,
                       old_diff_bit ? 0x00 : 0xFF,
                       static_cast<int32_t>(prefix_ignore_bits_len - (8 - bit_pos % 8) + 7) / 8);

                bit_pos--;
                prefix_contents[bit_pos / 8] &= ~BITMASK(8 - (bit_pos % 8));
                prefix_contents[bit_pos / 8] |= old_diff_bit << (7 - (bit_pos % 8));
                if (!old_diff_bit)
                    prefix_contents[(bit_pos + 1) / 8] |= BITMASK(8 - ((bit_pos + 1) % 8));
                if (prefix_ignore_bits_len > (7 - (bit_pos % 8))) {     // Missed the bits in the next byte...
                    const uint32_t bit_cnt = prefix_ignore_bits_len - (7 - (bit_pos % 8));
                    if (old_diff_bit)
                        prefix_contents[bit_pos / 8 + 1] &= (~BITMASK(bit_cnt)) << (8 - bit_cnt);
                    else
                        prefix_contents[bit_pos / 8 + 1] |= BITMASK(bit_cnt) << (8 - bit_cnt);
                }
            }
            last_old_diff_bit = old_diff_bit;
        }

        uint32_t prefix_len = prefix_shared_bits_len + prefix_ignore_bits_len + 1 + (!only_ignore_bits_grew);
        InfiniteByteString prefix = {prefix_contents, prefix_offset + prefix_len};

        // Adjust the infix size and prepend the prefix
        if (new_infix_size < old_infix_size) {
            prefix_contents[prefix.length / 8] &= ~(1UL << (7 - (prefix.length % 8)));
            prefix_contents[prefix.length / 8] |= (infix.infix_ >> (old_infix_size - 2)) 
                                                << (7 - (prefix.length % 8));
            infix.infix_ &= BITMASK(old_infix_size - 2);
            prefix_len++;
            prefix.length++;
        }
        else if (new_infix_size > old_infix_size) {
            infix.infix_ |= prefix.GetBitBitLength(prefix.length - 1) << (old_infix_size - 1);
            prefix_len--;
            prefix.length--;
        }
        const bool had_trie = infix.num_trie_bits_ != 0;
        infix.PrependPrefix(prefix, prefix_offset + 1, prefix_len - 1,
                new_infix_size - 1, infix_size_);
        infix.infix_ |= prefix.GetBitBitLength(prefix_offset) << (new_infix_size - 1);
        if (!had_trie)
            res.emplace_back(infix.infix_);
        else 
            res.push_back(infix);
    }
    return std::move(res);
}


template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::Adapt(uint64_t key, const uint32_t new_prefix_len) {
    key = __builtin_bswap64(key);
    Adapt(reinterpret_cast<const uint8_t *>(&key), sizeof(key), new_prefix_len);
}


template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::Adapt(std::string_view key, const uint32_t new_prefix_len) {
    Adapt(reinterpret_cast<const uint8_t *>(key.data()), key.size(), new_prefix_len);
}


template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::Adapt(const uint8_t *input_key, const uint32_t input_key_len,
                                                 const uint32_t new_prefix_len) {
    const bool it_write_lock = false;
    InfixStore *infix_store_ptr;
    void *leaves_to_unlock[3] = {};

    InfiniteByteString next_key {};
    InfiniteByteString prev_key {};
    InfiniteByteString key = {input_key, input_key_len};

    wormhole_int_iter it_int;
    wormhole_iter it;
    GetLowerUpperBounds(key, it_write_lock, leaves_to_unlock, it, it_int,
                        prev_key, next_key, infix_store_ptr);
    uint64_t prev_key_word, next_key_word;
    if constexpr (diva_type == DivaType::Int) {
        prev_key_word = *reinterpret_cast<const uint64_t *>(prev_key.str);
        prev_key.str = reinterpret_cast<const uint8_t *>(&prev_key_word);
        next_key_word = *reinterpret_cast<const uint64_t *>(next_key.str);
        next_key.str = reinterpret_cast<const uint8_t *>(&next_key_word);
    }

    InfixStore& infix_store = *infix_store_ptr;
    rwlock_lock_write(infix_store.rwlock);
    UnlockLeaves(leaves_to_unlock, it_write_lock);

    if (prev_key == key) { // Nothing to adapt
        rwlock_unlock_write(infix_store.rwlock);
        return;
    }

    auto [shared, ignore, implicit_size] = GetSharedIgnoreImplicitLengths(prev_key, next_key);
    // To adapt the binary tries
    const uint32_t key_start_bit = shared + ignore + implicit_size;
    if (new_prefix_len < key_start_bit + 1) {   // Nothing to adapt
        rwlock_unlock_write(infix_store.rwlock);
        return;
    }
    const uint32_t adapt_length = new_prefix_len - key_start_bit - 1;

    const uint64_t extraction = ExtractPartialKey(key, shared, ignore, implicit_size, key.GetBit(shared));
    const uint64_t next_implicit = ExtractPartialKey(next_key, shared, ignore, implicit_size, 1) >> infix_size_;
    const uint64_t prev_implicit = ExtractPartialKey(prev_key, shared, ignore, implicit_size, 0) >> infix_size_;
    const uint32_t total_implicit = next_implicit - prev_implicit + 1;
    const uint64_t adaptee = ((extraction | 1ULL) - (prev_implicit << infix_size_));
    AdaptRawInInfixStore(infix_store, adaptee, 
                         {key.str, 8 * key.length}, 
                         key_start_bit,
                         adapt_length,
                         total_implicit);
    rwlock_unlock_write(infix_store.rwlock);
}


// TODO: Add the trie tof here too
template <DivaType diva_type, PayloadType payload_type>
template <class t_itr>
inline void Diva<diva_type, payload_type>::BulkLoadFixedLength(const t_itr begin, const t_itr end, const uint32_t key_len,
                                                               const uint64_t **payloads) {
    using itr_type = typename std::iterator_traits<t_itr>::value_type;
    constexpr bool itr_type_is_string = std::is_pointer_v<itr_type>
        || std::is_same_v<std::decay_t<itr_type>, std::string>
        || std::is_same_v<std::decay_t<itr_type>, std::string_view>;

    void *dummy_locked_leaf_addrs[3] = {nullptr, nullptr, nullptr};
    uint64_t infix_list[infix_store_target_size], int_opt_buf[infix_store_target_size + 3];
    std::vector<Infix> infix_vec;
    infix_vec.reserve(infix_store_target_size);
    uint32_t payload_list_size = 1;
    if constexpr (payload_type == PayloadType::FixedLength)
        payload_list_size = (infix_store_target_size * payload_size_ + 63) / 64;
    uint64_t left_payload[(payload_size_ + 63) / 64 + 1], right_payload[(payload_size_ + 63) / 64 + 1];
    uint64_t payload_list[payload_list_size + 1];
    t_itr last_key_it = begin, key_it = begin;
    InfiniteByteString left_key {}, right_key {};
    std::string_view sv;
    if constexpr (diva_type == DivaType::Int) {
        int_opt_buf[0] = __builtin_bswap64(*key_it);
        left_key = {reinterpret_cast<const uint8_t *>(int_opt_buf + 0), key_len};
    }
    else {
        if constexpr (itr_type_is_string) {
            sv = *key_it;
            left_key = {reinterpret_cast<const uint8_t *>(sv.data()),
                        static_cast<uint32_t>(sv.size())};
        }
        else 
            left_key = {reinterpret_cast<const uint8_t *>(&(*key_it)), key_len};
    }
    if constexpr (payload_type == PayloadType::FixedLength)
        copy_bitmap_to_bitmap(payloads[0], 0, left_payload, 0, payload_size_);
    int32_t cnt = 1;
    for (++key_it; key_it != end; ++key_it) {
        if (cnt % infix_store_target_size == 0) {   // New boundary key
            if constexpr (diva_type == DivaType::Int) {
                int_opt_buf[1] = __builtin_bswap64(*key_it);
                right_key = {reinterpret_cast<const uint8_t *>(int_opt_buf + 1), key_len};
            }
            else {
                if constexpr (itr_type_is_string) {
                    sv = *key_it;
                    right_key = {reinterpret_cast<const uint8_t *>(sv.data()),
                                 static_cast<uint32_t>(sv.size())};
                }
                else
                    right_key = {reinterpret_cast<const uint8_t *>(&(*key_it)), key_len};
            }
            if constexpr (payload_type == PayloadType::FixedLength)
                copy_bitmap_to_bitmap(payloads[cnt], 0, right_payload, 0, payload_size_);

            const auto [shared, ignore, implicit_size] = GetSharedIgnoreImplicitLengths(left_key, right_key);
            const uint32_t key_start_bit = shared + ignore + implicit_size + infix_size_ - 1;
            const uint64_t prev_implicit = ExtractPartialKey(left_key, shared, ignore, implicit_size, 0) >> infix_size_;
            const uint64_t next_implicit = ExtractPartialKey(right_key, shared, ignore, implicit_size, 1) >> infix_size_;
            const uint32_t total_implicit = next_implicit - prev_implicit + 1;
            ++last_key_it;
            uint32_t last_key_pos = 0, last_infix_pos = 0;
            if constexpr (payload_type == PayloadType::FixedLength)
                last_key_pos = std::distance(begin, last_key_it);
            InfiniteByteString keys[infix_store_target_size - 1];
            infix_vec.clear();
            for (int32_t i = 0; i < infix_store_target_size - 1; i++) {
                if constexpr (diva_type == DivaType::Int) {
                    int_opt_buf[2 + i] = __builtin_bswap64(*last_key_it);
                    keys[i] = {reinterpret_cast<const uint8_t *>(int_opt_buf + 2 + i), key_len};
                }
                else {
                    if constexpr (itr_type_is_string) {
                        sv = *last_key_it;
                        keys[i] = {reinterpret_cast<const uint8_t *>(sv.data()),
                                   static_cast<uint32_t>(sv.size())};
                    }
                    else
                        keys[i] = {reinterpret_cast<const uint8_t *>(&(*last_key_it)), key_len};
                }
                const uint64_t extraction = ExtractPartialKey(keys[i], shared, ignore, implicit_size, keys[i].GetBit(shared));
                infix_list[i] = ((extraction | 1ULL) - (prev_implicit << infix_size_));
                if constexpr (diva_type == DivaType::BinaryTrie) {
                    keys[i].length *= 8;
                    if (infix_list[last_infix_pos] != infix_list[i]) {
                        infix_vec.emplace_back(infix_list[last_infix_pos]);
                        if (i - last_infix_pos > 1) {
                            infix_vec.back().BuildTrieAndSuffixes(keys + last_infix_pos,
                                    i - last_infix_pos, key_start_bit, infix_size_, 
                                    false, false, true);
                        }
                        last_infix_pos = i;
                    }
                }
                if constexpr (payload_type == PayloadType::FixedLength) {
                    copy_bitmap_to_bitmap(payloads[last_key_pos], 0, payload_list, i * payload_size_, payload_size_);
                    last_key_pos++;
                }
                ++last_key_it;
            }
            if constexpr (diva_type == DivaType::BinaryTrie) {
                infix_vec.emplace_back(infix_list[last_infix_pos]);
                if (infix_store_target_size - 1 - last_infix_pos > 1) {
                    infix_vec.back().BuildTrieAndSuffixes(keys + last_infix_pos,
                            infix_store_target_size - 1 - last_infix_pos, key_start_bit, infix_size_,
                            false, false, true);
                }
                last_infix_pos = infix_store_target_size - 1;
            }
            n_keys_.fetch_add(infix_store_target_size, std::memory_order_release);

            uint32_t allocation_size_grade = size_scalar_shrink_grow_sep;
            if constexpr (diva_type == DivaType::BinaryTrie) {
                uint32_t num_slots_filled = 0;
                for (auto& infix : infix_vec)
                    num_slots_filled += infix.GetNumSlots(infix_size_);
                allocation_size_grade = std::upper_bound(scaled_sizes_, scaled_sizes_ + size_scalar_count, num_slots_filled) - scaled_sizes_;
            }
            InfixStore store(scaled_sizes_[allocation_size_grade], infix_size_,
                             allocation_size_grade, payload_size_);
            if constexpr (diva_type == DivaType::BinaryTrie)
                LoadVectorToInfixStore(store, infix_vec, total_implicit, true, payload_list);
            else
                LoadListToInfixStore(store, infix_list, infix_store_target_size - 1, total_implicit, true, payload_list);
            if constexpr (payload_type == PayloadType::FixedLength) {
                uint64_t *sample_payloads = reinterpret_cast<uint64_t *>(malloc(((payload_size_ + 63) / 64) * sizeof(uint64_t)));
                store.ptr[1] = reinterpret_cast<uint64_t>(sample_payloads);
                AddSamplePayload(store, left_payload);
            }
            if constexpr (diva_type == DivaType::Int)
                wh_int_put(better_tree_int_, left_key.str, left_key.length, &store, sizeof(store), dummy_locked_leaf_addrs);
            else
                wh_put(better_tree_, left_key.str, left_key.length, &store, sizeof(store), dummy_locked_leaf_addrs);

            if constexpr (diva_type == DivaType::Int)
                int_opt_buf[0] = int_opt_buf[1];
            else
                left_key = right_key;
            if constexpr (payload_type == PayloadType::FixedLength)
                copy_bitmap_to_bitmap(right_payload, 0, left_payload, 0, payload_size_);
        }
        cnt++;
    }

    // Add what was left from the loop
    key_it--;
    if constexpr (diva_type == DivaType::Int) {
        int_opt_buf[1] = __builtin_bswap64(*key_it);
        right_key = {reinterpret_cast<const uint8_t *>(int_opt_buf + 1), key_len};
    }
    else {
        if constexpr (itr_type_is_string) {
            sv = *key_it;
            right_key = {reinterpret_cast<const uint8_t *>(sv.data()),
                         static_cast<uint32_t>(sv.size())};
        }
        else
            right_key = {reinterpret_cast<const uint8_t *>(&(*key_it)), key_len};
    }
    if constexpr (payload_type == PayloadType::FixedLength)
        copy_bitmap_to_bitmap(payloads[std::distance(begin, key_it)], 0, right_payload, 0, payload_size_);
    n_keys_.fetch_add(1, std::memory_order_release);

    if (key_it != last_key_it) {
        const auto [shared, ignore, implicit_size] = GetSharedIgnoreImplicitLengths(left_key, right_key);
        const uint32_t key_start_bit = shared + ignore + implicit_size + infix_size_ - 1;
        const uint64_t prev_implicit = ExtractPartialKey(left_key, shared, ignore, implicit_size, 0) >> infix_size_;
        const uint64_t next_implicit = ExtractPartialKey(right_key, shared, ignore, implicit_size, 1) >> infix_size_;
        const uint32_t total_implicit = next_implicit - prev_implicit + 1;
        int32_t i = 0;
        ++last_key_it;
        n_keys_.fetch_add(1, std::memory_order_release);
        uint32_t last_key_pos = 0, last_infix_pos = 0;
        if constexpr (payload_type == PayloadType::FixedLength)
            last_key_pos = std::distance(begin, last_key_it);
        InfiniteByteString keys[infix_store_target_size - 1];
        infix_vec.clear();
        while (last_key_it != key_it) {
            if constexpr (diva_type == DivaType::Int) {
                int_opt_buf[2 + i] = __builtin_bswap64(*last_key_it);
                keys[i] = {reinterpret_cast<const uint8_t *>(int_opt_buf + 2 + i), key_len};
            }
            else {
                if constexpr (itr_type_is_string) {
                    sv = *last_key_it;
                    keys[i] = {reinterpret_cast<const uint8_t *>(sv.data()),
                               static_cast<uint32_t>(sv.size())};
                }
                else
                    keys[i] = {reinterpret_cast<const uint8_t *>(&(*last_key_it)), key_len};
            }
            const uint64_t extraction = ExtractPartialKey(keys[i], shared, ignore, implicit_size, keys[i].GetBit(shared));
            infix_list[i] = ((extraction | 1ULL) - (prev_implicit << infix_size_));
            if constexpr (diva_type == DivaType::BinaryTrie) {
                keys[i].length *= 8;
                if (infix_list[last_infix_pos] != infix_list[i]) {
                    infix_vec.emplace_back(infix_list[last_infix_pos]);
                    if (i - last_infix_pos > 1) {
                        infix_vec.back().BuildTrieAndSuffixes(keys + last_infix_pos,
                                i - last_infix_pos, key_start_bit, infix_size_,
                                false, false, true);
                    }
                    last_infix_pos = i;
                }
            }
            if constexpr (payload_type == PayloadType::FixedLength) {
                copy_bitmap_to_bitmap(payloads[last_key_pos], 0, payload_list, i * payload_size_, payload_size_);
                last_key_pos++;
            }
            i++;
            ++last_key_it;
            n_keys_.fetch_add(1, std::memory_order_release);
        }
        if constexpr (diva_type == DivaType::BinaryTrie) {
            infix_vec.emplace_back(infix_list[last_infix_pos]);
            if (i - last_infix_pos > 1) {
                infix_vec.back().BuildTrieAndSuffixes(keys + last_infix_pos,
                        i - last_infix_pos, key_start_bit, infix_size_,
                        false, false, true);
            }
            last_infix_pos = i;
        }

        uint32_t allocation_size_grade = std::upper_bound(scaled_sizes_, scaled_sizes_ + size_scalar_count, i) - scaled_sizes_;
        if constexpr (diva_type == DivaType::BinaryTrie) {
            uint32_t num_slots_filled = 0;
            for (auto& infix : infix_vec)
                num_slots_filled += infix.GetNumSlots(infix_size_);
            allocation_size_grade = std::upper_bound(scaled_sizes_, scaled_sizes_ + size_scalar_count, num_slots_filled) - scaled_sizes_;
        }
        InfixStore store(scaled_sizes_[allocation_size_grade], infix_size_, allocation_size_grade, payload_size_);
        if constexpr (diva_type == DivaType::BinaryTrie)
            LoadVectorToInfixStore(store, infix_vec, total_implicit, true, payload_list);
        else
            LoadListToInfixStore(store, infix_list, i, total_implicit, true, payload_list);
        if constexpr (payload_type == PayloadType::FixedLength) {
            uint64_t *sample_payloads = reinterpret_cast<uint64_t *>(malloc(((payload_size_ + 63) / 64) * sizeof(uint64_t)));
            store.ptr[1] = reinterpret_cast<uint64_t>(sample_payloads);
            AddSamplePayload(store, left_payload);
        }
        if constexpr (diva_type == DivaType::Int)
            wh_int_put(better_tree_int_, left_key.str, left_key.length, &store, sizeof(store), dummy_locked_leaf_addrs);
        else
            wh_put(better_tree_, left_key.str, left_key.length, &store, sizeof(store), dummy_locked_leaf_addrs);

        if constexpr (payload_type == PayloadType::FixedLength)
            AddTreeKey(right_key.str, right_key.length, right_payload);
        else 
            AddTreeKey(right_key.str, right_key.length);
    }

    uint8_t max_str[key_len];
    memset(max_str, 0xFF, key_len);
    right_key = {max_str, key_len};
}


static uint64_t stats_num_duplicate_infixes = 0;

template <DivaType diva_type, PayloadType payload_type>
template <class t_itr>
inline void Diva<diva_type, payload_type>::BulkLoad(const t_itr begin, const t_itr end,
                                                    const uint64_t **payloads) {
    void *dummy_locked_leaf_addrs[3] = {nullptr, nullptr, nullptr};
    uint64_t infix_list[infix_store_target_size];
    std::vector<Infix> infix_vec;
    infix_vec.reserve(infix_store_target_size);
    uint32_t payload_list_size = 1;
    if constexpr (payload_type == PayloadType::FixedLength)
        payload_list_size = (infix_store_target_size * payload_size_ + 63) / 64;
    uint64_t left_payload[(payload_size_ + 63) / 64 + 1], right_payload[(payload_size_ + 63) / 64 + 1];
    uint64_t payload_list_contents[payload_list_size + 1];
    uint64_t *payload_list = nullptr;
    if constexpr (payload_type == PayloadType::FixedLength)
        payload_list = payload_list_contents;
    t_itr last_key_it = begin, key_it = begin;
    std::string_view sv {*key_it};
    InfiniteByteString left_key {reinterpret_cast<const uint8_t *>(sv.data()), 
                                 static_cast<uint32_t>(sv.size())};
    InfiniteByteString right_key {};
    if constexpr (payload_type == PayloadType::FixedLength)
        copy_bitmap_to_bitmap(payloads[0], 0, left_payload, 0, payload_size_);
    int32_t cnt = 1;
    uint32_t max_len = sv.size();
#if ACTUAL_SUFFIX_LEN_MODE == 0
    const uint32_t tof_bit_count = infix_size_;
#else
    const uint32_t tof_bit_count = 5;
#endif
    for (++key_it; key_it != end; ++key_it) {
        if (cnt % infix_store_target_size == 0) {   // New boundary key
            std::string_view sv = *key_it;
            right_key = {reinterpret_cast<const uint8_t *>(sv.data()), 
                         static_cast<uint32_t>(sv.size())};
            if constexpr (payload_type == PayloadType::FixedLength)
                copy_bitmap_to_bitmap(payloads[cnt], 0, right_payload, 0, payload_size_);

            const auto [shared, ignore, implicit_size] = GetSharedIgnoreImplicitLengths(left_key, right_key);
            const uint32_t key_start_bit = shared + ignore + implicit_size + tof_bit_count - 1;
            const uint64_t prev_implicit = ExtractPartialKey(left_key, shared, ignore, implicit_size, 0) >> infix_size_;
            const uint64_t next_implicit = ExtractPartialKey(right_key, shared, ignore, implicit_size, 1) >> infix_size_;
            const uint32_t total_implicit = next_implicit - prev_implicit + 1;
            ++last_key_it;
            uint32_t last_key_pos = 0, last_infix_pos = 0;
            if constexpr (payload_type == PayloadType::FixedLength)
                last_key_pos = std::distance(begin, last_key_it);
            InfiniteByteString keys[infix_store_target_size - 1];
            infix_vec.clear();
            bool has_duplicates = false;
            for (int32_t i = 0; i < infix_store_target_size - 1; i++) {
                sv = *last_key_it;
                keys[i] = {reinterpret_cast<const uint8_t *>(sv.data()), static_cast<uint32_t>(sv.size())};
                const uint64_t extraction = ExtractPartialKey(keys[i], shared, ignore, implicit_size, keys[i].GetBit(shared));
                infix_list[i] = ((extraction | 1ULL) - (prev_implicit << infix_size_));
                stats_num_duplicate_infixes += i > 0 && infix_list[i] == infix_list[i - 1];

                if (!has_duplicates && i > 0 &&
                        (infix_list[i - 1] & (~BITMASK(infix_size_ - tof_bit_count + 1)))
                            == (infix_list[i] & (~BITMASK(infix_size_ - tof_bit_count + 1)))) {
                    for (int32_t j = 0; j < i - 1; j++)
                        infix_vec.emplace_back(infix_list[j]);
                    has_duplicates = true;
                    last_infix_pos = i - 1;
                }
                keys[i].length *= 8;
                if (has_duplicates && diva_type == DivaType::BinaryTrie) {
                    if ((infix_list[last_infix_pos] & (~BITMASK(infix_size_ - tof_bit_count + 1)))
                            != (infix_list[i] & (~BITMASK(infix_size_ - tof_bit_count + 1)))) {
                        infix_vec.emplace_back(infix_list[last_infix_pos]);
                        if (i - last_infix_pos > 1) {
                            infix_vec.back().BuildTrieAndSuffixes(keys + last_infix_pos,
                                    i - last_infix_pos, key_start_bit, infix_size_,
                                    false, false, true);
                            infix_vec.back().infix_ &= ~BITMASK(infix_size_ - tof_bit_count);
                            infix_vec.back().infix_ |= 1UL << (infix_size_ - tof_bit_count);
                        }
                        last_infix_pos = i;
                    }
                }
                if constexpr (payload_type == PayloadType::FixedLength) {
                    copy_bitmap_to_bitmap(payloads[last_key_pos], 0, payload_list, i * payload_size_, payload_size_);
                    last_key_pos++;
                }
                ++last_key_it;
            }
            if (has_duplicates && diva_type == DivaType::BinaryTrie) {
                infix_vec.emplace_back(infix_list[last_infix_pos]);
                if (infix_store_target_size - 1 - last_infix_pos > 1) {
                    infix_vec.back().BuildTrieAndSuffixes(keys + last_infix_pos,
                            infix_store_target_size - 1 - last_infix_pos, key_start_bit, infix_size_,
                        false, false, true);
                    infix_vec.back().infix_ &= ~BITMASK(infix_size_ - tof_bit_count);
                    infix_vec.back().infix_ |= 1UL << (infix_size_ - tof_bit_count);
                }
                last_infix_pos = infix_store_target_size - 1;
                if (tof_bit_count < infix_size_)
                    std::stable_sort(infix_vec.begin(), infix_vec.end());
            }
            n_keys_.fetch_add(infix_store_target_size, std::memory_order_release);

            uint32_t allocation_size_grade = size_scalar_shrink_grow_sep;
            if (has_duplicates && diva_type == DivaType::BinaryTrie) {
                uint32_t num_slots_filled = 0;
                for (auto& infix : infix_vec)
                    num_slots_filled += infix.GetNumSlots(infix_size_);
                allocation_size_grade = std::upper_bound(scaled_sizes_, scaled_sizes_ + size_scalar_count, num_slots_filled) - scaled_sizes_;
            }
            InfixStore store(scaled_sizes_[allocation_size_grade], infix_size_,
                             allocation_size_grade, payload_size_);
            if (has_duplicates && diva_type == DivaType::BinaryTrie)
                LoadVectorToInfixStore(store, infix_vec, total_implicit, true, payload_list);
            else 
                LoadListToInfixStore(store, infix_list, infix_store_target_size - 1, total_implicit, true, payload_list);
            if constexpr (payload_type == PayloadType::FixedLength) {
                uint64_t *sample_payloads = reinterpret_cast<uint64_t *>(malloc(((payload_size_ + 63) / 64) * sizeof(uint64_t)));
                store.ptr[1] = reinterpret_cast<uint64_t>(sample_payloads);
                AddSamplePayload(store, left_payload);
            }
            if constexpr (diva_type == DivaType::Int)
                wh_int_put(better_tree_int_, left_key.str, left_key.length, &store, sizeof(store), dummy_locked_leaf_addrs);
            else
                wh_put(better_tree_, left_key.str, left_key.length, &store, sizeof(store), dummy_locked_leaf_addrs);
            left_key = right_key;
            if constexpr (payload_type == PayloadType::FixedLength)
                copy_bitmap_to_bitmap(right_payload, 0, left_payload, 0, payload_size_);
        }
        sv = *key_it;
        max_len = std::max<uint32_t>(max_len, sv.size());
        cnt++;
    }

    // Add what was left from the loop
    key_it--;
    sv = *key_it;
    right_key = {reinterpret_cast<const uint8_t *>(sv.data()), 
                 static_cast<uint32_t>(sv.size())};
    if constexpr (payload_type == PayloadType::FixedLength)
        copy_bitmap_to_bitmap(payloads[std::distance(begin, key_it)], 0, right_payload, 0, payload_size_);
    n_keys_.fetch_add(1, std::memory_order_release);

    if (key_it != last_key_it) {
        const auto [shared, ignore, implicit_size] = GetSharedIgnoreImplicitLengths(left_key, right_key);
        const uint32_t key_start_bit = shared + ignore + implicit_size + tof_bit_count - 1;
        const uint64_t prev_implicit = ExtractPartialKey(left_key, shared, ignore, implicit_size, 0) >> infix_size_;
        const uint64_t next_implicit = ExtractPartialKey(right_key, shared, ignore, implicit_size, 1) >> infix_size_;
        const uint32_t total_implicit = next_implicit - prev_implicit + 1;
        int32_t i = 0;
        ++last_key_it;
        n_keys_.fetch_add(1, std::memory_order_release);
        uint32_t last_key_pos = 0, last_infix_pos = 0;
        if constexpr (payload_type == PayloadType::FixedLength)
            last_key_pos = std::distance(begin, last_key_it);
        InfiniteByteString keys[infix_store_target_size - 1];
        infix_vec.clear();
        bool has_duplicates = false;
        while (last_key_it != key_it) {
            sv = *last_key_it;
            keys[i] = {reinterpret_cast<const uint8_t *>(sv.data()), static_cast<uint32_t>(sv.size())};
            const uint64_t extraction = ExtractPartialKey(keys[i], shared, ignore, implicit_size, keys[i].GetBit(shared));
            infix_list[i] = ((extraction | 1ULL) - (prev_implicit << infix_size_));
            stats_num_duplicate_infixes += i > 0 && infix_list[i] == infix_list[i - 1];

            if (!has_duplicates && i > 0 &&
                    (infix_list[i - 1] & (~BITMASK(infix_size_ - tof_bit_count + 1)))
                        == (infix_list[i] & (~BITMASK(infix_size_ - tof_bit_count + 1)))) {
                for (int32_t j = 0; j < i - 1; j++)
                    infix_vec.emplace_back(infix_list[j]);
                has_duplicates = true;
                last_infix_pos = i - 1;
            }
            keys[i].length *= 8;
            if (has_duplicates && diva_type == DivaType::BinaryTrie) {
                if ((infix_list[last_infix_pos] & (~BITMASK(infix_size_ - tof_bit_count + 1)))
                        != (infix_list[i] & (~BITMASK(infix_size_ - tof_bit_count + 1)))) {
                    infix_vec.emplace_back(infix_list[last_infix_pos]);
                    if (i - last_infix_pos > 1) {
                        infix_vec.back().BuildTrieAndSuffixes(keys + last_infix_pos,
                                i - last_infix_pos, key_start_bit, infix_size_,
                                false, false, true);
                        infix_vec.back().infix_ &= ~BITMASK(infix_size_ - tof_bit_count);
                        infix_vec.back().infix_ |= 1UL << (infix_size_ - tof_bit_count);
                    }
                    last_infix_pos = i;
                }
            }
            if constexpr (payload_type == PayloadType::FixedLength) {
                copy_bitmap_to_bitmap(payloads[last_key_pos], 0, payload_list, i * payload_size_, payload_size_);
                last_key_pos++;
            }
            i++;
            ++last_key_it;
            n_keys_.fetch_add(1, std::memory_order_release);
        }
        if (has_duplicates && diva_type == DivaType::BinaryTrie) {
            infix_vec.emplace_back(infix_list[last_infix_pos]);
            if (i - last_infix_pos > 1) {
                infix_vec.back().BuildTrieAndSuffixes(keys + last_infix_pos,
                        i - last_infix_pos, key_start_bit, infix_size_,
                        false, false, true);
                infix_vec.back().infix_ &= ~BITMASK(infix_size_ - tof_bit_count);
                infix_vec.back().infix_ |= 1UL << (infix_size_ - tof_bit_count);
            }
            last_infix_pos = i;
            if (tof_bit_count < infix_size_)
                std::stable_sort(infix_vec.begin(), infix_vec.end());
        }

        uint32_t allocation_size_grade = std::upper_bound(scaled_sizes_, scaled_sizes_ + size_scalar_count, i) - scaled_sizes_;
        if (has_duplicates && diva_type == DivaType::BinaryTrie) {
            uint32_t num_slots_filled = 0;
            for (auto& infix : infix_vec)
                num_slots_filled += infix.GetNumSlots(infix_size_);
            allocation_size_grade = std::upper_bound(scaled_sizes_, scaled_sizes_ + size_scalar_count, num_slots_filled) - scaled_sizes_;
        }
        InfixStore store(scaled_sizes_[allocation_size_grade], infix_size_, allocation_size_grade, payload_size_);
        if (has_duplicates && diva_type == DivaType::BinaryTrie)
            LoadVectorToInfixStore(store, infix_vec, total_implicit, true, payload_list);
        else 
            LoadListToInfixStore(store, infix_list, i, total_implicit, true, payload_list);
        if constexpr (payload_type == PayloadType::FixedLength) {
            uint64_t *sample_payloads = reinterpret_cast<uint64_t *>(malloc(((payload_size_ + 63) / 64) * sizeof(uint64_t)));
            store.ptr[1] = reinterpret_cast<uint64_t>(sample_payloads);
            AddSamplePayload(store, left_payload);
        }
        if constexpr (diva_type == DivaType::Int)
            wh_int_put(better_tree_int_, left_key.str, left_key.length, &store, sizeof(store), dummy_locked_leaf_addrs);
        else
            wh_put(better_tree_, left_key.str, left_key.length, &store, sizeof(store), dummy_locked_leaf_addrs);

        if constexpr (payload_type == PayloadType::FixedLength)
            AddTreeKey(right_key.str, right_key.length, right_payload);
        else 
            AddTreeKey(right_key.str, right_key.length);
    }

    uint8_t max_str[max_len];
    memset(max_str, 0xFF, max_len);
    AddTreeKey(max_str, max_len);
}


template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::BulkLoadStreaming(uint64_t key, const uint64_t *payload) {
    key = __builtin_bswap64(key);
    BulkLoadStreaming(reinterpret_cast<const uint8_t *>(&key), sizeof(key), payload);
}


template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::BulkLoadStreaming(std::string_view key, const uint64_t *payload) {
    BulkLoadStreaming(reinterpret_cast<const uint8_t *>(key.data()), key.size(), payload);
}


// TODO: Add the trie tof here too
template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::BulkLoadStreaming(const uint8_t *key, const uint32_t key_len,
                                                             const uint64_t *payload) {
    uint8_t *key_copy = new uint8_t[key_len];
    memcpy(key_copy, key, key_len);

    if (bulk_load_left_key_.str == nullptr) {
        bulk_load_left_key_ = {key_copy, key_len};
        if constexpr (payload_type == PayloadType::FixedLength)
            copy_bitmap_to_bitmap(payload, 0, bulk_load_left_payload_, 0, payload_size_);
        bulk_load_streaming_max_len_ = key_len;
        return;
    }
    bulk_load_streaming_max_len_ = std::max(bulk_load_streaming_max_len_, key_len);
    if (bulk_load_streaming_ind_ < infix_store_target_size - 1) {
        delete[] bulk_load_key_list_[bulk_load_streaming_ind_].str;
        bulk_load_key_list_[bulk_load_streaming_ind_] = {key_copy, key_len};
        if constexpr (payload_type == PayloadType::FixedLength)
            copy_bitmap_to_bitmap(payload, 0, bulk_load_payload_list_, bulk_load_streaming_ind_ * payload_size_, payload_size_);
        bulk_load_streaming_ind_++;
        return;
    }

    InfiniteByteString bulk_load_right_key {key_copy, key_len};

    uint64_t infix_list[infix_store_target_size];
    std::vector<Infix> infix_vec;
    uint32_t last_infix_pos = 0;
    const auto [shared, ignore, implicit_size] = GetSharedIgnoreImplicitLengths(bulk_load_left_key_, bulk_load_right_key);
    const uint32_t key_start_bit = shared + ignore + implicit_size + infix_size_ - 1;
    const uint64_t prev_implicit = ExtractPartialKey(bulk_load_left_key_, shared, ignore, implicit_size, 0) >> infix_size_;
    const uint64_t next_implicit = ExtractPartialKey(bulk_load_right_key, shared, ignore, implicit_size, 1) >> infix_size_;
    const uint32_t total_implicit = next_implicit - prev_implicit + 1;
    for (int32_t i = 0; i < bulk_load_streaming_ind_; i++) {
        const uint64_t extraction = ExtractPartialKey(bulk_load_key_list_[i], shared, ignore, implicit_size, bulk_load_key_list_[i].GetBit(shared));
        infix_list[i] = ((extraction | 1ULL) - (prev_implicit << infix_size_));
        if constexpr (diva_type == DivaType::BinaryTrie) {
            bulk_load_key_list_[i].length *= 8;
            if (infix_list[last_infix_pos] != infix_list[i]) {
                infix_vec.emplace_back(infix_list[last_infix_pos]);
                if (i - last_infix_pos > 1) {
                    infix_vec.back().BuildTrieAndSuffixes(bulk_load_key_list_ + last_infix_pos,
                            i - last_infix_pos, key_start_bit, infix_size_,
                            false, false, true);
                }
                last_infix_pos = i;
            }
        }
    }
    if constexpr (diva_type == DivaType::BinaryTrie) {
        infix_vec.emplace_back(infix_list[last_infix_pos]);
        if (bulk_load_streaming_ind_ - last_infix_pos > 1) {
            infix_vec.back().BuildTrieAndSuffixes(bulk_load_key_list_ + last_infix_pos,
                    bulk_load_streaming_ind_ - last_infix_pos, key_start_bit, infix_size_,
                    false, false, true);
        }
        last_infix_pos = bulk_load_streaming_ind_;
    }
    void *dummy_locked_leaf_addrs[3] = {nullptr, nullptr, nullptr};
    uint32_t allocation_size_grade = size_scalar_shrink_grow_sep;
    if constexpr (diva_type == DivaType::BinaryTrie) {
        uint32_t num_slots_filled = 0;
        for (auto& infix : infix_vec)
            num_slots_filled += infix.GetNumSlots(infix_size_);
        allocation_size_grade = std::upper_bound(scaled_sizes_, scaled_sizes_ + size_scalar_count, num_slots_filled) - scaled_sizes_;
    }
    InfixStore store(scaled_sizes_[allocation_size_grade], infix_size_,
                     allocation_size_grade, payload_size_);
    if constexpr (diva_type == DivaType::BinaryTrie)
        LoadVectorToInfixStore(store, infix_vec, total_implicit, true, bulk_load_payload_list_);
    else 
        LoadListToInfixStore(store, infix_list, bulk_load_streaming_ind_, total_implicit, true, bulk_load_payload_list_);
    if constexpr (payload_type == PayloadType::FixedLength) {
        uint64_t *sample_payloads = reinterpret_cast<uint64_t *>(malloc(((payload_size_ + 63) / 64) * sizeof(uint64_t)));
        store.ptr[1] = reinterpret_cast<uint64_t>(sample_payloads);
        AddSamplePayload(store, bulk_load_left_payload_);
    }
    if constexpr (diva_type == DivaType::Int) {
        wh_int_put(better_tree_int_, bulk_load_left_key_.str,
                bulk_load_left_key_.length, &store,
                sizeof(store),
                dummy_locked_leaf_addrs);
    }
    else {
        wh_put(better_tree_, bulk_load_left_key_.str,
                bulk_load_left_key_.length, &store,
                sizeof(store),
                dummy_locked_leaf_addrs);
    }

    delete[] bulk_load_left_key_.str;
    bulk_load_left_key_ = bulk_load_right_key;
    if constexpr (payload_type == PayloadType::FixedLength)
        copy_bitmap_to_bitmap(payload, 0, bulk_load_left_payload_, 0, payload_size_);
    bulk_load_streaming_ind_ = 0;

    n_keys_.fetch_add(infix_store_target_size, std::memory_order_release);
}


template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::BulkLoadStreamingFinish() {
    uint8_t *key_copy = new uint8_t[bulk_load_streaming_max_len_];
    memset(key_copy, 0x00, bulk_load_streaming_max_len_);
    AddTreeKey(key_copy, bulk_load_streaming_max_len_);
    memset(key_copy, 0xFF, bulk_load_streaming_max_len_);
    AddTreeKey(key_copy, bulk_load_streaming_max_len_);

    if (bulk_load_streaming_ind_ > 0) {
        const InfiniteByteString bulk_load_right_key = bulk_load_key_list_[bulk_load_streaming_ind_ - 1];
        bulk_load_key_list_[bulk_load_streaming_ind_ - 1] = {};
        bulk_load_streaming_ind_--;

        uint64_t infix_list[infix_store_target_size];
        std::vector<Infix> infix_vec;
        uint32_t last_infix_pos = 0;
        const auto [shared, ignore, implicit_size] = 
            GetSharedIgnoreImplicitLengths(bulk_load_left_key_, bulk_load_right_key);
        const uint32_t key_start_bit = shared + ignore + implicit_size + infix_size_ - 1;
        const uint64_t prev_implicit = ExtractPartialKey(bulk_load_left_key_, shared, ignore, implicit_size, 0) >> infix_size_;
        const uint64_t next_implicit = ExtractPartialKey(bulk_load_right_key, shared, ignore, implicit_size, 1) >> infix_size_;
        const uint32_t total_implicit = next_implicit - prev_implicit + 1;
        for (int32_t i = 0; i < bulk_load_streaming_ind_; i++) {
            const uint64_t extraction = ExtractPartialKey(bulk_load_key_list_[i], shared, ignore, implicit_size, bulk_load_key_list_[i].GetBit(shared));
            infix_list[i] = ((extraction | 1ULL) - (prev_implicit << infix_size_));
            if constexpr (diva_type == DivaType::BinaryTrie) {
                bulk_load_key_list_[i].length *= 8;
                if (infix_list[last_infix_pos] != infix_list[i]) {
                    infix_vec.emplace_back(infix_list[last_infix_pos]);
                    if (i - last_infix_pos > 1) {
                        infix_vec.back().BuildTrieAndSuffixes(bulk_load_key_list_ + last_infix_pos,
                                i - last_infix_pos, key_start_bit, infix_size_,
                                false, false, true);
                    }
                    last_infix_pos = i;
                }
            }
        }
        if constexpr (diva_type == DivaType::BinaryTrie) {
            infix_vec.emplace_back(infix_list[last_infix_pos]);
            if (bulk_load_streaming_ind_ - last_infix_pos > 1) {
                infix_vec.back().BuildTrieAndSuffixes(bulk_load_key_list_ + last_infix_pos,
                        bulk_load_streaming_ind_ - last_infix_pos, key_start_bit, infix_size_,
                        false, false, true);
            }
            last_infix_pos = bulk_load_streaming_ind_;
        }
        void *dummy_locked_leaf_addrs[3] = {nullptr, nullptr, nullptr};
        uint32_t allocation_size_grade = std::upper_bound(scaled_sizes_, scaled_sizes_ + size_scalar_count, bulk_load_streaming_ind_) - scaled_sizes_;
        if constexpr (diva_type == DivaType::BinaryTrie) {
            uint32_t num_slots_filled = 0;
            for (auto& infix : infix_vec)
                num_slots_filled += infix.GetNumSlots(infix_size_);
            allocation_size_grade = std::upper_bound(scaled_sizes_, scaled_sizes_ + size_scalar_count, num_slots_filled) - scaled_sizes_;
        }
        InfixStore store(scaled_sizes_[allocation_size_grade], infix_size_, allocation_size_grade, payload_size_);
        if constexpr (diva_type == DivaType::BinaryTrie)
            LoadVectorToInfixStore(store, infix_vec, total_implicit, true, bulk_load_payload_list_);
        else 
            LoadListToInfixStore(store, infix_list, bulk_load_streaming_ind_, total_implicit, true, bulk_load_payload_list_);
        if constexpr (payload_type == PayloadType::FixedLength) {
            uint64_t *sample_payloads = reinterpret_cast<uint64_t *>(malloc(((payload_size_ + 63) / 64) * sizeof(uint64_t)));
            store.ptr[1] = reinterpret_cast<uint64_t>(sample_payloads);
            AddSamplePayload(store, bulk_load_left_payload_);
        }
        if constexpr (diva_type == DivaType::Int) {
            wh_int_put(better_tree_int_, bulk_load_left_key_.str,
                    bulk_load_left_key_.length, &store,
                    sizeof(store),
                    dummy_locked_leaf_addrs);
        }
        else {
            wh_put(better_tree_, bulk_load_left_key_.str,
                    bulk_load_left_key_.length, &store,
                    sizeof(store),
                    dummy_locked_leaf_addrs);
        }
        uint64_t bulk_load_right_payload_[(payload_size_ + 63) / 64 + 1];
        copy_bitmap_to_bitmap(bulk_load_payload_list_, bulk_load_streaming_ind_ * payload_size_,
                              bulk_load_right_payload_, 0, payload_size_);
        AddTreeKey(bulk_load_right_key.str, bulk_load_right_key.length, bulk_load_right_payload_);
        delete[] bulk_load_right_key.str;

        n_keys_.fetch_add(bulk_load_streaming_ind_ + 2, std::memory_order_release);
    }

    delete[] bulk_load_left_key_.str;
    bulk_load_streaming_ind_ = 0;
    bulk_load_left_key_ = {};
    for (int32_t i = 0; i < infix_store_target_size; i++) {
        delete[] bulk_load_key_list_[i].str;
        bulk_load_key_list_[i] = {};
    }
    delete[] bulk_load_left_payload_;
    delete[] bulk_load_payload_list_;
}


template <DivaType diva_type, PayloadType payload_type>
inline uint64_t Diva<diva_type, payload_type>::GetNumKeys() const {
    return n_keys_.load(std::memory_order_acquire);
}


template <DivaType diva_type, PayloadType payload_type>
//__attribute__((always_inline))
inline uint32_t Diva<diva_type, payload_type>::RankOccupieds(const InfixStore &store, const uint32_t pos) const {
    const uint32_t *popcnts = reinterpret_cast<const uint32_t *>(store.ptr);
    const uint64_t *occupieds = store.ptr + num_metadata_offset_words;

    const bool cond = infix_store_target_size / 2 <= pos;
    uint32_t res = cond ? popcnts[0] : 0;
    for (int32_t i = cond ? infix_store_target_size / 128 : 0; i < pos / 64; i++)
        res += __builtin_popcountll(occupieds[i]);
    return res + bit_rank(occupieds[pos / 64], pos % 64);
}


template <DivaType diva_type, PayloadType payload_type>
//__attribute__((always_inline))
inline uint32_t Diva<diva_type, payload_type>::SelectRunends(const InfixStore &store, const uint32_t rank) const {
    const uint32_t *popcnts = reinterpret_cast<const uint32_t *>(store.ptr);
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t total_words = (scaled_sizes_[size_grade] + 63) / 64;
    const uint64_t *runends = store.ptr + num_metadata_offset_words + infix_store_target_size / 64;

    const bool cond = popcnts[1] <= rank;
    uint32_t old_total_set_bits = cond ? popcnts[1] - __builtin_popcountll(runends[infix_store_target_size / 128 - 1]) : 0;
    uint32_t total_set_bits = cond ? popcnts[1] : 0;
    uint32_t i;
    for (i = cond ? infix_store_target_size / 128 : 0; total_set_bits <= rank && i < total_words; i++) {
        old_total_set_bits = total_set_bits;
        total_set_bits += __builtin_popcountll(runends[i]);
    }
    i--;
    return i * 64 + bit_select(runends[i], rank - old_total_set_bits);
}


template <DivaType diva_type, PayloadType payload_type>
inline int32_t Diva<diva_type, payload_type>::NextOccupied(const InfixStore &store, const uint32_t pos) const {
    const uint64_t *occupieds = store.ptr + num_metadata_offset_words;
    int32_t res = pos + 1, lb_pos;
    do {
        lb_pos = lowbit_pos(occupieds[res / 64] & (~BITMASK(res % 64)));
        res += lb_pos - res % 64;
    } while (lb_pos == 64 && res < infix_store_target_size);
    return res;
}


template <DivaType diva_type, PayloadType payload_type>
//__attribute__((always_inline))
inline int32_t Diva<diva_type, payload_type>::PreviousOccupied(const InfixStore &store, const uint32_t pos) const {
    const uint64_t *occupieds = store.ptr + num_metadata_offset_words;
    int32_t res = pos - 1, hb_pos;
    do {
        const int32_t offset = res % 64;
        hb_pos = highbit_pos(occupieds[res / 64] & BITMASK(offset + 1));
        res += (hb_pos == -1 ? -offset - 1 : hb_pos - offset);
    } while (hb_pos == -1 && res >= 0);
#ifdef DEBUG
    assert(res >= -1);
#endif // DEBUG
    return res;
}


template <DivaType diva_type, PayloadType payload_type>
//__attribute__((always_inline))
inline int32_t Diva<diva_type, payload_type>::NextRunend(const InfixStore &store, const uint32_t pos) const {
    const uint64_t *runends = store.ptr + num_metadata_offset_words + infix_store_target_size / 64;
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t runends_size = scaled_sizes_[size_grade];
    int32_t res = pos + 1, lb_pos;
    do {
        lb_pos = lowbit_pos(runends[res / 64] & (~BITMASK(res % 64)));
        res += lb_pos - res % 64;
    } while (lb_pos == 64 && res < runends_size);
    return res;
}


template <DivaType diva_type, PayloadType payload_type>
//__attribute__((always_inline))
inline int32_t Diva<diva_type, payload_type>::PreviousRunend(const InfixStore &store, const uint32_t pos) const {
    const uint64_t *runends = store.ptr + num_metadata_offset_words + infix_store_target_size / 64;
    int32_t res = pos - 1, hb_pos;
    do {
        const int32_t offset = res % 64;
        hb_pos = highbit_pos(runends[res / 64] & BITMASK(offset + 1));
        res += (hb_pos == -1 ? -offset - 1 : hb_pos - offset);
    } while (hb_pos == -1 && res >= 0);
#ifdef DEBUG
    assert(res >= -1);
#endif // DEBUG
    return res;
}


template <DivaType diva_type, PayloadType payload_type>
//__attribute__((always_inline))
inline int32_t Diva<diva_type, payload_type>::GetMappedPos(const uint32_t implicit_part, const uint32_t size_grade,
                                                           const uint64_t implicit_scalar) const {
    uint32_t res = (implicit_part * size_scalars_[size_grade] * implicit_scalar)
                        >> (scale_shift + scale_implicit_shift);
    return std::min<uint32_t>(scaled_sizes_[size_grade] - 1, res);
}


template <DivaType diva_type, PayloadType payload_type>
//__attribute__((always_inline))
inline uint64_t Diva<diva_type, payload_type>::GetSlot(const InfixStore &store, const uint32_t pos) const {
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t bit_pos = 64 * num_metadata_offset_words + infix_store_target_size 
                           + scaled_sizes_[size_grade] + pos * infix_size_;
    const uint8_t *ptr = ((uint8_t *) store.ptr) + bit_pos / 8;
    uint64_t value;
    memcpy(&value, ptr, sizeof(value));
    return (value >> bit_pos % 8) & BITMASK(infix_size_);
}


template <DivaType diva_type, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<diva_type, payload_type>::SetSlot(InfixStore &store, const uint32_t pos, const uint64_t value) {
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t bit_pos = 64 * num_metadata_offset_words + infix_store_target_size 
                           + scaled_sizes_[size_grade] + pos * infix_size_;
    uint8_t *ptr = ((uint8_t *) store.ptr) + bit_pos / 8;
    uint64_t stamp;
    memcpy(&stamp, ptr, sizeof(stamp));
    stamp &= ~(BITMASK(infix_size_) << (bit_pos % 8));
    stamp |= value << (bit_pos % 8);
    memcpy(ptr, &stamp, sizeof(stamp));
}


template <DivaType diva_type, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<diva_type, payload_type>::SetSlot(InfixStore &store, const uint32_t pos,
                                                   const uint64_t value, const uint32_t width) {
#ifdef DEBUG
    assert(value > 0);
#endif // DEBUG
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t bit_pos = 64 * num_metadata_offset_words + infix_store_target_size 
                           + scaled_sizes_[size_grade] + pos * width;
    uint8_t *ptr = ((uint8_t *) store.ptr) + bit_pos / 8;
    uint64_t stamp;
    memcpy(&stamp, ptr, sizeof(stamp));
    stamp &= ~(BITMASK(width) << (bit_pos % 8));
    stamp |= value << (bit_pos % 8);
    memcpy(ptr, &stamp, sizeof(stamp));
}


template <DivaType diva_type, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<diva_type, payload_type>::GetPayload(const InfixStore &store, const uint32_t pos,
                                                      uint64_t *payload, const uint32_t payload_offset) const {
#ifdef DEBUG
    static_assert(payload_type == PayloadType::FixedLength);
    assert(payload != nullptr);
#endif // DEBUG
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t bit_pos = 64 * num_metadata_offset_words + infix_store_target_size 
                           + scaled_sizes_[size_grade] * (infix_size_ + 1) + pos * payload_size_;
    copy_bitmap_to_bitmap(store.ptr, bit_pos, payload, payload_offset, payload_size_);
}


template <DivaType diva_type, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<diva_type, payload_type>::GetSamplePayload(const InfixStore &store, const uint32_t pos,
                                                            uint64_t *payload, const uint32_t payload_offset) const {
    const uint64_t *payload_list = reinterpret_cast<const uint64_t *>(store.ptr[1]);
    const uint32_t bit_pos = pos * payload_size_;
    copy_bitmap_to_bitmap(payload_list, bit_pos, payload, payload_offset, payload_size_);
}


template <DivaType diva_type, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<diva_type, payload_type>::SetPayload(InfixStore &store, const uint32_t pos,
                                                      const uint64_t *payload, const uint32_t payload_offset) {
#ifdef DEBUG
    static_assert(payload_type == PayloadType::FixedLength);
    assert(payload != nullptr);
#endif // DEBUG
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t bit_pos = 64 * num_metadata_offset_words + infix_store_target_size 
                           + scaled_sizes_[size_grade] * (infix_size_ + 1) + pos * payload_size_;
    copy_bitmap_to_bitmap(payload, payload_offset, store.ptr, bit_pos, payload_size_);
}


template <DivaType diva_type, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<diva_type, payload_type>::AddSamplePayload(InfixStore &store,
                                                            const void *payload,
                                                            const uint32_t payload_offset) {
    uint64_t *payload_list = reinterpret_cast<uint64_t *>(store.ptr[1]);
    if (store.num_sample_payloads == 0) {
        const uint32_t malloc_size = (payload_size_ / 64 + 1) * 8;
        payload_list = reinterpret_cast<uint64_t *>(malloc(malloc_size));
        memset(payload_list, 0, malloc_size);
    }
    else {
        payload_list = reinterpret_cast<uint64_t *>(realloc(payload_list,
                                                            ((store.num_sample_payloads + 1) * payload_size_ / 64 + 1) * 8));
    }
    const uint32_t bit_pos = store.num_sample_payloads * payload_size_;
    copy_bitmap_to_bitmap(reinterpret_cast<const uint64_t *>(payload), payload_offset,
                          payload_list, bit_pos, payload_size_);
    store.num_sample_payloads++;
    store.ptr[1] = reinterpret_cast<uint64_t>(payload_list);
}


template <DivaType diva_type, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<diva_type, payload_type>::RemoveSamplePayload(InfixStore &store, const uint32_t pos) {
    uint64_t *payload_list = reinterpret_cast<uint64_t *>(store.ptr[1]);
    if (store.num_sample_payloads == 1) {
        free(payload_list);
        payload_list = nullptr;
    }
    else {
        const uint32_t l = (pos + 1) * payload_size_;
        const uint32_t r = store.num_sample_payloads * payload_size_ - 1;
        shift_bitmap_left_unaligned(payload_list, l, r, payload_size_);
        payload_list = reinterpret_cast<uint64_t *>(realloc(payload_list,
                                                            ((store.num_sample_payloads - 1) * payload_size_ + 7) / 8));
    }
    store.num_sample_payloads--;
    store.ptr[1] = reinterpret_cast<uint64_t>(payload_list);
}


template <DivaType diva_type, PayloadType payload_type>
//__attribute__((always_inline))
inline bool Diva<diva_type, payload_type>::GetOccupiedBit(const InfixStore &store, const uint32_t pos) const {
    const uint64_t *occupieds = store.ptr + num_metadata_offset_words;
    return get_bitmap_bit(occupieds, pos);
}

template <DivaType diva_type, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<diva_type, payload_type>::SetOccupiedBit(InfixStore &store, const uint32_t pos) {
    uint32_t *popcnts = reinterpret_cast<uint32_t *>(store.ptr);
    uint64_t *occupieds = store.ptr + num_metadata_offset_words;
    popcnts[0] += pos < infix_store_target_size / 2 ? 1 - get_bitmap_bit(occupieds, pos) : 0;
    set_bitmap_bit(occupieds, pos);
}

template <DivaType diva_type, PayloadType payload_type>
__attribute__((always_inline))
inline void Diva<diva_type, payload_type>::ResetOccupiedBit(InfixStore &store, const uint32_t pos) {
    uint32_t *popcnts = reinterpret_cast<uint32_t *>(store.ptr);
    uint64_t *occupieds = store.ptr + num_metadata_offset_words;
    popcnts[0] -= pos < infix_store_target_size / 2 ? get_bitmap_bit(occupieds, pos) : 0;
    reset_bitmap_bit(occupieds, pos);
}

template <DivaType diva_type, PayloadType payload_type>
//__attribute__((always_inline))
inline bool Diva<diva_type, payload_type>::GetRunendBit(const InfixStore &store, const uint32_t pos) const {
    const uint64_t *runends = store.ptr + num_metadata_offset_words + infix_store_target_size / 64;
    return get_bitmap_bit(runends, pos);
}

template <DivaType diva_type, PayloadType payload_type>
__attribute__((always_inline))
inline void Diva<diva_type, payload_type>::SetRunendBit(InfixStore &store, const uint32_t pos) {
    uint32_t *popcnts = reinterpret_cast<uint32_t *>(store.ptr);
    uint64_t *runends = store.ptr + num_metadata_offset_words + infix_store_target_size / 64;
    popcnts[1] += pos < infix_store_target_size / 2 ? 1 - get_bitmap_bit(runends, pos) : 0;
    set_bitmap_bit(runends, pos);
}

template <DivaType diva_type, PayloadType payload_type>
__attribute__((always_inline))
inline void Diva<diva_type, payload_type>::ResetRunendBit(InfixStore &store, const uint32_t pos) {
    uint32_t *popcnts = reinterpret_cast<uint32_t *>(store.ptr);
    uint64_t *runends = store.ptr + num_metadata_offset_words + infix_store_target_size / 64;
    popcnts[1] -= pos < infix_store_target_size / 2 ? get_bitmap_bit(runends, pos) : 0;
    reset_bitmap_bit(runends, pos);
}

template <DivaType diva_type, PayloadType payload_type>
__attribute__((always_inline))
inline void Diva<diva_type, payload_type>::UpdatePopcnts(InfixStore& store) {
    uint32_t *popcnts = reinterpret_cast<uint32_t *>(store.ptr);
    popcnts[0] = popcnts[1] = 0;
    const uint64_t *occupieds = store.ptr + num_metadata_offset_words;
    const uint64_t *runends = store.ptr + num_metadata_offset_words + infix_store_target_size / 64;
    const uint32_t size_grade = store.GetSizeGrade();
    for (int32_t i = 0; i < infix_store_target_size / 128; i++) {
        popcnts[0] += __builtin_popcountll(occupieds[i]);
        if (static_cast<int32_t>(scaled_sizes_[size_grade]) - i * 64 > 0) {
            const uint64_t mask = BITMASK(std::min(64UL, scaled_sizes_[size_grade] - i * 64));
            popcnts[1] += __builtin_popcountll(runends[i] & mask);
        }
    }
}

template <DivaType diva_type, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<diva_type, payload_type>::ShiftSlotsRight(InfixStore &store, const uint32_t l, const uint32_t r,
                                                           const uint32_t shamt) {
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t l_bit_pos = 64 * num_metadata_offset_words + infix_store_target_size 
                             + scaled_sizes_[size_grade] + l * infix_size_;
    const uint32_t r_bit_pos = 64 * num_metadata_offset_words + infix_store_target_size 
                             + scaled_sizes_[size_grade] + r * infix_size_ - 1;
    shift_bitmap_right(store.ptr, l_bit_pos, r_bit_pos, shamt * infix_size_);
}


template <DivaType diva_type, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<diva_type, payload_type>::ShiftSlotsLeft(InfixStore &store, const uint32_t l, const uint32_t r,
                                                          const uint32_t shamt) {
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t l_bit_pos = 64 * num_metadata_offset_words + infix_store_target_size 
                             + scaled_sizes_[size_grade] + l * infix_size_;
    const uint32_t r_bit_pos = 64 * num_metadata_offset_words + infix_store_target_size 
                             + scaled_sizes_[size_grade] + r * infix_size_ - 1;
    shift_bitmap_left(store.ptr, l_bit_pos, r_bit_pos, shamt * infix_size_);
}


template <DivaType diva_type, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<diva_type, payload_type>::ShiftPayloadsRight(InfixStore &store, const uint32_t l, const uint32_t r,
                                                              const uint32_t shamt) {
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t l_bit_pos = 64 * num_metadata_offset_words + infix_store_target_size 
                             + scaled_sizes_[size_grade] * (infix_size_ + 1) + payload_size_ * l;
    const uint32_t r_bit_pos = 64 * num_metadata_offset_words + infix_store_target_size 
                             + scaled_sizes_[size_grade] * (infix_size_ + 1) + payload_size_ * r - 1;
    shift_bitmap_right(store.ptr, l_bit_pos, r_bit_pos, shamt * payload_size_);
}


template <DivaType diva_type, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<diva_type, payload_type>::ShiftPayloadsLeft(InfixStore &store, const uint32_t l, const uint32_t r,
                                                             const uint32_t shamt) {
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t l_bit_pos = 64 * num_metadata_offset_words + infix_store_target_size 
                             + scaled_sizes_[size_grade] * (infix_size_ + 1) + payload_size_ * l;
    const uint32_t r_bit_pos = 64 * num_metadata_offset_words + infix_store_target_size 
                             + scaled_sizes_[size_grade] * (infix_size_ + 1) + payload_size_ * r - 1;
    shift_bitmap_left(store.ptr, l_bit_pos, r_bit_pos, shamt * payload_size_);
}


template <DivaType diva_type, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<diva_type, payload_type>::MoveSlotsRight(InfixStore &store, const uint32_t l, const uint32_t r,
                                                          const uint32_t shamt) {
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t l_bit_pos = 64 * num_metadata_offset_words + infix_store_target_size 
                             + scaled_sizes_[size_grade] + l * infix_size_;
    const uint32_t r_bit_pos = 64 * num_metadata_offset_words + infix_store_target_size 
                             + scaled_sizes_[size_grade] + r * infix_size_ - 1;
    move_bitmap_right(store.ptr, l_bit_pos, r_bit_pos, shamt * infix_size_);
    zero_out_bitmap(store.ptr, l_bit_pos, std::min(r_bit_pos, l_bit_pos + shamt * infix_size_ - 1));
}


template <DivaType diva_type, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<diva_type, payload_type>::MoveSlotsLeft(InfixStore &store, const uint32_t l, const uint32_t r,
                                                         const uint32_t shamt) {
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t l_bit_pos = 64 * num_metadata_offset_words + infix_store_target_size 
                             + scaled_sizes_[size_grade] + l * infix_size_;
    const uint32_t r_bit_pos = 64 * num_metadata_offset_words + infix_store_target_size 
                             + scaled_sizes_[size_grade] + r * infix_size_ - 1;
    move_bitmap_left(store.ptr, l_bit_pos, r_bit_pos, shamt * infix_size_);
    zero_out_bitmap(store.ptr, std::max(l_bit_pos, r_bit_pos - shamt * infix_size_ + 1), r_bit_pos);
}


template <DivaType diva_type, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<diva_type, payload_type>::MovePayloadsRight(InfixStore &store, const uint32_t l, const uint32_t r,
                                                             const uint32_t shamt) {
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t l_bit_pos = 64 * num_metadata_offset_words + infix_store_target_size 
                             + scaled_sizes_[size_grade] * (infix_size_ + 1) + payload_size_ * l;
    const uint32_t r_bit_pos = 64 * num_metadata_offset_words + infix_store_target_size 
                             + scaled_sizes_[size_grade] * (infix_size_ + 1) + payload_size_ * r - 1;
    move_bitmap_right(store.ptr, l_bit_pos, r_bit_pos, shamt * payload_size_);
    zero_out_bitmap(store.ptr, l_bit_pos, std::min(r_bit_pos, l_bit_pos + shamt * payload_size_ - 1));
}


template <DivaType diva_type, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<diva_type, payload_type>::MovePayloadsLeft(InfixStore &store, const uint32_t l, const uint32_t r,
                                                            const uint32_t shamt) {
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t l_bit_pos = 64 * num_metadata_offset_words + infix_store_target_size 
                             + scaled_sizes_[size_grade] * (infix_size_ + 1) + payload_size_ * l;
    const uint32_t r_bit_pos = 64 * num_metadata_offset_words + infix_store_target_size 
                             + scaled_sizes_[size_grade] * (infix_size_ + 1) + payload_size_ * r - 1;
    move_bitmap_left(store.ptr, l_bit_pos, r_bit_pos, shamt * payload_size_);
    zero_out_bitmap(store.ptr, std::max(l_bit_pos, r_bit_pos - shamt * payload_size_ + 1), r_bit_pos);
}


template <DivaType diva_type, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<diva_type, payload_type>::ZeroOutSlots(InfixStore &store, const uint32_t l, const uint32_t r) {
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t l_bit_pos = 64 * num_metadata_offset_words + infix_store_target_size 
                             + scaled_sizes_[size_grade] + l * infix_size_;
    const uint32_t r_bit_pos = 64 * num_metadata_offset_words + infix_store_target_size 
                             + scaled_sizes_[size_grade] + r * infix_size_ - 1;
    zero_out_bitmap(store.ptr, l_bit_pos, r_bit_pos);
}


template <DivaType diva_type, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<diva_type, payload_type>::ZeroOutPayloads(InfixStore &store, const uint32_t l, const uint32_t r) {
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t l_bit_pos = 64 * num_metadata_offset_words + infix_store_target_size 
                             + scaled_sizes_[size_grade] * (infix_size_ + 1) + payload_size_ * l;
    const uint32_t r_bit_pos = 64 * num_metadata_offset_words + infix_store_target_size 
                             + scaled_sizes_[size_grade] * (infix_size_ + 1) + payload_size_ * r - 1;
    zero_out_bitmap(store.ptr, l_bit_pos, r_bit_pos);
}


template <DivaType diva_type, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<diva_type, payload_type>::ShiftRunendsRight(InfixStore &store, const uint32_t l, const uint32_t r, 
                                                             const uint32_t shamt) {
    if (l >= r)
        return;
    uint32_t *popcnts = reinterpret_cast<uint32_t *>(store.ptr);
    uint64_t *runends = store.ptr + num_metadata_offset_words + infix_store_target_size / 64;
    // Update `popcnts`
    const uint32_t num_bits_to_read = std::max<int32_t>(r + shamt - std::max(infix_store_target_size / 2, l + shamt), 0)
                                    - std::max<int32_t>(r - std::max(infix_store_target_size / 2, l), 0);
    uint32_t read_bit_pos = std::min(infix_store_target_size / 2, r) - num_bits_to_read;
    uint64_t read_buf = 0;
    uint32_t read_buf_filled_len = 0, num_bits_read = 0;
    while (num_bits_read < num_bits_to_read) {
        const uint32_t amount_to_read = std::min(num_bits_to_read - num_bits_read, 64U);
        const uint64_t data = read_data_from_bitmap(runends,
                read_bit_pos, read_buf,
                read_buf_filled_len,
                amount_to_read);
        popcnts[1] -= __builtin_popcountll(data);
        num_bits_read += amount_to_read;
    }

    // Shift `runends`
    shift_bitmap_right(runends, l, r - 1, shamt);
}


template <DivaType diva_type, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<diva_type, payload_type>::ShiftRunendsLeft(InfixStore &store, const uint32_t l, const uint32_t r,
                                                            const uint32_t shamt) {
    if (l >= r)
        return;
    uint32_t *popcnts = reinterpret_cast<uint32_t *>(store.ptr);
    uint64_t *runends = store.ptr + num_metadata_offset_words + infix_store_target_size / 64;
    // Update `popcnts`
    const uint32_t num_bits_to_read = std::max<int32_t>(std::min(r - shamt, infix_store_target_size / 2) - (l - shamt), 0)
                                    - std::max<int32_t>(std::min(r, infix_store_target_size / 2) - l, 0);
    uint32_t read_bit_pos = std::max(infix_store_target_size / 2, l);
    uint64_t read_buf = 0;
    uint32_t read_buf_filled_len = 0, num_bits_read = 0;
    while (num_bits_read < num_bits_to_read) {
        const uint32_t amount_to_read = std::min(num_bits_to_read - num_bits_read, 64U);
        const uint64_t data = read_data_from_bitmap(runends, read_bit_pos, read_buf, read_buf_filled_len, amount_to_read);
        popcnts[1] += __builtin_popcountll(data);
        num_bits_read += amount_to_read;
    }

    // Shift `runends`
    shift_bitmap_left(runends, l, r - 1, shamt);
}


template <DivaType diva_type, PayloadType payload_type>
//__attribute__((always_inline))
inline int32_t Diva<diva_type, payload_type>::FindEmptySlotAfter(const InfixStore &store, const uint32_t runend_pos) const {
    const uint32_t size_grade = store.GetSizeGrade();
    int32_t current_pos = runend_pos;
    while (current_pos < scaled_sizes_[size_grade] && GetSlot(store, current_pos + 1)) {
        current_pos = NextRunend(store, current_pos);
    }
    return current_pos + 1;
}


template <DivaType diva_type, PayloadType payload_type>
//__attribute__((always_inline))
inline int32_t Diva<diva_type, payload_type>::FindEmptySlotBefore(const InfixStore &store, const uint32_t runend_pos) const {
    const uint32_t size_grade = store.GetSizeGrade();
    int32_t current_pos = runend_pos, previous_pos;
    do {
        previous_pos = current_pos;
        current_pos = PreviousRunend(store, current_pos);
    } while (current_pos >= 0 
            && (current_pos == scaled_sizes_[size_grade] - 1 || GetSlot(store, current_pos + 1)));

    do {
        current_pos++;
    } while (current_pos < previous_pos && GetSlot(store, current_pos) == 0);
    return current_pos - 1;
}


template <DivaType diva_type, PayloadType payload_type>
inline uint32_t Diva<diva_type, payload_type>::MakeRoomFromSlot(InfixStore &store, 
                                                                const uint32_t pos,
                                                                const uint32_t runend_pos,
                                                                const uint32_t num_slots,
                                                                bool in_run) {
    const uint32_t size_grade = store.GetSizeGrade();
    const int32_t store_size = static_cast<int32_t>(scaled_sizes_[size_grade]);
    uint32_t empty_slots_to_right[num_slots], num_empty_slots_to_right = 0;
    uint32_t empty_slots_to_left[num_slots], num_empty_slots_to_left = 0;
    const bool in_run_dec = in_run & (pos > runend_pos);

    // Find the empty slots
    int32_t current_pos = FindEmptySlotAfter(store, runend_pos);
    while (num_empty_slots_to_right < num_slots && current_pos < store_size) {
        if (GetSlot(store, current_pos) == 0) {
            #ifdef DEBUG
            assert(!GetRunendBit(store, current_pos));
            #endif // DEBUG
            empty_slots_to_right[num_empty_slots_to_right++] = current_pos;
            current_pos++;
        }
        else {
            current_pos = GetRunendBit(store, current_pos) ? current_pos : NextRunend(store, current_pos);
            current_pos = FindEmptySlotAfter(store, current_pos);
        }
    } 

    // Check if we need to look to the left for empty slots
    if (num_empty_slots_to_right < num_slots) {
        current_pos = FindEmptySlotBefore(store, runend_pos);
        while (num_empty_slots_to_right + num_empty_slots_to_left < num_slots && current_pos >= 0) {
            if (GetSlot(store, current_pos) == 0 && !GetRunendBit(store, current_pos)) {
                empty_slots_to_left[num_empty_slots_to_left++] = current_pos;
                current_pos--;
            }
            else 
                current_pos = FindEmptySlotBefore(store, current_pos);
        }
    }
#ifdef DEBUG
    assert(num_empty_slots_to_left + num_empty_slots_to_right == num_slots);
#endif // DEBUG

    // Do the shifting
    for (int32_t i = static_cast<int32_t>(num_empty_slots_to_right) - 1; i >= 0; i--) {
        const uint32_t l = i == 0 ? pos : empty_slots_to_right[i - 1];
        const uint32_t r = empty_slots_to_right[i];
        const uint32_t shamt = num_empty_slots_to_right - i;
        ShiftRunendsRight(store, l, r, shamt);
        ShiftSlotsRight(store, l, r, shamt);
        if constexpr (payload_type == PayloadType::FixedLength)
            ShiftPayloadsRight(store, l, r, shamt);
    }
    for (int32_t i = num_empty_slots_to_left - 1; i >= 0; i--) {
        const uint32_t l = empty_slots_to_left[i] + 1;
        const uint32_t r = i == 0 ? pos : empty_slots_to_left[i - 1];
        const uint32_t shamt = num_empty_slots_to_left - i;
        ShiftRunendsLeft(store, l, r - (i == 0 && in_run_dec), shamt);
        ShiftSlotsLeft(store, l, r, shamt);
        if constexpr (payload_type == PayloadType::FixedLength)
            ShiftPayloadsLeft(store, l, r, shamt);
    }

#ifdef DEBUG
    assert(num_empty_slots_to_left <= pos);
#endif // DEBUG
    return pos - num_empty_slots_to_left;
}


template <DivaType diva_type, PayloadType payload_type>
inline bool Diva<diva_type, payload_type>::SlotHasTrie(const InfixStore &store,
                                                       const uint32_t pos,
                                                       const uint32_t runend_pos) const {
    static_assert(diva_type == DivaType::BinaryTrie);
    const uint64_t current_slot = GetSlot(store, pos);
#ifdef DEBUG
    assert(current_slot);
#endif // DEBUG
    if (ACTUAL_SUFFIX_LEN_MODE == 0 && !(current_slot & 1))
        return false;
    if (pos < runend_pos) {
        const uint64_t lookahead = GetSlot(store, pos + 1);
        if (highbit_pos(lookahead) < highbit_pos(current_slot))
            return true;
    }
    return false;
}


template <DivaType diva_type, PayloadType payload_type>
inline std::pair<bool, uint32_t> Diva<diva_type, payload_type>::FindInfixInRun(const InfixStore &store, 
                                                                               const int32_t runstart_pos,
                                                                               const int32_t runend_pos,
                                                                               const uint64_t explicit_part) {
    const uint32_t size_grade = store.GetSizeGrade();
    for (int32_t i = runstart_pos; i <= runend_pos; i++) {
        const uint64_t current_slot = GetSlot(store, i);
        if (current_slot == explicit_part)
            return {true, i};
        else if ((current_slot & (current_slot - 1)) > explicit_part - 1)
            return {false, i};
        else if (i < runend_pos) {  // There might be a trie we should skip
            if constexpr (diva_type == DivaType::BinaryTrie) {
                if (SlotHasTrie(store, i, runend_pos)) {
                    const Infix infix_to_skip(store.ptr + num_metadata_offset_words,
                                              infix_store_target_size + scaled_sizes_[size_grade] + infix_size_ * i,
                                              infix_size_);
                    i += infix_to_skip.GetNumSlots(infix_size_) - 1;
                }
            }
        }
    }
    return {false, runend_pos + 1};
}


template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::InsertRawIntoInfixStore(InfixStore &store, const uint64_t key,
                                                                   const uint32_t total_implicit,
                                                                   const uint64_t *payload,
                                                                   const InfiniteByteString original_key,
                                                                   const uint32_t original_key_start_bit) {
    if constexpr (payload_type == PayloadType::FixedLength)
        assert(payload != nullptr);
    uint32_t size_grade = store.GetSizeGrade();

    const uint64_t full_slot_count = store.GetFullSlotCount();
    if (full_slot_count >= (size_grade ? scaled_sizes_[size_grade - 1] : exception_scaled_size_)) {
#ifdef DEBUG
        assert(size_grade < size_scalar_count);
#endif // DEBUG
        ResizeInfixStore(store, total_implicit);
    }
    size_grade = store.GetSizeGrade();

    const uint64_t implicit_part = key >> infix_size_;
    const uint64_t explicit_part = key & BITMASK(infix_size_);
    const uint64_t implicit_scalar = implicit_scalars_[total_implicit - infix_store_target_size / 2];
#ifdef DEBUG
    assert(implicit_part < total_implicit);
#endif // DEBUG
    const int32_t mapped_pos = GetMappedPos(implicit_part, size_grade, implicit_scalar);
    const uint32_t key_rank = RankOccupieds(store, implicit_part);
    const bool is_occupied = GetOccupiedBit(store, implicit_part);
    int32_t num_slots_filled = 1;
    if (!is_occupied) {
        const int32_t next_runend = std::min<int32_t>(SelectRunends(store, key_rank), scaled_sizes_[size_grade]);
        const int32_t previous_runend = key_rank > 0 ? static_cast<int32_t>(SelectRunends(store, key_rank - 1)) : -1;
#ifdef DEBUG
        assert(previous_runend < next_runend);
#endif // DEBUG
        const int32_t empty_before_next_runend = FindEmptySlotBefore(store, next_runend);
        int32_t next_empty, previous_empty;
        if (empty_before_next_runend > previous_runend) {
            next_empty = mapped_pos <= previous_runend ? previous_runend + 1 
                : (empty_before_next_runend < mapped_pos ? FindEmptySlotAfter(store, next_runend) : mapped_pos);
            previous_empty = mapped_pos <= previous_runend ? FindEmptySlotBefore(store, previous_runend) 
                : std::min(empty_before_next_runend, mapped_pos);
        }
        else {
            next_empty = FindEmptySlotAfter(store, next_runend < scaled_sizes_[size_grade] ? next_runend : previous_runend);
            previous_empty = empty_before_next_runend;
        }
#ifdef DEBUG
        assert(next_empty >= scaled_sizes_[size_grade] || mapped_pos <= next_runend);
#endif // DEBUG
        int32_t insert_pos;
        bool should_make_room = false;
        if (next_empty < scaled_sizes_[size_grade]) {
            insert_pos = std::max(previous_runend + 1, mapped_pos);
            should_make_room = previous_empty < insert_pos && insert_pos < next_empty;
        }
        else {
            insert_pos = previous_empty < previous_runend ? previous_runend + 1 : previous_empty;
            should_make_room = previous_empty < previous_runend;
        }
        if (should_make_room)
            insert_pos = MakeRoomFromSlot(store, insert_pos, next_empty - 1, 1, false);
#ifdef DEBUG
        assert(insert_pos < scaled_sizes_[size_grade]);
#endif // DEBUG
        SetSlot(store, insert_pos, explicit_part);
        if constexpr (payload_type == PayloadType::FixedLength)
            SetPayload(store, insert_pos, payload, 0);
        SetRunendBit(store, insert_pos);
    }
    else {
        const int32_t runend_pos = SelectRunends(store, key_rank);
#ifdef DEBUG
        assert(runend_pos < static_cast<int32_t>(scaled_sizes_[size_grade]));
#endif // DEBUG
        const int32_t runstart_pos = std::max(key_rank ? static_cast<int32_t>(SelectRunends(store, key_rank - 1)) : -1,
                                              FindEmptySlotBefore(store, runend_pos)) + 1;
        auto [found, insert_pos] = FindInfixInRun(store, runstart_pos, runend_pos, explicit_part);
        if constexpr (diva_type == DivaType::BinaryTrie) {
            if (found) {
                Infix infix_to_update;
                uint32_t original_num_slots = 1;
                if (SlotHasTrie(store, insert_pos, runend_pos)) {
                    infix_to_update = Infix(store.ptr + num_metadata_offset_words,
                                            infix_store_target_size + scaled_sizes_[size_grade] + infix_size_ * insert_pos,
                                            infix_size_);
                    original_num_slots = infix_to_update.GetNumSlots(infix_size_);
                }
                else {
                    infix_to_update.infix_ = explicit_part;
                    infix_to_update.num_prefix_keys_ = 0;
                    infix_to_update.num_trie_bits_ = 1;
                    infix_to_update.trie_ = {0b1};
                    infix_to_update.num_suffixes_ = 1;
                    infix_to_update.num_suffix_bits_ = infix_size_ - 1;
                    infix_to_update.trie_suffixes_ = {0b1};
                }
                infix_to_update.InsertTrie(original_key,
                        original_key_start_bit + infix_size_ - 1,
                        infix_size_);
                const uint32_t new_num_slots = infix_to_update.GetNumSlots(infix_size_);
                num_slots_filled = new_num_slots - original_num_slots;
                if (num_slots_filled > 0) {
                    insert_pos = MakeRoomFromSlot(store, insert_pos, runend_pos, new_num_slots - original_num_slots);
                    infix_to_update.SerializeToInfixStore(store, insert_pos, scaled_sizes_[size_grade], infix_size_);
                }
                else if (num_slots_filled == 0)
                    infix_to_update.SerializeToInfixStore(store, insert_pos, scaled_sizes_[size_grade], infix_size_);
                else {
                    // TODO: We should really handle this more nicely, but I'm just trying to get this over with.
                    for (int32_t tof = 0; tof < -num_slots_filled; tof++)
                        SetSlot(store, insert_pos + tof, infix_to_update.infix_);
                    infix_to_update.SerializeToInfixStore(store, insert_pos - num_slots_filled, scaled_sizes_[size_grade], infix_size_);
                    for (int32_t tof = 0; tof < -num_slots_filled; tof++)
                        DeleteRawFromInfixStore(store, key, total_implicit, nullptr, {nullptr, 0}, 0, true);
                }
            }
            else {
                insert_pos = MakeRoomFromSlot(store, insert_pos, runend_pos, 1);
#ifdef DEBUG
                assert(insert_pos < scaled_sizes_[size_grade]);
#endif // DEBUG
                SetSlot(store, insert_pos, explicit_part);
                if constexpr (payload_type == PayloadType::FixedLength)
                    SetPayload(store, insert_pos, payload, 0);
                if (insert_pos > runend_pos) {
                    ResetRunendBit(store, runend_pos);
                    SetRunendBit(store, insert_pos);
                }
            }
        }
        else {
            insert_pos = MakeRoomFromSlot(store, insert_pos, runend_pos, 1);
#ifdef DEBUG
            assert(insert_pos < scaled_sizes_[size_grade]);
#endif // DEBUG
            SetSlot(store, insert_pos, explicit_part);
            if constexpr (payload_type == PayloadType::FixedLength)
                SetPayload(store, insert_pos, payload, 0);
            if (insert_pos > runend_pos) {
                ResetRunendBit(store, runend_pos);
                SetRunendBit(store, insert_pos);
            }
        }
    }
    SetOccupiedBit(store, implicit_part);
    store.UpdateFullSlotCount(num_slots_filled);

#ifdef DEBUG
    {
        const uint32_t *store_popcnts = reinterpret_cast<const uint32_t *>(store.ptr);
        const uint64_t *occupieds = store.ptr + num_metadata_offset_words;
        const uint64_t *runends = store.ptr + num_metadata_offset_words + infix_store_target_size / 64;
        uint32_t occupied_count = 0, runend_count = 0, popcnts[2] = {};
        for (int32_t i = 0; i < infix_store_target_size / 64; i++) {
            if (i < infix_store_target_size / 128)
                popcnts[0] += __builtin_popcountll(occupieds[i]);
            occupied_count += __builtin_popcountll(occupieds[i]);
        }
        for (int32_t i = 0; i < scaled_sizes_[size_grade]; i++) {
            if (i < infix_store_target_size / 2)
                popcnts[1] += get_bitmap_bit(runends, i);
            runend_count += get_bitmap_bit(runends, i);
        }
        assert(occupied_count == runend_count);
        assert(store_popcnts[0] == popcnts[0]);
        assert(store_popcnts[1] == popcnts[1]);
    }
#endif // DEBUG
}


template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::AdaptRawInInfixStore(InfixStore &store, const uint64_t key,
                                                                const InfiniteByteString original_key,
                                                                const uint32_t original_key_start_bit,
                                                                const uint32_t adapt_length,
                                                                const uint32_t total_implicit,
                                                                const uint64_t *payload) {
    static_assert(diva_type == DivaType::BinaryTrie);
    if constexpr (payload_type == PayloadType::FixedLength)
        assert(payload != nullptr);
    const uint32_t size_grade = store.GetSizeGrade();
    const uint64_t implicit_part = key >> infix_size_;
    const uint64_t explicit_part = key & BITMASK(infix_size_);
#ifdef DEBUG
    assert(implicit_part < total_implicit);
#endif // DEBUG
    const uint32_t key_rank = RankOccupieds(store, implicit_part);
    const bool is_occupied = GetOccupiedBit(store, implicit_part);
#ifdef DEBUG
    assert(is_occupied);
#endif // DEBUG
    const int32_t runend_pos = SelectRunends(store, key_rank);
#ifdef DEBUG
    assert(runend_pos < static_cast<int32_t>(scaled_sizes_[size_grade]));
    assert(store.GetFullSlotCount() < scaled_sizes_[size_grade]);
#endif // DEBUG
    const int32_t runstart_pos = std::max(key_rank ? static_cast<int32_t>(SelectRunends(store, key_rank - 1)) : -1,
                                          FindEmptySlotBefore(store, runend_pos)) + 1;
    int32_t adapt_pos = -1, current_pos = runstart_pos;
    Infix infix_to_adapt;
    while (current_pos <= runend_pos) {
        const uint64_t current_slot = GetSlot(store, current_pos);
        if ((current_slot & (current_slot - 1)) > explicit_part)
            break;
        const uint64_t mask = ((current_slot & -current_slot) << 1) - 1;
        const uint32_t mask_size = lowbit_pos(current_slot) + 1;
        if ((current_slot | mask) == (explicit_part | mask)) {
            if (SlotHasTrie(store, current_pos, runend_pos)) {
                infix_to_adapt = Infix(store.ptr + num_metadata_offset_words, 
                                       infix_store_target_size + scaled_sizes_[size_grade] + infix_size_ * current_pos,
                                       infix_size_);
                const uint32_t key_start_bit = original_key_start_bit + infix_size_ - mask_size;
                adapt_pos = infix_to_adapt.GetLongestMatch(original_key, key_start_bit, infix_size_) >= 0 ? current_pos
                          : adapt_pos;
                current_pos += infix_to_adapt.GetNumSlots(infix_size_);
                break;
            }
            else {
                adapt_pos = current_pos;
                current_pos++;
            }
        }
        else if (SlotHasTrie(store, current_pos, runend_pos)) {
            const Infix infix_to_skip = Infix(store.ptr + num_metadata_offset_words, 
                                              infix_store_target_size + scaled_sizes_[size_grade] + infix_size_ * current_pos,
                                              infix_size_);
            current_pos += infix_to_skip.GetNumSlots(infix_size_);
        }
        else
            current_pos++;
    }
#ifdef DEBUG
    assert(adapt_pos != -1);
#endif // DEBUG
    const uint64_t adapt_slot = GetSlot(store, adapt_pos);
    const uint32_t mask_size = lowbit_pos(adapt_slot) + 1;
    if (adapt_length <= infix_size_ - mask_size)     // Nothing to adapt
        return;
    uint32_t num_slots_filled = 0;
    if (SlotHasTrie(store, adapt_pos, runend_pos)) {
        const uint32_t original_num_slots = infix_to_adapt.GetNumSlots(infix_size_);
        const uint32_t key_start_bit = original_key_start_bit + infix_size_ - mask_size;
        infix_to_adapt.AdaptTrie(original_key, key_start_bit, adapt_length - infix_size_ + mask_size, infix_size_);
        const uint32_t new_num_slots = infix_to_adapt.GetNumSlots(infix_size_);
        num_slots_filled = new_num_slots - original_num_slots;
        store.UpdateFullSlotCount(num_slots_filled);
        if (store.GetFullSlotCount() >= scaled_sizes_[size_grade]) {    // Resize to create enough room and try again
            ResizeInfixStore(store, total_implicit);
            store.UpdateFullSlotCount(-static_cast<int32_t>(num_slots_filled));
            //AdaptRawInInfixStore(store, key, original_key, original_key_start_bit, adapt_length, total_implicit, payload);
            return;
        }
        if (new_num_slots > original_num_slots)
            adapt_pos = MakeRoomFromSlot(store, adapt_pos, runend_pos, new_num_slots - original_num_slots);
        infix_to_adapt.SerializeToInfixStore(store, adapt_pos, scaled_sizes_[size_grade], infix_size_);
    }
    else {
        infix_to_adapt = Infix(explicit_part);
        const uint32_t key_start_bit = original_key_start_bit + infix_size_ - 1;
        if (adapt_length >= infix_size_) {
            infix_to_adapt.BuildTrieAndSuffixes(&original_key , 1, key_start_bit, infix_size_);
            infix_to_adapt.AdaptTrie(original_key, key_start_bit, adapt_length - infix_size_ + 1, infix_size_);
        }
        num_slots_filled = infix_to_adapt.GetNumSlots(infix_size_) - 1;
        if (num_slots_filled > 0) {
            store.UpdateFullSlotCount(num_slots_filled);
            if (store.GetFullSlotCount() >= scaled_sizes_[size_grade]) {    // Resize to create enough room and try again
                ResizeInfixStore(store, total_implicit);
                store.UpdateFullSlotCount(-static_cast<int32_t>(num_slots_filled));
                //AdaptRawInInfixStore(store, key, original_key, original_key_start_bit, adapt_length, total_implicit, payload);
                return;
            }
            adapt_pos = MakeRoomFromSlot(store, adapt_pos, runend_pos, num_slots_filled);
        }
        infix_to_adapt.SerializeToInfixStore(store, adapt_pos, scaled_sizes_[size_grade], infix_size_);
    }

#ifdef DEBUG
    {
        const uint32_t *store_popcnts = reinterpret_cast<const uint32_t *>(store.ptr);
        const uint64_t *occupieds = store.ptr + num_metadata_offset_words;
        const uint64_t *runends = store.ptr + num_metadata_offset_words + infix_store_target_size / 64;
        uint32_t occupied_count = 0, runend_count = 0, popcnts[2] = {};
        for (int32_t i = 0; i < infix_store_target_size / 64; i++) {
            if (i < infix_store_target_size / 128)
                popcnts[0] += __builtin_popcountll(occupieds[i]);
            occupied_count += __builtin_popcountll(occupieds[i]);
        }
        for (int32_t i = 0; i < scaled_sizes_[size_grade]; i++) {
            if (i < infix_store_target_size / 2)
                popcnts[1] += get_bitmap_bit(runends, i);
            runend_count += get_bitmap_bit(runends, i);
        }
        assert(occupied_count == runend_count);
        assert(store_popcnts[0] == popcnts[0]);
        assert(store_popcnts[1] == popcnts[1]);
    }
#endif // DEBUG
}


template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::DeleteRawFromInfixStore(InfixStore &store, const uint64_t key,
                                                                   const uint32_t total_implicit,
                                                                   std::function<bool(const uint64_t *)> should_remove,
                                                                   const InfiniteByteString original_key,
                                                                   const uint32_t original_key_start_bit,
                                                                   bool take_first_full_match) {
    uint32_t size_grade = store.GetSizeGrade();
    const uint64_t full_slot_count = store.GetFullSlotCount();
    if (!take_first_full_match && 
            (size_grade > 0 
                && full_slot_count <= (size_grade > 1 ? scaled_sizes_[size_grade - 2] : exception_scaled_size_))) {
        ResizeInfixStore(store, total_implicit);
    }
    size_grade = store.GetSizeGrade();
    const uint64_t store_size = scaled_sizes_[size_grade];

    const uint64_t implicit_part = key >> infix_size_;
    const uint64_t explicit_part = key & BITMASK(infix_size_);
    const uint64_t implicit_scalar = implicit_scalars_[total_implicit - infix_store_target_size / 2];

    uint32_t current_implicit = implicit_part;
    const bool is_occupied = GetOccupiedBit(store, implicit_part);
#ifdef DEBUG
    assert(is_occupied);
#endif // DEBUG

    // Find all positions we have to shift and remove the victim on the fly
    const int32_t rank = RankOccupieds(store, current_implicit);
    const int32_t runend_pos = SelectRunends(store, rank);
    const int32_t previous_empty = FindEmptySlotBefore(store, runend_pos);
    const int32_t runstart_pos = std::max<int32_t>(rank ? SelectRunends(store, rank - 1) : -1,
                                                   previous_empty) + 1;
#ifdef DEBUG
    assert(runstart_pos < store_size);
    assert(runend_pos < store_size);
#endif // DEBUG
    uint32_t implicits[2 * infix_store_target_size] = {static_cast<uint32_t>(current_implicit), 0};
    int32_t mapped_poses[2 * infix_store_target_size] = {GetMappedPos(current_implicit, size_grade, implicit_scalar), 0};
    int32_t l[2 * infix_store_target_size] = {runstart_pos, 0};
    int32_t orig_l[2 * infix_store_target_size] = {runstart_pos, 0};
    int32_t r[2 * infix_store_target_size] = {runend_pos + 1, 0};
    int32_t orig_r[2 * infix_store_target_size] = {runend_pos + 1, 0};
    int32_t candidate_run_ind = 0;
    int32_t current_pos = l[candidate_run_ind], current_runend_pos = runend_pos; 
    int32_t to_remove = -1;
    uint64_t current_slot = GetSlot(store, runstart_pos);
    bool found_empty_right = false;
    while (current_pos < store_size && !found_empty_right) {
        const uint64_t mask = ((current_slot & -current_slot) << 1) - 1;
        bool remove = take_first_full_match ? (to_remove == -1 && (implicit_part == current_implicit && current_slot == explicit_part))
                                       : (implicit_part == current_implicit && (current_slot | mask) == (explicit_part | mask));
        if constexpr (payload_type == PayloadType::FixedLength) {
            if (should_remove && remove) {
                uint64_t payload[(payload_size_ + 63) / 64 + 1];
                GetPayload(store, current_pos, payload);
                remove &= should_remove(payload);
            }
        }
        if constexpr (diva_type == DivaType::BinaryTrie) {  // TODO: Figure out integration of this with payloads
            if (remove && SlotHasTrie(store, current_pos, current_runend_pos)) {
                const Infix infix_to_get_longest_match(store.ptr + num_metadata_offset_words,
                        infix_store_target_size + store_size + infix_size_ * current_pos,
                        infix_size_);
                const uint32_t mask_size = lowbit_pos(infix_to_get_longest_match.infix_) + 1;
                const uint32_t key_start_bit = original_key_start_bit + infix_size_ - mask_size;
                remove &= infix_to_get_longest_match.GetLongestMatch(original_key, key_start_bit, infix_size_) >= 0;
            }
        }
        to_remove = remove ? current_pos : to_remove;

        // Advance to the next infix
        if constexpr (diva_type == DivaType::BinaryTrie) {
            if (SlotHasTrie(store, current_pos, current_runend_pos)) {
                const Infix infix_to_skip(store.ptr + num_metadata_offset_words,
                        infix_store_target_size + store_size + infix_size_ * current_pos,
                        infix_size_);
                current_pos += infix_to_skip.GetNumSlots(infix_size_);
            }
            else 
                current_pos++;
        }
        else 
            current_pos++;
        if (current_pos < store_size)
            current_slot = GetSlot(store, current_pos);
        found_empty_right = (current_slot == 0);
        if (GetRunendBit(store, current_pos - 1)) {    // Advance to the next run
            current_implicit = NextOccupied(store, current_implicit);
            current_runend_pos = NextRunend(store, current_runend_pos);
            if (current_pos < store_size && !found_empty_right) {
                candidate_run_ind++;
                implicits[candidate_run_ind] = current_implicit;
                mapped_poses[candidate_run_ind] = GetMappedPos(implicits[candidate_run_ind],
                                                               size_grade, implicit_scalar);
                l[candidate_run_ind] = current_pos;
                orig_l[candidate_run_ind] = l[candidate_run_ind];
                r[candidate_run_ind] = NextRunend(store, current_pos - 1) + 1;
                orig_r[candidate_run_ind] = r[candidate_run_ind];
            }
        }
    }
    uint32_t num_runs_to_left = 0;
    if (!found_empty_right) {   // Figure out the runs that were shifted to the left
        const uint32_t orig_candidate_run_ind = candidate_run_ind;
        current_implicit = implicit_part;
        for (int32_t i = runstart_pos - 1; i > previous_empty; i = PreviousRunend(store, i)) {
            if (candidate_run_ind != orig_candidate_run_ind) {
                l[candidate_run_ind] = i + 1;
                orig_l[candidate_run_ind] = l[candidate_run_ind];
            }
            candidate_run_ind++;
            current_implicit = PreviousOccupied(store, current_implicit);
            implicits[candidate_run_ind] = current_implicit;
            mapped_poses[candidate_run_ind] = GetMappedPos(current_implicit, size_grade, implicit_scalar);
            r[candidate_run_ind] = i + 1;
            orig_r[candidate_run_ind] = r[candidate_run_ind];
        }
        if (candidate_run_ind != orig_candidate_run_ind) {
            l[candidate_run_ind] = previous_empty + 1;
            orig_l[candidate_run_ind] = l[candidate_run_ind];
        }
#ifdef DEBUG
        assert(candidate_run_ind <= infix_store_target_size);
#endif // DEBUG

        num_runs_to_left = (candidate_run_ind - orig_candidate_run_ind);
        if (num_runs_to_left > 0) {     // Restore increasing order
            std::reverse(implicits + orig_candidate_run_ind + 1, implicits + candidate_run_ind + 1);
            std::reverse(mapped_poses + orig_candidate_run_ind + 1, mapped_poses + candidate_run_ind + 1);
            std::reverse(l + orig_candidate_run_ind + 1, l + candidate_run_ind + 1);
            std::reverse(orig_l + orig_candidate_run_ind + 1, orig_l + candidate_run_ind + 1);
            std::reverse(r + orig_candidate_run_ind + 1, r + candidate_run_ind + 1);
            std::reverse(orig_r + orig_candidate_run_ind + 1, orig_r + candidate_run_ind + 1);
        
            const uint32_t orig_size = (orig_candidate_run_ind + 1) * sizeof(uint32_t);
            const uint32_t added_size = num_runs_to_left * sizeof(uint32_t);
#ifdef DEBUG
            assert(num_runs_to_left <= infix_store_target_size);
#endif // DEBUG
            // Shift right
            memmove(implicits + num_runs_to_left, implicits, orig_size + added_size);
            memmove(mapped_poses + num_runs_to_left, mapped_poses, orig_size + added_size);
            memmove(l + num_runs_to_left, l, orig_size + added_size);
            memmove(orig_l + num_runs_to_left, orig_l, orig_size + added_size);
            memmove(r + num_runs_to_left, r, orig_size + added_size);
            memmove(orig_r + num_runs_to_left, orig_r, orig_size + added_size);
            // Move new ones to before (use memcpy for faster data movement)
            memcpy(implicits, implicits + candidate_run_ind + 1, added_size);
            memcpy(mapped_poses, mapped_poses + candidate_run_ind + 1, added_size);
            memcpy(l, l + candidate_run_ind + 1, added_size);
            memcpy(orig_l, orig_l + candidate_run_ind + 1, added_size);
            memcpy(r, r + candidate_run_ind + 1, added_size);
            memcpy(orig_r, orig_r + candidate_run_ind + 1, added_size);
        }
    }

#ifdef DEBUG
    assert(to_remove != -1);
#endif // DEBUG
    const uint64_t slot_to_remove = GetSlot(store, to_remove);
    uint32_t deleted_count = 1;
    if constexpr (diva_type == DivaType::BinaryTrie) {  // TODO: Figure out integration of this with payloads
        if ((slot_to_remove & 1) && SlotHasTrie(store, to_remove, runend_pos)) {
            Infix infix_to_delete_from(store.ptr + num_metadata_offset_words,
                    infix_store_target_size + store_size + infix_size_ * to_remove,
                    infix_size_);
            deleted_count = infix_to_delete_from.GetNumSlots(infix_size_);
            const uint32_t mask_size = lowbit_pos(infix_to_delete_from.infix_) + 1;
            const uint32_t key_start_bit = original_key_start_bit + infix_size_ - mask_size;
            infix_to_delete_from.DeleteTrie(original_key, key_start_bit, infix_size_);
            deleted_count -= infix_to_delete_from.GetNumSlots(infix_size_) - (infix_to_delete_from.num_trie_bits_ == 0);
            ShiftSlotsLeft(store, to_remove + deleted_count, r[num_runs_to_left], deleted_count);
            if (infix_to_delete_from.num_trie_bits_ > 0)
                infix_to_delete_from.SerializeToInfixStore(store, to_remove, store_size, infix_size_);
        }
        else {
            ShiftSlotsLeft(store, to_remove + deleted_count, r[num_runs_to_left], deleted_count);
            if constexpr (payload_type == PayloadType::FixedLength)
                ShiftPayloadsLeft(store, to_remove + deleted_count, r[num_runs_to_left], deleted_count);
        }
    }
    else {
        ShiftSlotsLeft(store, to_remove + deleted_count, r[num_runs_to_left], deleted_count);
        if constexpr (payload_type == PayloadType::FixedLength)
            ShiftPayloadsLeft(store, to_remove + deleted_count, r[num_runs_to_left], deleted_count);
    }
    r[num_runs_to_left] -= deleted_count;

    // Readjust the `l` and `r`s
    for (uint32_t i = 0; i <= candidate_run_ind; i++) {
        const int32_t delta = mapped_poses[i] - l[i];
        l[i] += delta;
        r[i] += delta;
    }
    if (found_empty_right) {
        const int32_t delta = runstart_pos - l[0];
        l[0] += delta;
        r[0] += delta;
    }
    for (uint32_t i = 1; i <= candidate_run_ind; i++) {
        const int32_t delta = std::max(r[i - 1], l[i]) - l[i];
        l[i] += delta;
        r[i] += delta;
    }
    if (!found_empty_right) {
        const int32_t delta = std::min<int32_t>(store_size, r[candidate_run_ind]) - r[candidate_run_ind];
        l[candidate_run_ind] += delta;
        r[candidate_run_ind] += delta;
        for (int32_t i = candidate_run_ind - 1; i >= 0; i--) {
            const int32_t delta = std::min(l[i + 1], r[i]) - r[i];
            l[i] += delta;
            r[i] += delta;
        }
    }

    // Shift slots and payloads
    int32_t shift_stack_bottom = -1;
    for (int32_t i = 0; i <= candidate_run_ind; i++) {
        if (orig_l[i] >= l[i]) {
            for (int32_t j = i; j > shift_stack_bottom; j--) {
                if (l[j] == r[j])
                    continue;
                const int32_t shamt = l[j] - orig_l[j];
                const int32_t length = r[j] - l[j];
#ifdef DEBUG
                assert(0 <= orig_l[j] + shamt);
#endif // DEBUG
                if (shamt > 0) {
#ifdef DEBUG
                    assert(orig_l[j] + length + shamt <= store_size);
#endif // DEBUG
                    ShiftSlotsRight(store, orig_l[j], orig_l[j] + length, shamt);
                    if constexpr (payload_type == PayloadType::FixedLength)
                        ShiftPayloadsRight(store, orig_l[j], orig_l[j] + length, shamt);
                }
                else if (shamt < 0) {
#ifdef DEBUG
                    assert(orig_l[j] + length <= store_size);
#endif // DEBUG
                    ShiftSlotsLeft(store, orig_l[j], orig_l[j] + length, -shamt);
                    if constexpr (payload_type == PayloadType::FixedLength)
                        ShiftPayloadsLeft(store, orig_l[j], orig_l[j] + length, -shamt);
                }
            }
            shift_stack_bottom = i;
        }
    }
    if (shift_stack_bottom < candidate_run_ind) {
        for (int32_t j = candidate_run_ind; j > shift_stack_bottom; j--) {
            if (l[j] == r[j])
                continue;
            const int32_t shamt = l[j] - orig_l[j];
            const int32_t length = r[j] - l[j];
#ifdef DEBUG
            assert(0 <= orig_l[j] + shamt);
#endif // DEBUG
            if (shamt > 0) {
#ifdef DEBUG
                assert(orig_l[j] + length + shamt <= store_size);
#endif // DEBUG
                ShiftSlotsRight(store, orig_l[j], orig_l[j] + length, shamt);
                if constexpr (payload_type == PayloadType::FixedLength)
                    ShiftPayloadsRight(store, orig_l[j], orig_l[j] + length, shamt);
            }
            else if (shamt < 0) {
#ifdef DEBUG
                assert(orig_l[j] + length <= store_size);
#endif // DEBUG
                ShiftSlotsLeft(store, orig_l[j], orig_l[j] + length, -shamt);
                if constexpr (payload_type == PayloadType::FixedLength)
                    ShiftPayloadsLeft(store, orig_l[j], orig_l[j] + length, -shamt);
            }
        }
    }
    // Reset the old runends and destroyed runs' occupieds
    for (uint32_t i = 0; i <= candidate_run_ind; i++) {
        ResetRunendBit(store, orig_r[i] - 1);
        if (l[i] == r[i])
            ResetOccupiedBit(store, implicits[i]);
    }
    // Set the new runends
    for (uint32_t i = 0; i <= candidate_run_ind; i++) {
        if (l[i] < r[i])
            SetRunendBit(store, r[i] - 1);
    }

    // Update `store`'s `popcnts` and the number of its elements.
    UpdatePopcnts(store);
    store.UpdateFullSlotCount(-static_cast<int32_t>(deleted_count));
}


template <DivaType diva_type, PayloadType payload_type>
inline std::pair<uint64_t, uint32_t>
Diva<diva_type, payload_type>::DeleteRawRangeFromInfixStore(InfixStore &store,
                                                            const uint64_t l_key,
                                                            const uint64_t r_key, 
                                                            const uint32_t total_implicit, 
                                                            std::function<bool(const uint64_t *)> should_remove) {
    static_assert(diva_type != DivaType::BinaryTrie);   // TODO: Figure out integration of this with payloads
    uint32_t deleted_count = 0;

    const uint32_t size_grade = store.GetSizeGrade();
    const uint64_t implicit_part_l = l_key >> infix_size_;
    const uint64_t explicit_part_l = (l_key & BITMASK(infix_size_)) - 1;  // Remove the age counter bit
    const uint64_t implicit_part_r = r_key >> infix_size_;
    const uint64_t explicit_part_r = r_key & BITMASK(infix_size_);
    const uint64_t implicit_scalar = implicit_scalars_[total_implicit - infix_store_target_size / 2];

    uint64_t *occupieds = store.ptr + num_metadata_offset_words;
    uint64_t *runends = store.ptr + num_metadata_offset_words + infix_store_target_size / 64;
    uint32_t current_implicit = implicit_part_l;
    if (!get_bitmap_bit(occupieds, current_implicit))
        current_implicit = NextOccupied(store, implicit_part_l);
    if (current_implicit > implicit_part_r)  // Ensure the range is actually non-empty
        return {current_implicit, deleted_count};
    if (current_implicit >= total_implicit) // Ensure we have a valid range
        return {std::numeric_limits<uint64_t>::max(), deleted_count};

    // Find all positions we have to shift and remove victims on the fly
    const int32_t rank = RankOccupieds(store, current_implicit);
    const int32_t runend_pos = SelectRunends(store, rank);
    const int32_t previous_empty = FindEmptySlotBefore(store, runend_pos);
    const int32_t runstart_pos = std::max<int32_t>(rank ? SelectRunends(store, rank - 1) : -1,
                                                   previous_empty) + 1;
#ifdef DEBUG
    assert(runstart_pos < scaled_sizes_[size_grade]);
    assert(runend_pos < scaled_sizes_[size_grade]);
#endif // DEBUG
    uint32_t implicits[2 * infix_store_target_size] = {static_cast<uint32_t>(current_implicit), 0};
    int32_t mapped_poses[2 * infix_store_target_size] = {GetMappedPos(current_implicit, size_grade, implicit_scalar), 0};
    int32_t l[2 * infix_store_target_size] = {runstart_pos, 0};
    int32_t orig_l[2 * infix_store_target_size] = {runstart_pos, 0};
    int32_t r[2 * infix_store_target_size] = {runend_pos + 1, 0};
    int32_t orig_r[2 * infix_store_target_size] = {runend_pos + 1, 0};
    int32_t candidate_run_ind = 0;
    int32_t current_pos = l[candidate_run_ind]; 
    int32_t last_removed = l[candidate_run_ind] - 1;
    int32_t last_written_pos = l[candidate_run_ind] - 1;
    uint64_t current_slot = GetSlot(store, runstart_pos);
    bool found_empty_right = false;
    while (current_pos < scaled_sizes_[size_grade] && !found_empty_right) {
        bool remove = (implicit_part_l < current_implicit && current_implicit < implicit_part_r);
        remove |= (implicit_part_l == current_implicit) && (explicit_part_l <= (current_slot & (current_slot - 1)));
        remove |= (implicit_part_r == current_implicit) && (explicit_part_r >= (current_slot | (current_slot - 1)));
        if constexpr (payload_type == PayloadType::FixedLength) {
            if (should_remove && remove) {
                uint64_t payload[(payload_size_ + 63) / 64 + 1];
                GetPayload(store, current_pos, payload);
                remove &= should_remove(payload);
            }
        }
        if (remove) {
            if (last_removed == current_pos - 1)
                last_removed++;
            else {
                const int32_t shamt = last_removed - last_written_pos;
                const uint32_t shift_length = current_pos - last_removed - 1;
#ifdef DEBUG
                assert(shamt >= 0);
                assert(0 <= last_removed + 1 - shamt);
#endif // DEBUG
                MoveSlotsLeft(store, last_removed + 1, current_pos, shamt);
                if constexpr (payload_type == PayloadType::FixedLength)
                    MovePayloadsLeft(store, last_removed + 1, current_pos, shamt);
                last_written_pos += shift_length;
                last_removed = current_pos;
            }
            r[candidate_run_ind]--;
            deleted_count++;
        }

        // Advance to the next slot
        current_pos++;
        if (current_pos < scaled_sizes_[size_grade])
            current_slot = GetSlot(store, current_pos);
        found_empty_right = (current_slot == 0);
        if (get_bitmap_bit(runends, current_pos - 1)) {     // Advance to the next run
            const int32_t shamt = last_removed - last_written_pos;
            const uint32_t shift_length = current_pos - last_removed - 1;
#ifdef DEBUG
            assert(shamt >= 0);
            assert(0 <= last_removed + 1 - shamt);
            assert(current_pos <= scaled_sizes_[size_grade]);
#endif // DEBUG
            MoveSlotsLeft(store, last_removed + 1, current_pos, shamt);
            if constexpr (payload_type == PayloadType::FixedLength)
                MovePayloadsLeft(store, last_removed + 1, current_pos, shamt);
            last_written_pos += shift_length;

            if (last_written_pos + 1 < current_pos) {
                ZeroOutSlots(store, last_written_pos + 1, current_pos);
                if constexpr (payload_type == PayloadType::FixedLength)
                    ZeroOutPayloads(store, last_written_pos + 1, current_pos);
            }

#ifdef DEBUG
            assert(last_written_pos == r[candidate_run_ind] - 1);
#endif // DEBUG

            current_implicit = NextOccupied(store, current_implicit);
            if (current_pos < scaled_sizes_[size_grade] && !found_empty_right) {
                candidate_run_ind++;
                implicits[candidate_run_ind] = current_implicit;
                mapped_poses[candidate_run_ind] = GetMappedPos(implicits[candidate_run_ind],
                                                               size_grade, implicit_scalar);
                l[candidate_run_ind] = current_pos;
                orig_l[candidate_run_ind] = l[candidate_run_ind];
                r[candidate_run_ind] = NextRunend(store, current_pos - 1) + 1;
                orig_r[candidate_run_ind] = r[candidate_run_ind];
                last_removed = l[candidate_run_ind] - 1;
                last_written_pos = l[candidate_run_ind] - 1;
            }
        }
    }

    if (!found_empty_right) {   // Figure out the runs that were shifted to the left
        const uint32_t orig_candidate_run_ind = candidate_run_ind;
        current_implicit = implicit_part_l;
        for (int32_t i = runstart_pos - 1; i > previous_empty; i = PreviousRunend(store, i)) {
            if (candidate_run_ind != orig_candidate_run_ind) {
                l[candidate_run_ind] = i + 1;
                orig_l[candidate_run_ind] = l[candidate_run_ind];
            }
            candidate_run_ind++;
            current_implicit = PreviousOccupied(store, current_implicit);
            implicits[candidate_run_ind] = current_implicit;
            mapped_poses[candidate_run_ind] = GetMappedPos(current_implicit, size_grade, implicit_scalar);
            r[candidate_run_ind] = i + 1;
            orig_r[candidate_run_ind] = r[candidate_run_ind];
        }
        if (candidate_run_ind != orig_candidate_run_ind) {
            l[candidate_run_ind] = previous_empty + 1;
            orig_l[candidate_run_ind] = l[candidate_run_ind];
        }
#ifdef DEBUG
        assert(candidate_run_ind <= infix_store_target_size);
#endif // DEBUG

        const uint32_t num_runs_to_left = (candidate_run_ind - orig_candidate_run_ind);

        if (num_runs_to_left > 0) {     // Restore increasing order
            std::reverse(implicits + orig_candidate_run_ind + 1, implicits + candidate_run_ind + 1);
            std::reverse(mapped_poses + orig_candidate_run_ind + 1, mapped_poses + candidate_run_ind + 1);
            std::reverse(l + orig_candidate_run_ind + 1, l + candidate_run_ind + 1);
            std::reverse(orig_l + orig_candidate_run_ind + 1, orig_l + candidate_run_ind + 1);
            std::reverse(r + orig_candidate_run_ind + 1, r + candidate_run_ind + 1);
            std::reverse(orig_r + orig_candidate_run_ind + 1, orig_r + candidate_run_ind + 1);
        
            const uint32_t orig_size = (orig_candidate_run_ind + 1) * sizeof(uint32_t);
            const uint32_t added_size = num_runs_to_left * sizeof(uint32_t);
#ifdef DEBUG
            assert(num_runs_to_left <= infix_store_target_size);
#endif // DEBUG
            // Shift right
            memmove(implicits + num_runs_to_left, implicits, orig_size + added_size);
            memmove(mapped_poses + num_runs_to_left, mapped_poses, orig_size + added_size);
            memmove(l + num_runs_to_left, l, orig_size + added_size);
            memmove(orig_l + num_runs_to_left, orig_l, orig_size + added_size);
            memmove(r + num_runs_to_left, r, orig_size + added_size);
            memmove(orig_r + num_runs_to_left, orig_r, orig_size + added_size);
            // Move new ones to before (use memcpy for faster data movement)
            memcpy(implicits, implicits + candidate_run_ind + 1, added_size);
            memcpy(mapped_poses, mapped_poses + candidate_run_ind + 1, added_size);
            memcpy(l, l + candidate_run_ind + 1, added_size);
            memcpy(orig_l, orig_l + candidate_run_ind + 1, added_size);
            memcpy(r, r + candidate_run_ind + 1, added_size);
            memcpy(orig_r, orig_r + candidate_run_ind + 1, added_size);
        }
    }

    // Readjust the `l` and `r`s
    for (uint32_t i = 0; i <= candidate_run_ind; i++) {
        const int32_t delta = mapped_poses[i] - l[i];
        l[i] += delta;
        r[i] += delta;
    }
    if (found_empty_right) {
        const int32_t delta = runstart_pos - l[0];
        l[0] += delta;
        r[0] += delta;
    }
    for (uint32_t i = 1; i <= candidate_run_ind; i++) {
        const int32_t delta = std::max(r[i - 1], l[i]) - l[i];
        l[i] += delta;
        r[i] += delta;
    }
    if (!found_empty_right) {
        const int32_t delta = std::min<int32_t>(scaled_sizes_[size_grade], r[candidate_run_ind])
                                    - r[candidate_run_ind];
        l[candidate_run_ind] += delta;
        r[candidate_run_ind] += delta;
        for (int32_t i = candidate_run_ind - 1; i >= 0; i--) {
            const int32_t delta = std::min(l[i + 1], r[i]) - r[i];
            l[i] += delta;
            r[i] += delta;
        }
    }

    // Shift slots and payloads
    int32_t shift_stack_bottom = -1;
    for (int32_t i = 0; i <= candidate_run_ind; i++) {
        if (orig_l[i] >= l[i]) {
            for (int32_t j = i; j > shift_stack_bottom; j--) {
                if (l[j] == r[j])
                    continue;
                const int32_t shamt = l[j] - orig_l[j];
                const int32_t length = r[j] - l[j];
#ifdef DEBUG
                assert(0 <= orig_l[j] + shamt);
#endif // DEBUG
                if (shamt > 0) {
#ifdef DEBUG
                    assert(orig_l[j] + length + shamt <= scaled_sizes_[size_grade]);
#endif // DEBUG
                    MoveSlotsRight(store, orig_l[j], orig_l[j] + length, shamt);
                    ZeroOutSlots(store, orig_l[j], std::min(orig_l[j] + length, orig_l[j] + shamt));
                    if constexpr (payload_type == PayloadType::FixedLength) {
                        MovePayloadsRight(store, orig_l[j], orig_l[j] + length, shamt);
                        ZeroOutPayloads(store, orig_l[j], std::min(orig_l[j] + length, orig_l[j] + shamt));
                    }
                }
                else if (shamt < 0) {
#ifdef DEBUG
                    assert(orig_l[j] + length <= scaled_sizes_[size_grade]);
#endif // DEBUG
                    MoveSlotsLeft(store, orig_l[j], orig_l[j] + length, -shamt);
                    ZeroOutSlots(store, std::max(orig_l[j], orig_l[j] + length - shamt), orig_l[j] + length);
                    if constexpr (payload_type == PayloadType::FixedLength) {
                        MovePayloadsLeft(store, orig_l[j], orig_l[j] + length, -shamt);
                        ZeroOutPayloads(store, std::max(orig_l[j], orig_l[j] + length - shamt), orig_l[j] + length);
                    }
                }
            }
            shift_stack_bottom = i;
        }
    }
    if (shift_stack_bottom < candidate_run_ind) {
        for (int32_t j = candidate_run_ind; j > shift_stack_bottom; j--) {
            if (l[j] == r[j])
                continue;
            const int32_t shamt = l[j] - orig_l[j];
            const int32_t length = r[j] - l[j];
#ifdef DEBUG
            assert(0 <= orig_l[j] + shamt);
#endif // DEBUG
            if (shamt > 0) {
#ifdef DEBUG
                assert(orig_l[j] + length + shamt <= scaled_sizes_[size_grade]);
#endif // DEBUG
                MoveSlotsRight(store, orig_l[j], orig_l[j] + length, shamt);
                ZeroOutSlots(store, orig_l[j], std::min(orig_l[j] + length, orig_l[j] + shamt));
                if constexpr (payload_type == PayloadType::FixedLength) {
                    MovePayloadsRight(store, orig_l[j], orig_l[j] + length, shamt);
                    ZeroOutPayloads(store, orig_l[j], std::min(orig_l[j] + length, orig_l[j] + shamt));
                }
            }
            else if (shamt < 0) {
#ifdef DEBUG
                assert(orig_l[j] + length <= scaled_sizes_[size_grade]);
#endif // DEBUG
                MoveSlotsLeft(store, orig_l[j], orig_l[j] + length, -shamt);
                ZeroOutSlots(store, std::max(orig_l[j], orig_l[j] + length - shamt), orig_l[j] + length);
                if constexpr (payload_type == PayloadType::FixedLength) {
                    MovePayloadsLeft(store, orig_l[j], orig_l[j] + length, -shamt);
                    ZeroOutPayloads(store, std::max(orig_l[j], orig_l[j] + length - shamt), orig_l[j] + length);
                }
            }
        }
    }
    // Reset the old runends and destroyed runs' occupieds
    for (uint32_t i = 0; i <= candidate_run_ind; i++) {
#ifdef DEBUG
        assert(get_bitmap_bit(runends, orig_r[i] - 1));
        if (i)
            assert(orig_r[i] > orig_r[i - 1]);
#endif // DEBUG
        reset_bitmap_bit(runends, orig_r[i] - 1);
        if (l[i] == r[i])
            reset_bitmap_bit(occupieds, implicits[i]);
    }
    // Set the new runends
    for (uint32_t i = 0; i <= candidate_run_ind; i++) {
        if (l[i] < r[i])
            set_bitmap_bit(runends, r[i] - 1);
    }

    // Update `store`'s `popcnts` and the number of its elements.
    UpdatePopcnts(store);
    store.UpdateFullSlotCount(-static_cast<int32_t>(deleted_count));

    return {implicits[candidate_run_ind] + 1, deleted_count};
}


template <DivaType diva_type, PayloadType payload_type>
inline int32_t Diva<diva_type, payload_type>::GetLongestMatchingInfixSize(const InfixStore &store, const uint64_t key,
                                                                          const uint32_t total_implicit,
                                                                          std::function<bool(const uint64_t*)> should_consider,
                                                                          const InfiniteByteString original_key,
                                                                          const uint32_t original_key_start_bit) const {
    const uint32_t store_size = scaled_sizes_[store.GetSizeGrade()];
    const uint64_t implicit_part = key >> infix_size_;
    const uint64_t explicit_part = key & BITMASK(infix_size_);

    if (!GetOccupiedBit(store, implicit_part))
        return 0;   // No matching infix found

    const uint32_t key_rank = RankOccupieds(store, implicit_part);
    const int32_t runend_pos = SelectRunends(store, key_rank);
    const int32_t runstart_pos = std::max(key_rank ? static_cast<int32_t>(SelectRunends(store, key_rank - 1)) : -1,
                                          static_cast<int32_t>(FindEmptySlotBefore(store, runend_pos))) + 1;
    uint32_t res = 0;
    if constexpr (diva_type == DivaType::BinaryTrie) {  // TODO: Figure out integration of this with payloads
        for (int32_t i = runstart_pos; i <= runend_pos; i++) {
            const uint64_t current_slot = GetSlot(store, i);
            const uint64_t mask = ((current_slot & -current_slot) << 1) - 1;
            if ((current_slot | mask) == (explicit_part | mask)) {
                if (SlotHasTrie(store, i, runend_pos)) {
                    const Infix infix_to_query(store.ptr + num_metadata_offset_words,
                            infix_store_target_size + store_size + infix_size_ * i,
                            infix_size_);
                    const uint32_t mask_size = lowbit_pos(infix_to_query.infix_) + 1;
                    const uint32_t key_start_bit = original_key_start_bit + infix_size_ - mask_size;
                    const int32_t trie_match_size = infix_to_query.GetLongestMatch(original_key,
                                                                                   key_start_bit,
                                                                                   infix_size_);
                    res = trie_match_size > 0 ? std::max(res, infix_size_ + trie_match_size) 
                                              : res;
                    break;
                }
                else 
                    res = infix_size_ - lowbit_pos(current_slot);
            }
            else if ((current_slot & (current_slot - 1)) > explicit_part)
                break;
        }
    }
    else {
        int32_t l = runstart_pos - 1, r = runend_pos + 1, mid;
        while (r - l > 1) {
            mid = (l + r) / 2;
            const uint64_t current_slot = GetSlot(store, mid);
            const bool cond = (current_slot & (current_slot - 1)) <= key - 1;
            l = cond ? mid : l;
            r = cond ? r : mid;
        }
        int32_t match_pos;
        for (match_pos = l; match_pos >= runstart_pos; match_pos--) {
            if constexpr (payload_type == PayloadType::FixedLength) {
                uint64_t payload[(payload_size_ + 63) / 64 + 1];
                GetPayload(store, match_pos, payload);
                if (!should_consider(payload))
                    continue;
            }
            const uint64_t current_slot = GetSlot(store, match_pos);
            const uint64_t mask = ((current_slot & -current_slot) << 1) - 1;
            if ((current_slot | mask) == (explicit_part | mask)) {
                res = infix_size_ - lowbit_pos(current_slot);
                break;
            }
        }
    }
    return res - 1;
}


template <DivaType diva_type, PayloadType payload_type>
inline bool Diva<diva_type, payload_type>::RangeQueryInfixStore(InfixStore &store, const uint64_t l_key, const uint64_t r_key,
                                                                const uint32_t total_implicit,
                                                                const InfiniteByteString original_l_key,
                                                                const InfiniteByteString original_r_key,
                                                                const uint32_t original_key_start_bit) const {
    const uint32_t store_size = scaled_sizes_[store.GetSizeGrade()];
    const uint64_t l_implicit_part = l_key >> infix_size_;
    const uint64_t l_explicit_part = l_key & BITMASK(infix_size_);
    const uint64_t r_implicit_part = r_key >> infix_size_;
    const uint64_t r_explicit_part = r_key & BITMASK(infix_size_);
    if (l_implicit_part < r_implicit_part) {
        if (NextOccupied(store, l_implicit_part) < r_implicit_part)
            return true;

        if (GetOccupiedBit(store, r_implicit_part)) {
            const uint32_t rank = RankOccupieds(store, r_implicit_part);
            const int32_t runend_pos = SelectRunends(store, rank);
            const int32_t runstart_pos = std::max<int32_t>(rank ? SelectRunends(store, rank - 1) : -1,
                                                           FindEmptySlotBefore(store, runend_pos)) + 1;
            const uint64_t current_slot = GetSlot(store, runstart_pos);
            const uint64_t current_slot_l = current_slot & (current_slot - 1);
            if constexpr (diva_type == DivaType::BinaryTrie) {
                if (current_slot_l <= r_explicit_part) {
                    if (SlotHasTrie(store, runstart_pos, runend_pos)) {
                        const Infix infix_to_query(store.ptr + num_metadata_offset_words,
                                infix_store_target_size + store_size + infix_size_ * runstart_pos,
                                infix_size_);
                        const uint32_t mask_size = lowbit_pos(infix_to_query.infix_) + 1;
                        const uint8_t zero_key[1] = {0};
                        if (infix_to_query.QueryTrie({zero_key, 1},
                                    original_r_key,
                                    original_key_start_bit + infix_size_ - mask_size,
                                    infix_size_))
                            return true;
                    }
                    else 
                        return true;
                }
            }
            else {
                if (current_slot_l <= r_explicit_part)
                    return true;
            }
        }
        if (GetOccupiedBit(store, l_implicit_part)) {
            const uint32_t rank = RankOccupieds(store, l_implicit_part);
            const int32_t runend_pos = SelectRunends(store, rank);
            const int32_t runstart_pos = std::max<int32_t>(rank ? SelectRunends(store, rank - 1) : -1,
                                                           FindEmptySlotBefore(store, runend_pos)) + 1;
            if constexpr (diva_type == DivaType::BinaryTrie) {
                for (int32_t pos = runstart_pos; pos < runend_pos; pos++) {
                    const uint64_t current_slot = GetSlot(store, pos);
                    const uint64_t current_slot_r = current_slot | (current_slot - 1);
                    if (SlotHasTrie(store, pos, runend_pos)) {
                        const Infix infix(store.ptr + num_metadata_offset_words,
                                infix_store_target_size + store_size + infix_size_ * pos,
                                infix_size_);
                        if (current_slot_r >= l_explicit_part) {
                                const uint32_t mask_size = lowbit_pos(infix.infix_) + 1;
                                const uint32_t one_key_max_len = (original_key_start_bit + (infix.GetNumSlots(infix_size_) + 1) * infix_size_ + 7) / 8;
                                uint8_t one_key[one_key_max_len];
                                memset(one_key, 0xFF, one_key_max_len);
                                if (infix.QueryTrie(original_l_key, 
                                            {one_key, one_key_max_len},
                                            original_key_start_bit + infix_size_ - mask_size,
                                            infix_size_))
                                    return true;
                        }
                        pos += infix.GetNumSlots(infix_size_) - 1;
                    }
                    else if (current_slot_r >= l_explicit_part)
                        return true;
                }
            }
            else {
                for (int32_t pos = runend_pos; pos >= runstart_pos; pos--) {
                    const uint64_t current_slot = GetSlot(store, pos);
                    const uint64_t current_slot_r = current_slot | (current_slot - 1);
                    if (current_slot_r >= l_explicit_part)
                        return true;
                }
            }
        }
        return false;
    }

    // l_implicit_part == r_implicit_part
    if (!GetOccupiedBit(store, l_implicit_part))
        return false;
    const uint32_t rank = RankOccupieds(store, l_implicit_part);
    const int32_t runend_pos = SelectRunends(store, rank);
    const int32_t runstart_pos = std::max<int32_t>(rank ? SelectRunends(store, rank - 1) : -1,
                                                   FindEmptySlotBefore(store, runend_pos)) + 1;
    for (int32_t i = runstart_pos; i <= runend_pos; i++) {
        const uint64_t current_slot = GetSlot(store, i);
        const uint64_t current_slot_l = current_slot & (current_slot - 1);
        const uint64_t current_slot_r = current_slot | (current_slot - 1);
        if constexpr (diva_type == DivaType::BinaryTrie) {
            if (current_slot_l > r_explicit_part - 1)
                break;
            else if (SlotHasTrie(store, i, runend_pos)) {
                const Infix infix(store.ptr + num_metadata_offset_words,
                        infix_store_target_size + store_size + infix_size_ * i,
                        infix_size_);
                if (current_slot_r >= l_explicit_part && current_slot_l <= r_explicit_part - 1) {
                    const uint32_t max_key_len = (original_key_start_bit + infix_size_
                            + infix.num_trie_bits_ + infix.num_suffix_bits_) / 8;
                    const uint32_t mask_size = lowbit_pos(infix.infix_) + 1;
                    uint8_t zeros[max_key_len];
                    memset(zeros, 0x00, max_key_len);
                    InfiniteByteString trie_l_key = 
                        (l_explicit_part | BITMASK(mask_size)) < (current_slot | BITMASK(mask_size)) 
                            ? InfiniteByteString(zeros, max_key_len) : original_l_key;

                    uint8_t ones[max_key_len];
                    memset(ones, 0xFF, max_key_len);
                    InfiniteByteString trie_r_key = 
                        (current_slot | BITMASK(mask_size)) < (r_explicit_part | BITMASK(mask_size)) 
                            ? InfiniteByteString(ones, max_key_len) : original_r_key;

                    if (infix.QueryTrie(trie_l_key, trie_r_key,
                                original_key_start_bit + infix_size_ - mask_size,
                                infix_size_))
                        return true;
                }
                i += infix.GetNumSlots(infix_size_) - 1;
            }
            else if (current_slot_r >= l_explicit_part && current_slot_l <= r_explicit_part - 1)
                return true;
        }
        else {
            if (current_slot_r >= l_explicit_part && current_slot_l <= r_explicit_part - 1)
                return true;
            else if (current_slot_l > r_explicit_part - 1)
                break;
        }
    }
    return false;
}


template <DivaType diva_type, PayloadType payload_type>
inline bool Diva<diva_type, payload_type>::PointQueryInfixStore(InfixStore &store, const uint64_t key,
                                                                const uint32_t total_implicit,
                                                                const InfiniteByteString original_key,
                                                                const uint32_t original_key_start_bit) const {
    const uint64_t implicit_part = key >> infix_size_;
    const uint64_t explicit_part = key & BITMASK(infix_size_);
    const uint32_t size_grade = store.GetSizeGrade();

    if (!GetOccupiedBit(store, implicit_part))
        return false;

    const uint32_t rank = RankOccupieds(store, implicit_part);
    const int32_t runend_pos = SelectRunends(store, rank);
    if constexpr (diva_type == DivaType::BinaryTrie) {
        const int32_t runstart_pos = std::max<int32_t>(rank ? SelectRunends(store, rank - 1) : -1,
                                                       FindEmptySlotBefore(store, runend_pos)) + 1;
        for (int32_t i = runstart_pos; i <= runend_pos; i++) {
            const uint64_t slot_value = GetSlot(store, i);
            const uint64_t mask = ((slot_value & (-slot_value)) << 1) - 1;
            const uint32_t mask_size = lowbit_pos(slot_value) + 1;
            if (SlotHasTrie(store, i, runend_pos)) {
                const Infix infix_to_query(store.ptr + num_metadata_offset_words,
                        infix_store_target_size + scaled_sizes_[size_grade] + infix_size_ * i, infix_size_);
                if ((explicit_part | mask) == (slot_value | mask)) {
                    const uint32_t key_start_bit = original_key_start_bit + infix_size_ - mask_size;
                    if (infix_to_query.QueryTrie(original_key, original_key, key_start_bit, infix_size_))
                        return true;
                }
                i += infix_to_query.GetNumSlots(infix_size_) - 1;
            }
            else if ((explicit_part | mask) == (slot_value | mask))
                return true;
            if ((slot_value & (slot_value - 1)) > explicit_part - 1)
                break;
        }
    }
    else {
        int32_t pos = runend_pos;
        uint64_t slot_value = GetSlot(store, pos);
        do {
            const uint64_t mask = ((slot_value & (-slot_value)) << 1) - 1;
            if ((explicit_part | mask) == (slot_value | mask))
                return true;
            if (pos == 0)
                break;
            slot_value = GetSlot(store, --pos);
        } while (slot_value && !GetRunendBit(store, pos));
    }
    return false;
}


template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::ResizeInfixStore(InfixStore &store, const uint32_t total_implicit) {
    // TODO: Optimize further?
    uint32_t size_grade = store.GetSizeGrade();
    const uint64_t full_slot_count = store.GetFullSlotCount();
    const bool should_allocate_on_heap = full_slot_count > heap_alloc_threshold;

    uint32_t payload_list_size = 1;
    if constexpr (payload_type == PayloadType::FixedLength)
        payload_list_size = (payload_size_ * full_slot_count + 63) / 64 + 1;

    uint64_t infix_list_contents[should_allocate_on_heap ? 1 : full_slot_count];
    uint64_t payload_list_contents[should_allocate_on_heap ? 1 : payload_list_size];
    uint64_t *infix_list = infix_list_contents;
    uint64_t *payload_list = payload_list_contents;
    if (should_allocate_on_heap) {
        if constexpr (diva_type == DivaType::Standard)
            infix_list = new uint64_t[full_slot_count];
        if constexpr (payload_type == PayloadType::FixedLength)
            payload_list = new uint64_t[payload_list_size];
    }
    std::vector<Infix> infix_vec;

    if constexpr (diva_type == DivaType::BinaryTrie)
        infix_vec = GetInfixVector(store, payload_list);
    else {
        const uint32_t num_infixes = GetInfixList(store, infix_list, payload_list);
#ifdef DEBUG
        assert(num_infixes == full_slot_count);
#endif // DEBUG
    }

    // Backup the pointer to the sample payload list, if necessary
    const uint64_t *sample_payload_list = reinterpret_cast<const uint64_t *>(store.ptr[1]);
    delete[] store.ptr;

    // Update `size_grade`
    if (full_slot_count >= (size_grade ? scaled_sizes_[size_grade - 1] : exception_scaled_size_)) {
        while (size_grade < size_scalar_count - 1 
                && full_slot_count >= (size_grade ? scaled_sizes_[size_grade - 1] : exception_scaled_size_))
            size_grade++;
    }
    else {
        while (size_grade > 0
                && full_slot_count <= (size_grade > 1 ? scaled_sizes_[size_grade - 2] : exception_scaled_size_))
            size_grade--;
    }

    store.SetSizeGrade(size_grade);
    const uint32_t next_size = scaled_sizes_[size_grade];
    const uint64_t word_count = InfixStore::GetPtrWordCount(next_size,
            infix_size_, payload_size_);
    store.ptr = new uint64_t[word_count];
    if constexpr (diva_type == DivaType::BinaryTrie)
        LoadVectorToInfixStore(store, infix_vec, total_implicit, true, payload_list);
    else 
        LoadListToInfixStore(store, infix_list, full_slot_count, total_implicit, true, payload_list);
    if constexpr (payload_type == PayloadType::FixedLength)
        store.ptr[1] = reinterpret_cast<uint64_t>(sample_payload_list);

    if (should_allocate_on_heap) {
        if constexpr (diva_type == DivaType::Standard)
            delete[] infix_list;
        if constexpr (payload_type == PayloadType::FixedLength)
            delete[] payload_list;
    }

#ifdef DEBUG
    {
        const uint32_t *store_popcnts = reinterpret_cast<const uint32_t *>(store.ptr);
        const uint64_t *occupieds = store.ptr + num_metadata_offset_words;
        const uint64_t *runends = store.ptr + num_metadata_offset_words + infix_store_target_size / 64;
        uint32_t occupied_count = 0, runend_count = 0, popcnts[2] = {};
        for (int32_t i = 0; i < infix_store_target_size / 64; i++) {
            if (i < infix_store_target_size / 128)
                popcnts[0] += __builtin_popcountll(occupieds[i]);
            occupied_count += __builtin_popcountll(occupieds[i]);
        }
        for (int32_t i = 0; i < scaled_sizes_[size_grade]; i++) {
            if (i < infix_store_target_size / 2)
                popcnts[1] += get_bitmap_bit(runends, i);
            runend_count += get_bitmap_bit(runends, i);
        }
        assert(occupied_count == runend_count);
        assert(store_popcnts[0] == popcnts[0]);
        assert(store_popcnts[1] == popcnts[1]);
    }

    GetInfixVector(store);
#endif // DEBUG
}


template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::LoadListToInfixStore(InfixStore &store, const uint64_t *list, const uint32_t list_len,
                                                                const uint32_t total_implicit, const bool zero_out,
                                                                const uint64_t *payload_list) {
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t total_size = scaled_sizes_[size_grade];
#ifdef DEBUG
    assert(total_implicit >= infix_store_target_size / 2);
#endif // DEBUG
    const uint64_t implicit_scalar = implicit_scalars_[total_implicit - infix_store_target_size / 2];

    if (zero_out)
        store.Reset(total_size, infix_size_, payload_size_);
    store.SetFullSlotCount(list_len);
    if (list_len == 0)
        return;

    int32_t l[infix_store_target_size + 1], r[infix_store_target_size + 1], ind = 0;
    // Make sure everything is in increasing order
#ifdef DEBUG
    for (int32_t i = 1; i < list_len; i++)
        assert((list[i - 1] >> infix_size_) <= (list[i] >> infix_size_));
    for (int32_t i = 0; i < list_len; i++)
        assert((list[i] >> infix_size_) < total_implicit);
    for (int32_t i = 1; i < list_len; i++)
        assert(list[i - 1] == list[i] || CompareInfixes(list[i - 1], list[i]));
#endif // DEBUG

    uint64_t *occupieds = store.ptr + num_metadata_offset_words;
    uint64_t *runends = store.ptr + num_metadata_offset_words + infix_store_target_size / 64;
    uint64_t old_implicit_part = list[0] >> infix_size_;
    l[0] = GetMappedPos(old_implicit_part, size_grade, implicit_scalar);
    r[0] = l[0];
    for (uint32_t i = 0; i < list_len; i++) {
        const uint64_t implicit_part = list[i] >> infix_size_;
        if (implicit_part != old_implicit_part) {
            ind++;
            l[ind] = std::max<int32_t>(r[ind - 1], GetMappedPos(implicit_part, size_grade, implicit_scalar));
            r[ind] = l[ind];
        }
        r[ind]++;
        old_implicit_part = implicit_part;
    }

    ind++;
    l[ind] = r[ind] = total_size;
    for (int32_t i = ind - 1; i >= 0; i--) {
        const int32_t diff = std::min(0, l[i + 1] - r[i]);
        l[i] += diff;
        r[i] += diff;
    }

    uint32_t write_head = 0;
    for (uint32_t i = 0; i < ind; i++) {
        for (uint32_t j = l[i]; j < r[i]; j++) {
            const uint64_t implicit_part = list[write_head] >> infix_size_;
#ifdef DEBUG
            assert(implicit_part < total_implicit);
#endif // DEBUG
            set_bitmap_bit(occupieds, implicit_part);
            const uint64_t explicit_part = list[write_head] & BITMASK(infix_size_);
            SetSlot(store, j, explicit_part);
            if constexpr (payload_type == PayloadType::FixedLength) {
                if (payload_list != nullptr) {
                    const uint32_t payload_offset = payload_size_ * write_head;
                    SetPayload(store, j, payload_list, payload_offset);
                }
            }
            write_head++;
        }
        set_bitmap_bit(runends, r[i] - 1);
    }

    uint32_t *popcnts = reinterpret_cast<uint32_t *>(store.ptr);
    for (int32_t i = 0; i < infix_store_target_size / 128; i++) {
        popcnts[0] += __builtin_popcountll(occupieds[i]);
        if (static_cast<int32_t>(scaled_sizes_[size_grade]) - i * 64 > 0) {
            const uint64_t mask = BITMASK(std::min(64UL, scaled_sizes_[size_grade] - i * 64));
            popcnts[1] += __builtin_popcountll(runends[i] & mask);
        }
    }

#ifdef DEBUG
    {
        uint32_t occupied_count = 0, runend_count = 0;
        for (int32_t i = 0; i < infix_store_target_size / 64; i++)
            occupied_count += __builtin_popcountll(occupieds[i]);
        for (int32_t i = 0; i < scaled_sizes_[size_grade]; i++)
            runend_count += get_bitmap_bit(runends, i);
        assert(occupied_count == runend_count);
        uint64_t retrieved_infix_list[2 * list_len];
        uint64_t retrieved_payload_list[2 * list_len * (payload_size_ + 63) / 64];
        assert(list_len == GetInfixList(store, retrieved_infix_list, retrieved_payload_list));
        for (uint32_t i = 0; i < list_len; i++)
            assert(retrieved_infix_list[i] == list[i]);
    }
#endif // DEBUG
}


template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::LoadVectorToInfixStore(InfixStore &store, const std::vector<Infix>& vec,
                                                                  const uint32_t total_implicit, const bool zero_out,
                                                                  const uint64_t *payload_list) {
#ifdef DEBUG
    assert(total_implicit >= infix_store_target_size / 2);
#endif // DEBUG

    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t total_size = scaled_sizes_[size_grade];
    const uint64_t implicit_scalar = implicit_scalars_[total_implicit - infix_store_target_size / 2];
    if (zero_out)
        store.Reset(total_size, infix_size_, payload_size_);
    if (vec.empty())
        return;

    // Make sure everything is in increasing order
#ifdef DEBUG
    for (int32_t i = 1; i < vec.size(); i++)
        assert(vec[i - 1] <= vec[i]);
    for (int32_t i = 0; i < vec.size(); i++)
        assert((vec[i].infix_ >> infix_size_) < total_implicit);
#endif // DEBUG

    std::vector<int32_t> l, r;
    l.reserve(vec.size());
    r.reserve(vec.size());
    uint64_t *occupieds = store.ptr + num_metadata_offset_words;
    uint64_t *runends = store.ptr + num_metadata_offset_words + infix_store_target_size / 64;
    uint64_t old_implicit_part = vec[0].infix_ >> infix_size_;
    l.push_back(GetMappedPos(old_implicit_part, size_grade, implicit_scalar));
    r.push_back(l.back());
    for (uint32_t i = 0; i < vec.size(); i++) {
        const uint64_t implicit_part = vec[i].infix_ >> infix_size_;
        if (implicit_part != old_implicit_part) {
            l.push_back(std::max<int32_t>(r.back(), GetMappedPos(implicit_part, size_grade, implicit_scalar)));
            r.push_back(l.back());
        }
        r.back() += vec[i].GetNumSlots(infix_size_);
        old_implicit_part = implicit_part;
    }

    // Fix positions to fit in the Infix Store, calculate the number of full slots
    l.push_back(total_size);
    r.push_back(total_size);
    uint32_t slot_full_count = 0;
    for (int32_t i = l.size() - 2; i >= 0; i--) {
        const int32_t diff = std::min(0, l[i + 1] - r[i]);
        l[i] += diff;
        r[i] += diff;
        slot_full_count += r[i] - l[i];
    }
    store.SetFullSlotCount(slot_full_count);

    uint32_t write_head = 0, payload_write_head = 0;
    for (uint32_t i = 0; i < l.size() - 1; i++) {
        for (uint32_t j = l[i]; j < r[i]; j += vec[write_head++].GetNumSlots(infix_size_)) {
            const uint64_t implicit_part = vec[write_head].infix_ >> infix_size_;
#ifdef DEBUG
            assert(implicit_part < total_implicit);
#endif // DEBUG
            set_bitmap_bit(occupieds, implicit_part);
            vec[write_head].SerializeToInfixStore(store, j, scaled_sizes_[size_grade], infix_size_);
            if constexpr (payload_type == PayloadType::FixedLength) {
                if (payload_list != nullptr) {
                    const uint32_t num_payloads = vec[write_head].num_trie_bits_ == 0 ? 1
                                                : vec[write_head].GetNumPrefixKeys() + vec[write_head].num_suffixes_;
                    for (uint32_t k = 0; k < num_payloads; k++) {
                        const uint32_t payload_offset = payload_size_ * payload_write_head;
                        SetPayload(store, j + k, payload_list, payload_offset);
                    }
                    payload_write_head += num_payloads;
                }
            }
        }
        set_bitmap_bit(runends, r[i] - 1);
    }
    UpdatePopcnts(store);

#ifdef DEBUG
    {
        const uint32_t *store_popcnts = reinterpret_cast<const uint32_t *>(store.ptr);
        const uint64_t *occupieds = store.ptr + num_metadata_offset_words;
        const uint64_t *runends = store.ptr + num_metadata_offset_words + infix_store_target_size / 64;
        uint32_t occupied_count = 0, runend_count = 0, popcnts[2] = {};
        for (int32_t i = 0; i < infix_store_target_size / 64; i++) {
            if (i < infix_store_target_size / 128)
                popcnts[0] += __builtin_popcountll(occupieds[i]);
            occupied_count += __builtin_popcountll(occupieds[i]);
        }
        for (int32_t i = 0; i < scaled_sizes_[size_grade]; i++) {
            if (i < infix_store_target_size / 2)
                popcnts[1] += get_bitmap_bit(runends, i);
            runend_count += get_bitmap_bit(runends, i);
        }
        assert(occupied_count == runend_count);
        assert(store_popcnts[0] == popcnts[0]);
        assert(store_popcnts[1] == popcnts[1]);
    }
#endif // DEBUG
}


template <DivaType diva_type, PayloadType payload_type>
inline typename Diva<diva_type, payload_type>::InfixStore Diva<diva_type, payload_type>::AllocateInfixStoreWithList(const uint64_t *list,
                                                                                                                    const uint32_t list_len,
                                                                                                                    const uint32_t total_implicit,
                                                                                                                    const uint64_t *payload_list) {
    const uint32_t scaled_len = (size_scalars_[size_scalar_shrink_grow_sep] * list_len) >> scale_shift;
    uint32_t size_grade;
    for (size_grade = 0; size_grade < size_scalar_count && scaled_sizes_[size_grade] < scaled_len; size_grade++);
    if constexpr (payload_type == PayloadType::FixedLength) {
        InfixStore res(scaled_sizes_[size_grade], infix_size_, size_grade, payload_size_);
        LoadListToInfixStore(res, list, list_len, total_implicit, true, payload_list);
        return res;
    }
    else {
        InfixStore res(scaled_sizes_[size_grade], infix_size_, size_grade);
        LoadListToInfixStore(res, list, list_len, total_implicit);
        return res;
    }
}


template <DivaType diva_type, PayloadType payload_type>
inline typename Diva<diva_type, payload_type>::InfixStore Diva<diva_type, payload_type>::AllocateInfixStoreWithVector(const std::vector<Infix>& vec,
                                                                                                                      const uint32_t total_implicit,
                                                                                                                      const uint64_t *payload_list) {
    uint32_t num_full_slots = 0;
    for (const Infix& infix : vec)
        num_full_slots += infix.GetNumSlots(infix_size_);
    const uint32_t scaled_num_full_slots = (size_scalars_[size_scalar_shrink_grow_sep] * num_full_slots) >> scale_shift;
    uint32_t size_grade;
    for (size_grade = 0; size_grade < size_scalar_count && scaled_sizes_[size_grade] < scaled_num_full_slots; size_grade++);
    InfixStore res(scaled_sizes_[size_grade], infix_size_, size_grade, payload_size_);
    LoadVectorToInfixStore(res, vec, total_implicit, true, payload_list);
    return res;
}


template <DivaType diva_type, PayloadType payload_type>
inline uint32_t Diva<diva_type, payload_type>::GetInfixList(const InfixStore &store, uint64_t *res,
                                                            uint64_t *res_payload) const {
#ifdef DEBUG
    if constexpr (payload_type == PayloadType::FixedLength)
        assert(res_payload != nullptr);
#endif // DEBUG
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t store_size = scaled_sizes_[size_grade];
    uint64_t implicit_part = GetOccupiedBit(store, 0) ? 0 : NextOccupied(store, 0);
    uint32_t ind = 0;
    for (int32_t i = 0; i < store_size; i++) {
        const uint64_t explicit_part = GetSlot(store, i);
        if (explicit_part) {
#ifdef DEBUG
            assert(implicit_part < infix_store_target_size);
#endif // DEBUG
            res[ind] = (implicit_part << infix_size_) | explicit_part;
            if constexpr (payload_type == PayloadType::FixedLength) {
                const uint32_t slot_and_bitmap_size = 64 * num_metadata_offset_words + infix_store_target_size 
                                                    + store_size * (infix_size_ + 1);
                const uint32_t pos_in = slot_and_bitmap_size + payload_size_ * i;
                const uint32_t pos_out = payload_size_ * ind;
                copy_bitmap_to_bitmap(store.ptr, pos_in, res_payload, pos_out, payload_size_);
            }
            ind++;
        }
        if (GetRunendBit(store, i))
            implicit_part = NextOccupied(store, implicit_part);
    }

    return ind;
}


template <DivaType diva_type, PayloadType payload_type>
inline std::vector<typename Diva<diva_type, payload_type>::Infix>
Diva<diva_type, payload_type>::GetInfixVector(const InfixStore &store, uint64_t *res_payload) const {
#ifdef DEBUG
    if constexpr (payload_type == PayloadType::FixedLength)
        throw std::runtime_error("Payloads are not yet implemented to work alongside the binary trie");
#endif // DEBUG
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t store_size = scaled_sizes_[size_grade];
    int32_t implicit_part = GetOccupiedBit(store, 0) ? 0 : NextOccupied(store, 0);
    int32_t runend_pos = SelectRunends(store, 0);
    std::vector<Infix> res;
    res.reserve(store.GetFullSlotCount());
    for (int32_t i = 0; i < store_size; i++) {
        const uint64_t explicit_part = GetSlot(store, i);
        if (explicit_part) {
#ifdef DEBUG
            assert(implicit_part < infix_store_target_size);
#endif // DEBUG
            res.emplace_back((implicit_part << infix_size_) | explicit_part);
            if constexpr (diva_type == DivaType::BinaryTrie) {
                if (SlotHasTrie(store, i, runend_pos)) {
                    res.back() = Infix(store.ptr + num_metadata_offset_words, 
                                       infix_store_target_size + scaled_sizes_[size_grade] + infix_size_ * i,
                                       infix_size_);
                    res.back().infix_ |= (implicit_part << infix_size_);
                    i += res.back().GetNumSlots(infix_size_) - 1;
                }
            }
            if constexpr (payload_type == PayloadType::FixedLength) {
                const uint32_t slot_and_bitmap_size = 64 * num_metadata_offset_words + infix_store_target_size 
                                                    + store_size * (infix_size_ + 1);
                const uint32_t pos_in = slot_and_bitmap_size + payload_size_ * i;
                const uint32_t pos_out = payload_size_ * (res.size() - 1);
                copy_bitmap_to_bitmap(store.ptr, pos_in, res_payload, pos_out, payload_size_);
            }
        }
        if (GetRunendBit(store, i)) {
            implicit_part = NextOccupied(store, implicit_part);
            runend_pos = NextRunend(store, runend_pos);
        }
    }

#ifdef DEBUG
    for (int32_t i = 1; i < res.size(); i++)
        assert(res[i - 1] <= res[i]);
#endif // DEBUG

    return std::move(res);
}


/*****************************************************************************
 **                                 Iterator                                **
 *****************************************************************************/
template <DivaType diva_type, PayloadType payload_type>
inline Diva<diva_type, payload_type>::Iterator::Iterator(Diva<diva_type, payload_type> *parent,
                                                         std::string_view start, std::string_view end,
                                                         std::function<bool(const uint64_t *)> should_remove):
        filter_(parent),
        should_remove_(should_remove),
        first_store_to_fetched_and_delete_(should_remove != nullptr) {
    if (start.size() == 0) {
        memset(next_to_fetch_contents_, 0, sizeof(next_to_fetch_contents_));
        next_to_fetch_ = {next_to_fetch_contents_, iterator_local_buf_len - 1};
    }
    else
        SetNextToFetch(reinterpret_cast<const uint8_t *>(start.data()), start.size());
    SetEnd(reinterpret_cast<const uint8_t *>(end.data()), end.size());
    if constexpr (payload_type == PayloadType::FixedLength) {
        if (should_remove_) {
            FetchDelete();
            return;
        }
    }
    Fetch();
}


template <DivaType diva_type, PayloadType payload_type>
inline Diva<diva_type, payload_type>::Iterator::Iterator(Diva<diva_type, payload_type> *parent,
                                                         const uint8_t *start, uint32_t start_len,
                                                         const uint8_t *end, uint32_t end_len,
                                                         std::function<bool(const uint64_t *)> should_remove):
        filter_(parent),
        should_remove_(should_remove),
        first_store_to_fetched_and_delete_(should_remove != nullptr) {
    if (start == nullptr || start_len == 0) {
        memset(next_to_fetch_contents_, 0, sizeof(next_to_fetch_contents_));
        next_to_fetch_ = {next_to_fetch_contents_, iterator_local_buf_len - 1};
    }
    else
        SetNextToFetch(start, start_len);
    SetEnd(end, end_len);
    if constexpr (payload_type == PayloadType::FixedLength) {
        if (should_remove_) {
            FetchDelete();
            return;
        }
    }
    Fetch();
}


template <DivaType diva_type, PayloadType payload_type>
inline Diva<diva_type, payload_type>::Iterator::Iterator(Diva<diva_type, payload_type> *filter,
                                                         uint64_t start, uint64_t end, 
                                                         std::function<bool(const uint64_t *)> should_remove):
        filter_(filter),
        should_remove_(should_remove),
        first_store_to_fetched_and_delete_(should_remove != nullptr) {
    start = to_big_endian_order(start);
    SetNextToFetch(reinterpret_cast<const uint8_t *>(&start), sizeof(start));
    if (end == std::numeric_limits<uint64_t>::max())
        SetEnd(nullptr, 0);
    else 
        SetEnd(reinterpret_cast<const uint8_t *>(&end), sizeof(end));
    if constexpr (payload_type == PayloadType::FixedLength) {
        if (should_remove_) {
            FetchDelete();
            return;
        }
    }
    Fetch();
}


template <DivaType diva_type, PayloadType payload_type>
inline typename Diva<diva_type, payload_type>::Iterator& Diva<diva_type, payload_type>::Iterator::operator=(const Iterator &other) {
    assert(filter_ == other.filter_);

    SetNextToFetch(other.next_to_fetch_.str, other.next_to_fetch_.length);
    SetEnd(other.end_key_.str, other.end_key_.length);
    shared_ = other.shared_;
    ignore_ = other.ignore_;
    implicit_ = other.implicit_;
    infixes_ = other.infixes_;
    bit_counts_ = other.bit_counts_;
    payloads_ = other.payloads_;
    ind_ = other.ind_;
    should_remove_ = other.should_remove_;
    first_store_to_fetched_and_delete_ = other.first_store_to_fetched_and_delete_;
    memcpy(shared_prefix_, other.shared_prefix_, (shared_ + 7) / 8);
    memcpy(current_key_contents_, other.current_key_contents_,
            (shared_ + ignore_ + implicit_ + filter_->infix_size_ + 6) / 8);
    return *this;
}


template <DivaType diva_type, PayloadType payload_type>
inline Diva<diva_type, payload_type>::Iterator::Iterator(const Iterator& other):
        filter_(other.filter_),
        shared_(other.shared_),
        ignore_(other.ignore_),
        implicit_(other.implicit_),
        infixes_(other.infixes_),
        bit_counts_(other.bit_counts_),
        payloads_(other.payloads_),
        ind_(other.ind_),
        should_remove_(other.should_remove_),
        first_store_to_fetched_and_delete_(other.first_store_to_fetched_and_delete_){
    SetNextToFetch(other.next_to_fetch_.str, other.next_to_fetch_.length);
    SetEnd(other.end_key_.str, other.end_key_.length);
    memcpy(shared_prefix_, other.shared_prefix_, (shared_ + 7) / 8);
    memcpy(current_key_contents_, other.current_key_contents_,
            (shared_ + ignore_ + implicit_ + filter_->infix_size_ + 6) / 8);
}


template <DivaType diva_type, PayloadType payload_type>
inline typename Diva<diva_type, payload_type>::Iterator& Diva<diva_type, payload_type>::Iterator::operator++() {
    if constexpr (payload_type == PayloadType::FixedLength) {
        if (should_remove_) {
            FetchDelete();
            return *this;
        }
    }
    ind_++;
    if (ind_ >= infixes_.size() && next_to_fetch_.str != nullptr)
        Fetch();
    return *this;
}


template <DivaType diva_type, PayloadType payload_type>
inline typename Diva<diva_type, payload_type>::Iterator Diva<diva_type, payload_type>::Iterator::operator++(int) {
    auto old = *this;
    operator++();
    return old;
}


template <DivaType diva_type, PayloadType payload_type>
inline std::pair<typename Diva<diva_type, payload_type>::Iterator::KeyType, uint32_t> Diva<diva_type, payload_type>::Iterator::operator*() {
    if (!IsValid()) {
        if constexpr (diva_type == DivaType::Int)
            return {0, 0};
        else 
            return {nullptr, 0};
    }
    assert(ind_ < infixes_.size());

    // Reconstruct the known prefix of the key
    const uint32_t key_length = (bit_counts_[ind_] + 7) / 8;
    memset(current_key_contents_, 0, key_length);
    uint64_t infix = infixes_[ind_];
    if (infix) {
        memcpy(current_key_contents_, shared_prefix_, (shared_ + 7) / 8);
        current_key_contents_[shared_ / 8] &= BITMASK(shared_ % 8) << (8 - shared_ % 8);
        const uint32_t extraction_size = implicit_ + filter_->infix_size_;
        const uint32_t explicit_part_length = filter_->infix_size_ - lowbit_pos(infix) - 1;
        if (infix >> (extraction_size - 1))
            current_key_contents_[shared_ / 8] |= 1ULL << (7 - shared_ % 8);
        else if (ignore_ > 0) {
            uint32_t bit_pos = shared_ + 1;
            uint32_t pos_rem = 8 - bit_pos % 8;
            if (pos_rem < ignore_) {
                current_key_contents_[bit_pos / 8] |= BITMASK(pos_rem);
                uint32_t tmp_ignore = ignore_ - pos_rem;
                bit_pos += pos_rem;
                if (tmp_ignore >= 8) {
                    memset(current_key_contents_ + bit_pos / 8, 0xFF, tmp_ignore / 8);
                    bit_pos += tmp_ignore - tmp_ignore % 8;
                    tmp_ignore %= 8;
                }
                current_key_contents_[bit_pos / 8] |= BITMASK(tmp_ignore) << (8 - tmp_ignore);
            }
            else
                current_key_contents_[bit_pos / 8] |= BITMASK(ignore_) << (8 - bit_pos % 8 - ignore_);
        }
        uint32_t bit_pos = shared_ + ignore_ + 1;
        infix <<= 65 - extraction_size;
        infix = __builtin_bswap64(infix >> (bit_pos % 8));
        const uint32_t loop_end_i = (bit_pos % 8 + implicit_ + explicit_part_length - 1 + 7) / 8;
        for (uint32_t i = 0; i < loop_end_i; i++) {
            current_key_contents_[bit_pos / 8] |= infix & 0xFF;
            infix >>= 8;
            bit_pos += 8 - bit_pos % 8;
        }
    }
    else 
        memcpy(current_key_contents_, shared_prefix_, key_length);

    // Return the reconstructed key
    if constexpr (diva_type == DivaType::Int) {
        uint64_t res = 0;
        memcpy(&res, current_key_contents_, key_length);
        return {__builtin_bswap64(res), bit_counts_[ind_]};
    }
    else {
        return {std::string(reinterpret_cast<const char *>(current_key_contents_), key_length),
                bit_counts_[ind_]};
    }
}


template <DivaType diva_type, PayloadType payload_type>
inline bool Diva<diva_type, payload_type>::Iterator::operator==(const Iterator& rhs) const {
    if (filter_ != rhs.filter_ || infixes_ != rhs.infixes_)
        return false;
    if (ind_ == rhs.ind_) {
        auto [fetch_a, bit_count_a] = *this;
        auto [fetch_b, bit_count_b] = *rhs;
        return fetch_a == fetch_b && bit_count_a == bit_count_b;
    }
    return false;
}


template <DivaType diva_type, PayloadType payload_type>
inline bool Diva<diva_type, payload_type>::Iterator::operator!=(const Iterator& rhs) const {
    return !(*this == rhs);
}


template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::Iterator::GetPayload(uint64_t *out) const {
    static_assert(payload_type == PayloadType::FixedLength);
    assert(ind_ < infixes_.size());
    copy_bitmap_to_bitmap(payloads_.data(), filter_->payload_size_ * ind_,
                          out, 0,
                          filter_->payload_size_);
}


template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::Iterator::SetDeleteFunction(std::function<bool(const uint64_t *)> should_remove) {
    should_remove_ = should_remove;
    first_store_to_fetched_and_delete_ = (should_remove != nullptr);
}


template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::Iterator::SetNextToFetchFromExtraction(const InfiniteByteString key, uint64_t extraction) {
    const uint32_t extraction_size = implicit_ + filter_->infix_size_;
    const uint32_t key_length_bits = shared_ + ignore_ + extraction_size;
    const uint32_t key_length = (key_length_bits + 7) / 8;

    memset(next_to_fetch_contents_, 0, key_length);
    memcpy(next_to_fetch_contents_, key.str, (shared_ + 7) / 8);
    next_to_fetch_contents_[shared_ / 8] &= BITMASK(shared_ % 8) << (8 - shared_ % 8);
    if (extraction >> (extraction_size - 1))
        next_to_fetch_contents_[shared_ / 8] |= 1ULL << (7 - shared_ % 8);
    else if (ignore_ > 0) {
        uint32_t bit_pos = shared_ + 1;
        uint32_t pos_rem = 8 - bit_pos % 8;
        if (pos_rem < ignore_) {
            next_to_fetch_contents_[bit_pos / 8] |= BITMASK(pos_rem);
            uint32_t tmp_ignore = ignore_ - pos_rem;
            bit_pos += pos_rem;
            if (tmp_ignore >= 8) {
                memset(next_to_fetch_contents_ + bit_pos / 8, 0xFF, tmp_ignore / 8);
                bit_pos += tmp_ignore - tmp_ignore % 8;
                tmp_ignore %= 8;
            }
            next_to_fetch_contents_[bit_pos / 8] |= BITMASK(tmp_ignore) << (8 - tmp_ignore);
        }
        else
            next_to_fetch_contents_[bit_pos / 8] |= BITMASK(ignore_) << (8 - bit_pos % 8 - ignore_);
    }
    uint32_t bit_pos = shared_ + ignore_ + 1;
    uint64_t infix = extraction << (65 - extraction_size);
    infix = __builtin_bswap64(infix >> (bit_pos % 8));
    const uint32_t loop_end_i = (bit_pos % 8 + extraction_size - 1 + 7) / 8;
    for (uint32_t i = 0; i < loop_end_i; i++) {
        next_to_fetch_contents_[bit_pos / 8] |= infix & 0xFF;
        infix >>= 8;
        bit_pos += 8 - bit_pos % 8;
    }
    SetNextToFetch(next_to_fetch_contents_, key_length);
}

template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::Iterator::Fetch() {
    ind_ = 0;
    infixes_.clear();
    infixes_.reserve(64);
    bit_counts_.clear();
    bit_counts_.reserve(64);
    if constexpr (payload_type == PayloadType::FixedLength) {
        payloads_.clear();
        payloads_.reserve(2 * filter_->payload_size_);
    }

    const bool it_write_lock = false;
    
IteratorRefetchLowerUpperBounds:
    InfixStore *infix_store_ptr;
    void *leaves_to_unlock[3] = {};
    InfiniteByteString next_key {};
    InfiniteByteString prev_key {};
    wormhole_int_iter it_int;
    wormhole_iter it;
    assert(next_to_fetch_.str);
    filter_->GetLowerUpperBounds(next_to_fetch_, it_write_lock, leaves_to_unlock, it, it_int,
                                 prev_key, next_key, infix_store_ptr);
    if (next_key.str == nullptr) {
        filter_->UnlockLeaves(leaves_to_unlock, it_write_lock);
        SetNextToFetch(nullptr, 0);
        return;
    }

    uint64_t prev_key_word, next_key_word;
    if constexpr (diva_type == DivaType::Int) {
        prev_key_word = *reinterpret_cast<const uint64_t *>(prev_key.str);
        prev_key.str = reinterpret_cast<const uint8_t *>(&prev_key_word);
        next_key_word = *reinterpret_cast<const uint64_t *>(next_key.str);
        next_key.str = reinterpret_cast<const uint8_t *>(&next_key_word);
    }

    InfixStore& infix_store = *infix_store_ptr;
    rwlock_lock_read(infix_store.rwlock);
    filter_->UnlockLeaves(leaves_to_unlock, it_write_lock);

    if (next_to_fetch_ <= prev_key) {
        // Previous key was a partial key and a prefix of the query key
        if (end_key_.str != nullptr && prev_key > end_key_) {
            SetNextToFetch(nullptr, 0);
            rwlock_unlock_read(infix_store.rwlock);
            return;
        }
        if constexpr (payload_type == PayloadType::FixedLength) {
            for (uint32_t i = 0; i < infix_store.num_sample_payloads; i++) {
                infixes_.push_back(0);
                bit_counts_.push_back(8 * prev_key.length);
                payloads_.resize((filter_->payload_size_ * infixes_.size() + 63) / 64 + 1);
                filter_->GetSamplePayload(infix_store, i, payloads_.data(), filter_->payload_size_ * (infixes_.size() - 1));
            }
        }
        else {
            infixes_.push_back(0);
            bit_counts_.push_back(8 * prev_key.length);
        }
    }

    auto [shared, ignore, implicit_size] = filter_->GetSharedIgnoreImplicitLengths(prev_key, next_key);
    const uint64_t prev_implicit = filter_->ExtractPartialKey(prev_key, shared, ignore, implicit_size, 0) >> filter_->infix_size_;
    const uint64_t next_implicit = filter_->ExtractPartialKey(next_key, shared, ignore, implicit_size, 1) >> filter_->infix_size_;
    const uint64_t total_implicit = next_implicit - prev_implicit + 1;
    const uint64_t extraction_l = filter_->ExtractPartialKey(next_to_fetch_, shared, ignore, implicit_size, next_to_fetch_.GetBit(shared))
                                         - (prev_implicit << filter_->infix_size_);
    const uint64_t extraction_r = end_key_.str == nullptr || next_key <= end_key_ ? std::numeric_limits<uint64_t>::max()
                                : filter_->ExtractPartialKey(end_key_, shared, ignore, implicit_size, end_key_.GetBit(shared)) 
                                         - (prev_implicit << filter_->infix_size_);

    shared_ = shared;
    ignore_ = ignore;
    implicit_ = implicit_size;
    memcpy(shared_prefix_, prev_key.str, prev_key.length);

    // Get infixes and payloads from Infix Store
    uint64_t implicit_part_l = extraction_l >> filter_->infix_size_;
    uint64_t explicit_part_l = extraction_l & (BITMASK(filter_->infix_size_ - 1) << 1);
    uint64_t implicit_part_r = extraction_r >> filter_->infix_size_;
    uint64_t explicit_part_r = extraction_r & BITMASK(filter_->infix_size_);

    const uint64_t *occupieds = infix_store.ptr + num_metadata_offset_words;
    if (!get_bitmap_bit(occupieds, implicit_part_l)) {
        implicit_part_l = filter_->NextOccupied(infix_store, implicit_part_l);
        explicit_part_l = 0;
    }
    if (implicit_part_l > implicit_part_r) {    // Make sure the range is actually non-empty
        SetNextToFetch(nullptr, 0);
        rwlock_unlock_read(infix_store.rwlock);
        return;
    }

    if (implicit_part_l >= total_implicit) {    // Go to the next infix store if this one is done
        if (end_key_.str != nullptr && end_key_ < next_key) {
            SetNextToFetch(nullptr, 0);
            rwlock_unlock_read(infix_store.rwlock);
            return;
        }
        SetNextToFetch(next_key.str, next_key.length);
        rwlock_unlock_read(infix_store.rwlock);
        if (!infixes_.empty())
            return;
        goto IteratorRefetchLowerUpperBounds;
    }

    const int32_t rank = filter_->RankOccupieds(infix_store, implicit_part_l);
    const int32_t runend_pos = filter_->SelectRunends(infix_store, rank);
    const int32_t runstart_pos = std::max(rank ? static_cast<int32_t>(filter_->SelectRunends(infix_store, rank - 1)) : -1,
                                          static_cast<int32_t>(filter_->FindEmptySlotBefore(infix_store, runend_pos))) + 1;
    const uint64_t recovered_implicit = prev_implicit + implicit_part_l;
    explicit_part_r = implicit_part_l == implicit_part_r ? explicit_part_r 
                                                         : BITMASK(filter_->infix_size_);
    for (int32_t pos = runstart_pos; pos <= runend_pos; pos++) {
        const uint64_t slot_value = filter_->GetSlot(infix_store, pos);
#ifdef DEBUG
        assert(slot_value);
#endif // DEBUG
        const uint64_t slot_l = slot_value & (slot_value - 1);
        const uint64_t slot_r = slot_value | (slot_value - 1);
        if (slot_r >= explicit_part_l && slot_l <= explicit_part_r) {
            const uint32_t explicit_part_length = filter_->infix_size_ - lowbit_pos(slot_value) - 1;
#ifdef DEBUG
            assert(explicit_part_length <= filter_->infix_size_);
#endif // DEBUG
            infixes_.push_back((recovered_implicit << filter_->infix_size_) | slot_value);
            bit_counts_.push_back(shared + ignore + implicit_size + explicit_part_length);
            if constexpr (payload_type == PayloadType::FixedLength) {
                payloads_.resize((filter_->payload_size_ * infixes_.size() + 63) / 64 + 1);
                filter_->GetPayload(infix_store, pos, payloads_.data(), filter_->payload_size_ * (infixes_.size() - 1));
            }
        }
    }

    // Update `next_to_fetch_`
    implicit_part_l = filter_->NextOccupied(infix_store, implicit_part_l);
    if (implicit_part_l <= std::min(implicit_part_r, total_implicit - 1)) {
        const uint64_t recovered_extraction = (prev_implicit + implicit_part_l) << filter_->infix_size_;
        SetNextToFetchFromExtraction(prev_key, recovered_extraction);
    }
    else if (end_key_.str != nullptr && end_key_ < next_key)
        SetNextToFetch(nullptr, 0);
    else 
        SetNextToFetch(next_key.str, next_key.length);

    rwlock_unlock_read(infix_store.rwlock);     // Done with `infix_store`

    if (infixes_.size() == 0 && next_to_fetch_.str)
        goto IteratorRefetchLowerUpperBounds;
}


template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::Iterator::FetchDelete() {
    ind_ = 0;
    infixes_.clear();
    bit_counts_.clear();
    payloads_.clear();

    const bool it_write_lock = true;
    
    InfixStore *infix_store_ptr;
    void *leaves_to_unlock[3] = {};
    InfiniteByteString next_key {};
    InfiniteByteString prev_key {};
    wormhole_int_iter it_int;
    wormhole_iter it;
    assert(next_to_fetch_.str);
    filter_->GetLowerUpperBounds(next_to_fetch_, it_write_lock, leaves_to_unlock, it, it_int,
                                 prev_key, next_key, infix_store_ptr);
    if (next_key.str == nullptr) {
        filter_->UnlockLeaves(leaves_to_unlock, it_write_lock);
        SetNextToFetch(nullptr, 0);
        return;
    }

    uint64_t prev_key_word, next_key_word;
    if constexpr (diva_type == DivaType::Int) {
        prev_key_word = *reinterpret_cast<const uint64_t *>(prev_key.str);
        prev_key.str = reinterpret_cast<const uint8_t *>(&prev_key_word);
        next_key_word = *reinterpret_cast<const uint64_t *>(next_key.str);
        next_key.str = reinterpret_cast<const uint8_t *>(&next_key_word);
    }

    rwlock_lock_write(infix_store_ptr->rwlock);
    InfixStore& infix_store = *infix_store_ptr;
    filter_->UnlockLeaves(leaves_to_unlock, it_write_lock);
    if (next_to_fetch_ <= prev_key) {
        // Previous key was a partial key and a prefix of the query key
        if (end_key_.str != nullptr && prev_key > end_key_) {
            rwlock_unlock_write(infix_store.rwlock);
            SetNextToFetch(nullptr, 0);
            return;
        }
        if constexpr (payload_type == PayloadType::FixedLength) {
            const uint16_t orig_num_sample_payloads = infix_store.num_sample_payloads;
            uint64_t payload[(filter_->payload_size_ + 63) / 64 + 1];
            for (int32_t i = infix_store.num_sample_payloads - 1; i >= 0; i--) {
                filter_->GetSamplePayload(infix_store, i, payload);
                if (should_remove_(payload))
                    filter_->RemoveSamplePayload(infix_store, i);
            }
            filter_->n_keys_.fetch_sub(orig_num_sample_payloads - infix_store.num_sample_payloads);
            if (infix_store.num_sample_payloads == 0) {
                filter_->n_keys_.fetch_add(1);
                rwlock_unlock_write(infix_store.rwlock);
                filter_->DeleteMerge(prev_key);
                return;
            }
        }
        else {
            rwlock_unlock_write(infix_store.rwlock);
            filter_->DeleteMerge(prev_key);
            return;
        }
    }

    auto [shared, ignore, implicit_size] = filter_->GetSharedIgnoreImplicitLengths(prev_key, next_key);
    const uint64_t prev_implicit = filter_->ExtractPartialKey(prev_key, shared, ignore, implicit_size, 0) >> filter_->infix_size_;
    const uint64_t next_implicit = filter_->ExtractPartialKey(next_key, shared, ignore, implicit_size, 1) >> filter_->infix_size_;
    const uint64_t total_implicit = next_implicit - prev_implicit + 1;
    uint64_t extraction_l = (filter_->ExtractPartialKey(next_to_fetch_, shared, ignore, implicit_size, next_to_fetch_.GetBit(shared)) | 1ULL)
                                         - (prev_implicit << filter_->infix_size_);
    // Remove the extraction from the left endpoint to ensure that all relevant
    // keys at the beginning of the infix store are deleted.
    if (!first_store_to_fetched_and_delete_)
        extraction_l &= ~(BITMASK(filter_->infix_size_ - 1) << 1);
    uint64_t extraction_r = end_key_.str == nullptr || next_key <= end_key_ ? std::numeric_limits<uint64_t>::max()
                            : (filter_->ExtractPartialKey(end_key_, shared, ignore, implicit_size, end_key_.GetBit(shared)) | 1ULL)
                                         - (prev_implicit << filter_->infix_size_);

    shared_ = shared;
    ignore_ = ignore;
    implicit_ = implicit_size;

    const auto [new_implicit, deleted_count] = filter_->DeleteRawRangeFromInfixStore(infix_store,
                                                                                     extraction_l,
                                                                                     extraction_r,
                                                                                     total_implicit,
                                                                                     should_remove_);
    filter_->n_keys_.fetch_sub(deleted_count, std::memory_order_release);

    // Update `next_to_fetch_`
    bool check_resize = true;
    if (new_implicit <= std::min((extraction_r >> filter_->infix_size_), total_implicit - 1)) {
        const uint64_t recovered_extraction = (prev_implicit + new_implicit) << filter_->infix_size_;
        SetNextToFetchFromExtraction(prev_key, recovered_extraction);
        check_resize = false;
    }
    else if (end_key_.str != nullptr && end_key_ < next_key)
        SetNextToFetch(nullptr, 0);
    else {
        SetNextToFetch(next_key.str, next_key.length);
        first_store_to_fetched_and_delete_ = false;
    }

    if (check_resize) {     // Resize the infix store if it now contains too few infixes
        const uint32_t size_grade = infix_store.GetSizeGrade();
        if (size_grade) {
            const uint32_t threshold = (size_grade > 1 ? filter_->scaled_sizes_[size_grade - 2] 
                                                       : filter_->exception_scaled_size_);
            if (infix_store.GetFullSlotCount() <= threshold)
                filter_->ResizeInfixStore(infix_store, total_implicit);
        }
    }

    rwlock_unlock_write(infix_store.rwlock);
}


template <DivaType diva_type, PayloadType payload_type>
inline typename Diva<diva_type, payload_type>::Iterator Diva<diva_type, payload_type>::GetIterator(std::string_view start,
                                                                                                   std::string_view end,
                                                                                                   std::function<bool(const uint64_t *)> should_remove) {
    return Iterator(this, start, end, should_remove);
}


template <DivaType diva_type, PayloadType payload_type>
inline typename Diva<diva_type, payload_type>::Iterator Diva<diva_type, payload_type>::GetIterator(const uint8_t *start, const uint32_t start_len,
                                                                                                   const uint8_t *end, const uint32_t end_len,
                                                                                                   std::function<bool(const uint64_t *)> should_remove) {
    return Iterator(this, start, start_len, end, end_len, should_remove);
}


template <DivaType diva_type, PayloadType payload_type>
inline typename Diva<diva_type, payload_type>::Iterator Diva<diva_type, payload_type>::GetIterator(uint64_t start, uint64_t end,
                                                                                                   std::function<bool(const uint64_t *)> should_remove) {
    return Iterator(this, start, end, should_remove);
}

template <DivaType diva_type, PayloadType payload_type>
inline Diva<diva_type, payload_type>::Iterator::~Iterator() {
    infixes_.clear();
    bit_counts_.clear();
    payloads_.clear();
}


template <DivaType diva_type, PayloadType payload_type>
inline bool Diva<diva_type, payload_type>::Iterator::IsValid() const {
    return ind_ < infixes_.size() || next_to_fetch_.str;
}


/*****************************************************************************
 **                                  Infix                                  **
 *****************************************************************************/
template <DivaType diva_type, PayloadType payload_type>
inline Diva<diva_type, payload_type>::Infix::Infix(uint64_t *ptr, 
                                                   uint32_t bit_pos,
                                                   uint32_t slot_size) {
    DeserializeFromPtr(ptr, bit_pos, slot_size);
}

template <DivaType diva_type, PayloadType payload_type>
inline Diva<diva_type, payload_type>::Infix::Infix(const Infix& other):
        infix_(other.infix_),
        num_suffixes_(other.num_suffixes_),
        num_suffix_bits_(other.num_suffix_bits_),
        num_prefix_keys_(other.num_prefix_keys_),
        num_trie_bits_(other.num_trie_bits_) {
    trie_ = other.trie_;
    trie_suffixes_ = other.trie_suffixes_;
}


template <DivaType diva_type, PayloadType payload_type>
inline typename Diva<diva_type, payload_type>::Infix&
Diva<diva_type, payload_type>::Infix::operator=(const Infix& other) {
    infix_ = other.infix_;
    num_suffixes_ = other.num_suffixes_;
    num_suffix_bits_ = other.num_suffix_bits_;
    num_prefix_keys_ = other.num_prefix_keys_;
    num_trie_bits_ = other.num_trie_bits_;
    trie_ = other.trie_;
    trie_suffixes_ = other.trie_suffixes_;
    return *this;
}


template <DivaType diva_type, PayloadType payload_type>
inline bool Diva<diva_type, payload_type>::Infix::operator==(const Infix& rhs) const {
    if (infix_ != rhs.infix_)
        return false;
    if (num_suffixes_ != rhs.num_suffixes_)
        return false;
    if (num_suffix_bits_ != rhs.num_suffix_bits_)
        return false;
    if (num_prefix_keys_ != rhs.num_prefix_keys_)
        return false;
    if (num_trie_bits_ != rhs.num_trie_bits_)
        return false;
    const uint32_t trie_size_bytes = sizeof(uint64_t) * ((num_trie_bits_ + 63) / 64);
    const uint32_t trie_suffixes_size_bytes = sizeof(uint64_t) * ((num_suffix_bits_ + 63) / 64);
    return (memcmp(trie_.data(), rhs.trie_.data(), trie_size_bytes) == 0) 
        && (memcmp(trie_suffixes_.data(), rhs.trie_suffixes_.data(), trie_suffixes_size_bytes) == 0);
}


template <DivaType diva_type, PayloadType payload_type>
inline bool Diva<diva_type, payload_type>::Infix::HasPrefixKeys() const {
    return num_prefix_keys_ > 0;
}

template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::Infix::SetHasPrefixKeys(bool has_prefix_keys) {
    num_prefix_keys_ = has_prefix_keys;
}

template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::Infix::UpdateNumPrefixKeys(int64_t delta) {
    num_prefix_keys_ += delta;
}

template <DivaType diva_type, PayloadType payload_type>
inline uint32_t Diva<diva_type, payload_type>::Infix::GetNumPrefixKeys() const {
    return num_prefix_keys_ - (num_prefix_keys_ > 0);
}

template <DivaType diva_type, PayloadType payload_type>
inline int32_t Diva<diva_type, payload_type>::Infix::CompareStringToBitmap(const void *bitmap, uint32_t bitmap_pos,
                                                                           const InfiniteByteString str, uint32_t str_pos,
                                                                           uint32_t num_bits_to_compare) const {
    int64_t match_len = 0;
    const uint32_t bitmap_pos_start = bitmap_pos;
    bitmap_pos += num_bits_to_compare;
    while (bitmap_pos > bitmap_pos_start) {
        const uint32_t bit_count_to_compare = std::min(bitmap_pos - bitmap_pos_start,
                                                       64 - str_pos % 64);
        bitmap_pos -= bit_count_to_compare;

        uint64_t data_bitmap_buf = 0;
        uint32_t data_bitmap_buf_filled_bits = 0, read_bitmap_pos = bitmap_pos;
        uint64_t data_bitmap = read_data_from_bitmap(bitmap, read_bitmap_pos,
                data_bitmap_buf, data_bitmap_buf_filled_bits,
                bit_count_to_compare);
        uint64_t data_str = str.BitsAtBitLength(str_pos, bit_count_to_compare);

        const uint64_t data_diff = data_bitmap ^ data_str;
        const uint32_t first_diff_bit_pos = highbit_pos(data_diff);
        if (first_diff_bit_pos < 64)
            return (match_len + bit_count_to_compare - first_diff_bit_pos)
                    * (((data_str >> first_diff_bit_pos) & 1) ? 1 : -1);
        match_len += bit_count_to_compare;
        str_pos += bit_count_to_compare;
    }
    return 0;
}


template <DivaType diva_type, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<diva_type, payload_type>::Infix::AddBitsToTrie(uint64_t bits,
                                                                uint32_t bit_count) {
    if (num_trie_bits_ + bit_count >= 64 * trie_.size())
        trie_.push_back(0);
    write_bits_to_bitmap(trie_.data(), num_trie_bits_,
                         bits, bit_count);
    num_trie_bits_ += bit_count;
}


template <DivaType diva_type, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<diva_type, payload_type>::Infix::AddBitsToTrie(const void *bits,
                                                                uint32_t bit_count,
                                                                uint32_t bit_offset) {
    if (num_trie_bits_ + bit_count >= 64 * trie_.size())
        trie_.resize((num_trie_bits_ + bit_count) / 64 + 1);
    write_bits_from_string_to_bitmap(trie_.data(), num_trie_bits_,
                                     bits, bit_offset,
                                     bit_count);
    num_trie_bits_ += bit_count;
}


template <DivaType diva_type, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<diva_type, payload_type>::Infix::AddBitsFromBitmapToTrie(const void *bits,
                                                                          uint32_t bit_count,
                                                                          uint32_t bit_offset) {
    if (num_trie_bits_ + bit_count >= 64 * trie_.size())
        trie_.resize((num_trie_bits_ + bit_count) / 64 + 1);
    copy_bitmap_to_bitmap(bits, bit_offset,
                          trie_.data(), num_trie_bits_,
                          bit_count);
    num_trie_bits_ += bit_count;
}


template <DivaType diva_type, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<diva_type, payload_type>::Infix::AddCounterToTrie(uint64_t counter) {
    uint32_t digit_count = GetCounterDigitCount(counter);
    uint32_t bit_pos = digit_count + 1;
    uint64_t counter_encoding = 1ULL << digit_count;
    while (counter) {
        counter_encoding |= (counter & BITMASK(varlen_counter_encoding_fragment_length)) << bit_pos;
        counter >>= varlen_counter_encoding_fragment_length;
        bit_pos += varlen_counter_encoding_fragment_length;
    }
    AddBitsToTrie(counter_encoding, digit_count * (varlen_counter_encoding_fragment_length + 1) + 1);
}


template <DivaType diva_type, PayloadType payload_type>
//__attribute__((always_inline))
inline uint32_t Diva<diva_type, payload_type>::Infix::GetCounterDigitCount(uint64_t counter) const {
    uint32_t digit_count = 1;
    for (uint64_t pw = 1UL << varlen_counter_encoding_fragment_length;
            pw <= counter; pw <<= varlen_counter_encoding_fragment_length)
        digit_count++;
    return digit_count;
}


template <DivaType diva_type, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<diva_type, payload_type>::Infix::AddSuffixToTrie(const InfiniteByteString suffix,
                                                                  uint32_t suffix_bit_count,
                                                                  uint32_t suffix_bit_pos,
                                                                  uint32_t slot_size,
                                                                  uint32_t actual_suffix_len) {
    assert(!trie_.empty());
    const uint32_t num_bits_to_add = suffix_bit_count - suffix_bit_pos;
#ifdef DEBUG
    assert(num_bits_to_add < 1e6);
#endif // DEBUG
    uint32_t added_bit_count = 0;   // PERF: Computing using a loop to avoid division
    uint32_t part_len = actual_suffix_len, num_iters = 0;
    for (uint32_t bits_covered = 0; bits_covered < num_bits_to_add; ) {
        added_bit_count += part_len;
        bits_covered += part_len - 1;
        part_len = slot_size;
        num_iters++;
    }
    added_bit_count += num_bits_to_add == 0 ? actual_suffix_len : 0;
    added_bit_count += num_bits_to_add + num_iters == added_bit_count ? slot_size : 0;
    if (num_suffix_bits_ + added_bit_count >= 64 * trie_suffixes_.size()) {
        const uint32_t old_size = trie_suffixes_.size();
        trie_suffixes_.resize((num_suffix_bits_ + added_bit_count + 63) / 64);
        const uint32_t size_diff = sizeof(trie_suffixes_[0]) * (trie_suffixes_.size() - old_size);
        memset(trie_suffixes_.data() + old_size, 0, size_diff);
    }
    WriteSuffixToSuffixes(suffix, suffix_bit_count, suffix_bit_pos,
            num_suffix_bits_,
            actual_suffix_len, slot_size);
    num_suffix_bits_ += added_bit_count;
    num_suffixes_++;
}


template <DivaType diva_type, PayloadType payload_type>
//__attribute__((always_inline))
inline uint32_t Diva<diva_type, payload_type>::Infix::WriteSuffixToSuffixes(const InfiniteByteString suffix,
                                                                            uint32_t suffix_len,
                                                                            uint32_t suffix_offset,
                                                                            uint32_t write_pos,
                                                                            uint32_t actual_suffix_len,
                                                                            uint32_t slot_size) {
    assert(!trie_.empty());

    if (suffix_len - suffix_offset == 0) {
        write_bits_to_bitmap(trie_suffixes_.data(), write_pos,
                (actual_suffix_len > 1), actual_suffix_len);
        return actual_suffix_len;
    }

    const uint32_t orig_write_pos = write_pos;
    uint32_t write_size = actual_suffix_len;
    do {
        const uint32_t bits_to_copy = write_size > 1 ? std::min(write_size - 1, suffix_len - suffix_offset) 
                                                     : 0;
        const uint64_t data = suffix.BitsAtBitLength(suffix_offset, bits_to_copy) 
                            | (1ULL << bits_to_copy);
        write_bits_to_bitmap(trie_suffixes_.data(), write_pos,
                             data, write_size);
        write_pos += write_size;
        suffix_offset += write_size - 1;
        write_size = slot_size;
    } while (suffix_offset <= suffix_len);
    return write_pos - orig_write_pos;
}


// Assumes `key_1` and `key_2` are identical up to `key_start_bit`-th bit.
template <DivaType diva_type, PayloadType payload_type>
inline uint32_t Diva<diva_type, payload_type>::Infix::GetSharedPrefixLen(const uint8_t *key_1,
                                                                         uint32_t bit_length_1,
                                                                         const uint8_t *key_2,
                                                                         uint32_t bit_length_2,
                                                                         uint32_t key_start_bit) const {
    if (key_start_bit >= std::min(bit_length_1, bit_length_2))
        return 0;
    uint32_t res = 0, bit_pos = key_start_bit;
    uint64_t read_1, read_2;
    const uint32_t min_length = std::min(bit_length_1, bit_length_2);
    uint32_t bytes_to_copy, bits_to_copy, delta;
    do {
        bytes_to_copy = std::min(8U, (min_length - bit_pos + bit_pos % 8 + 7) / 8);
        bits_to_copy = 8 * bytes_to_copy - bit_pos % 8;
        memcpy(&read_1, key_1 + bit_pos / 8, bytes_to_copy);
        read_1 = __builtin_bswap64(read_1) << (bit_pos % 8);
        memcpy(&read_2, key_2 + bit_pos / 8, bytes_to_copy);
        read_2 = __builtin_bswap64(read_2) << (bit_pos % 8);

        const uint32_t advance_bits = std::min(min_length - bit_pos, bits_to_copy);
        delta = std::min<uint32_t>(advance_bits, __builtin_ia32_lzcnt_u64(read_1 ^ read_2));
        res += delta;
        bit_pos += advance_bits;
    } while (delta == bits_to_copy && bit_pos < min_length);
    return res;
}


template <DivaType diva_type, PayloadType payload_type>
inline uint32_t Diva<diva_type, payload_type>::Infix::GetSharedPrefixLen(const InfiniteByteString key_1,
                                                                         uint32_t start_bit_1,
                                                                         const uint8_t *key_2,
                                                                         uint32_t bit_count_2) const {
    uint32_t res = 0, bit_pos_1 = start_bit_1, bit_pos_2 = 0;
    uint64_t read_1, read_2;
    uint32_t delta, num_bits_to_copy;
    do {
        num_bits_to_copy = std::min(56U, bit_count_2 - bit_pos_2);
        read_1 = key_1.BitsAtBitLength(bit_pos_1, num_bits_to_copy);
        read_2 = 0;
        write_bits_from_string_to_bitmap(&read_2, 0,
                key_2, bit_pos_2, num_bits_to_copy);

        delta = num_bits_to_copy - highbit_pos(read_1 ^ read_2) - 1;
        res += delta;
        bit_pos_1 += num_bits_to_copy;
        bit_pos_2 += num_bits_to_copy;
    } while (delta == num_bits_to_copy && bit_pos_2 < bit_count_2);
    return res;
}


static uint32_t stats_total_trie_count = 0;
static uint64_t stats_total_trie_size = 0;
static uint32_t stats_total_trie_key_count = 0;
static uint32_t stats_total_binary_length_tries_count = 0;

template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::Infix::BuildTrieAndSuffixes(const InfiniteByteString *keys,
                                                                       uint32_t key_count, 
                                                                       uint32_t key_start_bit, 
                                                                       uint32_t slot_size,
                                                                       bool force_prefix_keys,
                                                                       bool store_full_keys,
                                                                       bool pad_short_keys) {
    num_suffixes_ = 0;
    num_suffix_bits_ = 0;
    num_prefix_keys_ = 0;
    num_trie_bits_ = 0;
    trie_.clear();
    trie_.push_back(0);
    trie_suffixes_.clear();
    trie_suffixes_.push_back(0);

    bool has_prefix_keys = force_prefix_keys;
    for (uint32_t i = 1; i < key_count; i++) {
        const uint32_t shared_prefix_len = GetSharedPrefixLen(keys[i - 1].str,
                                                              keys[i - 1].length,
                                                              keys[i].str,
                                                              keys[i].length,
                                                              key_start_bit);
        if (key_start_bit + shared_prefix_len >= keys[i - 1].length || shared_prefix_len > 64) {
            has_prefix_keys = true;
            break;
        }
    }
    SetHasPrefixKeys(has_prefix_keys);

    // Build Trie
    std::vector<uint32_t> suffix_offsets;
    suffix_offsets.reserve(key_count);
    BuildTrieRecurse(keys, key_count, key_start_bit, slot_size, store_full_keys, pad_short_keys, suffix_offsets);

    stats_total_trie_size += num_trie_bits_;
    stats_total_trie_key_count += key_count;

    // Setup suffixes
    const uint32_t actual_suffix_len = GetActualSuffixLen(slot_size);
    const uint32_t actual_used_bits_in_slot = actual_suffix_len - 1 - (actual_suffix_len > 1);
    num_suffixes_ = 0;  // WARN: Had to set this in the recursive function to make `GetActualSuffixLen` work
    for (int32_t i = 0; i < key_count; i++) {
        if (suffix_offsets[i] == std::numeric_limits<uint32_t>::max())
            continue;
        const uint32_t key_remaining_length = keys[i].length - suffix_offsets[i];
        if (!store_full_keys && pad_short_keys && key_remaining_length < actual_used_bits_in_slot) {
            const uint32_t new_bit_length = suffix_offsets[i] + actual_used_bits_in_slot;
            uint8_t key_copy[(new_bit_length + 7) / 8];
            memset(key_copy, 0, (new_bit_length + 7) / 8);
            memcpy(key_copy, keys[i].str, (keys[i].length + 7) / 8);
            AddSuffixToTrie({key_copy, new_bit_length}, new_bit_length,
                    suffix_offsets[i], slot_size, actual_suffix_len);
        }
        else {
            const uint32_t suffix_len = store_full_keys ? key_remaining_length
                                      : std::min(actual_used_bits_in_slot, key_remaining_length);
            AddSuffixToTrie(keys[i], suffix_offsets[i] + suffix_len,
                    suffix_offsets[i], slot_size, actual_suffix_len);
        }
    }

    stats_total_binary_length_tries_count += HasPrefixKeys();
    stats_total_trie_count++;
}


static uint64_t stats_total_trie_depths = 0;
static uint64_t stats_total_path_lens = 0;
static uint32_t stats_build_trie_recurse_cur_depth = 0;

template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::Infix::BuildTrieRecurse(const InfiniteByteString *keys,
                                                                   uint32_t key_count, 
                                                                   uint32_t key_start_bit, 
                                                                   uint32_t slot_size,
                                                                   bool store_full_keys,
                                                                   bool pad_short_keys,
                                                                   std::vector<uint32_t>& suffix_offsets) {
    if (key_count == 1) {   // Base Case
        AddBitsToTrie(1, 1);
        suffix_offsets.push_back(key_start_bit);
        num_suffixes_++;
        stats_total_trie_depths += stats_build_trie_recurse_cur_depth;
        return;
    }

    // Recurse
    const uint32_t shared_prefix_bits = GetSharedPrefixLen(keys[0].str,
                                                           keys[0].length,
                                                           keys[key_count - 1].str,
                                                           keys[key_count - 1].length,
                                                           key_start_bit);
    int32_t split_pos = -1, prefix_key_count = 0;
    for (uint32_t i = 0; i < key_count; i++) {
        prefix_key_count += (key_start_bit + shared_prefix_bits >= keys[i].length);
        split_pos = split_pos != -1 ? split_pos 
            : (keys[i].GetBitBitLength(key_start_bit + shared_prefix_bits) ? i : split_pos);
    }
    assert(prefix_key_count == 0 || HasPrefixKeys());

    stats_build_trie_recurse_cur_depth += shared_prefix_bits + 1;
    stats_total_path_lens += shared_prefix_bits;

    const uint32_t new_start_bit = key_start_bit + shared_prefix_bits + 1 - (prefix_key_count > 1);
    if (HasPrefixKeys()) {
        // +1 to differentiate this counter from the counters indicating prefix keys
        // "Internal node" bit is set within `AddCounterToTrie`
        AddCounterToTrie(shared_prefix_bits + 1);
        AddBitsToTrie(keys[0].str, shared_prefix_bits, key_start_bit);
        if (prefix_key_count > 0) {     // Add internal node corresponding to a prefix key
            AddCounterToTrie(0);
            AddBitsToTrie((prefix_key_count < split_pos 
                            || (prefix_key_count > 0 && split_pos == -1)), 1);  // Has left child
            AddBitsToTrie(split_pos != -1, 1);                                  // Has right child
            UpdateNumPrefixKeys(1);
            suffix_offsets.push_back(std::numeric_limits<uint32_t>::max());
        }
        const int32_t has_prefix_key = prefix_key_count > 0;
        if (has_prefix_key < split_pos) {
            BuildTrieRecurse(keys + has_prefix_key , split_pos - has_prefix_key,
                    new_start_bit, slot_size, store_full_keys, pad_short_keys, 
                    suffix_offsets);
        }
        else if (has_prefix_key && split_pos == -1) {
            BuildTrieRecurse(keys + has_prefix_key, key_count - has_prefix_key,
                    new_start_bit, slot_size, store_full_keys, pad_short_keys,
                    suffix_offsets);
        }
        if (split_pos != -1) {
            BuildTrieRecurse(keys + split_pos, key_count - split_pos, new_start_bit,
                             slot_size, store_full_keys, pad_short_keys, suffix_offsets);
        }
    }
    else {
        // +1 for the "internal node" bit
        AddBitsToTrie(nullptr, shared_prefix_bits + 1, 0);
        AddBitsToTrie(1, 1);
        AddBitsToTrie(keys[0].str, shared_prefix_bits, key_start_bit);
        BuildTrieRecurse(keys, split_pos, new_start_bit,
                         slot_size, store_full_keys, pad_short_keys, suffix_offsets);
        BuildTrieRecurse(keys + split_pos, key_count - split_pos, new_start_bit,
                         slot_size, store_full_keys, pad_short_keys, suffix_offsets);
    }

    stats_build_trie_recurse_cur_depth -= (shared_prefix_bits + 1);
}


template <DivaType diva_type, PayloadType payload_type>
inline uint32_t Diva<diva_type, payload_type>::Infix::GetActualSuffixLen(uint32_t slot_size) const {
#if ACTUAL_SUFFIX_LEN_MODE == 0     // Normal operation
    const uint32_t key_count = GetNumPrefixKeys() + num_suffixes_;
    const uint32_t actual_suffix_len = slot_size * std::max<int32_t>(key_count, 0) < num_trie_bits_ ? 0 
                                       : (slot_size * std::max<int32_t>(key_count, 0) - num_trie_bits_) / num_suffixes_;
#else
    const uint32_t key_count = GetNumPrefixKeys() + num_suffixes_;
    const uint32_t actual_suffix_len = slot_size * std::max<int32_t>(key_count - 1, 0) < num_trie_bits_ ? 0 
                                       : (slot_size * std::max<int32_t>(key_count - 1, 0) - num_trie_bits_) / num_suffixes_;
#endif
#if ACTUAL_SUFFIX_LEN_MODE == 0     // Normal operation
    return std::max<int32_t>(actual_suffix_len, 1);
#elif ACTUAL_SUFFIX_LEN_MODE == 1   // EnWiki dataset
    return std::max<int32_t>(actual_suffix_len, slot_size / 2 + 0);
#elif ACTUAL_SUFFIX_LEN_MODE == 2   // Emails dataset
    return std::max<int32_t>(actual_suffix_len, slot_size / 2 - slot_size / 10);
#else                               // Quotes dataset
    return std::max<int32_t>(actual_suffix_len, slot_size / 2 - 3);
#endif
}


template <DivaType diva_type, PayloadType payload_type>
inline uint32_t Diva<diva_type, payload_type>::Infix::GetSuffixBitPos(uint32_t suffix_rank,
                                                                      uint32_t slot_size,
                                                                      int32_t actual_suffix_len_,
                                                                      uint32_t prev_suffix_bit_pos) const {
    return GetSuffixBitPos(trie_suffixes_.data(), suffix_rank,
                           slot_size, actual_suffix_len_, prev_suffix_bit_pos);
}


template <DivaType diva_type, PayloadType payload_type>
inline uint32_t Diva<diva_type, payload_type>::Infix::GetSuffixBitPos(const uint64_t *ptr,
                                                                      uint32_t suffix_rank,
                                                                      uint32_t slot_size,
                                                                      int32_t actual_suffix_len_,
                                                                      uint32_t prev_suffix_bit_pos) const {
    const uint32_t actual_suffix_len = actual_suffix_len_ == -1 ? GetActualSuffixLen(slot_size) 
                                                                : actual_suffix_len_;
    uint32_t res = prev_suffix_bit_pos;
    for (uint32_t i = 0; i < suffix_rank; i++) {
        res += actual_suffix_len;
        bool has_extras = (ptr[(res - 1) / 64] >> ((res - 1) % 64)) & 1UL;
        if (has_extras) {   // Have more suffixes to read
            do {
                res += slot_size;
                has_extras = (ptr[(res - 1) / 64] >> ((res - 1) % 64)) & 1UL;
            } while (has_extras);
        }
    }
    return res;
}


template <DivaType diva_type, PayloadType payload_type>
inline std::pair<uint32_t, uint32_t> Diva<diva_type, payload_type>::Infix::GetSuffixLength(uint32_t suffix_bit_pos,
                                                                                           uint32_t slot_size,
                                                                                           int32_t actual_suffix_len_) const {
    const uint32_t actual_suffix_len = actual_suffix_len_ == -1 ? GetActualSuffixLen(slot_size)
                                                                : actual_suffix_len_;
    uint64_t suffix_read_buf = 0, suffix;
    uint32_t suffix_read_buf_filled_bits = 0, part_len = actual_suffix_len;
    uint32_t res = 0, res_with_meta = 0;
    bool not_done = true;
    while (not_done) {
        suffix = read_data_from_bitmap(trie_suffixes_.data(), suffix_bit_pos,
                suffix_read_buf, suffix_read_buf_filled_bits,
                part_len);
        res += part_len > 1 ? highbit_pos(suffix) : 0;
        res_with_meta += part_len;
        not_done = suffix >> (part_len - 1);
        part_len = slot_size;
    }
    return {res, res_with_meta};
}


template <DivaType diva_type, PayloadType payload_type>
inline uint32_t Diva<diva_type, payload_type>::Infix::GetSuffixString(uint32_t suffix_bit_pos,
                                                                      uint32_t slot_size,
                                                                      uint8_t *res,
                                                                      uint32_t res_bit_pos,
                                                                      int32_t actual_suffix_len_) const {
    uint32_t res_len_with_meta = 0;
    const uint32_t actual_suffix_len = actual_suffix_len_ == -1 ? GetActualSuffixLen(slot_size)
                                                                : actual_suffix_len_;
    uint64_t suffix_read_buf = 0, suffix;
    uint32_t suffix_read_buf_filled_bits = 0, part_len = actual_suffix_len;
    bool not_done = true;
    while (not_done) {
        suffix = read_data_from_bitmap(trie_suffixes_.data(), suffix_bit_pos,
                suffix_read_buf, suffix_read_buf_filled_bits,
                part_len);
        const uint32_t valid_len = part_len > 1 ? highbit_pos(suffix) : 0;
        not_done = suffix >> (part_len - 1);
        suffix = __builtin_bswap64((suffix & BITMASK(valid_len)) << (64 - valid_len - res_bit_pos % 8));
        uint64_t stamp;
        memcpy(&stamp, res + res_bit_pos / 8, sizeof(stamp));
        stamp &= __builtin_bswap64(~(std::numeric_limits<uint64_t>::max() >> (res_bit_pos % 8)));
        stamp |= suffix;
        memcpy(res + res_bit_pos / 8, &stamp, sizeof(stamp));
        res_bit_pos += valid_len;
        res_len_with_meta += part_len;
        part_len = slot_size;
    }
    return res_len_with_meta;
}


template <DivaType diva_type, PayloadType payload_type>
inline bool Diva<diva_type, payload_type>::Infix::QueryTrie(const InfiniteByteString l_key,
                                                            const InfiniteByteString r_key, 
                                                            uint32_t key_start_bit,
                                                            uint32_t slot_size) const {
    TrieIterator it(trie_.data());
    int32_t last_depth = -1;
    int32_t diverge_depth = std::numeric_limits<int32_t>::max();
    int32_t l_key_dont_care_depth = std::numeric_limits<int32_t>::max();
    int32_t r_key_dont_care_depth = std::numeric_limits<int32_t>::max();
    bool second_path = false;
    if (num_trie_bits_ == 1) {  // Might have only one suffix, so there's nothing to traverse
        it.depth_branch_.clear();
        goto QueryTrieAfterLoop;
    }
QueryTrieDivergedPathRetry:
    while (true) {
        it.Advance(HasPrefixKeys());
        if (it.depth_branch_.empty())
            break;
        auto [depth, children] = it.depth_branch_.back();
        if (last_depth >= depth)
            last_depth = it.depth_branch_[it.depth_branch_.size() - 2].first;

        bool l_key_dont_care = l_key_dont_care_depth <= depth || second_path;
        bool r_key_dont_care = r_key_dont_care_depth <= depth || (diverge_depth <= depth && !second_path);
        
        // Compare l_key to path, or ignore
        const int32_t current_str_bit_pos = key_start_bit + last_depth + 1;
        const uint32_t compare_len = depth - last_depth - 1;
        const uint32_t compare_len_l = std::min<int32_t>(compare_len,
                        std::max<int32_t>(0, l_key.length - last_depth));
        const int32_t compare_l = l_key_dont_care ? 0
                : CompareStringToBitmap(it.buf_, it.bit_pos_ - compare_len_l,
                        l_key, current_str_bit_pos, 
                        compare_len_l);
        l_key_dont_care_depth = compare_l < 0 ? depth : l_key_dont_care_depth;

        if (compare_l > 0) {
            if (r_key_dont_care) {
                it.SkipSubtree(HasPrefixKeys());
                if (children == 0b11)
                    it.SkipSubtree(HasPrefixKeys());
                if (it.depth_branch_.back().first >= r_key_dont_care_depth)
                    return true;
                else if (it.depth_branch_.back().first > diverge_depth)
                    return true;
                else if (it.depth_branch_.back().first == diverge_depth) {
                    second_path = true;
                    if (it.AtLeaf())
                        break;
                    continue;
                }
            }
            return false;
        }

        // Compare r_key to path, or ignore
        const uint32_t compare_len_r = std::min<int32_t>(compare_len,
                std::max<int32_t>(0, r_key.length - current_str_bit_pos));
        int32_t compare_r = r_key_dont_care ? 0
                : CompareStringToBitmap(it.buf_, it.bit_pos_ - compare_len_r,
                        r_key, current_str_bit_pos, 
                        compare_len_r);
        if (compare_len_r < compare_len) {
            uint8_t zeros[compare_len / 8 + 2] = {};
            compare_r = CompareStringToBitmap(it.buf_, it.bit_pos_ - compare_len, 
                                              {zeros, compare_len / 8 + 2}, 0, 
                                              compare_len - compare_len_r);
        }
        r_key_dont_care_depth = compare_r > 0 ? depth : r_key_dont_care_depth;

        l_key_dont_care = l_key_dont_care_depth <= depth || second_path;
        r_key_dont_care = r_key_dont_care_depth <= depth || (diverge_depth <= depth && !second_path);

        if (l_key_dont_care && r_key_dont_care)
            return true;
        if (!r_key_dont_care && compare_r < 0)
            return false;
        if (it.AtPrefixKey(HasPrefixKeys()))
            return true;

        const uint32_t l_bit = l_key.GetBitBitLength(key_start_bit + depth) & (!l_key_dont_care);
        const uint32_t r_bit = r_key.GetBitBitLength(key_start_bit + depth) | r_key_dont_care;
        if (r_bit == 0 && (children & 1) == 0)
            return false;
        if (l_key_dont_care && (children & 1) == 1 && r_bit == 1)
            return true;
        if (l_bit == 0 && (children & 2) == 2 && r_key_dont_care)
            return true;
        if (!l_key_dont_care && !r_key_dont_care
                && compare_l == 0 && compare_r == 0 
                && children == 0b11 && ((l_bit ^ 1) & (r_bit & 1)))
            diverge_depth = depth;
        if (l_bit == 1 && (children & 1) == 1)
            it.SkipSubtree(HasPrefixKeys());

        if (it.AtLeaf())
            break;
        last_depth = depth;
    }
QueryTrieAfterLoop:
    assert(it.depth_branch_.empty() || it.depth_branch_.back().first >= 0);
    uint32_t depth = (it.depth_branch_.empty() ? -1 : it.depth_branch_.back().first) + 1;
    bool l_key_dont_care = l_key_dont_care_depth <= depth || (diverge_depth <= depth && second_path);
    bool r_key_dont_care = r_key_dont_care_depth <= depth || (diverge_depth <= depth && !second_path);
    const uint32_t suffix_rank = it.num_keys_read_;
    const uint32_t actual_suffix_len = GetActualSuffixLen(slot_size);
    uint32_t suffix_bit_pos = GetSuffixBitPos(suffix_rank, slot_size);

    // Check corresponding suffix for inclusion
    uint64_t suffix_read_buf = 0;
    uint32_t suffix_read_buf_filled_bits = 0;
    uint64_t suffix = read_data_from_bitmap(trie_suffixes_.data(), suffix_bit_pos,
            suffix_read_buf, suffix_read_buf_filled_bits,
            actual_suffix_len);
    if (actual_suffix_len == 1 && suffix == 0)  // Nothing to compare
        return true;
    uint32_t valid_len = highbit_pos(suffix);
    uint64_t valid_mask = BITMASK(valid_len);
    const bool check_l = !l_key_dont_care 
        && (suffix & valid_mask) < (l_key.BitsAtBitLength(key_start_bit + depth, valid_len));
    const bool check_r = !r_key_dont_care 
        && (suffix & valid_mask) > (r_key.BitsAtBitLength(key_start_bit + depth, valid_len));
    if (check_l || check_r) {
        if (diverge_depth < depth && !second_path) {    // Still have to check the key on the other path
            second_path = true;
            goto QueryTrieDivergedPathRetry;
        }
        return false;
    }
    l_key_dont_care |= (suffix & valid_mask) > (l_key.BitsAtBitLength(key_start_bit + depth, valid_len));
    r_key_dont_care |= (suffix & valid_mask) < (r_key.BitsAtBitLength(key_start_bit + depth, valid_len));
    if (l_key_dont_care && r_key_dont_care)
        return true;
    depth += valid_len;
    if (suffix >> (actual_suffix_len - 1)) {    // Have more suffixes to consider
        do {
            suffix = read_data_from_bitmap(trie_suffixes_.data(), suffix_bit_pos,
                    suffix_read_buf, suffix_read_buf_filled_bits,
                    slot_size);
            valid_len = highbit_pos(suffix);
            valid_mask = BITMASK(valid_len);
            const bool check_l = !l_key_dont_care 
                && (suffix & valid_mask) < l_key.BitsAtBitLength(key_start_bit + depth, valid_len);
            const bool check_r = !r_key_dont_care 
                && (suffix & valid_mask) > r_key.BitsAtBitLength(key_start_bit + depth, valid_len);
            if (check_l || check_r) {
                if (diverge_depth < depth && !second_path) {    // Still have to check the key on the other path
                    second_path = true;
                    goto QueryTrieDivergedPathRetry;
                }
                return false;
            }
            l_key_dont_care |= (suffix & valid_mask) > (l_key.BitsAtBitLength(key_start_bit + depth, valid_len));
            r_key_dont_care |= (suffix & valid_mask) < (r_key.BitsAtBitLength(key_start_bit + depth, valid_len));
            if (l_key_dont_care && r_key_dont_care)
                return true;
            depth += valid_len;
        } while (suffix >> (slot_size - 1));
    }
    return true;
}


template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::Infix::AdjustActualSuffixLen(uint32_t old_actual_suffix_len,
                                                                        uint32_t new_actual_suffix_len,
                                                                        uint32_t slot_size) {
    uint8_t suffix_str[sizeof(trie_suffixes_[0]) * trie_suffixes_.size() + 8];
    uint32_t old_suffixes_bit_pos = 0;
    Infix res;
    res.trie_.push_back(0);  // Just to keep `AddSuffixToTrie` from complaining
    res.trie_suffixes_.reserve(1024);
    for (int32_t i = 0; i < num_suffixes_; i++) {
        const auto [suffix_len, suffix_len_with_meta] = GetSuffixLength(old_suffixes_bit_pos, slot_size, 
                                                        old_actual_suffix_len);
        GetSuffixString(old_suffixes_bit_pos, slot_size, suffix_str, 0,
                        old_actual_suffix_len);
        old_suffixes_bit_pos += suffix_len_with_meta;
        res.AddSuffixToTrie({suffix_str, suffix_len}, suffix_len, 0,
                slot_size, new_actual_suffix_len);
    }
    num_suffix_bits_ = res.num_suffix_bits_;
    trie_suffixes_ = res.trie_suffixes_;
    // PERF: Is it faster to read and write words at the same time? Do we care?
}


template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::Infix::SwitchTrieEncoding(bool has_prefix_keys, uint32_t slot_size,
                                                                     int32_t old_actual_suffix_len) {
    if (trie_[0] & 1UL) {   // The entire trie is just a leaf, so just have to adjust actual suffix length if needed
        SetHasPrefixKeys(has_prefix_keys);
        const uint32_t actual_suffix_len = num_suffix_bits_;
        const uint32_t new_actual_suffix_len = std::max(slot_size - 1, 1U);
        if (actual_suffix_len != new_actual_suffix_len)
            AdjustActualSuffixLen(actual_suffix_len, new_actual_suffix_len, slot_size);
        return;
    }

    std::vector<uint64_t> trie_backup = trie_;
    const uint32_t actual_suffix_len_backup = old_actual_suffix_len == -1 ? GetActualSuffixLen(slot_size)
                                                                          : old_actual_suffix_len;
    num_trie_bits_ = 0;
    trie_.resize(1);
    trie_[0] = 0;

    TrieIterator it(trie_backup.data());
    do {
        const bool at_leaf = it.AtLeaf();
        if (!has_prefix_keys || at_leaf)    // The `AddCounter` function already adds an additional zero (bad design)
            AddBitsToTrie(at_leaf, 1);

        const int32_t last_depth = it.depth_branch_.back().first;
        it.Advance(HasPrefixKeys());
        if (at_leaf)    // No length counter or string to handle
            continue;
        const int32_t depth = it.depth_branch_.back().first;
        assert(has_prefix_keys || !it.AtPrefixKey(HasPrefixKeys()));
        const int32_t path_len = depth - last_depth - 1;

        // Setup the length counter
        if (has_prefix_keys)
            AddCounterToTrie(path_len + 1);
        else {
            AddBitsToTrie(nullptr, path_len, 0);
            AddBitsToTrie(1, 1);
        }
        // Copy the path string
        if (num_trie_bits_ + path_len >= 64 * trie_.size())
            trie_.resize((num_trie_bits_ + path_len + 63) / 64 + 1);
        copy_bitmap_to_bitmap(trie_backup.data(), it.bit_pos_ - path_len,
                              trie_.data(), num_trie_bits_,
                              path_len);
        num_trie_bits_ += path_len;
    } while (!it.depth_branch_.empty() && it.depth_branch_.back().first != -1);
    SetHasPrefixKeys(has_prefix_keys);

    // Ensure that the suffixes follow the proper format with their "actual length."
    const uint32_t actual_suffix_len = GetActualSuffixLen(slot_size);
    if (actual_suffix_len == actual_suffix_len_backup)
        return;
    AdjustActualSuffixLen(actual_suffix_len_backup, 
                          actual_suffix_len,
                          slot_size);
}


template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::Infix::InsertTrie(const InfiniteByteString key,
                                                             uint32_t key_start_bit,
                                                             uint32_t slot_size) {
    if (num_trie_bits_ == 0) {  // Just add a suffix
        BuildTrieAndSuffixes(&key, 1, key_start_bit, slot_size);
        return;
    }

    TrieIterator it(trie_.data());
    const bool has_prefix_keys = HasPrefixKeys();
    const uint32_t actual_suffix_len_backup = GetActualSuffixLen(slot_size);
    bool found_exact_match = true;
    int32_t last_depth = -1, last_bit_pos = 0;
    if (num_trie_bits_ == 1) {  // Might have only one suffix, so there's nothing to traverse
        it.depth_branch_.clear();
        goto InsertTrieAfterLoop;
    }
    while (true) {
        it.Advance(has_prefix_keys);
        auto [depth, children] = it.depth_branch_.back();
        const int32_t current_str_bit_pos = key_start_bit + last_depth + 1;
        const uint32_t path_len = depth - last_depth - 1;
        if (path_len > 0) {
            const uint32_t compare_len = std::min<int32_t>(path_len,
                    std::max<int32_t>(0, key.length - last_depth));
            const int32_t compare = CompareStringToBitmap(it.buf_, it.bit_pos_ - compare_len,
                                                          key, current_str_bit_pos, 
                                                          compare_len);
            if (compare != 0) {         // Create diverging path
                found_exact_match = false;
                uint64_t path[(path_len + 63) / 64];
                memset(path, 0, sizeof(path));
                copy_bitmap_to_bitmap(trie_.data(), it.bit_pos_ - path_len,
                                      path, 0,
                                      path_len);

                const uint32_t branch_pos = std::abs(compare) - 1;
                const uint64_t branch_bit = compare > 0;
                const uint32_t path_len_1 = branch_pos;
                const uint32_t path_len_2 = path_len - branch_pos - 1;
                uint32_t old_meta_offset, meta_offset_1, meta_offset_2;
                if (has_prefix_keys) {
                    old_meta_offset = GetCounterDigitCount(path_len + 1) * (varlen_counter_encoding_fragment_length + 1) + 1;
                    meta_offset_1 = GetCounterDigitCount(path_len_1 + 1) * (varlen_counter_encoding_fragment_length + 1) + 1;
                    meta_offset_2 = GetCounterDigitCount(path_len_2 + 1) * (varlen_counter_encoding_fragment_length + 1) + 1;
                }
                else {
                    old_meta_offset = path_len + 2;
                    meta_offset_1 = path_len_1 + 2;
                    meta_offset_2 = path_len_2 + 2;
                }

                const uint32_t added_bit_count = meta_offset_1 + meta_offset_2 - old_meta_offset;
                AddBitsToTrie(nullptr, added_bit_count, 0);

                // The right-shift below moves the whole tail (incl. trailing zeros) right by
                // `meta_offset_2 + path_len_2 + ...`, so the buffer must reach that destination.
                const uint32_t rshift_max_bit = num_trie_bits_ + meta_offset_2 + path_len_2 + 8;
                if ((rshift_max_bit + 63) / 64 > trie_.size())
                    trie_.resize((rshift_max_bit + 63) / 64);

                uint32_t write_pos = last_bit_pos;
                shift_bitmap_left(trie_.data(),
                        write_pos + old_meta_offset - meta_offset_1 + path_len_2 + 1, num_trie_bits_ - 1,
                        old_meta_offset - meta_offset_1 + path_len_2 + 1);
                if (has_prefix_keys) {
                    write_varlen_counter_to_bitmap(trie_.data(), write_pos,
                            path_len_1 + 1, varlen_counter_encoding_fragment_length);
                }
                else {
                    write_bits_from_string_to_bitmap(trie_.data(), write_pos,
                            nullptr, 0, path_len_1 + 1);
                    write_bits_to_bitmap(trie_.data(), write_pos + path_len_1 + 1,
                            1, 1);
                }
                write_pos += meta_offset_1;
                //copy_bitmap_to_bitmap(path, path_len_1, trie_.data(), write_pos, branch_pos);
                write_pos += path_len_1;
                it.buf_ = trie_.data();
                it.bit_pos_ = write_pos;
                it.depth_branch_[it.depth_branch_.size() - 1] = {last_depth + path_len_1 + 1, 0b11};
                shift_bitmap_right(trie_.data(),
                                   write_pos,
                                   num_trie_bits_ - 1,
                                   meta_offset_2 + path_len_2 + (branch_bit == 0));
                if (branch_bit == 0) {
                    write_bits_to_bitmap(trie_.data(), write_pos, 1, 1);
                    write_pos++;
                }
                if (has_prefix_keys) {
                    write_varlen_counter_to_bitmap(trie_.data(), write_pos,
                            path_len_2 + 1, varlen_counter_encoding_fragment_length);
                }
                else {
                    write_bits_from_string_to_bitmap(trie_.data(), write_pos,
                            nullptr, 0, path_len_2 + 1);
                    write_bits_to_bitmap(trie_.data(), write_pos + path_len_2 + 1,
                            1, 1);
                }
                write_pos += meta_offset_2;
                copy_bitmap_to_bitmap(path, 0, trie_.data(), write_pos, path_len_2);
                write_pos += path_len_2;
                if (branch_bit == 1) {
                    it.SkipSubtree(has_prefix_keys);
                    write_pos = it.bit_pos_;
                    shift_bitmap_right(trie_.data(),
                                       write_pos, num_trie_bits_ - 1,
                                       1);
                    write_bits_to_bitmap(trie_.data(), write_pos, 1, 1);
                    write_pos++;
                }
                break;
            }
        }
        if (it.AtPrefixKey(has_prefix_keys)) {
            // Handle prefix insertion
            last_bit_pos = it.bit_pos_;
            it.Advance(has_prefix_keys);
            const auto [new_depth, new_children] = it.depth_branch_.back();
            depth = new_depth;
            children = new_children;
            if ((children & 1) == 0 && key.GetBitBitLength(key_start_bit + depth) == 0) {    
                found_exact_match = false;
                write_bits_to_bitmap(trie_.data(), last_bit_pos + 2 + varlen_counter_encoding_fragment_length,
                        1, 1);
                it.buf_ = trie_.data();     // Might've been reallocated on the heap
                it.depth_branch_[it.depth_branch_.size() - 1].second = 0b11;
                AddBitsToTrie(0, 1);
                shift_bitmap_right(trie_.data(),
                        last_bit_pos + 4 + varlen_counter_encoding_fragment_length, num_trie_bits_ - 1,
                        1);
                write_bits_to_bitmap(trie_.data(), last_bit_pos + 4 + varlen_counter_encoding_fragment_length,
                        1, 1);
                break;
            }
            else if ((children & 0b10) == 0 && key.GetBitBitLength(key_start_bit + depth) == 1) {
                found_exact_match = false;
                write_bits_to_bitmap(trie_.data(), last_bit_pos + 3 + varlen_counter_encoding_fragment_length,
                            1, 1);
                AddBitsToTrie(0, 1);
                it.buf_ = trie_.data();
                it.depth_branch_[it.depth_branch_.size() - 1].second = 0b11;
                it.SkipSubtree(has_prefix_keys);
                shift_bitmap_right(trie_.data(),
                                   it.bit_pos_, num_trie_bits_ - 1,
                                   1);
                write_bits_to_bitmap(trie_.data(), it.bit_pos_,
                                     1, 1);
                break;
            }
        }
        if ((children & 1) == 1 && key.GetBitBitLength(key_start_bit + depth) == 1)
            it.SkipSubtree(has_prefix_keys);
        if (it.AtLeaf())
            break;
        last_depth = depth;
        last_bit_pos = it.bit_pos_;
    }
InsertTrieAfterLoop:
    assert(it.depth_branch_.empty() || it.depth_branch_.back().first >= 0);
    uint32_t depth = (it.depth_branch_.empty() ? -1 : it.depth_branch_.back().first) + 1;

    // Check if suffixes match
    uint32_t suffix_bit_pos = GetSuffixBitPos(it.num_keys_read_, slot_size, actual_suffix_len_backup);
    uint32_t suffix_len = 0, suffix_len_with_meta = 0;
    if (suffix_bit_pos < num_suffix_bits_) {
        auto [len, len_with_meta] =
            GetSuffixLength(suffix_bit_pos, slot_size, actual_suffix_len_backup);
        suffix_len = len;
        suffix_len_with_meta = len_with_meta;
    }
    uint8_t suffix_contents[suffix_len / 8 + 8];
    memset(suffix_contents, 0, suffix_len / 8 + 8);
    const InfiniteByteString suffix = {suffix_contents, suffix_len};
    int32_t suffix_mismatch_pos = -1;
    bool suffix_diff_bit = 0;
    if (found_exact_match) {
        GetSuffixString(suffix_bit_pos, slot_size,
                suffix_contents, 0,
                actual_suffix_len_backup);
        for (uint32_t i = 0; i < suffix_len; i += 56) {
            const uint32_t bits_to_compare = std::min(56U, suffix_len - i);
            const uint64_t read_key = key.BitsAtBitLength(key_start_bit + depth + i,
                                                          bits_to_compare);
            const uint64_t read_suffix = suffix.BitsAtBitLength(i, bits_to_compare);
            const uint64_t diff = read_key ^ read_suffix;
            if (diff) {
                found_exact_match = false;
                suffix_mismatch_pos = i + bits_to_compare - highbit_pos(diff) - 1;
                suffix_diff_bit = key.GetBitBitLength(key_start_bit + depth + suffix_mismatch_pos);
                break;
            }
        }
    }

    if (!found_exact_match && suffix_mismatch_pos != -1) {     // Add new path to trie
        uint32_t meta_offset;
        if (has_prefix_keys) {
            meta_offset = GetCounterDigitCount(suffix_mismatch_pos + 1) 
                            * (varlen_counter_encoding_fragment_length + 1) + 1;
        }
        else
            meta_offset = suffix_mismatch_pos + 2;
        const uint32_t shamt = meta_offset + suffix_mismatch_pos + 1;
        const uint32_t old_num_trie_bits = num_trie_bits_;
        AddBitsToTrie(nullptr, shamt, 0);
        uint32_t write_pos = it.bit_pos_;
        shift_bitmap_right(trie_.data(),
                           write_pos,
                           old_num_trie_bits - 1,
                           shamt);
        if (has_prefix_keys) {
            write_varlen_counter_to_bitmap(trie_.data(), write_pos,
                    suffix_mismatch_pos + 1,
                    varlen_counter_encoding_fragment_length);
        }
        else {
            write_bits_from_string_to_bitmap(trie_.data(), write_pos,
                    nullptr, 0, suffix_mismatch_pos + 1);
            write_bits_to_bitmap(trie_.data(), write_pos + suffix_mismatch_pos + 1,
                    1, 1);
        }
        write_pos += meta_offset;
        write_bits_from_string_to_bitmap(trie_.data(), write_pos,
                suffix.str, 0, suffix_mismatch_pos);
        write_pos += suffix_mismatch_pos;
        write_bits_to_bitmap(trie_.data(), write_pos,
                             0b11, 2);

        // Update the suffixes
        const uint32_t suffix_bits_written = WriteSuffixToSuffixes(suffix, suffix_len, suffix_mismatch_pos + 1, 
                suffix_bit_pos, actual_suffix_len_backup, slot_size);
        shift_bitmap_left(trie_suffixes_.data(),
                          suffix_bit_pos + suffix_len_with_meta, num_suffix_bits_ - 1,
                          suffix_len_with_meta - suffix_bits_written);
        num_suffix_bits_ -= suffix_len_with_meta - suffix_bits_written;
        if (suffix_diff_bit)
            suffix_bit_pos += suffix_bits_written;
    }

    // Handle prefix key
    if (found_exact_match) {
        if (!has_prefix_keys) {
            SwitchTrieEncoding(true, slot_size);
            InsertTrie(key, key_start_bit, slot_size);
        }
        else {
            // Update trie
            const uint32_t meta_offset_1 = GetCounterDigitCount(suffix_len + 1) 
                                            * (varlen_counter_encoding_fragment_length + 1) + 1;
            const uint32_t meta_offset_2 = varlen_counter_encoding_fragment_length + 4;
            const uint32_t old_num_trie_bits = num_trie_bits_;
            AddBitsToTrie(nullptr, meta_offset_1 + meta_offset_2 + suffix_len, 0);
            uint32_t write_pos = it.bit_pos_;
            shift_bitmap_right(trie_.data(),
                               write_pos, old_num_trie_bits - 1,
                               meta_offset_1 + meta_offset_2 + suffix_len);
            write_varlen_counter_to_bitmap(trie_.data(), write_pos,
                    suffix_len + 1, varlen_counter_encoding_fragment_length);
            write_pos += meta_offset_1;
            write_bits_from_string_to_bitmap(trie_.data(), write_pos,
                    suffix.str, 0, suffix_len);
            write_pos += suffix_len;
            write_varlen_counter_to_bitmap(trie_.data(), write_pos,
                    0, varlen_counter_encoding_fragment_length);
            write_bits_to_bitmap(trie_.data(), write_pos + meta_offset_2 - 2,
                                 1ULL << key.GetBitBitLength(key_start_bit + depth + suffix_len),
                                                                   2);
            UpdateNumPrefixKeys(1);

            // Rewrite suffix
            const uint32_t actual_suffix_len = GetActualSuffixLen(slot_size);
            if (actual_suffix_len != actual_suffix_len_backup) {
                AdjustActualSuffixLen(actual_suffix_len_backup,
                        actual_suffix_len, slot_size);
                suffix_bit_pos = GetSuffixBitPos(it.num_keys_read_ + suffix_diff_bit,
                                                 slot_size, actual_suffix_len);
                auto [new_suffix_len, new_suffix_len_with_meta] =
                    GetSuffixLength(suffix_bit_pos, slot_size, actual_suffix_len);
                suffix_len_with_meta = new_suffix_len_with_meta;
            }
            shift_bitmap_left(trie_suffixes_.data(),
                              suffix_bit_pos + suffix_len_with_meta,
                              num_suffix_bits_ - 1,
                              suffix_len_with_meta - actual_suffix_len);
            num_suffix_bits_ -= suffix_len_with_meta - actual_suffix_len;
            const uint64_t new_suffix = actual_suffix_len > 1 ? key.BitsAtBitLength(key_start_bit + depth + suffix_len + 1,
                                                                                    actual_suffix_len - 2) 
                                                                    | (1ULL << (actual_suffix_len - 2))
                                                              : 0;
            write_bits_to_bitmap(trie_suffixes_.data(),
                    suffix_bit_pos, new_suffix,
                    actual_suffix_len);
        }
        return;
    }

    // Ensure that the suffixes follow the proper format with their "actual length"
    num_suffixes_++;    // Ugly increment and decrement to make sure new suffix lengths
                        // are computed for the new number of suffixes
    const uint32_t actual_suffix_len = GetActualSuffixLen(slot_size);
    num_suffixes_--;
    if (actual_suffix_len != actual_suffix_len_backup) {
        AdjustActualSuffixLen(actual_suffix_len_backup, actual_suffix_len, slot_size);
        suffix_bit_pos = GetSuffixBitPos(it.num_keys_read_ + suffix_diff_bit, slot_size, actual_suffix_len);
    }
    num_suffixes_++;

    // Insert suffix
    if (num_suffix_bits_ + actual_suffix_len >= 64 * trie_suffixes_.size())
        trie_suffixes_.push_back(0);
    shift_bitmap_right(trie_suffixes_.data(),
                       suffix_bit_pos,
                       num_suffix_bits_ - 1,
                       actual_suffix_len);
    num_suffix_bits_ += actual_suffix_len;
    const uint64_t new_suffix = actual_suffix_len > 1 ? key.BitsAtBitLength(key_start_bit + depth + suffix_mismatch_pos + 1, actual_suffix_len - 2) 
                                                            | (1ULL << (actual_suffix_len - 2))
                                                      : 0;
    write_bits_to_bitmap(trie_suffixes_.data(),
            suffix_bit_pos, new_suffix,
            actual_suffix_len);
}


template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::Infix::DeleteTrie(const InfiniteByteString key,
                                                             uint32_t key_start_bit,
                                                             uint32_t slot_size) {
    // TODO: Perhaps it would be faster to handle deletes without getting the full strings?
    auto [keys, keys_contents] = GetStrings(slot_size);
    int32_t remove_candidate = -1;
    for (uint32_t i = 0; i < keys.size(); i++) {
        const uint32_t match_bit_count = GetSharedPrefixLen(key, key_start_bit,
                                                            keys[i].str, keys[i].length);
        assert(match_bit_count <= keys[i].length);
        if (match_bit_count == keys[i].length)
            remove_candidate = (remove_candidate == -1 || keys[remove_candidate].length < keys[i].length) 
                             ? i : remove_candidate;
    }
    assert(remove_candidate != -1);     // Deletee must exist.
    keys.erase(keys.begin() + remove_candidate);
    if (keys.empty()) {
        num_prefix_keys_ = 0;
        num_trie_bits_ = 0;
        trie_.clear();
        num_suffixes_ = 0;
        num_suffix_bits_ = 0;
        trie_suffixes_.clear();
    }
    else {
        BuildTrieAndSuffixes(keys.data(), keys.size(), 0, slot_size,
                false, true);
    }
}


template <DivaType diva_type, PayloadType payload_type>
inline std::pair<std::vector<typename Diva<diva_type, payload_type>::InfiniteByteString>, std::vector<uint8_t>> 
Diva<diva_type, payload_type>::Infix::GetStrings(uint32_t slot_size) const {
    std::vector<InfiniteByteString> res;
    std::vector<uint8_t> res_contents;
    if (num_trie_bits_ == 0) {
        res_contents.push_back(0);
        res.emplace_back(res_contents.data(), 0);
        return {std::move(res), std::move(res_contents)};
    }
    res_contents.reserve(4096);
    std::vector<uint8_t> current_string;
    current_string.reserve(1024);

    TrieIterator it(trie_.data());
    const bool has_prefix_keys = HasPrefixKeys();
    const uint32_t actual_suffix_len = GetActualSuffixLen(slot_size);
    int32_t last_depth = -1, suffix_bit_pos = 0;
    do {
        if (num_trie_bits_ != 1)    // Nothing to traverse if the trie is only a leaf
            it.Advance(has_prefix_keys);
        auto [depth, children] = it.depth_branch_.back();
        current_string.resize(depth / 8 + 1);
        if (last_depth < depth) {
            const uint32_t path_len = depth - last_depth - 1;
            write_bits_from_bitmap_to_string(current_string.data(), last_depth + 1,
                                             trie_.data(), it.bit_pos_ - path_len,
                                             path_len);
        }
        current_string[depth / 8] &= ~BITMASK(8 - depth % 8);
        current_string[depth / 8] |= ((children & -children) - 1) << (7 - depth % 8);

        if (it.AtPrefixKey(has_prefix_keys)) {   // Add string to results
            const uint32_t old_res_contents_size = res_contents.size();
            res_contents.resize(old_res_contents_size + current_string.size());
            memcpy(res_contents.data() + old_res_contents_size, current_string.data(), current_string.size());
            res.emplace_back(reinterpret_cast<const uint8_t *>(old_res_contents_size), depth);
            it.Advance(has_prefix_keys);
            last_depth = depth;
            depth = it.depth_branch_.back().first;
            children = it.depth_branch_.back().second;
            current_string[depth / 8] &= ~BITMASK(8 - depth % 8);
            current_string[depth / 8] |= ((children & -children) - 1) << (7 - depth % 8);
        }
        if (it.AtLeaf()) {
            const auto [suffix_len, suffix_len_with_meta] =
                GetSuffixLength(suffix_bit_pos, slot_size, actual_suffix_len);
            const uint32_t old_res_contents_size = res_contents.size();
            const uint32_t current_string_byte_len = (depth + 1 + suffix_len + 7) / 8;
            res_contents.resize(old_res_contents_size + current_string_byte_len);
            memcpy(res_contents.data() + old_res_contents_size, current_string.data(), current_string.size());
            if (depth != -1)
                res_contents[old_res_contents_size + depth / 8] |= ((children & -children) - 1) << (7 - depth % 8);
            GetSuffixString(suffix_bit_pos, slot_size, res_contents.data() + old_res_contents_size, depth + 1);
            res.emplace_back(reinterpret_cast<const uint8_t *>(old_res_contents_size), depth + 1 + suffix_len);
            suffix_bit_pos += suffix_len_with_meta;
        }
        last_depth = depth;
    } while (it.depth_branch_.back().first != -1);

    // Ensure pointers are valid.
    for (uint32_t i = 0; i < res.size(); i++)
        res[i] = {res_contents.data() + (uint64_t) (res[i].str), res[i].length};

    return {std::move(res), std::move(res_contents)};
}


template <DivaType diva_type, PayloadType payload_type>
int32_t Diva<diva_type, payload_type>::Infix::GetLongestMatch(const InfiniteByteString key, 
                                                              uint32_t key_start_bit,
                                                              uint32_t slot_size) const {
    TrieIterator it(trie_.data());
    const bool has_prefix_keys = HasPrefixKeys();
    const uint32_t actual_suffix_len_backup = GetActualSuffixLen(slot_size);
    int32_t res = -1;
    int32_t last_depth = -1;
    if (num_trie_bits_ == 1) {  // Might have only one suffix, so there's nothing to traverse
        it.depth_branch_.clear();
        goto GetLongestMatchAfterLoop;
    }
    while (true) {
        it.Advance(has_prefix_keys);
        auto [depth, children] = it.depth_branch_.back();
        const int32_t current_str_bit_pos = key_start_bit + last_depth + 1;
        const uint32_t path_len = depth - last_depth - 1;
        if (path_len > 0) {
            const uint32_t compare_len = std::min<int32_t>(path_len,
                    std::max<int32_t>(0, key.length - last_depth));
            const int32_t compare = CompareStringToBitmap(it.buf_, it.bit_pos_ - compare_len,
                                                          key, current_str_bit_pos, 
                                                          compare_len);
            if (compare != 0)
                return res;
        }
        if (it.AtPrefixKey(has_prefix_keys)) {
            res = depth;
            it.Advance(has_prefix_keys);
            depth = it.depth_branch_.back().first;
            children = it.depth_branch_.back().second;
        }
        if (((children >> key.GetBitBitLength(key_start_bit + depth)) & 1) == 0)
            return res;
        if ((children & 1) == 1 && key.GetBitBitLength(key_start_bit + depth) == 1)
            it.SkipSubtree(has_prefix_keys);
        if (it.AtLeaf())
            break;
        last_depth = depth;
    }
GetLongestMatchAfterLoop:
    assert(it.depth_branch_.empty() || it.depth_branch_.back().first >= 0);
    uint32_t depth = (it.depth_branch_.empty() ? -1 : it.depth_branch_.back().first) + 1;

    // Check if suffixes match
    uint32_t suffix_bit_pos = GetSuffixBitPos(it.num_keys_read_, slot_size, actual_suffix_len_backup);
    auto [suffix_len, suffix_len_with_meta] =
        GetSuffixLength(suffix_bit_pos, slot_size, actual_suffix_len_backup);
    uint8_t suffix_contents[suffix_len / 8 + 8];
    memset(suffix_contents, 0, suffix_len / 8 + 8);
    const InfiniteByteString suffix = {suffix_contents, suffix_len};
    GetSuffixString(suffix_bit_pos, slot_size,
                    suffix_contents, 0,
                    actual_suffix_len_backup);
    for (uint32_t i = 0; i < suffix_len; i += 56) {
        const uint32_t bits_to_compare = std::min(56U, suffix_len - i);
        const uint64_t read_key = key.BitsAtBitLength(key_start_bit + depth + i,
                                                      bits_to_compare);
        const uint64_t read_suffix = suffix.BitsAtBitLength(i, bits_to_compare);
        const uint64_t diff = read_key ^ read_suffix;
        if (diff)
            return res;
        depth += bits_to_compare;
    }
    res = depth;
    return res;
}


template <DivaType diva_type, PayloadType payload_type>
void Diva<diva_type, payload_type>::Infix::AdaptTrie(const InfiniteByteString key, 
                                                     uint32_t key_start_bit,
                                                     uint32_t adapt_length,
                                                     uint32_t slot_size) {
    TrieIterator it(trie_.data());
    const bool has_prefix_keys = HasPrefixKeys();
    const uint32_t actual_suffix_len_backup = GetActualSuffixLen(slot_size);
    int32_t last_depth = -1;
    bool prefix_match = false, should_check_suffix = true;
    if (num_trie_bits_ == 1) {  // Might have only one suffix, so there's nothing to traverse
        it.depth_branch_.clear();
        goto AdaptTrieAfterLoop;
    }
    while (true) {
        it.Advance(has_prefix_keys);
        auto [depth, children] = it.depth_branch_.back();
        const int32_t current_str_bit_pos = key_start_bit + last_depth + 1;
        const uint32_t path_len = depth - last_depth - 1;
        if (path_len > 0) {
            const uint32_t compare_len = std::min<int32_t>(path_len,
                    std::max<int32_t>(0, key.length - last_depth));
            const int32_t compare = CompareStringToBitmap(it.buf_, it.bit_pos_ - compare_len,
                                                          key, current_str_bit_pos, 
                                                          compare_len);
            if (compare != 0) {
                assert(prefix_match);
                DeleteTrie(key, key_start_bit, slot_size);
                InsertTrie(key, key_start_bit, slot_size);
                AdaptTrie(key, key_start_bit, adapt_length, slot_size);
                return;
            }
        }
        if (it.AtPrefixKey(has_prefix_keys)) {
            prefix_match = true;
            it.Advance(has_prefix_keys);
            depth = it.depth_branch_.back().first;
            children = it.depth_branch_.back().second;
        }
        if ((children & 1) == 1 && key.GetBitBitLength(key_start_bit + depth) == 1)
            it.SkipSubtree(has_prefix_keys);
        else if (((children >> key.GetBitBitLength(key_start_bit + depth)) & 1) == 0) {
            should_check_suffix = false;
            break;
        }
        if (it.AtLeaf())
            break;
        last_depth = depth;
    }
AdaptTrieAfterLoop:
    assert(it.depth_branch_.empty() || it.depth_branch_.back().first >= 0);
    uint32_t depth = (it.depth_branch_.empty() ? -1 : it.depth_branch_.back().first) + 1;

    bool suffix_match = false;
    uint32_t suffix_bit_pos = 0, suffix_len = 0, suffix_len_with_meta = 0;
    if (should_check_suffix) {  // Check if suffixes match
        suffix_bit_pos = GetSuffixBitPos(it.num_keys_read_, slot_size, actual_suffix_len_backup);
        auto [suffix_len_, suffix_len_with_meta_] =
            GetSuffixLength(suffix_bit_pos, slot_size, actual_suffix_len_backup);
        suffix_len = suffix_len_;
        suffix_len_with_meta = suffix_len_with_meta_;
        uint8_t suffix_contents[suffix_len / 8 + 8];
        memset(suffix_contents, 0, suffix_len / 8 + 8);
        const InfiniteByteString suffix = {suffix_contents, suffix_len};
        GetSuffixString(suffix_bit_pos, slot_size,
                        suffix_contents, 0,
                        actual_suffix_len_backup);
        suffix_match = true;
        for (uint32_t i = 0; i < suffix_len; i += 56) {
            const uint32_t bits_to_compare = std::min(56U, suffix_len - i);
            const uint64_t read_key = key.BitsAtBitLength(key_start_bit + depth + i,
                                                          bits_to_compare);
            const uint64_t read_suffix = suffix.BitsAtBitLength(i, bits_to_compare);
            const uint64_t diff = read_key ^ read_suffix;
            if (diff) {
                suffix_match = false;
                break;
            }
        }
    }

    if (suffix_match) {
        if (depth + suffix_len < adapt_length) {    // Only update suffixes if we're actually extending something
            const uint32_t old_key_len = depth + suffix_len;

            // Find the last fragment written of the previous key
            uint32_t old_write_len = 0, write_len = GetActualSuffixLen(slot_size) - 1, encoding_bit_offset = 0;
            while (depth < old_key_len) {
                depth += write_len;
                encoding_bit_offset += write_len + 1;
                old_write_len = write_len;
                write_len = slot_size - 1;
            }
            if (old_write_len > 0) {
                write_len = old_write_len;
                depth -= write_len;
                encoding_bit_offset -= write_len + 1;
            }

            // Encode new fragments
            uint64_t encoded_suffix[(adapt_length - depth) / 32 + 1];
            memset(encoded_suffix, 0, ((adapt_length - depth) / 32 + 1) * sizeof(encoded_suffix[0]));
            uint32_t encoding_bit_pos = 0;
            while (depth <= adapt_length) {
                uint64_t data = key.BitsAtBitLength(key_start_bit + depth, write_len) | (1ULL << write_len);
                data >>= write_len - std::min(write_len, adapt_length - depth);
                write_bits_to_bitmap(encoded_suffix, encoding_bit_pos,
                        data, write_len + 1);
                encoding_bit_pos += write_len + 1;
                depth += write_len;
                write_len = slot_size - 1;
            }
            
            // Expand, shift, and write
            const uint32_t added_bit_count = encoding_bit_pos + encoding_bit_offset - suffix_len_with_meta;
            if (num_suffix_bits_ + added_bit_count >= 64 * trie_suffixes_.size()) {
                const uint32_t old_size = trie_suffixes_.size();
                trie_suffixes_.resize((num_suffix_bits_ + added_bit_count + 63) / 64);
                const uint32_t size_diff = (trie_suffixes_.size() - old_size) * sizeof(trie_suffixes_[0]);
                memset(trie_suffixes_.data() + old_size, 0, size_diff);
            }
            shift_bitmap_right(trie_suffixes_.data(),
                    suffix_bit_pos + encoding_bit_offset, num_suffix_bits_ - 1,
                    added_bit_count);
            copy_bitmap_to_bitmap(encoded_suffix, 0,
                    trie_suffixes_.data(), suffix_bit_pos + encoding_bit_offset,
                    encoding_bit_pos);
            num_suffix_bits_ += added_bit_count;
        }
    }
    else {
        assert(prefix_match);
        DeleteTrie(key, key_start_bit, slot_size);
        InsertTrie(key, key_start_bit, slot_size);
        AdaptTrie(key, key_start_bit, adapt_length, slot_size);
    }
}


template <DivaType diva_type, PayloadType payload_type>
std::vector<typename Diva<diva_type, payload_type>::Infix>
Diva<diva_type, payload_type>::Infix::SplitPrefixBits(uint32_t num_bits, uint32_t slot_size) const {
    const int32_t num_split_bits = num_bits;
    std::vector<Infix> res;
    if (num_trie_bits_ == 0) {  // No trie to split, so just sacrifice bits from the infix
        const uint32_t lb = lowbit_pos(infix_);
        res.emplace_back(num_split_bits <= 64 - lb ? (infix_ << num_split_bits) : (1UL << 63));
        return std::move(res);
    }
    else if (num_split_bits == 0) {
        res.emplace_back(*this);
        return std::move(res);
    }
    uint64_t current_prefix = 0;
    const uint64_t infix_without_age_shifted = num_split_bits < 64 ? (infix_ ^ 1) << num_split_bits : 0;

    TrieIterator it(trie_.data());
    const bool has_prefix_keys = HasPrefixKeys();
    const uint32_t actual_suffix_len = GetActualSuffixLen(slot_size);
    int32_t suffix_bit_pos = 0;
    do {
        const int32_t last_depth = it.depth_branch_.back().first;
        if (!it.AtLeaf())
            it.Advance(has_prefix_keys);
        auto [depth, children] = it.depth_branch_.back();
        if (depth >= num_split_bits - 64) {     // Update `current_prefix`
            if (last_depth < depth) {
                const int32_t path_len = depth - last_depth - 1 + std::min(0, num_split_bits - depth);
                const int32_t prefix_bit_pos = std::max(num_split_bits - depth, 0);
                copy_bitmap_to_bitmap(trie_.data(), it.bit_pos_ - path_len,
                                      &current_prefix, prefix_bit_pos + 1,
                                      std::min(std::max(0, 63 - prefix_bit_pos), path_len));
            }
            const int32_t shamt = std::min(63, std::max(0, num_split_bits - depth));
            current_prefix &= ~BITMASK(shamt + 1);
            current_prefix |= (depth < num_split_bits) ? (static_cast<uint64_t>((children & -children) - 1) << shamt) 
                                                       : 0UL;
        }

        // Add `current_prefix` to results if needed
        if (depth > num_split_bits - 1 || (depth == num_split_bits - 1 
                                            && (!it.AtLeaf() && !it.AtPrefixKey(has_prefix_keys)))) {   // Take whole subtrie(s)
            uint32_t total_skips = __builtin_popcountll(children);
            const uint32_t num_skips = (depth == num_split_bits - 1) ? 1 : 2;
            for (uint32_t skip_count = 0; skip_count < total_skips; skip_count += num_skips) {
                // Update `current_prefix` if skipped a subtrie already
                if (num_skips == 1 && skip_count == 1)
                    current_prefix |= 0b10UL;

                const uint32_t last_bit_pos = it.bit_pos_;
                const uint32_t last_num_keys_read = it.num_keys_read_;
                const uint32_t last_num_prefix_keys_read = it.num_prefix_keys_read_;
                const uint32_t last_suffix_bit_pos = suffix_bit_pos;
                if (it.AtPrefixKey(has_prefix_keys)) {
                    it.Advance(has_prefix_keys);
                    // Make sure to update `depth` and `children`, since we can't be over-skipping
                    depth = it.depth_branch_.back().first;
                    children = it.depth_branch_.back().second;
                    total_skips = __builtin_popcountll(children);
                }
                for (uint32_t i = 0; i < std::min(num_skips, total_skips); i++)
                    it.SkipSubtree(has_prefix_keys);

                // Figure out the position of suffixes we have to copy
                suffix_bit_pos = GetSuffixBitPos(it.num_keys_read_ - last_num_keys_read,
                                                 slot_size,
                                                 actual_suffix_len,
                                                 suffix_bit_pos);

                Infix new_infix(infix_without_age_shifted | current_prefix | 1UL);
                new_infix.num_prefix_keys_ = it.num_prefix_keys_read_ - last_num_prefix_keys_read + has_prefix_keys;
                if (depth >= num_split_bits) {      // Copy part of path that is a prefix of everything
                    const uint32_t path_len = depth - num_split_bits;
                    if (has_prefix_keys)
                        new_infix.AddCounterToTrie(path_len + 1);
                    else {
                        new_infix.AddBitsToTrie(nullptr, path_len + 1, 0);
                        new_infix.AddBitsToTrie(1, 1);
                    }
                    new_infix.AddBitsFromBitmapToTrie(trie_.data(),
                            path_len, last_bit_pos - (depth - last_depth - 1));
                }
                // Copy full subtrie
                new_infix.AddBitsFromBitmapToTrie(trie_.data(),
                        it.bit_pos_ - last_bit_pos, last_bit_pos);

                // Setup suffixes
                new_infix.num_suffixes_ = it.num_keys_read_ - last_num_keys_read;
                new_infix.num_suffix_bits_ = suffix_bit_pos - last_suffix_bit_pos;
                new_infix.trie_suffixes_.resize((new_infix.num_suffix_bits_ + 63) / 64);
                copy_bitmap_to_bitmap(trie_suffixes_.data(), last_suffix_bit_pos,
                                      new_infix.trie_suffixes_.data(), 0, 
                                      new_infix.num_suffix_bits_);

                const uint32_t new_actual_suffix_len = new_infix.GetActualSuffixLen(slot_size);
                const bool should_remove_trie = (new_infix.num_trie_bits_ == 1 && new_infix.num_suffixes_ == 1) 
                                                && new_infix.trie_suffixes_[0] == (actual_suffix_len == 1 ? 0UL : 1UL);
                if (!should_remove_trie) {
                    if (GetNumPrefixKeys() > 0 && new_infix.GetNumPrefixKeys() == 0) {  // Switch trie encoding if possible 
                        new_infix.SwitchTrieEncoding(false, slot_size,
                                actual_suffix_len);
                    }
                    else if (new_actual_suffix_len != actual_suffix_len) {  // Adjust actual suffix length if needed
                        new_infix.AdjustActualSuffixLen(actual_suffix_len,
                                new_actual_suffix_len, slot_size);
                    }
                }
                else {                                                      // Remove the trie and its suffixes if nothing there
                    new_infix.num_prefix_keys_ = 0;
                    new_infix.num_trie_bits_ = 0;
                    new_infix.trie_.clear();
                    new_infix.num_suffixes_ = 0;
                    new_infix.num_suffix_bits_ = 0;
                    new_infix.trie_suffixes_.clear();
                }

                res.push_back(new_infix);
            }
        }
        else if (it.AtLeaf()) {
            const auto [suffix_len, suffix_len_with_meta] = 
                GetSuffixLength(suffix_bit_pos, slot_size, actual_suffix_len);
            uint8_t suffix_contents[suffix_len / 8 + 8];
            memset(suffix_contents, 0, suffix_len / 8 + 8);
            const InfiniteByteString suffix = {suffix_contents, suffix_len};
            GetSuffixString(suffix_bit_pos, slot_size, suffix_contents, 0, actual_suffix_len);

            // Puts relevant part of the suffix into `current_prefix`
            const int32_t shamt = std::min(63, num_split_bits - depth - 1);
            current_prefix &= ~BITMASK(shamt + 1);
            const int32_t suffix_bit_count = depth + static_cast<int32_t>(suffix_len) + 1 - num_split_bits;
            current_prefix |= suffix.BitsAtBitLength(num_split_bits - depth - shamt - 1,
                                                     shamt + 1);

            Infix new_infix(infix_without_age_shifted | current_prefix |
                    (1UL << std::max(std::min(63, -suffix_bit_count), 0)));
            if (suffix_bit_count > 0) {   // Has a single suffix to add
                new_infix.AddBitsToTrie(1, 1);
                new_infix.AddSuffixToTrie(suffix,
                        suffix_len, suffix_len - suffix_bit_count,
                        slot_size, slot_size - 1);
            }
            res.push_back(new_infix);
            it.Advance(has_prefix_keys);
            suffix_bit_pos += suffix_len_with_meta;
        }
        else if (it.AtPrefixKey(has_prefix_keys)) {
            Infix new_infix(infix_without_age_shifted | current_prefix 
                            | (1UL << std::min(63, num_split_bits - depth)));
            res.push_back(new_infix);
            it.Advance(has_prefix_keys);
        }

        // Update current_prefix
        const auto [current_prefix_update_depth, current_prefix_update_children] = it.depth_branch_.back();
        const int32_t current_prefix_update_shamt = std::min(63, num_split_bits - current_prefix_update_depth);
        current_prefix &= ~BITMASK(current_prefix_update_shamt + 1);
        current_prefix |= ((current_prefix_update_children & -current_prefix_update_children) - 1) 
                                << current_prefix_update_shamt;
    } while (it.depth_branch_.back().first != -1);

    return std::move(res);
}


template <DivaType diva_type, PayloadType payload_type>
void Diva<diva_type, payload_type>::Infix::Merge(const Infix& other, uint32_t slot_size) {
    auto [keys, keys_contents] = GetStrings(slot_size);
    auto [other_keys, other_keys_contents] = other.GetStrings(slot_size);
    keys.insert(keys.end(), other_keys.begin(), other_keys.end());
    BuildTrieAndSuffixes(keys.data(), keys.size(),
                         0, slot_size,
                         false, true);
}


template <DivaType diva_type, PayloadType payload_type>
void Diva<diva_type, payload_type>::Infix::PrependPrefix(const InfiniteByteString prefix,
                                                         uint32_t prefix_offset,
                                                         uint32_t prefix_len,
                                                         uint32_t infix_len,
                                                         uint32_t slot_size) {
    if (prefix_len == 0)
        return;
    const uint32_t old_actual_suffix_len = num_trie_bits_ > 0 ? GetActualSuffixLen(slot_size) : 0;

    // Create the InfiniteByteString that includes `infix_`
    uint8_t trie_prefix_contents[prefix_len / 8 + sizeof(infix_) + 2];
    memset(trie_prefix_contents, 0, prefix_len / 8 + sizeof(infix_) + 2);
    const int32_t lowbit_position = lowbit_pos(infix_);
    const uint32_t last_byte_bits = (prefix_offset + prefix_len) % 8;
    const uint64_t infix_prefix = __builtin_bswap64(infix_ << (64 - last_byte_bits - infix_len))
        | (prefix.BitsAtBitLength(prefix_offset + prefix_len - last_byte_bits, last_byte_bits) 
                << (8 - last_byte_bits));
    memcpy(trie_prefix_contents,
           prefix.str + prefix_offset / 8,
           std::min((prefix_offset % 8 + prefix_len + 7) / 8,
                       (prefix.length - prefix_offset + prefix_offset % 8 + 7) / 8));
    memcpy(trie_prefix_contents + (prefix_offset % 8 + prefix_len) / 8,
           &infix_prefix,
           sizeof(infix_prefix));
    prefix_offset %= 8;
    InfiniteByteString trie_prefix = {trie_prefix_contents,
        prefix_len + prefix_offset + infix_len - lowbit_position - 1};

    // Update `infix_` first
    infix_ = (prefix_len < infix_len ? (infix_ >> prefix_len) : 0UL)
           | (trie_prefix.BitsAtBitLength(prefix_offset, std::min(prefix_len, infix_len)) 
                   << (prefix_len < infix_len ? infix_len - prefix_len : 0))
           | (prefix_len >= lowbit_position);
    prefix_offset += infix_len - 1;
    
    // Update the trie and the suffixes
    if (num_trie_bits_ == 0 && prefix_len > lowbit_position) {  // Create a trie to store the last bits as a suffix
        prefix_len -= lowbit_position;
        AddBitsToTrie(1, 1);
        AddSuffixToTrie(trie_prefix,
                prefix_offset + prefix_len, prefix_offset,
                slot_size, slot_size - 1);
    }
    else if (num_trie_bits_ == 1 && trie_[0] == 1) {            // Have a single leaf in the trie, so prepend to suffix
        const auto [suffix_len, suffix_len_with_meta] = GetSuffixLength(0, slot_size);
        const uint32_t new_suffix_len = prefix_len + suffix_len;
        uint8_t new_suffix_contents[suffix_len / 8 + 8];
        memset(new_suffix_contents, 0, suffix_len / 8 + 8);
        const uint32_t actual_suffix_len = GetActualSuffixLen(slot_size);
        GetSuffixString(0, slot_size, new_suffix_contents, prefix_len,
                actual_suffix_len);
        for (uint32_t i = 0; i < prefix_len; i += std::min(64U, prefix_len)) {
            const uint32_t bits_to_copy = std::min(64U, prefix_len);
            const uint64_t data = __builtin_bswap64(trie_prefix.BitsAtBitLength(prefix_offset + i, bits_to_copy) 
                                                        << (64 - bits_to_copy));
            reinterpret_cast<uint64_t *>(new_suffix_contents)[i / 64] |= data;
        }
        num_suffixes_ = 0;
        num_suffix_bits_ = 0;
        trie_suffixes_.clear();
        const uint32_t added_bit_count = actual_suffix_len + (new_suffix_len >= actual_suffix_len - 1 ? ((new_suffix_len - actual_suffix_len + 1) 
                                                                                                            / (slot_size - 1) + 1) * slot_size
                                                                                         : 0);
        if (num_suffix_bits_ + added_bit_count >= 64 * trie_suffixes_.size()) {
            const uint32_t old_size = trie_suffixes_.size();
            trie_suffixes_.resize((num_suffix_bits_ + added_bit_count + 63) / 64);
            const uint32_t size_diff = sizeof(trie_suffixes_[0]) * (trie_suffixes_.size() - old_size);
            memset(trie_suffixes_.data() + old_size, 0, size_diff);
        }
        WriteSuffixToSuffixes({new_suffix_contents, new_suffix_len}, new_suffix_len, 0, 
                              num_suffix_bits_, actual_suffix_len, slot_size);
        num_suffix_bits_ += added_bit_count;
        num_suffixes_++;
    }
    else if (num_trie_bits_ > 0)
        PrependPrefixToTrie(trie_prefix, prefix_offset, prefix_len);

    if (num_suffixes_ > 0) {
        // Adjust suffix sizes if needed
        const uint32_t new_actual_suffix_len = GetActualSuffixLen(slot_size);
        if (old_actual_suffix_len > 0 && old_actual_suffix_len != new_actual_suffix_len)
            AdjustActualSuffixLen(old_actual_suffix_len, new_actual_suffix_len, slot_size);
    }
}

template <DivaType diva_type, PayloadType payload_type>
void Diva<diva_type, payload_type>::Infix::PrependPrefixToTrie(const InfiniteByteString prefix,
                                                               uint32_t prefix_offset,
                                                               uint32_t prefix_len) {
    uint32_t lowbit_bit_pos = 0;
    while ((trie_[lowbit_bit_pos / 64] >> (lowbit_bit_pos % 64)) == 0)
        lowbit_bit_pos += 64 - lowbit_bit_pos % 64;
    lowbit_bit_pos += lowbit_pos(trie_[lowbit_bit_pos / 64] >> (lowbit_bit_pos % 64));
    uint32_t prepend_start_pos = lowbit_bit_pos + 1;

    // Update metadata
    if (!HasPrefixKeys()) {
        if (const uint32_t added_bit_count = 2 * prefix_len; 
                num_trie_bits_ + added_bit_count >= trie_.size() * 64) {
            const uint32_t old_size = trie_.size();
            trie_.resize((num_trie_bits_ + added_bit_count + 63) / 64);
            const uint32_t size_diff = sizeof(trie_[0]) * (trie_.size() - old_size);
            memset(trie_.data() + old_size, 0, size_diff);
        }
        prepend_start_pos += lowbit_bit_pos - 1;
        shift_bitmap_right(trie_.data(),
                           prepend_start_pos, num_trie_bits_ - 1,
                           prefix_len);
        num_trie_bits_ += prefix_len;
        shift_bitmap_right(trie_.data(),
                           0, num_trie_bits_ - 1,
                           prefix_len);
        num_trie_bits_ += prefix_len;
        prepend_start_pos += prefix_len;
    }
    else {
        uint64_t counter = 0, pw = 1, read_buf = 0;
        uint32_t read_buf_filled_len = 0, read_bit_pos = lowbit_bit_pos + 1;
        const uint32_t num_fragments_to_read = lowbit_bit_pos;
        for (uint32_t i = 0; i < num_fragments_to_read; i++) {
            counter += read_data_from_bitmap(trie_.data(), read_bit_pos,
                    read_buf, read_buf_filled_len,
                    varlen_counter_encoding_fragment_length) * pw;
            pw <<= varlen_counter_encoding_fragment_length;
            prepend_start_pos += varlen_counter_encoding_fragment_length;
        }
        counter += prefix_len;
        const uint32_t new_fragment_count = GetCounterDigitCount(counter);
        const uint32_t added_bit_count = prefix_len + (new_fragment_count - num_fragments_to_read) 
                                                        * (varlen_counter_encoding_fragment_length + 1);
        if (num_trie_bits_ + added_bit_count >= trie_.size() * 64) {
            const uint32_t old_size = trie_.size();
            trie_.resize((num_trie_bits_ + added_bit_count + 63) / 64);
            const uint32_t size_diff = sizeof(trie_[0]) * (trie_.size() - old_size);
            memset(trie_.data() + old_size, 0, size_diff);
        }

        prepend_start_pos += counter - 1 - prefix_len;
        shift_bitmap_right(trie_.data(),
                           prepend_start_pos, num_trie_bits_ - 1,
                           prefix_len);
        num_trie_bits_ += prefix_len;

        shift_bitmap_right(trie_.data(),
                           0, num_trie_bits_ - 1,
                           added_bit_count - prefix_len);
        prepend_start_pos += added_bit_count - prefix_len;
        num_trie_bits_ += added_bit_count - prefix_len;

        write_varlen_counter_to_bitmap(trie_.data(), 0,
                counter, varlen_counter_encoding_fragment_length);
    }

    // Prepend suffix
    uint32_t write_bit_pos = prepend_start_pos + prefix_len;
    for (uint32_t i = 0; i < prefix_len; i += std::min(64U, prefix_len - i)) {
        const uint32_t write_amount = std::min(64U, prefix_len - i);
        write_bit_pos -= write_amount;
        const uint64_t data = prefix.BitsAtBitLength(prefix_offset + i, write_amount);
        write_bits_to_bitmap(trie_.data(), write_bit_pos, data, write_amount);
    }
}


template <DivaType diva_type, PayloadType payload_type>
void Diva<diva_type, payload_type>::Infix::SerializeToInfixStore(InfixStore& store,
                                                                 uint32_t pos,
                                                                 uint32_t store_size,
                                                                 uint32_t slot_size) const {
    const uint32_t bit_pos = infix_store_target_size + store_size + slot_size * pos;
    SerializeToPtr(store.ptr + num_metadata_offset_words, bit_pos, slot_size);
}


template <DivaType diva_type, PayloadType payload_type>
void Diva<diva_type, payload_type>::Infix::SerializeToPtr(void *ptr,
                                                          uint32_t bit_pos,
                                                          uint32_t slot_size) const {
    const uint32_t original_bit_pos = bit_pos;
    const uint64_t explicit_part = infix_ & BITMASK(slot_size);
    write_bits_to_bitmap(ptr, bit_pos,
                         explicit_part, slot_size);
    bit_pos += slot_size;
    if (num_trie_bits_ == 0)
        return;

    const uint32_t escape_sequence_cost = slot_size - highbit_pos(explicit_part);
    write_bits_to_bitmap(ptr, bit_pos, 
                         static_cast<uint64_t>(HasPrefixKeys()) << escape_sequence_cost,
                         escape_sequence_cost + 1);
    bit_pos += escape_sequence_cost + 1;
    copy_bitmap_to_bitmap(trie_.data(), 0, 
                          ptr, bit_pos,
                          num_trie_bits_);
    bit_pos += num_trie_bits_;
    copy_bitmap_to_bitmap(trie_suffixes_.data(), 0, 
                          ptr, bit_pos,
                          num_suffix_bits_);
    bit_pos += num_suffix_bits_;
    const uint32_t bit_pos_rem = (bit_pos - original_bit_pos) % slot_size;
    write_bits_to_bitmap(ptr, bit_pos,
                         0, (bit_pos_rem == 0 ? 0 : slot_size - bit_pos_rem));

    if (explicit_part != 1) {
        // Convert first slot to the correct format with an escape sequence
        uint64_t read_buf;
        uint32_t read_buf_filled_len = 0, read_bit_pos = original_bit_pos + slot_size;
        const uint64_t second_slot = read_data_from_bitmap(ptr, read_bit_pos,
                read_buf, read_buf_filled_len,
                slot_size);
        write_bits_to_bitmap(ptr, original_bit_pos + slot_size,
                second_slot >> escape_sequence_cost, slot_size);
    }
}


template <DivaType diva_type, PayloadType payload_type>
void Diva<diva_type, payload_type>::Infix::DeserializeFromPtr(void *ptr,
                                                              uint32_t bit_pos,
                                                              uint32_t slot_size) {
    // TODO: This has concurrency issues with the modification it makes to ptr. Make it work without modifying ptr.
    uint64_t read_buf;
    uint32_t read_buf_filled_len = 0, read_bit_pos = bit_pos;
    infix_ = read_data_from_bitmap(ptr, read_bit_pos,
            read_buf, read_buf_filled_len,
            slot_size);
    const uint64_t second_slot_backup = read_data_from_bitmap(ptr, read_bit_pos,
            read_buf, read_buf_filled_len,
            slot_size);
    uint64_t second_slot = second_slot_backup;
    if (second_slot != 0 && (second_slot | (second_slot - 1)) >= infix_)
        return;
    const uint64_t escape_sequence_cost = slot_size - highbit_pos(infix_);

    bool has_prefix_keys = false;
    uint32_t trie_start;
    if (infix_ != 1) {
        has_prefix_keys = second_slot & 1;
        // Change bitmap to ease parsing
        second_slot <<= escape_sequence_cost;
        write_bits_to_bitmap(ptr, bit_pos + slot_size, second_slot, slot_size);
        trie_start = bit_pos + slot_size + escape_sequence_cost + 1;
    }
    else {
        has_prefix_keys = read_data_from_bitmap(ptr, read_bit_pos,
            read_buf, read_buf_filled_len,
            1);
        trie_start = bit_pos + 2 * slot_size + 1;
    }

    // Parse trie
    TrieIterator it(ptr);
    it.bit_pos_ = trie_start;
    do {
        it.Advance(has_prefix_keys);
    } while (it.depth_branch_.back().first != -1);

    // Setup infix
    num_prefix_keys_ = has_prefix_keys + it.num_prefix_keys_read_;
    num_trie_bits_ = it.bit_pos_ - trie_start;
    trie_.resize((num_trie_bits_ + 63) / 64);
    memset(trie_.data(), 0, trie_.size() * sizeof(trie_[0]));
    copy_bitmap_to_bitmap(ptr, trie_start, trie_.data(), 0, num_trie_bits_);
    num_suffixes_ = it.num_keys_read_;
    num_suffix_bits_ = GetSuffixBitPos(reinterpret_cast<const uint64_t *>(ptr), num_suffixes_, 
                                       slot_size, GetActualSuffixLen(slot_size), it.bit_pos_) - it.bit_pos_;
    trie_suffixes_.resize((num_suffix_bits_ + 63) / 64);
    memset(trie_suffixes_.data(), 0, trie_suffixes_.size() * sizeof(trie_suffixes_[0]));
    copy_bitmap_to_bitmap(ptr, it.bit_pos_,
                          trie_suffixes_.data(), 0,
                          num_suffix_bits_);

    if (infix_ != 1) {  // Revert changes
        write_bits_to_bitmap(ptr, bit_pos + slot_size,
                second_slot_backup, slot_size);
    }
}


template <DivaType diva_type, PayloadType payload_type>
uint32_t Diva<diva_type, payload_type>::Infix::GetNumSlots(uint32_t slot_size) const {
    if (num_trie_bits_ == 0)
        return 1;
    const uint32_t escape_sequence_cost = slot_size - highbit_pos(infix_ & BITMASK(slot_size));
    const uint64_t total_bit_count = slot_size + escape_sequence_cost + 1 + num_trie_bits_ + num_suffix_bits_;
    return (total_bit_count + slot_size - 1) / slot_size;
}


/*****************************************************************************
 **                              Trie Iterator                              **
 *****************************************************************************/
template <DivaType diva_type, PayloadType payload_type>
inline Diva<diva_type, payload_type>::Infix::TrieIterator::TrieIterator(const void *buf):
        buf_(reinterpret_cast<const uint64_t *>(buf)) {
    depth_branch_.reserve(depth_reserve_size);
}

template <DivaType diva_type, PayloadType payload_type>
inline Diva<diva_type, payload_type>::Infix::TrieIterator::TrieIterator(const Infix *infix):
        buf_(infix->trie_.data()) {
    depth_branch_.reserve(depth_reserve_size);
}

template <DivaType diva_type, PayloadType payload_type>
inline Diva<diva_type, payload_type>::Infix::TrieIterator::TrieIterator(const Infix& infix): 
        buf_(infix.trie_.data()) {
    depth_branch_.reserve(depth_reserve_size);
}

template <DivaType diva_type, PayloadType payload_type>
inline Diva<diva_type, payload_type>::Infix::TrieIterator::TrieIterator(const TrieIterator& other):
        buf_(other.buf_),
        num_prefix_keys_read_(other.num_prefix_keys_read_),
        num_keys_read_(other.num_keys_read_),
        bit_pos_(other.bit_pos_),
        depth_branch_(other.depth_branch_) { 
    depth_branch_.reserve(depth_reserve_size);
}

template <DivaType diva_type, PayloadType payload_type>
inline typename Diva<diva_type, payload_type>::Infix::TrieIterator& 
Diva<diva_type, payload_type>::Infix::TrieIterator::operator=(const TrieIterator& other) {
    buf_ = other.buf_;
    num_prefix_keys_read_ = other.num_prefix_keys_read_;
    num_keys_read_ = other.num_keys_read_;
    bit_pos_ = other.bit_pos_;
    depth_branch_ = other.depth_branch_;
}


template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::Infix::TrieIterator::Advance(bool has_prefix_keys) {
    const bool is_leaf = AtLeaf();
    bit_pos_++;
    if (is_leaf) {
        while (!depth_branch_.empty()) {
            if (depth_branch_.back().second == 0b11) {
                depth_branch_[depth_branch_.size() - 1].second = 0b10;
                break;
            }
            depth_branch_.pop_back();
        }
        num_keys_read_++;
        return;
    }

    uint32_t advance_bits = bit_pos_;
    while ((buf_[bit_pos_ / 64] >> (bit_pos_ % 64)) == 0)
        bit_pos_ += 64 - bit_pos_ % 64;
    bit_pos_ += lowbit_pos(buf_[bit_pos_ / 64] >> (bit_pos_ % 64)) + 1;
    advance_bits = bit_pos_ - advance_bits - 1;
    if (has_prefix_keys) {
        uint64_t counter = 0, pw = 1, read_buf = 0;
        uint32_t read_buf_filled_len = 0, read_bit_pos = bit_pos_;
        const uint32_t num_fragments_to_read = advance_bits + 1;
        for (uint32_t i = 0; i < num_fragments_to_read; i++) {
            counter += read_data_from_bitmap(buf_, read_bit_pos,
                    read_buf, read_buf_filled_len,
                    varlen_counter_encoding_fragment_length) * pw;
            pw <<= varlen_counter_encoding_fragment_length;
            bit_pos_ += varlen_counter_encoding_fragment_length;
        }
        if (counter == 0) {
            num_prefix_keys_read_++;
            depth_branch_[depth_branch_.size() - 1].second = read_data_from_bitmap(buf_, read_bit_pos,
                    read_buf, read_buf_filled_len, 
                    2);
            bit_pos_ += 2;
            return;
        }
        bit_pos_ += counter - 1;
        depth_branch_.emplace_back(depth_branch_.back().first + counter, 0b11);
    }
    else {
        bit_pos_ += advance_bits;
        depth_branch_.emplace_back(depth_branch_.back().first + advance_bits + 1, 0b11);
    }
}


template <DivaType diva_type, PayloadType payload_type>
inline bool Diva<diva_type, payload_type>::Infix::TrieIterator::AtLeaf() {
    return (buf_[bit_pos_ / 64] >> (bit_pos_ % 64)) & 1;
}


template <DivaType diva_type, PayloadType payload_type>
inline bool Diva<diva_type, payload_type>::Infix::TrieIterator::AtPrefixKey(bool has_prefix_keys) {
    if (!has_prefix_keys || AtLeaf())
        return false;
    uint64_t read_buf = 0;
    uint32_t read_buf_filled_len = 0, read_bit_pos = bit_pos_;
    const uint64_t data = read_data_from_bitmap(buf_, read_bit_pos, read_buf, read_buf_filled_len, 2 + varlen_counter_encoding_fragment_length);
    return data == 0b10UL;
}


template <DivaType diva_type, PayloadType payload_type>
inline void Diva<diva_type, payload_type>::Infix::TrieIterator::SkipSubtree(bool has_prefix_keys) {
    const int32_t original_depth = depth_branch_.back().first;
    do {
        Advance(has_prefix_keys);
    } while (original_depth < depth_branch_.back().first);
}

}


