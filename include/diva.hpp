#pragma once

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <endian.h>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <mutex>
#include <random>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <x86intrin.h>

#include "wormhole/wh.h"
#include "util.hpp"
#include "wormhole/wh_int.h"


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

enum PayloadType {
    None,
    FixedLength,
    VarLength
};

template <bool int_optimized=false, PayloadType payload_type=PayloadType::None>
class Diva {
    friend class Iterator;
    friend class DivaTests;
    friend class InfixStoreTests;

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

        __attribute__((always_inline))
        uint64_t WordAt(const uint32_t byte_pos) const {
            if (byte_pos >= length)
                return 0;
            uint64_t res = 0;
            memcpy(&res, str + byte_pos, std::min<uint32_t>(sizeof(res), length - byte_pos));
            return __builtin_bswap64(res);
        };

        __attribute__((always_inline))
        uint64_t BitsAt(const uint32_t bit_pos, const uint32_t res_width) const {
            if (bit_pos / 8 >= length)
                return 0;
            uint64_t res = 0;
            memcpy(&res, str + bit_pos / 8, std::min<uint32_t>(sizeof(res), length - bit_pos / 8));
            res = __builtin_bswap64(res) >> (8 * sizeof(res) - res_width - bit_pos % 8);
            return res & BITMASK(res_width);
        };

        __attribute__((always_inline))
        uint32_t GetBit(const uint32_t pos) const {
            return (pos / 8 < length ? (str[pos / 8] >> (7 - pos % 8)) & 1 : 0);
        };

        __attribute__((always_inline))
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
        friend class Diva<int_optimized, payload_type>;
        friend class DivaTests;

        using KeyType = std::conditional_t<int_optimized, uint64_t, std::string>;

    public:
        Iterator(Diva<int_optimized, payload_type> *parent):
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
        Diva<int_optimized, payload_type> *filter_;
        InfiniteByteString next_to_fetch_, end_key_;
        uint32_t shared_, ignore_, implicit_;
        uint8_t next_to_fetch_contents_[iterator_local_buf_len], end_key_contents_[iterator_local_buf_len];
        uint8_t shared_prefix_[iterator_local_buf_len], current_key_contents_[iterator_local_buf_len];
        std::vector<uint64_t> infixes_, bit_counts_, payloads_;
        uint32_t ind_ = 0;
        std::function<bool(const uint64_t *)> should_remove_;
        bool first_store_to_fetched_and_delete_ = true;

        Iterator(Diva<int_optimized, payload_type> *parent,
                 std::string_view start, std::string_view end,
                 std::function<bool(const uint64_t *)> should_remove=nullptr);
        Iterator(Diva<int_optimized, payload_type> *parent, 
                 const uint8_t *start, uint32_t start_len, 
                 const uint8_t *end, uint32_t end_len, 
                 std::function<bool(const uint64_t *)> should_remove=nullptr);
        Iterator(Diva<int_optimized, payload_type> *parent,
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
    static constexpr uint32_t infix_store_target_size = 1024;
    static_assert(infix_store_target_size % 64 == 0);
    static constexpr uint32_t base_implicit_size = __builtin_ctz(infix_store_target_size);
    static constexpr uint32_t scale_shift = 15;
    static constexpr uint32_t scale_implicit_shift = 15;
    static constexpr uint32_t size_scalar_count = 500;
    static constexpr uint32_t heap_alloc_threshold = 20000U;
    static constexpr uint64_t max_exp_backoff = BITMASK(14);

    struct InfixStore {
        static const uint32_t size_grade_bit_count = 8;
        static const uint32_t elem_count_bit_count = 20;

        uint32_t status = 0;
        uint16_t num_sample_payloads = 0;
        std::atomic<lock_t> rwlock {0};
        uint64_t *ptr = nullptr;

        InfixStore(const uint32_t slot_count, const uint32_t slot_size,
                   const uint32_t size_grade, const uint32_t payload_size=0) {
            SetSizeGrade(size_grade);
            const uint32_t word_count = GetPtrWordCount(slot_count, slot_size, payload_size);
            rwlock.store(0, std::memory_order::memory_order_release);
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
            rwlock.store(0, std::memory_order::memory_order_release);
        }
        InfixStore(InfixStore &&other) = default;
        InfixStore &operator=(const InfixStore &other) = default;

        void Reset(const uint32_t slot_count, const uint32_t slot_size, const uint32_t payload_size=0) {
#ifdef DEBUG
            assert(ptr);
#endif // DEBUG
            memset(ptr, 0, GetPtrWordCount(slot_count, slot_size, payload_size) * sizeof(uint64_t));
        }

        static uint32_t GetPtrWordCount(const uint32_t slot_count, const uint32_t slot_size, const uint32_t payload_size=0) {
            if constexpr (payload_type == PayloadType::FixedLength)
                return 2 + (Diva::infix_store_target_size + slot_count * (slot_size + 1 + payload_size) + 64 + 63) / 64;
            return 2 + (Diva::infix_store_target_size + slot_count * (slot_size + 1) + 63) / 64;
        }

        uint32_t GetElemCount() const {
            return status & BITMASK(elem_count_bit_count);
        }

        void SetElemCount(const int32_t elem_count) {
            status &= ~BITMASK(elem_count_bit_count);
            status |= elem_count;
        }

        void UpdateElemCount(const int32_t delta) {
            status += delta;
        }

        uint32_t GetSizeGrade() const {
            return (status >> elem_count_bit_count) & BITMASK(size_grade_bit_count);
        }

        void SetSizeGrade(const uint32_t size_grade) {
            status &= ~(BITMASK(size_grade_bit_count) << elem_count_bit_count);
            status |= size_grade << elem_count_bit_count;
        }

        uint32_t GetInvalidBits() const {
            return status >> (elem_count_bit_count + size_grade_bit_count) & 7U;
        }

        void SetInvalidBits(const uint32_t invalid_bits) {
            status &= ~(7U << (elem_count_bit_count + size_grade_bit_count));
            status |= invalid_bits << (elem_count_bit_count + size_grade_bit_count);
        }

        bool IsPartialKey() const {
            return status >> 31;
        }

        void SetPartialKey(bool val) {
            if (val)
                status |= (1U << 31);
            else 
                status &= ~(1U << 31);
        }
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
    void InsertSplit(const InfiniteByteString key, const void *payload=nullptr);
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
                             Diva<int_optimized, payload_type>::InfixStore *& infix_store_ptr) const;
    void DeleteGetLowerMiddleUpperBounds(const InfiniteByteString& key, void *leaves[3],
                                   wormhole_iter& it, wormhole_int_iter& it_int,
                                   InfiniteByteString& left_key, InfiniteByteString& middle_key, InfiniteByteString& right_key,
                                   Diva<int_optimized, payload_type>::InfixStore *& left_store_ptr,
                                   Diva<int_optimized, payload_type>::InfixStore *& right_store_ptr) const;
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
    void GetSamplePayload(InfixStore &store, const uint32_t pos, uint64_t *payload, const uint32_t payload_offset=0) const;
    void SetPayload(InfixStore &store, const uint32_t pos, const uint64_t *payload, const uint32_t payload_offset=0);
    void AddSamplePayload(InfixStore &store, const void *payload, const uint32_t payload_offset=0);
    void RemoveSamplePayload(InfixStore &store, const uint32_t pos);

    // Works with the inclusive-exclusive range [`l`, `r`).
    void ShiftSlotsRight(const InfixStore &store, const uint32_t l, const uint32_t r, const uint32_t shamt);
    // Works with the inclusive-exclusive range [`l`, `r`).
    void ShiftSlotsLeft(const InfixStore &store, const uint32_t l, const uint32_t r, const uint32_t shamt);
    // Works with the inclusive-exclusive range [`l`, `r`).
    void ShiftRunendsRight(const InfixStore &store, const uint32_t l, const uint32_t r, const uint32_t shamt);
    // Works with the inclusive-exclusive range [`l`, `r`).
    void ShiftRunendsLeft(const InfixStore &store, const uint32_t l, const uint32_t r, const uint32_t shamt);
    // Works with the inclusive-exclusive range [`l`, `r`).
    void ShiftPayloadsRight(const InfixStore &store, const uint32_t l, const uint32_t r, const uint32_t shamt);
    // Works with the inclusive-exclusive range [`l`, `r`).
    void ShiftPayloadsLeft(const InfixStore &store, const uint32_t l, const uint32_t r, const uint32_t shamt);

    // Works with the inclusive-exclusive range [`l`, `r`).
    void MoveSlotsRight(const InfixStore &store, const uint32_t l, const uint32_t r, const uint32_t shamt);
    // Works with the inclusive-exclusive range [`l`, `r`).
    void MoveSlotsLeft(const InfixStore &store, const uint32_t l, const uint32_t r, const uint32_t shamt);
    // Works with the inclusive-exclusive range [`l`, `r`).
    void MovePayloadsRight(const InfixStore &store, const uint32_t l, const uint32_t r, const uint32_t shamt);
    // Works with the inclusive-exclusive range [`l`, `r`).
    void MovePayloadsLeft(const InfixStore &store, const uint32_t l, const uint32_t r, const uint32_t shamt);

    // Works with the inclusive-exclusive range [`l`, `r`).
    void ZeroOutSlots(const InfixStore &store, const uint32_t l, const uint32_t r);
    // Works with the inclusive-exclusive range [`l`, `r`).
    void ZeroOutPayloads(const InfixStore &store, const uint32_t l, const uint32_t r);


    int32_t FindEmptySlotAfter(const InfixStore &store, const uint32_t runend_pos) const;
    int32_t FindEmptySlotBefore(const InfixStore &store, const uint32_t runend_pos) const;
    void InsertRawIntoInfixStore(InfixStore &store, const uint64_t key,
                                 const uint32_t total_implicit=infix_store_target_size,
                                 const uint64_t *payload=nullptr);
    // Assumes that `key` is a full length infix.
    void DeleteRawFromInfixStore(InfixStore &store, const uint64_t key,
                                 const uint32_t total_implicit=infix_store_target_size,
                                 std::function<bool(const uint64_t *)> should_remove=nullptr);
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
    uint32_t GetLongestMatchingInfixSize(const InfixStore &store, const uint64_t key,
                                         const uint32_t total_implicit=infix_store_target_size,
                                         std::function<bool(const uint64_t*)> should_consider=nullptr) const;
    // Assumes that `l_key` and `r_key` are full length infixes. Returnes true
    // if there is an infix that may lie within the range [`l_key`, `r_key`].
    bool RangeQueryInfixStore(InfixStore &store, const uint64_t l_key, const uint64_t r_key,
                              const uint32_t total_implicit=infix_store_target_size) const;
    // Assumes that `key` is a full length infix. Returns true if there is an
    // infix that may match `key`.
    bool PointQueryInfixStore(InfixStore &store, const uint64_t key,
                              const uint32_t total_implicit=infix_store_target_size) const;
    // Resizes the infix store to the appropriate size based on its number of
    // elements.
    void ResizeInfixStore(InfixStore &store, const uint32_t total_implicit=infix_store_target_size);
    void ShrinkInfixStoreInfixSize(InfixStore &store, const uint32_t new_infix_size);
    void LoadListToInfixStore(InfixStore &store, const uint64_t *list, const uint32_t list_len,
                              const uint32_t total_implicit=infix_store_target_size, const bool zero_out=false,
                              const uint64_t *payload_list=nullptr);
    InfixStore AllocateInfixStoreWithList(const uint64_t *list, const uint32_t list_len,
                                          const uint32_t total_implicit=infix_store_target_size,
                                          const uint64_t *payload_list=nullptr);
    uint32_t GetInfixList(const InfixStore &store, uint64_t *res, uint64_t *res_payload=nullptr) const;
    std::tuple<uint32_t, bool> GetExpandedInfixListLength(const uint64_t *list, const uint32_t list_len,
                                                          const uint32_t implicit_size, const uint32_t shamt,
                                                          const uint64_t lower_lim, const uint64_t upper_lim);
    void UpdateInfixList(const uint64_t *list, const uint32_t list_len, const uint32_t shamt, 
                         const uint64_t lower_lim, const uint64_t upper_lim,
                         uint64_t *res, const uint32_t res_len, 
                         const bool expanded,
                         const uint64_t *payload_list=nullptr, const uint32_t payload_list_offset=0,
                         uint64_t *res_payload=nullptr) const;
    void UpdateInfixListDelete(const uint32_t shared, const uint32_t ignore, const uint32_t implicit_size,
                               const InfiniteByteString left_key, const InfiniteByteString right_key,
                               uint64_t *infix_list, const uint32_t infix_list_len);

    bool CompareInfixes(uint64_t a, uint64_t b) const;

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


template <bool int_optimized, PayloadType payload_type>
inline Diva<int_optimized, payload_type>::Diva(const uint32_t infix_size, const uint32_t rng_seed,
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
    if constexpr (int_optimized) {
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
        if constexpr (int_optimized)
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


template <bool int_optimized, PayloadType payload_type>
template <class t_itr>
Diva<int_optimized, payload_type>::Diva(const uint32_t infix_size, const t_itr begin, const t_itr end, const uint32_t key_len,
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
    if constexpr (int_optimized) {
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


template <bool int_optimized, PayloadType payload_type>
template <class t_itr>
Diva<int_optimized, payload_type>::Diva(const uint32_t infix_size, const t_itr begin, const t_itr end, 
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
    if constexpr (int_optimized) {
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



template <bool int_optimized, PayloadType payload_type>
inline void Diva<int_optimized, payload_type>::SetupScaleFactors() {
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


template <bool int_optimized, PayloadType payload_type>
inline void Diva<int_optimized, payload_type>::GetLowerUpperBounds(const InfiniteByteString& key, bool write, void *leaves[3],
                                                                   wormhole_iter& it, wormhole_int_iter& it_int,
                                                                   InfiniteByteString& prev_key, InfiniteByteString& next_key,
                                                                   Diva<int_optimized, payload_type>::InfixStore *& infix_store_ptr) const {
    const bool unlock = false;
    uint32_t exp_backoff = 2;
GetLowerUpperBoundsRetry:
    uint32_t l_ind = 0, r_ind = 0;
    InfixStore *dummy_infix_store_ptr;
    uint32_t dummy_val;
    if constexpr (int_optimized) {
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


template <bool int_optimized, PayloadType payload_type>
inline void Diva<int_optimized, payload_type>::DeleteGetLowerMiddleUpperBounds(const InfiniteByteString& key, void *leaves[3],
                                                                               wormhole_iter& it, wormhole_int_iter& it_int,
                                                                               InfiniteByteString& left_key,
                                                                               InfiniteByteString& middle_key,
                                                                               InfiniteByteString& right_key,
                                                                               Diva<int_optimized, payload_type>::InfixStore *& left_store_ptr,
                                                                               Diva<int_optimized, payload_type>::InfixStore *& right_store_ptr) const {
    const bool write = true, unlock = false;
    uint32_t exp_backoff = 1;

GetLowerMiddleUpperBoundsRetry:
    uint32_t l_ind = 0, r_ind = 0;
    InfixStore *dummy_infix_store_ptr;
    uint32_t dummy_val;
    if constexpr (int_optimized) {
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


template <bool int_optimized, PayloadType payload_type>
inline void Diva<int_optimized, payload_type>::OrderLeaves(void *leaves[3], uint32_t l, uint32_t r) const {
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

template <bool int_optimized, PayloadType payload_type>
inline void Diva<int_optimized, payload_type>::UnlockLeaves(void *leaves[3], bool write) const {
    for (uint32_t i = 0; i < 3; i++) {
        if (leaves[i] == nullptr)
            continue;
        if (write) {
            if constexpr (int_optimized)
                wormleaf_int_unlock_write(reinterpret_cast<struct wormleaf_int *>(leaves[i]));
            else
                wormleaf_unlock_write(reinterpret_cast<struct wormleaf *>(leaves[i]));
        }
        else {
            if constexpr (int_optimized)
                wormleaf_int_unlock_read(reinterpret_cast<struct wormleaf_int *>(leaves[i]));
            else
                wormleaf_unlock_read(reinterpret_cast<struct wormleaf *>(leaves[i]));
        }
        leaves[i] = nullptr;
    }
}


template <bool int_optimized, PayloadType payload_type>
inline void Diva<int_optimized, payload_type>::Insert(uint64_t key,
                                                      const void *payload,
                                                      uint32_t random_number) {
    key = __builtin_bswap64(key);
    Insert(reinterpret_cast<const uint8_t *>(&key), sizeof(key), payload, random_number);
}


template <bool int_optimized, PayloadType payload_type>
inline void Diva<int_optimized, payload_type>::Insert(std::string_view key,
                                                      const void *payload,
                                                      uint32_t random_number) {
    Insert(reinterpret_cast<const uint8_t *>(key.data()), key.size(), payload, random_number);
}


template <bool int_optimized, PayloadType payload_type>
inline void Diva<int_optimized, payload_type>::Insert(const uint8_t *key, const uint32_t key_len,
                                                      const void *payload,
                                                      uint32_t random_number) {
    const InfiniteByteString converted_key {key, static_cast<uint32_t>(key_len)};
    random_number = random_number == 0 ? rng_() : random_number;
    if (random_number % infix_store_target_size == 0)
        InsertSplit(converted_key, payload);
    else 
        InsertSimple(converted_key, payload);
    n_keys_.fetch_add(1, std::memory_order_release);
}

template <bool int_optimized, PayloadType payload_type>
inline void Diva<int_optimized, payload_type>::InsertSimple(const InfiniteByteString key,
                                                            const void *payload) {
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
    if constexpr (int_optimized) {
        prev_key_word = *reinterpret_cast<const uint64_t *>(prev_key.str);
        prev_key.str = reinterpret_cast<const uint8_t *>(&prev_key_word);
        next_key_word = *reinterpret_cast<const uint64_t *>(next_key.str);
        next_key.str = reinterpret_cast<const uint8_t *>(&next_key_word);
    }

    InfixStore& infix_store = *infix_store_ptr;
    rwlock_lock_write(infix_store.rwlock);
    UnlockLeaves(leaves_to_unlock, it_write_lock);

    if (prev_key == key) {
        // Add new sample payload
        AddSamplePayload(infix_store, payload);
        rwlock_unlock_write(infix_store.rwlock);
        n_keys_.fetch_add(1, std::memory_order_release);
        return;
    }

    auto [shared, ignore, implicit_size] = GetSharedIgnoreImplicitLengths(prev_key, next_key);
    const uint64_t extraction = ExtractPartialKey(key, shared, ignore, implicit_size, key.GetBit(shared));
    const uint64_t next_implicit = ExtractPartialKey(next_key, shared, ignore, implicit_size, 1) >> infix_size_;
    const uint64_t prev_implicit = ExtractPartialKey(prev_key, shared, ignore, implicit_size, 0) >> infix_size_;
    const uint32_t total_implicit = next_implicit - prev_implicit + 1;
    const uint64_t insertee = ((extraction | 1ULL) - (prev_implicit << infix_size_));
    if constexpr (payload_type == PayloadType::FixedLength)
        InsertRawIntoInfixStore(infix_store, insertee, total_implicit, reinterpret_cast<const uint64_t *>(payload));
    else 
        InsertRawIntoInfixStore(infix_store, insertee, total_implicit);
    rwlock_unlock_write(infix_store.rwlock);
}


template <bool int_optimized, PayloadType payload_type>
inline bool Diva<int_optimized, payload_type>::RangeQuery(uint64_t l, uint64_t r) const {
    l = __builtin_bswap64(l);
    r = __builtin_bswap64(r);
    return RangeQuery(reinterpret_cast<const uint8_t *>(&l), sizeof(l),
                      reinterpret_cast<const uint8_t *>(&r), sizeof(r));
}


template <bool int_optimized, PayloadType payload_type>
inline bool Diva<int_optimized, payload_type>::RangeQuery(std::string_view input_l, std::string_view input_r) const {
    return RangeQuery(reinterpret_cast<const uint8_t *>(input_l.data()), input_l.size(),
                      reinterpret_cast<const uint8_t *>(input_r.data()), input_r.size());
}


template <bool int_optimized, PayloadType payload_type>
inline bool Diva<int_optimized, payload_type>::RangeQuery(const uint8_t *input_l, const uint32_t input_l_len,
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
    if constexpr (int_optimized) {
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

    if constexpr (int_optimized) {
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
        const bool res = RangeQueryInfixStore(infix_store, l_val, r_val, total_implicit);

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
        const bool res = RangeQueryInfixStore(infix_store, l_val, r_val, total_implicit);

        rwlock_unlock_read(infix_store.rwlock);
        return res;
    }
}


template <bool int_optimized, PayloadType payload_type>
inline bool Diva<int_optimized, payload_type>::PointQuery(uint64_t key) const {
    key = __builtin_bswap64(key);
    return PointQuery(reinterpret_cast<const uint8_t *>(&key), sizeof(key));
}


template <bool int_optimized, PayloadType payload_type>
inline bool Diva<int_optimized, payload_type>::PointQuery(std::string_view key) const {
    return PointQuery(reinterpret_cast<const uint8_t *>(key.data()), key.size());
}


template <bool int_optimized, PayloadType payload_type>
inline bool Diva<int_optimized, payload_type>::PointQuery(const uint8_t *input_key, const uint32_t key_len) const {
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
    if constexpr (int_optimized) {
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
        // Previous key was a partial key and a prefix of the query key
        rwlock_unlock_read(infix_store.rwlock);
        return true;
    }
    else if (next_key.str == nullptr) {
        rwlock_unlock_read(infix_store.rwlock);
        return false;
    }

    auto [shared, ignore, implicit_size] = GetSharedIgnoreImplicitLengths(prev_key, next_key);
    const uint64_t extraction = ExtractPartialKey(key, shared, ignore, implicit_size, key.GetBit(shared));
    const uint64_t prev_implicit = ExtractPartialKey(prev_key, shared, ignore, implicit_size, 0) >> infix_size_;
    const uint64_t next_implicit = ExtractPartialKey(next_key, shared, ignore, implicit_size, 1) >> infix_size_;
    const uint32_t total_implicit = next_implicit - prev_implicit + 1;
    const uint64_t query_key = extraction - (prev_implicit << infix_size_);
    const bool res = PointQueryInfixStore(infix_store, query_key, total_implicit);

    rwlock_unlock_read(infix_store.rwlock);
    return res;
}


template <bool int_optimized, PayloadType payload_type>
inline void Diva<int_optimized, payload_type>::AddTreeKey(const uint8_t *key, const uint32_t key_len, const uint64_t *payload) {
    InfixStore infix_store(scaled_sizes_[size_scalar_shrink_grow_sep], infix_size_,
                           size_scalar_shrink_grow_sep, payload_size_);
    if constexpr (payload_type == PayloadType::FixedLength) {
        if (payload)
            AddSamplePayload(infix_store, payload);
    }
    void *dummy_locked_leaf_addrs[3] = {nullptr, nullptr, nullptr};
    if constexpr (int_optimized)
        wh_int_put(better_tree_int_, key, key_len, &infix_store, sizeof(infix_store), dummy_locked_leaf_addrs);
    else
        wh_put(better_tree_, key, key_len, &infix_store, sizeof(infix_store), dummy_locked_leaf_addrs);
}


template <bool int_optimized, PayloadType payload_type>
inline void Diva<int_optimized, payload_type>::InsertSplit(const InfiniteByteString key,
                                                           const void *payload) {
    const bool it_write_lock = true;
    InfixStore *infix_store_ptr;
    void *leaves_to_unlock[3] = {};

    InfiniteByteString next_key {};
    InfiniteByteString prev_key {};

    wormhole_int_iter it_int;
    wormhole_iter it;
    GetLowerUpperBounds(key, it_write_lock, leaves_to_unlock, it, it_int,
                        prev_key, next_key, infix_store_ptr);
    uint64_t prev_key_word, next_key_word;
    if constexpr (int_optimized) {
        prev_key_word = *reinterpret_cast<const uint64_t *>(prev_key.str);
        prev_key.str = reinterpret_cast<const uint8_t *>(&prev_key_word);
        next_key_word = *reinterpret_cast<const uint64_t *>(next_key.str);
        next_key.str = reinterpret_cast<const uint8_t *>(&next_key_word);
    }

    InfixStore& infix_store = *infix_store_ptr;
    rwlock_lock_write(infix_store.rwlock);

    if (prev_key == key) {
        // Add new sample payload
        AddSamplePayload(infix_store, payload);
        rwlock_unlock_write(infix_store.rwlock);
        UnlockLeaves(leaves_to_unlock, it_write_lock);
        n_keys_.fetch_add(1, std::memory_order_release);
        return;
    }

    auto [shared, ignore, implicit_size] = GetSharedIgnoreImplicitLengths(prev_key, next_key);
    uint64_t extraction = ExtractPartialKey(key, shared, ignore, implicit_size, key.GetBit(shared));
    uint64_t prev_extraction = ExtractPartialKey(prev_key, shared, ignore, implicit_size, 0);
    uint64_t next_extraction = ExtractPartialKey(next_key, shared, ignore, implicit_size, 1);
    const uint64_t separator = (extraction | 1ULL) - (prev_extraction & (BITMASK(implicit_size) << infix_size_));

    const uint32_t infix_list_len = infix_store.GetElemCount();
    uint64_t infix_list_contents[infix_list_len > heap_alloc_threshold ? 1 : infix_list_len + 1];
    uint64_t payload_list_contents[infix_list_len > heap_alloc_threshold ? 1 : infix_list_len * payload_size_ / (8 * sizeof(uint64_t)) + 1];
    uint64_t *infix_list = infix_list_contents;
    uint64_t *payload_list = payload_list_contents;
    if (infix_list_len > heap_alloc_threshold) {
        infix_list = new uint64_t[infix_list_len + 1];
        if constexpr (payload_type == PayloadType::FixedLength)
            payload_list = new uint64_t[infix_list_len * payload_size_ / (8 * sizeof(uint64_t)) + 1];
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
        payload_list_right_half_ptr = new uint64_t[(right_half_len * payload_size_) / 64 + 2];
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
        memcpy(infix_list_right_half_ptr + ind, infix_list + sep_r, (right_half_len - ind) * sizeof(infix_list[0]));
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
    const uint64_t left_start = prev_key.BitsAt(shared + ignore + implicit_size, shamt_lt) << infix_size_;
    const uint64_t left_end = (((extraction >> infix_size_) - (prev_extraction >> infix_size_)) << (infix_size_ + shamt_lt))
                              | (key.BitsAt(shared + ignore + implicit_size, shamt_lt) << infix_size_);
    const uint32_t total_implicit_lt = ((extraction_lt >> infix_size_) - (prev_extraction_lt >> infix_size_)) + 1;

    auto [shared_gt, ignore_gt, implicit_size_gt] = GetSharedIgnoreImplicitLengths(
            {key.str + shared_word_byte, key.length < shared_word_byte ? 0 : key.length - shared_word_byte},
            {next_key.str + shared_word_byte, next_key.length < shared_word_byte ? 0 : next_key.length - shared_word_byte});
    shared_gt += shared_word_byte * 8;
    const int32_t shamt_gt = shared_gt + ignore_gt + implicit_size_gt - shared - ignore - implicit_size;
    const uint64_t extraction_gt = ExtractPartialKey(key, shared_gt, ignore_gt, implicit_size_gt, 0);
    const uint64_t next_extraction_gt = ExtractPartialKey(next_key, shared_gt, ignore_gt, implicit_size_gt, 1);
    const uint64_t right_start = (((extraction >> infix_size_) - (prev_extraction >> infix_size_)) << (infix_size_ + shamt_gt))
                                | (key.BitsAt(shared + ignore + implicit_size, shamt_gt) << infix_size_);
    const uint64_t right_end = (((next_extraction >> infix_size_) - (prev_extraction >> infix_size_)) << (infix_size_ + shamt_gt))
                                | (next_key.BitsAt(shared + ignore + implicit_size, shamt_gt) << infix_size_);
    const uint32_t total_implicit_gt = ((next_extraction_gt >> infix_size_) - (extraction_gt >> infix_size_)) + 1;

    const auto [left_list_len, left_exp] = GetExpandedInfixListLength(infix_list,
                                                                      left_half_len,
                                                                      implicit_size,
                                                                      shamt_lt,
                                                                      left_start, left_end);
    uint64_t left_infix_list_contents[left_list_len > heap_alloc_threshold ? 1 : left_list_len];
    uint64_t left_payload_list_contents[left_list_len > heap_alloc_threshold ? 1 : (left_list_len * payload_size_ + 63) / 64 + 1];
    uint64_t *left_infix_list = left_infix_list_contents;
    uint64_t *left_payload_list = left_payload_list_contents;
    if (left_list_len > heap_alloc_threshold) {
        left_infix_list = new uint64_t[left_list_len];
        if constexpr (payload_type == PayloadType::FixedLength)
            left_payload_list = new uint64_t[(left_list_len * payload_size_ + 63) / 64 + 1];
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
    uint64_t right_payload_list_contents[right_list_len > heap_alloc_threshold ? 1 : (right_list_len * payload_size_ + 63) / 64 + 1];
    uint64_t *right_infix_list = right_infix_list_contents;
    uint64_t *right_payload_list = right_payload_list_contents;
    if (right_list_len > heap_alloc_threshold) {
        right_infix_list = new uint64_t[right_list_len];
        if constexpr (payload_type == PayloadType::FixedLength)
            right_payload_list = new uint64_t[(right_list_len * payload_size_ + 63) / 64 + 1];
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
    store_lt.SetInvalidBits(infix_store.GetInvalidBits());
    store_lt.SetPartialKey(infix_store.IsPartialKey());
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
    if constexpr (int_optimized)
        wh_int_put(better_tree_int_, key.str, key.length, &store_gt, sizeof(InfixStore), leaves_to_unlock);
    else
        wh_put(better_tree_, key.str, key.length, &store_gt, sizeof(InfixStore), leaves_to_unlock);

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

    n_keys_.fetch_add(left_list_len + right_list_len - infix_count + 1, std::memory_order_release);
}


template <bool int_optimized, PayloadType payload_type>
//__attribute__((always_inline))
inline std::tuple<uint32_t, bool> Diva<int_optimized, payload_type>::GetExpandedInfixListLength(const uint64_t *list, const uint32_t list_len,
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


template <bool int_optimized, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<int_optimized, payload_type>::UpdateInfixList(const uint64_t *list, const uint32_t list_len, const uint32_t shamt,
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


template <bool int_optimized, PayloadType payload_type>
inline std::tuple<uint32_t, uint32_t, uint32_t> 
Diva<int_optimized, payload_type>::GetSharedIgnoreImplicitLengths(const InfiniteByteString key_1,
                                                                  const InfiniteByteString key_2) const {
    uint32_t share = 0, ignore = 0, implicit = 0;

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


template <bool int_optimized, PayloadType payload_type>
inline void Diva<int_optimized, payload_type>::ShrinkInfixSize(const uint32_t new_infix_size) {
    InfixStore *store_ptr;
    const uint8_t *key;
    uint32_t key_len, dummy_val;
    const bool write = false, unlock = true;

    if constexpr (int_optimized) {
        wormhole_int_iter it_int;
        it_int.ref = better_tree_int_;
        it_int.map = better_tree_int_->map;
        it_int.leaf = nullptr;
        it_int.is = 0;
        wh_int_iter_seek(&it_int, nullptr, 0, write);
        do {
            wh_int_iter_peek_ref(&it_int, reinterpret_cast<const void **>(&key), &key_len,
                                          reinterpret_cast<void **>(&store_ptr), &dummy_val);
            ShrinkInfixStoreInfixSize(*store_ptr, new_infix_size);
            wh_int_iter_skip1(&it_int, write, unlock);
        } while (wh_int_iter_valid(&it_int));
        if (it_int.leaf)
            wormleaf_int_unlock_read(it_int.leaf);
    }
    else {
        wormhole_iter it;
        it.ref = better_tree_;
        it.map = better_tree_->map;
        it.leaf = nullptr;
        it.is = 0;
        wh_iter_seek(&it, nullptr, 0, write);
        do {
            wh_iter_peek_ref(&it, reinterpret_cast<const void **>(&key), &key_len,
                                  reinterpret_cast<void **>(&store_ptr), &dummy_val);
            ShrinkInfixStoreInfixSize(*store_ptr, new_infix_size);
            wh_iter_skip1(&it, write, unlock);
        } while (wh_iter_valid(&it));
        if (it.leaf)
            wormleaf_unlock_read(it.leaf);
    }

    infix_size_ = new_infix_size;
}


template <bool int_optimized, PayloadType payload_type>
inline uint64_t Diva<int_optimized, payload_type>::Size() const {
    uint64_t res = sizeof(bool) + sizeof(infix_store_target_size) 
                 + sizeof(base_implicit_size) + sizeof(scale_shift)
                 + sizeof(scale_implicit_shift) + sizeof(size_scalar_count)
                 + sizeof(size_scalar_shrink_grow_sep) + sizeof(load_factor_)
                 + sizeof(load_factor_alt_) + sizeof(infix_size_) 
                 + sizeof(rng_seed_) + sizeof(n_keys_) 
                 + sizeof(InfixStore::size_grade_bit_count)
                 + sizeof(InfixStore::elem_count_bit_count);

    if constexpr (payload_type == PayloadType::FixedLength)
        res += sizeof(payload_size_);

    const uint8_t *tree_key, *last_tree_key = nullptr;
    uint32_t tree_key_len, last_tree_key_len = 0, dummy;
    InfixStore *store;
    const bool write = false, unlock = true;

    if constexpr (int_optimized) {
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
            /*
            if (last_tree_key != nullptr) {
                for (uint32_t i = 0; i < std::min(tree_key_len, last_tree_key_len) && last_tree_key[i] == tree_key[i]; i++)
                    res--;
            }
            */
            res += sizeof(store->status); // + sizeof(store->ptr);
            if constexpr (payload_type == PayloadType::FixedLength) {
                res += sizeof(store->num_sample_payloads);
                res += (store->num_sample_payloads * payload_size_ + 7) / 8;
            }
            if (store->ptr != nullptr) {
                const uint64_t word_count = store->GetPtrWordCount(scaled_sizes_[store->GetSizeGrade()], infix_size_, payload_size_);
                res += word_count * sizeof(uint64_t);
                //res += (store->GetElemCount() * (infix_size_ + 1) + infix_store_target_size + 7) / 8;
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


template <bool int_optimized, PayloadType payload_type>
inline uint32_t Diva<int_optimized, payload_type>::Serialize(char *out) const {
    uint64_t res = SerializeMetadata(out);

    const uint8_t *tree_key;
    uint32_t tree_key_len, dummy;
    InfixStore *store;
    const bool write = false, unlock = true;

    if constexpr (int_optimized) {
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


template <bool int_optimized, PayloadType payload_type>
inline uint32_t Diva<int_optimized, payload_type>::SerializeMetadata(char *out) const {
    uint32_t res = 0;
    // Diva Version
    out[res++] = static_cast<char>(int_optimized);

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

    memcpy(out + res, &InfixStore::elem_count_bit_count, sizeof(InfixStore::elem_count_bit_count));
    res += sizeof(InfixStore::elem_count_bit_count);

    return res;
}


template <bool int_optimized, PayloadType payload_type>
inline uint64_t Diva<int_optimized, payload_type>::SerializeInfixStore(char *out,
                                                                       const Diva<int_optimized, payload_type>::InfixStore& store) const {
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


template <bool int_optimized, PayloadType payload_type>
inline Diva<int_optimized, payload_type>::~Diva() {
    const bool write = true;
    const bool unlock = true;
    const uint8_t *tree_key;
    uint32_t tree_key_len, dummy;
    InfixStore *store;

    if constexpr (int_optimized) {
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


template <bool int_optimized, PayloadType payload_type>
inline Diva<int_optimized, payload_type>::Diva(const char *deser_buf):
        bulk_load_streaming_ind_(0) {
    uint32_t ind = DeserializeMetadata(deser_buf);
    if constexpr (int_optimized) {
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
        if constexpr (int_optimized) {
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


template <bool int_optimized, PayloadType payload_type>
inline uint32_t Diva<int_optimized, payload_type>::DeserializeMetadata(const char *deser_buf) {
    uint32_t res = 0;
    uint32_t buf32;
    float buf_float;

    // Diva Version
    assert(static_cast<bool>(deser_buf[res]) == int_optimized && "Mismatched Diva version");
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

    memcpy(&buf32, deser_buf + res, sizeof(InfixStore::elem_count_bit_count));
    assert(buf32 == InfixStore::elem_count_bit_count && "Mismatched Diva version");
    res += sizeof(InfixStore::elem_count_bit_count);

    return res;
}


template <bool int_optimized, PayloadType payload_type>
inline uint32_t Diva<int_optimized, payload_type>::DeserializeInfixStore(const char *deser_buf,
                                                                         Diva<int_optimized, payload_type>::InfixStore& store) const {
    uint32_t offset = 0;
    memcpy(&store.status, deser_buf, sizeof(store.status));
    offset += sizeof(store.status);

    if constexpr (payload_type == PayloadType::FixedLength) {
        memcpy(&store.num_sample_payloads, deser_buf + offset, sizeof(store.num_sample_payloads));
        offset += sizeof(store.num_sample_payloads);
    }

    const uint32_t word_count = store.GetPtrWordCount(scaled_sizes_[store.GetSizeGrade()], infix_size_, payload_size_);
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


template <bool int_optimized, PayloadType payload_type>
//__attribute__((always_inline))
inline uint64_t Diva<int_optimized, payload_type>::ExtractPartialKey(const InfiniteByteString key,
                                                                     const uint32_t shared, const uint32_t ignore,
                                                                     const uint32_t implicit_size, const uint64_t msb) const {
    const uint32_t real_diff_pos = shared + ignore;
    uint64_t res = key.WordAt(real_diff_pos / 8);
    res >>= (63 - (implicit_size - 1) - infix_size_ - real_diff_pos % 8);
    res &= BITMASK(implicit_size - 1 + infix_size_);
    res |= msb << (implicit_size - 1 + infix_size_);
    return res;
}


template <bool int_optimized, PayloadType payload_type>
inline void Diva<int_optimized, payload_type>::Delete(uint64_t key,
                                                      std::function<bool(const uint64_t *)> should_remove) {
    key = __builtin_bswap64(key);
    Delete(reinterpret_cast<const uint8_t *>(&key), sizeof(key), should_remove);
}


template <bool int_optimized, PayloadType payload_type>
inline void Diva<int_optimized, payload_type>::Delete(std::string_view input_key,
                                                      std::function<bool(const uint64_t *)> should_remove) {
    Delete(reinterpret_cast<const uint8_t *>(input_key.data()), input_key.size(), should_remove);
}

template <bool int_optimized, PayloadType payload_type>
inline void Diva<int_optimized, payload_type>::Delete(const uint8_t *input_key, const uint32_t input_key_len,
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
    if constexpr (int_optimized) {
        prev_key_word = *reinterpret_cast<const uint64_t *>(prev_key.str);
        prev_key.str = reinterpret_cast<const uint8_t *>(&prev_key_word);
        next_key_word = *reinterpret_cast<const uint64_t *>(next_key.str);
        next_key.str = reinterpret_cast<const uint8_t *>(&next_key_word);
    }

    InfixStore& infix_store = *infix_store_ptr;
    rwlock_lock_write(infix_store.rwlock);
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

    const uint64_t extraction = ExtractPartialKey(key, shared, ignore, implicit_size, key.GetBit(shared));
    const uint64_t next_implicit = ExtractPartialKey(next_key, shared, ignore, implicit_size, 1) >> infix_size_;
    const uint64_t prev_implicit = ExtractPartialKey(prev_key, shared, ignore, implicit_size, 0) >> infix_size_;
    const uint32_t total_implicit = next_implicit - prev_implicit + 1;
    const uint64_t deletee = ((extraction | 1ULL) - (prev_implicit << infix_size_));

    DeleteRawFromInfixStore(infix_store, deletee, total_implicit, should_remove);
    rwlock_unlock_write(infix_store.rwlock);
    n_keys_.fetch_sub(1, std::memory_order_release);
}


template <bool int_optimized, PayloadType payload_type>
inline void Diva<int_optimized, payload_type>::DeleteRange(uint64_t l, uint64_t r,
                                                           std::function<bool(const uint64_t *)> should_remove) {
    l = __builtin_bswap64(l);
    r = __builtin_bswap64(r);
    DeleteRange(reinterpret_cast<const uint8_t *>(&l), sizeof(l),
                reinterpret_cast<const uint8_t *>(&r), sizeof(r),
                should_remove);
}


template <bool int_optimized, PayloadType payload_type>
inline void Diva<int_optimized, payload_type>::DeleteRange(std::string_view input_l,
                                                           std::string_view input_r,
                                                           std::function<bool(const uint64_t *)> should_remove) {
    Delete(reinterpret_cast<const uint8_t *>(input_l.data()), input_l.size(),
           reinterpret_cast<const uint8_t *>(input_r.data()), input_r.size(),
           should_remove);
}

template <bool int_optimized, PayloadType payload_type>
inline void Diva<int_optimized, payload_type>::DeleteRange(const uint8_t *input_l, const uint32_t input_l_len,
                                                           const uint8_t *input_r, const uint32_t input_r_len,
                                                           std::function<bool(const uint64_t *)> should_remove) {
    auto it = GetIterator(input_l, input_l_len, input_r, input_r_len,
                          should_remove ? should_remove 
                                        : [](const uint64_t *payload) { return true; });
    while (it.IsValid())
        it++;
}


template <bool int_optimized, PayloadType payload_type>
inline bool Diva<int_optimized, payload_type>::CompareInfixes(uint64_t a, uint64_t b) const {
    const uint64_t a_lb = a & -a;
    a -= a_lb;
    const uint64_t b_lb = b & -b;
    b -= b_lb;
    return (a == b ? a_lb > b_lb : a < b);
}


template <bool int_optimized, PayloadType payload_type>
inline void Diva<int_optimized, payload_type>::DeleteMerge(InfiniteByteString key) {
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
    if constexpr (int_optimized) {
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

    uint32_t total_elem_count = store_l->GetElemCount() + store_r->GetElemCount();
    const bool should_allocate_on_heap = total_elem_count > heap_alloc_threshold;
    uint64_t infix_list_contents[should_allocate_on_heap ? 1 : total_elem_count + 1];
    uint64_t *infix_list = infix_list_contents;
    uint32_t payload_list_size = 1, right_payload_list_size = 1;
    if constexpr (payload_type == PayloadType::FixedLength) {
        payload_list_size = total_elem_count * ((payload_size_ + 63) / 64);
        right_payload_list_size = store_r->GetElemCount() * ((payload_size_ + 63) / 64);
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
    GetInfixList(*store_r, infix_list + store_l->GetElemCount(), right_payload_list);
    if constexpr (payload_type == PayloadType::FixedLength) {
        copy_bitmap_to_bitmap(right_payload_list, 0,
                              payload_list, payload_size_ * store_l->GetElemCount(),
                              payload_size_ * store_r->GetElemCount());
    }

    UpdateInfixListDelete(shared, ignore, implicit_size, left_key, middle_key,
                          infix_list, store_l->GetElemCount());
    UpdateInfixListDelete(shared, ignore, implicit_size, middle_key, right_key,
                          infix_list + store_l->GetElemCount(), store_r->GetElemCount());
    const uint64_t implicit = ExtractPartialKey(left_key, shared, ignore, implicit_size, 0) >> infix_size_;
    for (int32_t i = 0; i < total_elem_count; i++)
        infix_list[i] -= implicit << infix_size_;

    // Make sure the merged list is sorted by the infix starts
    if (0 < store_l->GetElemCount() && store_l->GetElemCount() < total_elem_count) {
        const uint64_t last_infix_l = infix_list[store_l->GetElemCount() - 1];
        const uint64_t first_infix_r = infix_list[store_l->GetElemCount()];
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
                std::merge(pair_list, pair_list + store_l->GetElemCount(),
                           pair_list + store_l->GetElemCount(), pair_list + total_elem_count,
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

                std::merge(infix_list, infix_list + store_l->GetElemCount(),
                           infix_list + store_l->GetElemCount(), infix_list + total_elem_count,
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

    store.SetInvalidBits(store_l->GetInvalidBits());
    store.SetPartialKey(store_l->IsPartialKey());
    store_l->status = store.status;
    store_l->ptr = store.ptr;
    store_l->rwlock.store(store.rwlock.load(std::memory_order_acquire), std::memory_order_release);
    delete[] old_store_l_ptr;
    free(reinterpret_cast<void *>(store_r->ptr[1]));
    delete[] store_r->ptr;
    if (should_allocate_on_heap) {
        delete[] infix_list;
        if constexpr (payload_type == PayloadType::FixedLength) {
            delete[] payload_list;
            delete[] right_payload_list;
        }
    }

    if constexpr (int_optimized)
        wh_int_del(better_tree_int_, middle_key.str, middle_key.length, leaves_to_unlock);
    else
        wh_del(better_tree_, middle_key.str, middle_key.length, leaves_to_unlock);

    /*
#ifdef DEBUG
    {
        uint64_t dummy_infix_list[total_elem_count + 1];
        GetInfixList(*store_l, dummy_infix_list);
    }
#endif // DEBUG
    */
    UnlockLeaves(leaves_to_unlock, it_write_lock);
    n_keys_.fetch_sub(1, std::memory_order_release);
}

template <bool int_optimized, PayloadType payload_type>
inline void Diva<int_optimized, payload_type>::UpdateInfixListDelete(const uint32_t shared, const uint32_t ignore, const uint32_t implicit_size,
                                                                     const InfiniteByteString left_key, const InfiniteByteString right_key,
                                                                     uint64_t *infix_list, const uint32_t infix_list_len) {
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
    /*
#ifdef DEBUG
    for (uint32_t i = 1; i < infix_list_len; i++)
        assert(infix_list[i - 1] == infix_list[i] || CompareInfixes(infix_list[i - 1], infix_list[i]));
#endif // DEBUG
    */
}


template <bool int_optimized, PayloadType payload_type>
template <class t_itr>
inline void Diva<int_optimized, payload_type>::BulkLoadFixedLength(const t_itr begin, const t_itr end, const uint32_t key_len,
                                                                   const uint64_t **payloads) {
    void *dummy_locked_leaf_addrs[3] = {nullptr, nullptr, nullptr};
    uint64_t infix_list[infix_store_target_size], int_opt_buf[3];
    uint32_t payload_list_size = 1;
    if constexpr (payload_type == PayloadType::FixedLength)
        payload_list_size = (infix_store_target_size * payload_size_ + 63) / 64;
    uint64_t left_payload[(payload_size_ + 63) / 64 + 1], right_payload[(payload_size_ + 63) / 64 + 1];
    uint64_t payload_list[payload_list_size + 1];
    t_itr last_key_it = begin, key_it = begin;
    InfiniteByteString left_key {}, right_key {};
    if constexpr (int_optimized) {
        int_opt_buf[0] = __builtin_bswap64(*key_it);
        left_key = {reinterpret_cast<const uint8_t *>(int_opt_buf + 0), key_len};
    }
    else
        left_key = {reinterpret_cast<const uint8_t *>(&(*key_it)), key_len};
    if constexpr (payload_type == PayloadType::FixedLength)
        copy_bitmap_to_bitmap(payloads[0], 0, left_payload, 0, payload_size_);
    int32_t cnt = 1;
    for (++key_it; key_it != end; ++key_it) {
        if (cnt % infix_store_target_size == 0) {   // New boundary key
            if constexpr (int_optimized) {
                int_opt_buf[1] = __builtin_bswap64(*key_it);
                right_key = {reinterpret_cast<const uint8_t *>(int_opt_buf + 1), key_len};
            }
            else
                right_key = {reinterpret_cast<const uint8_t *>(&(*key_it)), key_len};
            if constexpr (payload_type == PayloadType::FixedLength)
                copy_bitmap_to_bitmap(payloads[cnt], 0, right_payload, 0, payload_size_);

            auto [shared, ignore, implicit_size] = GetSharedIgnoreImplicitLengths(left_key, right_key);
            const uint64_t prev_implicit = ExtractPartialKey(left_key, shared, ignore, implicit_size, 0) >> infix_size_;
            const uint64_t next_implicit = ExtractPartialKey(right_key, shared, ignore, implicit_size, 1) >> infix_size_;
            const uint32_t total_implicit = next_implicit - prev_implicit + 1;
            ++last_key_it;
            uint32_t last_key_pos = 0;
            if constexpr (payload_type == PayloadType::FixedLength)
                last_key_pos = std::distance(begin, last_key_it);
            for (int32_t i = 0; i < infix_store_target_size - 1; i++) {
                InfiniteByteString key;
                if constexpr (int_optimized) {
                    int_opt_buf[2] = __builtin_bswap64(*last_key_it);
                    key = {reinterpret_cast<const uint8_t *>(int_opt_buf + 2), key_len};
                }
                else 
                    key = {reinterpret_cast<const uint8_t *>(&(*last_key_it)), key_len};
                const uint64_t extraction = ExtractPartialKey(key, shared, ignore, implicit_size, key.GetBit(shared));
                infix_list[i] = ((extraction | 1ULL) - (prev_implicit << infix_size_));
                if constexpr (payload_type == PayloadType::FixedLength) {
                    copy_bitmap_to_bitmap(payloads[last_key_pos], 0, payload_list, i * payload_size_, payload_size_);
                    last_key_pos++;
                }
                ++last_key_it;
            }
            n_keys_.fetch_add(infix_store_target_size, std::memory_order_release);
            /*
#ifdef DEBUG
            {
                const uint32_t infix_count = infix_store_target_size - 1;
                const uint64_t prev_extraction = ExtractPartialKey(left_key, shared, ignore, implicit_size, 0);
                const uint64_t next_extraction = ExtractPartialKey(right_key, shared, ignore, implicit_size, 1);
                validate_infixes_and_bounds(infix_count, infix_list, infix_size_, prev_extraction, next_extraction);
            }
#endif // DEBUG
            */

            InfixStore store(scaled_sizes_[size_scalar_shrink_grow_sep], infix_size_,
                             size_scalar_shrink_grow_sep, payload_size_);
            if constexpr (payload_type == PayloadType::FixedLength) {
                const uint64_t *sample_payloads = reinterpret_cast<const uint64_t *>(store.ptr[1]);
                LoadListToInfixStore(store, infix_list, infix_store_target_size - 1, total_implicit, true, payload_list);
                store.ptr[1] = reinterpret_cast<uint64_t>(sample_payloads);
                AddSamplePayload(store, left_payload);
            }
            else 
                LoadListToInfixStore(store, infix_list, infix_store_target_size - 1, total_implicit);
            if constexpr (int_optimized)
                wh_int_put(better_tree_int_, left_key.str, left_key.length, &store, sizeof(store), dummy_locked_leaf_addrs);
            else
                wh_put(better_tree_, left_key.str, left_key.length, &store, sizeof(store), dummy_locked_leaf_addrs);

            if constexpr (int_optimized)
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
    if constexpr (int_optimized) {
        int_opt_buf[1] = __builtin_bswap64(*key_it);
        right_key = {reinterpret_cast<const uint8_t *>(int_opt_buf + 1), key_len};
    }
    else
        right_key = {reinterpret_cast<const uint8_t *>(&(*key_it)), key_len};
    if constexpr (payload_type == PayloadType::FixedLength)
        copy_bitmap_to_bitmap(payloads[std::distance(begin, key_it)], 0, right_payload, 0, payload_size_);
    n_keys_.fetch_add(1, std::memory_order_release);

    if (key_it != last_key_it) {
        auto [shared, ignore, implicit_size] = GetSharedIgnoreImplicitLengths(left_key, right_key);
        const uint64_t prev_implicit = ExtractPartialKey(left_key, shared, ignore, implicit_size, 0) >> infix_size_;
        const uint64_t next_implicit = ExtractPartialKey(right_key, shared, ignore, implicit_size, 1) >> infix_size_;
        const uint32_t total_implicit = next_implicit - prev_implicit + 1;
        int32_t i = 0;
        ++last_key_it;
        n_keys_.fetch_add(1, std::memory_order_release);
        uint32_t last_key_pos = 0;
        if constexpr (payload_type == PayloadType::FixedLength)
            last_key_pos = std::distance(begin, last_key_it);
        while (last_key_it != key_it) {
            InfiniteByteString key;
            if constexpr (int_optimized) {
                int_opt_buf[2] = __builtin_bswap64(*last_key_it);
                key = {reinterpret_cast<const uint8_t *>(int_opt_buf + 2), key_len};
            }
            else 
                key = {reinterpret_cast<const uint8_t *>(&(*last_key_it)), key_len};
            const uint64_t extraction = ExtractPartialKey(key, shared, ignore, implicit_size, key.GetBit(shared));
            infix_list[i] = ((extraction | 1ULL) - (prev_implicit << infix_size_));
            if constexpr (payload_type == PayloadType::FixedLength) {
                copy_bitmap_to_bitmap(payloads[last_key_pos], 0, payload_list, i * payload_size_, payload_size_);
                last_key_pos++;
            }
            i++;
            ++last_key_it;
            n_keys_.fetch_add(1, std::memory_order_release);
        }
        /*
#ifdef DEBUG
        {
            const uint32_t infix_count = i;
            const uint64_t prev_extraction = ExtractPartialKey(left_key, shared, ignore, implicit_size, 0);
            const uint64_t next_extraction = ExtractPartialKey(right_key, shared, ignore, implicit_size, 1);
            validate_infixes_and_bounds(infix_count, infix_list, infix_size_, prev_extraction, next_extraction);
        }
#endif // DEBUG
        */

        const uint32_t size_scalar = std::lower_bound(scaled_sizes_, scaled_sizes_ + size_scalar_count, i) - scaled_sizes_;
        InfixStore store(scaled_sizes_[size_scalar], infix_size_, size_scalar, payload_size_);
        if constexpr (payload_type == PayloadType::FixedLength) {
            const uint64_t *sample_payloads = reinterpret_cast<const uint64_t *>(store.ptr[1]);
            LoadListToInfixStore(store, infix_list, i, total_implicit, true, payload_list);
            store.ptr[1] = reinterpret_cast<uint64_t>(sample_payloads);
            AddSamplePayload(store, left_payload);
        }
        else
            LoadListToInfixStore(store, infix_list, i, total_implicit);
        if constexpr (int_optimized)
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


template <bool int_optimized, PayloadType payload_type>
template <class t_itr>
inline void Diva<int_optimized, payload_type>::BulkLoad(const t_itr begin, const t_itr end,
                                                        const uint64_t **payloads) {
    void *dummy_locked_leaf_addrs[3] = {nullptr, nullptr, nullptr};
    uint64_t infix_list[infix_store_target_size];
    uint32_t payload_list_size = 1;
    if constexpr (payload_type == PayloadType::FixedLength)
        payload_list_size = (infix_store_target_size * payload_size_ + 63) / 64;
    uint64_t left_payload[(payload_size_ + 63) / 64 + 1], right_payload[(payload_size_ + 63) / 64 + 1];
    uint64_t payload_list[payload_list_size + 1];
    t_itr last_key_it = begin, key_it = begin;
    std::string_view sv {*key_it};
    InfiniteByteString left_key {reinterpret_cast<const uint8_t *>(sv.data()), 
                                 static_cast<uint32_t>(sv.size())};
    InfiniteByteString right_key {};
    if constexpr (payload_type == PayloadType::FixedLength)
        copy_bitmap_to_bitmap(payloads[0], 0, left_payload, 0, payload_size_);
    int32_t cnt = 1;
    uint32_t max_len = sv.size();
    for (++key_it; key_it != end; ++key_it) {
        if (cnt % infix_store_target_size == 0) {   // New boundary key
            std::string_view sv = *key_it;
            right_key = {reinterpret_cast<const uint8_t *>(sv.data()), 
                         static_cast<uint32_t>(sv.size())};
            if constexpr (payload_type == PayloadType::FixedLength)
                copy_bitmap_to_bitmap(payloads[cnt], 0, right_payload, 0, payload_size_);

            auto [shared, ignore, implicit_size] = GetSharedIgnoreImplicitLengths(left_key, right_key);
            const uint64_t prev_implicit = ExtractPartialKey(left_key, shared, ignore, implicit_size, 0) >> infix_size_;
            const uint64_t next_implicit = ExtractPartialKey(right_key, shared, ignore, implicit_size, 1) >> infix_size_;
            const uint32_t total_implicit = next_implicit - prev_implicit + 1;
            ++last_key_it;
            uint32_t last_key_pos = 0;
            if constexpr (payload_type == PayloadType::FixedLength)
                last_key_pos = std::distance(begin, last_key_it);
            for (int32_t i = 0; i < infix_store_target_size - 1; i++) {
                sv = *last_key_it;
                const InfiniteByteString key {reinterpret_cast<const uint8_t *>(sv.data()), 
                                              static_cast<uint32_t>(sv.size())};
                const uint64_t extraction = ExtractPartialKey(key, shared, ignore, implicit_size, key.GetBit(shared));
                infix_list[i] = ((extraction | 1ULL) - (prev_implicit << infix_size_));
                if constexpr (payload_type == PayloadType::FixedLength) {
                    copy_bitmap_to_bitmap(payloads[last_key_pos], 0, payload_list, i * payload_size_, payload_size_);
                    last_key_pos++;
                }
                ++last_key_it;
            }
            n_keys_.fetch_add(infix_store_target_size, std::memory_order_release);
            /*
#ifdef DEBUG
            {
                const uint32_t infix_count = infix_store_target_size - 1;
                const uint64_t prev_extraction = ExtractPartialKey(left_key, shared, ignore, implicit_size, 0);
                const uint64_t next_extraction = ExtractPartialKey(right_key, shared, ignore, implicit_size, 1);
                validate_infixes_and_bounds(infix_count, infix_list, infix_size_, prev_extraction, next_extraction);
            }
#endif // DEBUG
            */

            InfixStore store(scaled_sizes_[size_scalar_shrink_grow_sep], infix_size_,
                             size_scalar_shrink_grow_sep, payload_size_);
            if constexpr (payload_type == PayloadType::FixedLength) {
                const uint64_t *sample_payloads = reinterpret_cast<const uint64_t *>(store.ptr[1]);
                LoadListToInfixStore(store, infix_list, infix_store_target_size - 1, total_implicit, true, payload_list);
                store.ptr[1] = reinterpret_cast<uint64_t>(sample_payloads);
                AddSamplePayload(store, left_payload);
            }
            else 
                LoadListToInfixStore(store, infix_list, infix_store_target_size - 1, total_implicit);
            if constexpr (int_optimized)
                wh_int_put(better_tree_int_, left_key.str, left_key.length, &store, sizeof(store), dummy_locked_leaf_addrs);
            else
                wh_put(better_tree_, left_key.str, left_key.length, &store, sizeof(store), dummy_locked_leaf_addrs);

            /*
#ifdef DEBUG
            {
                last_key_it--;
                sv = *last_key_it;
                const InfiniteByteString key {reinterpret_cast<const uint8_t *>(sv.data()), 
                                              static_cast<uint32_t>(sv.size())};
                last_key_it++;

                const bool it_write_lock = false;
                InfixStore *infix_store_ptr;
                void *leaves_to_unlock[3] = {};

                InfiniteByteString next_key {};
                InfiniteByteString prev_key {};

                wormhole_int_iter it_int;
                wormhole_iter it;
                GetLowerUpperBounds(key, it_write_lock, leaves_to_unlock, it, it_int,
                        prev_key, next_key, infix_store_ptr);
                assert(prev_key == left_key);
                UnlockLeaves(leaves_to_unlock, it_write_lock);
            }
#endif // DEBUG

#ifdef DEBUG
            {
                const uint32_t infix_count = infix_store_target_size - 1;
                const uint64_t prev_extraction = ExtractPartialKey(left_key, shared, ignore, implicit_size, 0);
                const uint64_t next_extraction = ExtractPartialKey(right_key, shared, ignore, implicit_size, 1);
                validate_infixes_and_bounds(infix_count, infix_list, infix_size_, prev_extraction, next_extraction);
            }
#endif // DEBUG
            */

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
        auto [shared, ignore, implicit_size] = GetSharedIgnoreImplicitLengths(left_key, right_key);
        const uint64_t prev_implicit = ExtractPartialKey(left_key, shared, ignore, implicit_size, 0) >> infix_size_;
        const uint64_t next_implicit = ExtractPartialKey(right_key, shared, ignore, implicit_size, 1) >> infix_size_;
        const uint32_t total_implicit = next_implicit - prev_implicit + 1;
        int32_t i = 0;
        ++last_key_it;
        n_keys_.fetch_add(1, std::memory_order_release);
        uint32_t last_key_pos = 0;
        if constexpr (payload_type == PayloadType::FixedLength)
            last_key_pos = std::distance(begin, last_key_it);
        while (last_key_it != key_it) {
            sv = *last_key_it;
            const InfiniteByteString key {reinterpret_cast<const uint8_t *>(sv.data()), 
                                          static_cast<uint32_t>(sv.size())};
            const uint64_t extraction = ExtractPartialKey(key, shared, ignore, implicit_size, key.GetBit(shared));
            infix_list[i] = ((extraction | 1ULL) - (prev_implicit << infix_size_));
            if constexpr (payload_type == PayloadType::FixedLength) {
                copy_bitmap_to_bitmap(payloads[last_key_pos], 0, payload_list, i * payload_size_, payload_size_);
                last_key_pos++;
            }
            i++;
            ++last_key_it;
            n_keys_.fetch_add(1, std::memory_order_release);
        }
        /*
#ifdef DEBUG
        {
            const uint32_t infix_count = i;
            const uint64_t prev_extraction = ExtractPartialKey(left_key, shared, ignore, implicit_size, 0);
            const uint64_t next_extraction = ExtractPartialKey(right_key, shared, ignore, implicit_size, 1);
            validate_infixes_and_bounds(infix_count, infix_list, infix_size_, prev_extraction, next_extraction);
        }
#endif // DEBUG
        */

        const uint32_t size_scalar = std::lower_bound(scaled_sizes_, scaled_sizes_ + size_scalar_count, i) - scaled_sizes_;
        InfixStore store(scaled_sizes_[size_scalar], infix_size_, size_scalar, payload_size_);
        if constexpr (payload_type == PayloadType::FixedLength) {
            const uint64_t *sample_payloads = reinterpret_cast<const uint64_t *>(store.ptr[1]);
            LoadListToInfixStore(store, infix_list, i, total_implicit, true, payload_list);
            store.ptr[1] = reinterpret_cast<uint64_t>(sample_payloads);
            AddSamplePayload(store, left_payload);
        }
        else
            LoadListToInfixStore(store, infix_list, i, total_implicit);
        if constexpr (int_optimized)
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


template <bool int_optimized, PayloadType payload_type>
inline void Diva<int_optimized, payload_type>::BulkLoadStreaming(uint64_t key, const uint64_t *payload) {
    key = __builtin_bswap64(key);
    BulkLoadStreaming(reinterpret_cast<const uint8_t *>(&key), sizeof(key), payload);
}


template <bool int_optimized, PayloadType payload_type>
inline void Diva<int_optimized, payload_type>::BulkLoadStreaming(std::string_view key, const uint64_t *payload) {
    BulkLoadStreaming(reinterpret_cast<const uint8_t *>(key.data()), key.size(), payload);
}


template <bool int_optimized, PayloadType payload_type>
inline void Diva<int_optimized, payload_type>::BulkLoadStreaming(const uint8_t *key, const uint32_t key_len,
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
    auto [shared, ignore, implicit_size] = GetSharedIgnoreImplicitLengths(bulk_load_left_key_, bulk_load_right_key);
    const uint64_t prev_implicit = ExtractPartialKey(bulk_load_left_key_, shared, ignore, implicit_size, 0) >> infix_size_;
    const uint64_t next_implicit = ExtractPartialKey(bulk_load_right_key, shared, ignore, implicit_size, 1) >> infix_size_;
    const uint32_t total_implicit = next_implicit - prev_implicit + 1;
    for (int32_t i = 0; i < bulk_load_streaming_ind_; i++) {
        const uint64_t extraction = ExtractPartialKey(bulk_load_key_list_[i], shared, ignore, implicit_size, bulk_load_key_list_[i].GetBit(shared));
        infix_list[i] = ((extraction | 1ULL) - (prev_implicit << infix_size_));
    }
    void *dummy_locked_leaf_addrs[3] = {nullptr, nullptr, nullptr};
    InfixStore store(scaled_sizes_[size_scalar_shrink_grow_sep], infix_size_,
                     size_scalar_shrink_grow_sep, payload_size_);
    if constexpr (payload_type == PayloadType::FixedLength) {
        const uint64_t *sample_payloads = reinterpret_cast<const uint64_t *>(store.ptr[1]);
        LoadListToInfixStore(store, infix_list, bulk_load_streaming_ind_, total_implicit, true, bulk_load_payload_list_);
        store.ptr[1] = reinterpret_cast<uint64_t>(sample_payloads);
        AddSamplePayload(store, bulk_load_left_payload_);
    }
    else 
        LoadListToInfixStore(store, infix_list, bulk_load_streaming_ind_, total_implicit);
    if constexpr (int_optimized)
        wh_int_put(better_tree_int_, bulk_load_left_key_.str, bulk_load_left_key_.length, &store, sizeof(store), dummy_locked_leaf_addrs);
    else
        wh_put(better_tree_, bulk_load_left_key_.str, bulk_load_left_key_.length, &store, sizeof(store), dummy_locked_leaf_addrs);

    delete[] bulk_load_left_key_.str;
    bulk_load_left_key_ = bulk_load_right_key;
    if constexpr (payload_type == PayloadType::FixedLength)
        copy_bitmap_to_bitmap(payload, 0, bulk_load_left_payload_, 0, payload_size_);
    bulk_load_streaming_ind_ = 0;

    n_keys_.fetch_add(infix_store_target_size, std::memory_order_release);
}


template <bool int_optimized, PayloadType payload_type>
inline void Diva<int_optimized, payload_type>::BulkLoadStreamingFinish() {
    uint8_t *key_copy = new uint8_t[bulk_load_streaming_max_len_];
    memset(key_copy, 0x00, bulk_load_streaming_max_len_);
    AddTreeKey(key_copy, bulk_load_streaming_max_len_);
    memset(key_copy, 0xFF, bulk_load_streaming_max_len_);
    AddTreeKey(key_copy, bulk_load_streaming_max_len_);

    if (bulk_load_streaming_ind_ > 0) {
        InfiniteByteString bulk_load_right_key {bulk_load_key_list_[bulk_load_streaming_ind_ - 1].str,
                                                bulk_load_key_list_[bulk_load_streaming_ind_ - 1].length};
        bulk_load_key_list_[bulk_load_streaming_ind_ - 1] = {};
        bulk_load_streaming_ind_--;

        uint64_t infix_list[infix_store_target_size];
        auto [shared, ignore, implicit_size] = GetSharedIgnoreImplicitLengths(bulk_load_left_key_, bulk_load_right_key);
        const uint64_t prev_implicit = ExtractPartialKey(bulk_load_left_key_, shared, ignore, implicit_size, 0) >> infix_size_;
        const uint64_t next_implicit = ExtractPartialKey(bulk_load_right_key, shared, ignore, implicit_size, 1) >> infix_size_;
        const uint32_t total_implicit = next_implicit - prev_implicit + 1;
        for (int32_t i = 0; i < bulk_load_streaming_ind_; i++) {
            const uint64_t extraction = ExtractPartialKey(bulk_load_key_list_[i], shared, ignore, implicit_size, bulk_load_key_list_[i].GetBit(shared));
            infix_list[i] = ((extraction | 1ULL) - (prev_implicit << infix_size_));
        }
        void *dummy_locked_leaf_addrs[3] = {nullptr, nullptr, nullptr};
        const uint32_t size_scalar = std::lower_bound(scaled_sizes_, scaled_sizes_ + size_scalar_count, bulk_load_streaming_ind_) - scaled_sizes_;
        InfixStore store(scaled_sizes_[size_scalar], infix_size_, size_scalar, payload_size_);
        if constexpr (payload_type == PayloadType::FixedLength) {
            const uint64_t *sample_payloads = reinterpret_cast<const uint64_t *>(store.ptr[1]);
            LoadListToInfixStore(store, infix_list, bulk_load_streaming_ind_, total_implicit, true, bulk_load_payload_list_);
            store.ptr[1] = reinterpret_cast<uint64_t>(sample_payloads);
            AddSamplePayload(store, bulk_load_left_payload_);
        }
        else 
            LoadListToInfixStore(store, infix_list, bulk_load_streaming_ind_, total_implicit);
        if constexpr (int_optimized)
            wh_int_put(better_tree_int_, bulk_load_left_key_.str, bulk_load_left_key_.length, &store, sizeof(store), dummy_locked_leaf_addrs);
        else
            wh_put(better_tree_, bulk_load_left_key_.str, bulk_load_left_key_.length, &store, sizeof(store), dummy_locked_leaf_addrs);
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


template <bool int_optimized, PayloadType payload_type>
inline uint64_t Diva<int_optimized, payload_type>::GetNumKeys() const {
    return n_keys_.load(std::memory_order_acquire);
}


template <bool int_optimized, PayloadType payload_type>
//__attribute__((always_inline))
inline uint32_t Diva<int_optimized, payload_type>::RankOccupieds(const InfixStore &store, const uint32_t pos) const {
    const uint32_t *popcnts = reinterpret_cast<const uint32_t *>(store.ptr);
    const uint64_t *occupieds = store.ptr + 2;

    const bool cond = infix_store_target_size / 2 <= pos;
    uint32_t res = cond ? popcnts[0] : 0;
    for (int32_t i = cond ? infix_store_target_size / 128 : 0; i < pos / 64; i++)
        res += __builtin_popcountll(occupieds[i]);
    return res + bit_rank(occupieds[pos / 64], pos % 64);
}


template <bool int_optimized, PayloadType payload_type>
//__attribute__((always_inline))
inline uint32_t Diva<int_optimized, payload_type>::SelectRunends(const InfixStore &store, const uint32_t rank) const {
    const uint32_t *popcnts = reinterpret_cast<const uint32_t *>(store.ptr);
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t total_words = (scaled_sizes_[size_grade] + 63) / 64;
    const uint64_t *runends = store.ptr + 2 + infix_store_target_size / 64;

    const bool cond = popcnts[1] <= rank;
    uint32_t i, old_total_set_bits = cond ? popcnts[1] : 0, total_set_bits = cond ? popcnts[1] : 0;
    for (i = cond ? infix_store_target_size / 128 : 0; total_set_bits <= rank && i < total_words; i++) {
        old_total_set_bits = total_set_bits;
        total_set_bits += __builtin_popcountll(runends[i]);
    }
    i--;
    return i * 64 + bit_select(runends[i], rank - old_total_set_bits);
}


template <bool int_optimized, PayloadType payload_type>
inline int32_t Diva<int_optimized, payload_type>::NextOccupied(const InfixStore &store, const uint32_t pos) const {
    const uint64_t *occupieds = store.ptr + 2;
    int32_t res = pos + 1, lb_pos;
    do {
        lb_pos = lowbit_pos(occupieds[res / 64] & (~BITMASK(res % 64)));
        res += lb_pos - res % 64;
    } while (lb_pos == 64 && res < infix_store_target_size);
    return res;
}


template <bool int_optimized, PayloadType payload_type>
//__attribute__((always_inline))
inline int32_t Diva<int_optimized, payload_type>::PreviousOccupied(const InfixStore &store, const uint32_t pos) const {
    const uint64_t *occupieds = store.ptr + 2;
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


template <bool int_optimized, PayloadType payload_type>
//__attribute__((always_inline))
inline int32_t Diva<int_optimized, payload_type>::NextRunend(const InfixStore &store, const uint32_t pos) const {
    const uint64_t *runends = store.ptr + 2 + infix_store_target_size / 64;
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t runends_size = scaled_sizes_[size_grade];
    int32_t res = pos + 1, lb_pos;
    do {
        lb_pos = lowbit_pos(runends[res / 64] & (~BITMASK(res % 64)));
        res += lb_pos - res % 64;
    } while (lb_pos == 64 && res < runends_size);
    return res;
}


template <bool int_optimized, PayloadType payload_type>
//__attribute__((always_inline))
inline int32_t Diva<int_optimized, payload_type>::PreviousRunend(const InfixStore &store, const uint32_t pos) const {
    const uint64_t *runends = store.ptr + 2 + infix_store_target_size / 64;
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


template <bool int_optimized, PayloadType payload_type>
//__attribute__((always_inline))
inline int32_t Diva<int_optimized, payload_type>::GetMappedPos(const uint32_t implicit_part, const uint32_t size_grade,
                                                               const uint64_t implicit_scalar) const {
    uint32_t res = (implicit_part * size_scalars_[size_grade] * implicit_scalar)
                        >> (scale_shift + scale_implicit_shift);
    return std::min<uint32_t>(scaled_sizes_[size_grade] - 1, res);
}


template <bool int_optimized, PayloadType payload_type>
//__attribute__((always_inline))
inline uint64_t Diva<int_optimized, payload_type>::GetSlot(const InfixStore &store, const uint32_t pos) const {
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t bit_pos = 128 + infix_store_target_size + scaled_sizes_[size_grade] + pos * infix_size_;
    const uint8_t *ptr = ((uint8_t *) store.ptr) + bit_pos / 8;
    uint64_t value;
    memcpy(&value, ptr, sizeof(value));
    return (value >> bit_pos % 8) & BITMASK(infix_size_);
}


template <bool int_optimized, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<int_optimized, payload_type>::SetSlot(InfixStore &store, const uint32_t pos, const uint64_t value) {
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t bit_pos = 128 + infix_store_target_size + scaled_sizes_[size_grade] + pos * infix_size_;
    uint8_t *ptr = ((uint8_t *) store.ptr) + bit_pos / 8;
    uint64_t stamp;
    memcpy(&stamp, ptr, sizeof(stamp));
    stamp &= ~(BITMASK(infix_size_) << (bit_pos % 8));
    stamp |= value << (bit_pos % 8);
    memcpy(ptr, &stamp, sizeof(stamp));
}


template <bool int_optimized, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<int_optimized, payload_type>::SetSlot(InfixStore &store, const uint32_t pos,
                                                       const uint64_t value, const uint32_t width) {
#ifdef DEBUG
    assert(value > 0);
#endif // DEBUG
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t bit_pos = 128 + infix_store_target_size + scaled_sizes_[size_grade] + pos * width;
    uint8_t *ptr = ((uint8_t *) store.ptr) + bit_pos / 8;
    uint64_t stamp;
    memcpy(&stamp, ptr, sizeof(stamp));
    stamp &= ~(BITMASK(width) << (bit_pos % 8));
    stamp |= value << (bit_pos % 8);
    memcpy(ptr, &stamp, sizeof(stamp));
}


template <bool int_optimized, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<int_optimized, payload_type>::GetPayload(const InfixStore &store, const uint32_t pos,
                                                          uint64_t *payload, const uint32_t payload_offset) const {
#ifdef DEBUG
    static_assert(payload_type == PayloadType::FixedLength);
    assert(payload != nullptr);
#endif // DEBUG
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t bit_pos = 128 + infix_store_target_size + scaled_sizes_[size_grade] * (infix_size_ + 1) + pos * payload_size_;
    copy_bitmap_to_bitmap(store.ptr, bit_pos, payload, payload_offset, payload_size_);
}


template <bool int_optimized, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<int_optimized, payload_type>::GetSamplePayload(InfixStore &store, const uint32_t pos,
                                                                uint64_t *payload, const uint32_t payload_offset) const {
    const uint64_t *payload_list = reinterpret_cast<const uint64_t *>(store.ptr[1]);
    const uint32_t bit_pos = pos * payload_size_;
    copy_bitmap_to_bitmap(payload_list, bit_pos, payload, payload_offset, payload_size_);
}


template <bool int_optimized, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<int_optimized, payload_type>::SetPayload(InfixStore &store, const uint32_t pos,
                                                          const uint64_t *payload, const uint32_t payload_offset) {
#ifdef DEBUG
    static_assert(payload_type == PayloadType::FixedLength);
    assert(payload != nullptr);
#endif // DEBUG
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t bit_pos = 128 + infix_store_target_size + scaled_sizes_[size_grade] * (infix_size_ + 1) + pos * payload_size_;
    copy_bitmap_to_bitmap(payload, payload_offset, store.ptr, bit_pos, payload_size_);
}


template <bool int_optimized, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<int_optimized, payload_type>::AddSamplePayload(InfixStore &store,
                                                                const void *payload,
                                                                const uint32_t payload_offset) {
    uint64_t *payload_list = reinterpret_cast<uint64_t *>(store.ptr[1]);
    if (store.num_sample_payloads == 0) {
        const uint32_t malloc_size = (payload_size_ + 7) / 8;
        payload_list = reinterpret_cast<uint64_t *>(malloc(malloc_size));
        memset(payload_list, 0, malloc_size);
    }
    else {
        payload_list = reinterpret_cast<uint64_t *>(realloc(payload_list,
                                                            ((store.num_sample_payloads + 1) * payload_size_ + 7) / 8));
    }
    const uint32_t bit_pos = store.num_sample_payloads * payload_size_;
    copy_bitmap_to_bitmap(reinterpret_cast<const uint64_t *>(payload), payload_offset,
                          payload_list, bit_pos, payload_size_);
    store.num_sample_payloads++;
    store.ptr[1] = reinterpret_cast<uint64_t>(payload_list);
}


template <bool int_optimized, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<int_optimized, payload_type>::RemoveSamplePayload(InfixStore &store, const uint32_t pos) {
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


template <bool int_optimized, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<int_optimized, payload_type>::ShiftSlotsRight(const InfixStore &store, const uint32_t l, const uint32_t r,
                                                               const uint32_t shamt) {
#ifdef NAIVE_SLOT_SHIFT
    for (int32_t i = r - 1; i >= l; i--)
        SetSlot(store, i + shamt, GetSlot(store, i));
    for (int32_t i = l; i < l + shamt; i++)
        SetSlot(store, i, 0ULL);
#else
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t l_bit_pos = 128 + infix_store_target_size + scaled_sizes_[size_grade] + l * infix_size_;
    const uint32_t r_bit_pos = 128 + infix_store_target_size + scaled_sizes_[size_grade] + r * infix_size_ - 1;
    shift_bitmap_right(store.ptr, l_bit_pos, r_bit_pos, shamt * infix_size_);
#endif
}


template <bool int_optimized, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<int_optimized, payload_type>::ShiftSlotsLeft(const InfixStore &store, const uint32_t l, const uint32_t r,
                                                              const uint32_t shamt) {
#ifdef NAIVE_SLOT_SHIFT
    for (int32_t i = l; i < r; i--)
        SetSlot(store, i - shamt, GetSlot(store, i));
#else
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t l_bit_pos = 128 + infix_store_target_size + scaled_sizes_[size_grade] + l * infix_size_;
    const uint32_t r_bit_pos = 128 + infix_store_target_size + scaled_sizes_[size_grade] + r * infix_size_ - 1;
    shift_bitmap_left(store.ptr, l_bit_pos, r_bit_pos, shamt * infix_size_);
#endif
}


template <bool int_optimized, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<int_optimized, payload_type>::ShiftPayloadsRight(const InfixStore &store, const uint32_t l, const uint32_t r,
                                                                  const uint32_t shamt) {
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t l_bit_pos = 128 + infix_store_target_size + scaled_sizes_[size_grade] * (infix_size_ + 1) + payload_size_ * l;
    const uint32_t r_bit_pos = 128 + infix_store_target_size + scaled_sizes_[size_grade] * (infix_size_ + 1) + payload_size_ * r - 1;
    shift_bitmap_right(store.ptr, l_bit_pos, r_bit_pos, shamt * payload_size_);
}


template <bool int_optimized, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<int_optimized, payload_type>::ShiftPayloadsLeft(const InfixStore &store, const uint32_t l, const uint32_t r,
                                                                 const uint32_t shamt) {
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t l_bit_pos = 128 + infix_store_target_size + scaled_sizes_[size_grade] * (infix_size_ + 1) + payload_size_ * l;
    const uint32_t r_bit_pos = 128 + infix_store_target_size + scaled_sizes_[size_grade] * (infix_size_ + 1) + payload_size_ * r - 1;
    shift_bitmap_left(store.ptr, l_bit_pos, r_bit_pos, shamt * payload_size_);
}


template <bool int_optimized, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<int_optimized, payload_type>::MoveSlotsRight(const InfixStore &store, const uint32_t l, const uint32_t r,
                                                              const uint32_t shamt) {
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t l_bit_pos = 128 + infix_store_target_size + scaled_sizes_[size_grade] + l * infix_size_;
    const uint32_t r_bit_pos = 128 + infix_store_target_size + scaled_sizes_[size_grade] + r * infix_size_ - 1;
    move_bitmap_right(store.ptr, l_bit_pos, r_bit_pos, shamt * infix_size_);
    zero_out_bitmap(store.ptr, l_bit_pos, std::min(r_bit_pos, l_bit_pos + shamt * infix_size_ - 1));
}


template <bool int_optimized, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<int_optimized, payload_type>::MoveSlotsLeft(const InfixStore &store, const uint32_t l, const uint32_t r,
                                                             const uint32_t shamt) {
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t l_bit_pos = 128 + infix_store_target_size + scaled_sizes_[size_grade] + l * infix_size_;
    const uint32_t r_bit_pos = 128 + infix_store_target_size + scaled_sizes_[size_grade] + r * infix_size_ - 1;
    move_bitmap_left(store.ptr, l_bit_pos, r_bit_pos, shamt * infix_size_);
    zero_out_bitmap(store.ptr, std::max(l_bit_pos, r_bit_pos - shamt * infix_size_ + 1), r_bit_pos);
}


template <bool int_optimized, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<int_optimized, payload_type>::MovePayloadsRight(const InfixStore &store, const uint32_t l, const uint32_t r,
                                                                 const uint32_t shamt) {
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t l_bit_pos = 128 + infix_store_target_size + scaled_sizes_[size_grade] * (infix_size_ + 1) + payload_size_ * l;
    const uint32_t r_bit_pos = 128 + infix_store_target_size + scaled_sizes_[size_grade] * (infix_size_ + 1) + payload_size_ * r - 1;
    move_bitmap_right(store.ptr, l_bit_pos, r_bit_pos, shamt * payload_size_);
    zero_out_bitmap(store.ptr, l_bit_pos, std::min(r_bit_pos, l_bit_pos + shamt * payload_size_ - 1));
}


template <bool int_optimized, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<int_optimized, payload_type>::MovePayloadsLeft(const InfixStore &store, const uint32_t l, const uint32_t r,
                                                                const uint32_t shamt) {
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t l_bit_pos = 128 + infix_store_target_size + scaled_sizes_[size_grade] * (infix_size_ + 1) + payload_size_ * l;
    const uint32_t r_bit_pos = 128 + infix_store_target_size + scaled_sizes_[size_grade] * (infix_size_ + 1) + payload_size_ * r - 1;
    move_bitmap_left(store.ptr, l_bit_pos, r_bit_pos, shamt * payload_size_);
    zero_out_bitmap(store.ptr, std::max(l_bit_pos, r_bit_pos - shamt * payload_size_ + 1), r_bit_pos);
}


template <bool int_optimized, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<int_optimized, payload_type>::ZeroOutSlots(const InfixStore &store, const uint32_t l, const uint32_t r) {
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t l_bit_pos = 128 + infix_store_target_size + scaled_sizes_[size_grade] + l * infix_size_;
    const uint32_t r_bit_pos = 128 + infix_store_target_size + scaled_sizes_[size_grade] + r * infix_size_ - 1;
    zero_out_bitmap(store.ptr, l_bit_pos, r_bit_pos);
}


template <bool int_optimized, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<int_optimized, payload_type>::ZeroOutPayloads(const InfixStore &store, const uint32_t l, const uint32_t r) {
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t l_bit_pos = 128 + infix_store_target_size + scaled_sizes_[size_grade] * (infix_size_ + 1) + payload_size_ * l;
    const uint32_t r_bit_pos = 128 + infix_store_target_size + scaled_sizes_[size_grade] * (infix_size_ + 1) + payload_size_ * r - 1;
    zero_out_bitmap(store.ptr, l_bit_pos, r_bit_pos);
}


template <bool int_optimized, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<int_optimized, payload_type>::ShiftRunendsRight(const InfixStore &store, const uint32_t l, const uint32_t r, 
                                                                 const uint32_t shamt) {
    shift_bitmap_right(store.ptr + 2 + infix_store_target_size / 64, l, r - 1, shamt);
}


template <bool int_optimized, PayloadType payload_type>
//__attribute__((always_inline))
inline void Diva<int_optimized, payload_type>::ShiftRunendsLeft(const InfixStore &store, const uint32_t l, const uint32_t r,
                                                                const uint32_t shamt) {
    shift_bitmap_left(store.ptr + 2 + infix_store_target_size / 64, l, r - 1, shamt);
}


template <bool int_optimized, PayloadType payload_type>
//__attribute__((always_inline))
inline int32_t Diva<int_optimized, payload_type>::FindEmptySlotAfter(const InfixStore &store, const uint32_t runend_pos) const {
    const uint32_t size_grade = store.GetSizeGrade();
    int32_t current_pos = runend_pos;
    while (current_pos < scaled_sizes_[size_grade] && GetSlot(store, current_pos + 1)) {
        current_pos = NextRunend(store, current_pos);
    }
    return current_pos + 1;
}


template <bool int_optimized, PayloadType payload_type>
//__attribute__((always_inline))
inline int32_t Diva<int_optimized, payload_type>::FindEmptySlotBefore(const InfixStore &store, const uint32_t runend_pos) const {
    int32_t current_pos = runend_pos, previous_pos;
    do {
        previous_pos = current_pos;
        current_pos = PreviousRunend(store, current_pos);
    } while (current_pos >= 0 && GetSlot(store, current_pos + 1));

    do {
        previous_pos--;
    } while (current_pos < previous_pos && GetSlot(store, previous_pos));
    return previous_pos;

    // Maybe binary searching would be better?
    int32_t l = current_pos, r = previous_pos, mid;
    while (r - l > 1) {
        mid = (l + r) / 2;
        const bool cond = GetSlot(store, mid) == 0;
        l = cond ? mid : l;
        r = cond ? r : mid;
    }
    return l;
}


template <bool int_optimized, PayloadType payload_type>
inline void Diva<int_optimized, payload_type>::InsertRawIntoInfixStore(InfixStore &store, const uint64_t key,
                                                                       const uint32_t total_implicit,
                                                                       const uint64_t *payload) {
    if constexpr (payload_type == PayloadType::FixedLength) {
        assert(payload != nullptr);
    }
    uint32_t size_grade = store.GetSizeGrade();
    const uint32_t elem_count = store.GetElemCount();
    if (elem_count >= (size_grade ? scaled_sizes_[size_grade - 1] : exception_scaled_size_)) {
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

    uint32_t *popcnts = reinterpret_cast<uint32_t *>(store.ptr);
    uint64_t *occupieds = store.ptr + 2;
    uint64_t *runends = store.ptr + 2 + infix_store_target_size / 64;
    
    const int32_t mapped_pos = GetMappedPos(implicit_part, size_grade, implicit_scalar);
    const uint32_t key_rank = RankOccupieds(store, implicit_part);
    const bool is_occupied = get_bitmap_bit(occupieds, implicit_part);
    if (!is_occupied && GetSlot(store, mapped_pos) == 0) {
#ifdef DEBUG
        if (key_rank > 0) {
            const int32_t prev_runend_pos = SelectRunends(store, key_rank - 1);
            assert(prev_runend_pos < mapped_pos);
        }
#endif // DEBUG
        SetSlot(store, mapped_pos, explicit_part);
        if constexpr (payload_type == PayloadType::FixedLength)
            SetPayload(store, mapped_pos, payload, 0);
        set_bitmap_bit(runends, mapped_pos);
        popcnts[0] += implicit_part < infix_store_target_size / 2;
        popcnts[1] += mapped_pos < infix_store_target_size / 2;
    }
    else if (is_occupied) {
        const int32_t runend_pos = SelectRunends(store, key_rank);
        assert(runend_pos < static_cast<int32_t>(scaled_sizes_[size_grade]));
        const int32_t next_empty = FindEmptySlotAfter(store, mapped_pos);
#ifdef DEBUG
        const int32_t runstart_pos = std::max(key_rank ? static_cast<int32_t>(SelectRunends(store, key_rank - 1)) : -1,
                                          static_cast<int32_t>(FindEmptySlotBefore(store, runend_pos))) + 1;
        /*
        if (runend_pos - runstart_pos + 1 > 100) {
            std::cerr << "implicit_part=" << implicit_part << ' ' << runend_pos - runstart_pos << ' ';
            const uint64_t val = GetSlot(store, runstart_pos);
            for (int32_t i = infix_size_ - 1; i >= 0; i--)
                std::cerr << ((val >> i) & 1);
            std::cerr << std::endl;
        }
        */
        assert(next_empty >= scaled_sizes_[size_grade] || mapped_pos <= runend_pos);
#endif // DEBUG
        const int32_t previous_empty = FindEmptySlotBefore(store, mapped_pos);

        int32_t l = std::max(PreviousRunend(store, runend_pos), previous_empty);
        int32_t r = runend_pos + 1;
        int32_t mid;
        while (r - l > 1) {
            mid = (l + r) / 2;
            const uint64_t range_l = GetSlot(store, mid);
            const bool cond = (range_l - (range_l & -range_l)) <= explicit_part - 1;
            l = cond ? mid : l;
            r = cond ? r : mid;
        }
        if (next_empty < scaled_sizes_[size_grade]) {
            ShiftSlotsRight(store, r, next_empty, 1);
            if constexpr (payload_type == PayloadType::FixedLength)
                ShiftPayloadsRight(store, r, next_empty, 1);
            ShiftRunendsRight(store, runend_pos, next_empty, 1);
            if (runend_pos < infix_store_target_size / 2 
                    && infix_store_target_size / 2 <= next_empty)
                popcnts[1] -= get_bitmap_bit(runends, infix_store_target_size / 2);
            SetSlot(store, r, explicit_part);
            if constexpr (payload_type == PayloadType::FixedLength)
                SetPayload(store, r, payload, 0);
        }
        else {
            ShiftSlotsLeft(store, previous_empty + 1, r, 1);
            if constexpr (payload_type == PayloadType::FixedLength)
                ShiftPayloadsLeft(store, previous_empty + 1, r, 1);
            if (previous_empty + 1 <= infix_store_target_size / 2
                    && infix_store_target_size / 2 < std::min(runend_pos, r))
                popcnts[1] += get_bitmap_bit(runends, infix_store_target_size / 2);
            ShiftRunendsLeft(store, previous_empty + 1, std::min(runend_pos, r), 1);
            SetSlot(store, r - 1, explicit_part);
            if constexpr (payload_type == PayloadType::FixedLength)
                SetPayload(store, r - 1, payload, 0);
        }
    }
    else {
        const int32_t runend_pos = key_rank == 0 ? -1 : SelectRunends(store, key_rank - 1);
        assert(runend_pos < static_cast<int32_t>(scaled_sizes_[size_grade]));
        const int32_t next_empty = FindEmptySlotAfter(store, mapped_pos);
        if (next_empty < scaled_sizes_[size_grade]) {
            const int32_t shift_start = std::max(runend_pos + 1, mapped_pos);
            ShiftSlotsRight(store, shift_start, next_empty, 1);
            if constexpr (payload_type == PayloadType::FixedLength)
                ShiftPayloadsRight(store, shift_start, next_empty, 1);
            ShiftRunendsRight(store, shift_start, next_empty, 1);
            if (shift_start < infix_store_target_size / 2 
                    && infix_store_target_size / 2 <= next_empty)
                popcnts[1] -= get_bitmap_bit(runends, infix_store_target_size / 2);
            SetSlot(store, shift_start, explicit_part);
            if constexpr (payload_type == PayloadType::FixedLength)
                SetPayload(store, shift_start, payload, 0);
            set_bitmap_bit(runends, shift_start);
            popcnts[1] += shift_start < infix_store_target_size / 2;
        }
        else {
            const int32_t previous_empty = FindEmptySlotBefore(store, mapped_pos);
            const int32_t target_pos = std::max(runend_pos, previous_empty);
            ShiftSlotsLeft(store, previous_empty + 1, target_pos + 1, 1);
            if constexpr (payload_type == PayloadType::FixedLength)
                ShiftPayloadsLeft(store, previous_empty + 1, target_pos + 1, 1);
            if (previous_empty + 1 <= infix_store_target_size / 2 
                    && infix_store_target_size / 2 < target_pos + 1)
                popcnts[1] += get_bitmap_bit(runends, infix_store_target_size / 2);
            ShiftRunendsLeft(store, previous_empty + 1, target_pos + 1, 1);
            SetSlot(store, target_pos, explicit_part);
            if constexpr (payload_type == PayloadType::FixedLength)
                SetPayload(store, target_pos, payload, 0);
            set_bitmap_bit(runends, target_pos);
            popcnts[1] += target_pos < infix_store_target_size / 2;
        }
        popcnts[0] += implicit_part < infix_store_target_size / 2;
    }
    set_bitmap_bit(occupieds, implicit_part);
    store.UpdateElemCount(1);

    /*
#ifdef DEBUG
    {
        uint32_t occupied_count = 0, runend_count = 0;
        for (int32_t i = 0; i < infix_store_target_size / 64; i++)
            occupied_count += __builtin_popcountll(occupieds[i]);
        for (int32_t i = 0; i < scaled_sizes_[size_grade]; i++)
            runend_count += get_bitmap_bit(runends, i);
        assert(occupied_count == runend_count);

        uint32_t check_popcnts[2] = {};
        for (int32_t i = 0; i < infix_store_target_size / 128; i++) {
            check_popcnts[0] += __builtin_popcountll(occupieds[i]);
            if (static_cast<int32_t>(scaled_sizes_[size_grade]) - i * 64 > 0) {
                const uint64_t mask = BITMASK(std::min(64UL, scaled_sizes_[size_grade] - i * 64));
                check_popcnts[1] += __builtin_popcountll(runends[i] & mask);
            }
        }
        assert(popcnts[0] == check_popcnts[0]);
        assert(popcnts[1] == check_popcnts[1]);

        if (elem_count < infix_store_target_size) {
            uint64_t infix_list[infix_store_target_size];
            uint64_t payload_list[infix_store_target_size * payload_size_ / 64 + 20];
            assert(GetInfixList(store, infix_list, payload_list));
        }
    }
#endif // DEBUG
    */
}


template <bool int_optimized, PayloadType payload_type>
inline void Diva<int_optimized, payload_type>::DeleteRawFromInfixStore(InfixStore &store, const uint64_t key,
                                                                       const uint32_t total_implicit,
                                                                       std::function<bool(const uint64_t *)> should_remove) {
    uint32_t size_grade = store.GetSizeGrade();
    const uint32_t elem_count = store.GetElemCount();
    if (size_grade > 0 && elem_count <= (size_grade > 1 ? scaled_sizes_[size_grade - 2] : exception_scaled_size_))
        ResizeInfixStore(store, total_implicit);
    size_grade = store.GetSizeGrade();

    const uint64_t implicit_part = key >> infix_size_;
    const uint64_t explicit_part = key & BITMASK(infix_size_);
    const uint64_t implicit_scalar = implicit_scalars_[total_implicit - infix_store_target_size / 2];

    uint32_t *popcnts = reinterpret_cast<uint32_t *>(store.ptr);
    uint64_t *occupieds = store.ptr + 2;
    uint64_t *runends = store.ptr + 2 + infix_store_target_size / 64;
    const bool is_occupied = get_bitmap_bit(occupieds, implicit_part);
#ifdef DEBUG
    /*
    if (!is_occupied) 
        return;
    */
    assert(is_occupied);
#endif // DEBUG

    const uint32_t key_rank = RankOccupieds(store, implicit_part);
    const int32_t runend_pos = SelectRunends(store, key_rank);
    const int32_t runstart_pos = std::max(key_rank ? static_cast<int32_t>(SelectRunends(store, key_rank - 1)) : -1,
                                          static_cast<int32_t>(FindEmptySlotBefore(store, runend_pos))) + 1;
    const bool run_destroyed = runstart_pos == runend_pos;

    int32_t l = runstart_pos - 1, r = runend_pos + 1, mid;
    while (r - l > 1) {
        mid = (l + r) / 2;
        const uint64_t value = GetSlot(store, mid);
        const bool cond = (value & (value - 1)) <= explicit_part - 1;
        l = cond ? mid : l;
        r = cond ? r : mid;
    }
    int32_t match_pos;
    for (match_pos = l; match_pos >= runstart_pos; match_pos--) {
        const uint64_t value = GetSlot(store, match_pos);
        const uint64_t mask = ((value & -value) << 1) - 1;
        if constexpr (payload_type == PayloadType::FixedLength) {
            uint64_t payload[(payload_size_ + 63) / 64 + 1];
            GetPayload(store, match_pos, payload);
            if ((value | mask) == (explicit_part | mask) && (should_remove == nullptr || should_remove(payload)))
                break;
        }
        else {
            if ((value | mask) == (explicit_part | mask))
                break;
        }
    }
#ifdef DEBUG
    {
        /*
        if (match_pos < runstart_pos)
            return;
        */
        assert(match_pos >= runstart_pos);
        const uint64_t value = GetSlot(store, match_pos);
        /*
        if (value == 0)
            return;
        */
        assert(value);
        const uint64_t mask = ((value & -value) << 1) - 1;
        /*
        if ((value | mask) != (explicit_part | mask))
            return;
        */
        assert((value | mask) == (explicit_part | mask));
    }
#endif // DEBUG

    // TODO: Perhaps I can avoid shifting the cluster by changing the inserts?
    // But that may slow down inserts...
    
    bool found_empty_right = false;     // If we find an empty to the right, we
                                        // can be sure we have to shift to the
                                        // left. But if we don't, we have to do
                                        // further checking.
    int32_t cur_occupied = implicit_part;
    int32_t cur_runend = runend_pos, prev_runend;
    int32_t shift_start = -1, shift_end = -1;
    while (cur_runend < scaled_sizes_[size_grade]) {
        // Find last run that starts in its canonical slot.
        prev_runend = cur_runend;
        if (prev_runend + 1 < scaled_sizes_[size_grade] && GetSlot(store, prev_runend + 1) == 0) {
            found_empty_right = true;
            break;
        }
        cur_runend = NextRunend(store, cur_runend);
        cur_occupied = NextOccupied(store, cur_occupied);
        const uint32_t mapped_pos = GetMappedPos(cur_occupied, size_grade, implicit_scalar);
        if (shift_end == -1 && mapped_pos >= prev_runend + 1)
            shift_end = prev_runend;
    }
    shift_end = (shift_end == -1 ? prev_runend : shift_end);
    if (!found_empty_right) {
        // Check if should shift to the right.
        int32_t cur_occupied = implicit_part;
        int32_t cur_runend = PreviousRunend(store, runend_pos), prev_runend = runend_pos;
        while (cur_runend >= 0) {
            if (GetSlot(store, cur_runend + 1) == 0) {
                const int32_t runstart = FindEmptySlotBefore(store, prev_runend) + 1;
                const uint32_t mapped_pos = GetMappedPos(cur_occupied, size_grade, implicit_scalar);
                shift_start = (mapped_pos > runstart ? runstart : shift_start);
                break;
            }
            const uint32_t mapped_pos = GetMappedPos(cur_occupied, size_grade, implicit_scalar);
            shift_start = (mapped_pos > cur_runend + 1 ? cur_runend + 1 : shift_start);
            prev_runend = cur_runend;
            cur_runend = PreviousRunend(store, cur_runend);
            cur_occupied = PreviousOccupied(store, cur_occupied);
        }
        if (cur_runend < 0) {
            const uint32_t mapped_pos = GetMappedPos(cur_occupied, size_grade, implicit_scalar);
            const int32_t first_empty_slot_before = FindEmptySlotBefore(store, runend_pos);
            shift_start = (mapped_pos > first_empty_slot_before + 1 ? first_empty_slot_before + 1 : shift_start);
        }
    }
    else {
        const uint32_t mapped_pos = GetMappedPos(implicit_part, size_grade, implicit_scalar);
#ifdef DEBUG
        assert(mapped_pos <= runstart_pos);
#endif // DEBUG
    }

    if (shift_start == -1) {    // Shift to the left
        if (match_pos <= infix_store_target_size / 2 
                && infix_store_target_size / 2 < shift_end + 1)
            popcnts[1] += get_bitmap_bit(runends, infix_store_target_size / 2);
        ShiftSlotsLeft(store, match_pos + 1, shift_end + 1, 1);
        if constexpr (payload_type == PayloadType::FixedLength)
            ShiftPayloadsLeft(store, match_pos + 1, shift_end + 1, 1);
        ShiftRunendsLeft(store, match_pos + 1, shift_end + 1, 1);
        if (match_pos == shift_end) {
            SetSlot(store, match_pos, 0);
            if constexpr (payload_type == PayloadType::FixedLength) {
                const uint64_t zero_payload[(payload_size_ + 63) / 64 + 1] = {};
                SetPayload(store, match_pos, zero_payload);
            }
            reset_bitmap_bit(runends, match_pos);
        }
        if (!run_destroyed)
            set_bitmap_bit(runends, runend_pos - 1);
        else
            popcnts[1] -= runend_pos <= infix_store_target_size / 2;
    }
    else {  // Shift to the right
        ShiftSlotsRight(store, shift_start, match_pos, 1);
        if constexpr (payload_type == PayloadType::FixedLength)
            ShiftPayloadsRight(store, shift_start, match_pos, 1);
        ShiftRunendsRight(store, shift_start, match_pos, 1);
        if (match_pos == shift_start) {
            SetSlot(store, match_pos, 0);
            if constexpr (payload_type == PayloadType::FixedLength) {
                const uint64_t zero_payload[(payload_size_ + 63) / 64 + 1] = {};
                SetPayload(store, match_pos, zero_payload);
            }
            if (run_destroyed)
                reset_bitmap_bit(runends, runend_pos);
        }
        if (shift_start < infix_store_target_size / 2 
                && infix_store_target_size / 2 <= match_pos)
            popcnts[1] -= get_bitmap_bit(runends, infix_store_target_size / 2);
        if (!run_destroyed)
            set_bitmap_bit(runends, runend_pos);
        else
            popcnts[1] -= runend_pos < infix_store_target_size / 2;
    }
    
    if (run_destroyed) {
        reset_bitmap_bit(occupieds, implicit_part);
        popcnts[0] -= implicit_part < infix_store_target_size / 2;
    }
    store.UpdateElemCount(-1);

    /*
#ifdef DEBUG
    {
        uint32_t occupied_count = 0, runend_count = 0;
        for (int32_t i = 0; i < infix_store_target_size / 64; i++)
            occupied_count += __builtin_popcountll(occupieds[i]);
        for (int32_t i = 0; i < scaled_sizes_[size_grade]; i++)
            runend_count += get_bitmap_bit(runends, i);
        assert(occupied_count == runend_count);

        uint32_t check_popcnts[2] = {};
        for (int32_t i = 0; i < infix_store_target_size / 128; i++) {
            check_popcnts[0] += __builtin_popcountll(occupieds[i]);
            if (static_cast<int32_t>(scaled_sizes_[size_grade]) - i * 64 > 0) {
                const uint64_t mask = BITMASK(std::min(64UL, scaled_sizes_[size_grade] - i * 64));
                check_popcnts[1] += __builtin_popcountll(runends[i] & mask);
            }
        }
        assert(popcnts[0] == check_popcnts[0]);
        assert(popcnts[1] == check_popcnts[1]);

        if (elem_count < infix_store_target_size) {
            uint64_t infix_list[infix_store_target_size];
            GetInfixList(store, infix_list);
        }
    }
#endif // DEBUG
    */
}


template <bool int_optimized, PayloadType payload_type>
inline std::pair<uint64_t, uint32_t>
Diva<int_optimized, payload_type>::DeleteRawRangeFromInfixStore(InfixStore &store,
                                                                const uint64_t l_key,
                                                                const uint64_t r_key, 
                                                                const uint32_t total_implicit, 
                                                                std::function<bool(const uint64_t *)> should_remove) {
    uint32_t deleted_count = 0;

    const uint32_t size_grade = store.GetSizeGrade();
    const uint64_t implicit_part_l = l_key >> infix_size_;
    const uint64_t explicit_part_l = (l_key & BITMASK(infix_size_)) - 1;  // Remove the age counter bit
    const uint64_t implicit_part_r = r_key >> infix_size_;
    const uint64_t explicit_part_r = r_key & BITMASK(infix_size_);
    const uint64_t implicit_scalar = implicit_scalars_[total_implicit - infix_store_target_size / 2];

    uint64_t *occupieds = store.ptr + 2;
    uint64_t *runends = store.ptr + 2 + infix_store_target_size / 64;
    /*
#ifdef DEBUG
    {
        uint64_t infix_list[scaled_sizes_[size_grade]];
        uint64_t payload_list[scaled_sizes_[size_grade] * (payload_size_ + 63) / 64];
        const uint32_t sanity_infix_count = GetInfixList(store, infix_list, payload_list);
        assert(sanity_infix_count == store.GetElemCount());
    }
    {
        uint32_t occupied_count = 0, runend_count = 0;
        for (int32_t i = 0; i < infix_store_target_size / 64; i++)
            occupied_count += __builtin_popcountll(occupieds[i]);
        for (int32_t i = 0; i < scaled_sizes_[size_grade]; i++)
            runend_count += get_bitmap_bit(runends, i);
        assert(occupied_count == runend_count);
    }
#endif // DEBUG
    */
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
        if constexpr (PayloadType::FixedLength) {
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

    /*
#ifdef DEBUG
    assert(candidate_run_ind <= infix_store_target_size);
    for (uint32_t i = 0; i <= candidate_run_ind; i++) {
        for (uint32_t j = l[i]; j < r[i]; j++) {
            assert(GetSlot(store, j));
            uint64_t payload[(payload_size_ + 63) / 64];
            GetPayload(store, j, payload);
            assert(!should_remove(payload));
        }
    }
    for (uint32_t i = 0; i < orig_r[candidate_run_ind]; i++) {
        uint64_t payload[(payload_size_ + 63) / 64];
        GetPayload(store, i, payload);
        if (GetSlot(store, i))
            assert(!should_remove(payload));
        else {
            uint64_t zero[(payload_size_ + 63) / 64] = {0};
            assert(compare_bitmap_to_bitmap(payload, 0, zero, 0, payload_size_));
        }
    }
    {
        uint32_t sum = 0;
        for (uint32_t i = 0; i <= candidate_run_ind; i++)
            sum += (orig_r[i] - orig_l[i]) - (r[i] - l[i]);
        assert(sum == deleted_count);
    }
    {
        for (uint32_t i = 0; i <= candidate_run_ind; i++) {
            for (uint32_t j = l[i]; j < r[i]; j++)
                assert(GetSlot(store, j));
        }
    }
    for (uint32_t i = 0; i <= candidate_run_ind; i++) {
        assert(l[i] <= r[i]);
        assert(orig_l[i] <= orig_r[i]);
        if (i) {
            assert(orig_l[i] > orig_l[i - 1]);
            assert(orig_r[i] > orig_r[i - 1]);
        }
    }
#endif // DEBUG
    */

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
    uint32_t *popcnts = reinterpret_cast<uint32_t *>(store.ptr);
    popcnts[0] = popcnts[1] = 0;
    for (int32_t i = 0; i < infix_store_target_size / 128; i++) {
        popcnts[0] += __builtin_popcountll(occupieds[i]);
        if (scaled_sizes_[size_grade] > 64 * i) {
            const uint64_t mask = BITMASK(std::min(64UL, scaled_sizes_[size_grade] - 64 * i));
            popcnts[1] += __builtin_popcountll(runends[i] & mask);
        }
    }
    store.UpdateElemCount(-static_cast<int32_t>(deleted_count));

    /*
#ifdef DEBUG
    {
        uint32_t occupied_count = 0, runend_count = 0;
        for (int32_t i = 0; i < infix_store_target_size / 64; i++)
            occupied_count += __builtin_popcountll(occupieds[i]);
        for (int32_t i = 0; i < scaled_sizes_[size_grade]; i++)
            runend_count += get_bitmap_bit(runends, i);
        assert(occupied_count == runend_count);
    }
    {
        for (uint32_t i = 0; i <= candidate_run_ind; i++) {
            for (uint32_t j = l[i]; j < r[i]; j++)
                assert(GetSlot(store, j));
        }
    }
    {
        uint64_t infix_list[scaled_sizes_[size_grade]];
        uint64_t payload_list[scaled_sizes_[size_grade] * (payload_size_ + 63) / 64];
        const uint32_t sanity_infix_count = GetInfixList(store, infix_list, payload_list);
        assert(sanity_infix_count == store.GetElemCount());
    }
    for (uint32_t i = 0; i <= candidate_run_ind; i++) {
        for (uint32_t j = l[i]; j < r[i]; j++) {
            assert(GetSlot(store, j));
            uint64_t payload[(payload_size_ + 63) / 64];
            GetPayload(store, j, payload);
            assert(!should_remove(payload));
        }
    }
    for (uint32_t i = 0; i < orig_r[candidate_run_ind]; i++) {
        uint64_t payload[(payload_size_ + 63) / 64];
        GetPayload(store, i, payload);
        if (GetSlot(store, i))
            assert(!should_remove(payload));
        else {
            uint64_t zero[(payload_size_ + 63) / 64] = {0};
            assert(compare_bitmap_to_bitmap(payload, 0, zero, 0, payload_size_));
        }
    }
#endif // DEBUG
    */

    return {implicits[candidate_run_ind] + 1, deleted_count};
}


template <bool int_optimized, PayloadType payload_type>
inline uint32_t Diva<int_optimized, payload_type>::GetLongestMatchingInfixSize(const InfixStore &store, const uint64_t key,
                                                                               const uint32_t total_implicit,
                                                                               std::function<bool(const uint64_t*)> should_consider) const {
    const uint64_t implicit_part = key >> infix_size_;
    const uint64_t explicit_part = key & BITMASK(infix_size_);
    const uint64_t implicit_scalar = implicit_scalars_[total_implicit - infix_store_target_size / 2];

    uint64_t *occupieds = store.ptr + 2;
    if (!get_bitmap_bit(occupieds, implicit_part))
        return 0;   // No matching infix found

    const uint32_t key_rank = RankOccupieds(store, implicit_part);
    const int32_t runend_pos = SelectRunends(store, key_rank);
    const int32_t runstart_pos = std::max(key_rank ? static_cast<int32_t>(SelectRunends(store, key_rank - 1)) : -1,
                                          static_cast<int32_t>(FindEmptySlotBefore(store, runend_pos))) + 1;
    const bool run_destroyed = runstart_pos == runend_pos;

    int32_t l = runstart_pos - 1, r = runend_pos + 1, mid;
    while (r - l > 1) {
        mid = (l + r) / 2;
        uint64_t value = GetSlot(store, mid);
        value -= value & -value;
        if (value <= key - 1)
            l = mid;
        else 
            r = mid;
    }
    int32_t match_pos;
    for (match_pos = l; match_pos >= runstart_pos; match_pos--) {
        if constexpr (payload_type == PayloadType::FixedLength) {
            uint64_t payload[(payload_size_ + 63) / 64 + 1];
            GetPayload(store, match_pos, payload);
            if (!should_consider(payload))
                continue;
        }
        const uint64_t value = GetSlot(store, match_pos);
        const uint64_t mask = ((value & -value) << 1) - 1;
        if ((value | mask) == (explicit_part | mask))
            return infix_size_ - lowbit_pos(value);
    }
    return 0;   // No matching infix found
}


template <bool int_optimized, PayloadType payload_type>
inline bool Diva<int_optimized, payload_type>::RangeQueryInfixStore(InfixStore &store, const uint64_t l_key, const uint64_t r_key,
                                                                    const uint32_t total_implicit) const {
    const uint64_t l_implicit_part = l_key >> infix_size_;
    const uint64_t l_explicit_part = l_key & BITMASK(infix_size_);
    const uint64_t r_implicit_part = r_key >> infix_size_;
    const uint64_t r_explicit_part = r_key & BITMASK(infix_size_);
    const uint64_t *occupieds = store.ptr + 2;
    const uint64_t *runends = store.ptr + 2 + infix_store_target_size / 64;

    if (l_implicit_part < r_implicit_part) {
        if (NextOccupied(store, l_implicit_part) < r_implicit_part)
            return true;

        if (get_bitmap_bit(occupieds, r_implicit_part)) {
            const uint32_t r_rank = RankOccupieds(store, r_implicit_part);
            const uint32_t runend_pos = SelectRunends(store, r_rank);
            const uint32_t runstart_pos = std::max(r_rank ? static_cast<int32_t>(SelectRunends(store, r_rank - 1)) : -1,
                                                   static_cast<int32_t>(FindEmptySlotBefore(store, runend_pos))) + 1;
            const uint64_t slot_value = GetSlot(store, runstart_pos);
            if (slot_value - (slot_value & -slot_value) <= r_explicit_part)
                return true;
        }
        if (get_bitmap_bit(occupieds, l_implicit_part)) {
            const uint32_t l_rank = RankOccupieds(store, l_implicit_part);
            const uint32_t runend_pos = SelectRunends(store, l_rank);
            uint32_t pos = runend_pos;
            uint64_t slot_value = GetSlot(store, pos);
            do {
                if (l_explicit_part <= (slot_value | (slot_value - 1)))
                    return true;
                if (pos == 0)
                    break;
                slot_value = GetSlot(store, --pos);
            } while (slot_value && !get_bitmap_bit(runends, pos));
        }
        return false;
    }

    // l_implicit_part == r_implicit_part
    if (!get_bitmap_bit(occupieds, l_implicit_part))
        return false;
    const uint32_t rank = RankOccupieds(store, l_implicit_part);
    const uint32_t runend_pos = SelectRunends(store, rank);
    uint32_t pos = runend_pos;
    uint64_t slot_value = GetSlot(store, pos);

    // TODO: Faster implementation via broadword operations?
    do {
        if (l_explicit_part <= (slot_value | (slot_value - 1))
            && (slot_value - (slot_value & (-slot_value))) <= r_explicit_part)
            return true;
        if (pos == 0)
            break;
        slot_value = GetSlot(store, --pos);
    } while (slot_value && !get_bitmap_bit(runends, pos));
    return false;
}


template <bool int_optimized, PayloadType payload_type>
inline bool Diva<int_optimized, payload_type>::PointQueryInfixStore(InfixStore &store, const uint64_t key, const uint32_t total_implicit) const {
    const uint64_t implicit_part = key >> infix_size_;
    const uint64_t explicit_part = key & BITMASK(infix_size_);
    const uint64_t implicit_scalar = implicit_scalars_[total_implicit - infix_store_target_size / 2];

    const uint64_t *occupieds = store.ptr + 2;
    const uint64_t *runends = store.ptr + 2 + infix_store_target_size / 64;

    if (!get_bitmap_bit(occupieds, implicit_part))
        return false;

    const uint32_t rank = RankOccupieds(store, implicit_part);
    const uint32_t runend_pos = SelectRunends(store, rank);
    uint32_t pos = runend_pos;
    uint64_t slot_value = GetSlot(store, pos);
    // TODO: Faster implementation via broadword operations?
    do {
        const uint64_t mask = ((slot_value & (-slot_value)) << 1) - 1;
        if ((explicit_part | mask) == (slot_value | mask))
            return true;
        if (pos == 0)
            break;
        slot_value = GetSlot(store, --pos);
    } while (slot_value && !get_bitmap_bit(runends, pos));
    return false;
}


template <bool int_optimized, PayloadType payload_type>
inline void Diva<int_optimized, payload_type>::ResizeInfixStore(InfixStore &store, const uint32_t total_implicit) {
    // TODO: Optimize further?
    uint32_t size_grade = store.GetSizeGrade();
    const uint32_t infix_count = store.GetElemCount();
    const bool should_allocate_on_heap = infix_count > heap_alloc_threshold;

    uint64_t infix_list_contents[should_allocate_on_heap ? 1 : infix_count];
    uint32_t payload_list_size = 1;
    if constexpr (payload_type == PayloadType::FixedLength)
        payload_list_size = (payload_size_ * infix_count + 63) / 64 + 1;
    uint64_t payload_list_contents[should_allocate_on_heap ? 1 : payload_list_size];
    uint64_t *infix_list = infix_list_contents;
    uint64_t *payload_list = payload_list_contents;
    if (should_allocate_on_heap) {
        infix_list = new uint64_t[infix_count];
        if constexpr (payload_type == PayloadType::FixedLength)
            payload_list = new uint64_t[payload_list_size];
    }

    if constexpr (payload_type == PayloadType::FixedLength) {
        const uint32_t sanity_infix_count = GetInfixList(store, infix_list, payload_list);
#ifdef DEBUG
        assert(sanity_infix_count == infix_count);
#endif // DEBUG
    }
    else
        GetInfixList(store, infix_list);
    const uint64_t *sample_payload_list = reinterpret_cast<const uint64_t *>(store.ptr[1]);
    delete[] store.ptr;

    // Update `size_grade`
    if (infix_count >= (size_grade ? scaled_sizes_[size_grade - 1] : exception_scaled_size_))
        size_grade++;
    else {
        while (size_grade && infix_count <= (size_grade > 1 ? scaled_sizes_[size_grade - 2] : exception_scaled_size_))
            size_grade--;
    }

    store.SetSizeGrade(size_grade);
    const uint32_t next_size = scaled_sizes_[size_grade];
    const uint32_t word_count = InfixStore::GetPtrWordCount(next_size, infix_size_, payload_size_);
    uint64_t *new_ptr = new uint64_t[word_count];
    store.ptr = new_ptr;
    if constexpr (payload_type == PayloadType::FixedLength)
        LoadListToInfixStore(store, infix_list, infix_count, total_implicit, true, payload_list);
    else 
        LoadListToInfixStore(store, infix_list, infix_count, total_implicit, true);
    store.ptr[1] = reinterpret_cast<uint64_t>(sample_payload_list);

    if (should_allocate_on_heap) {
        delete[] infix_list;
        if constexpr (payload_type == PayloadType::FixedLength)
            delete[] payload_list;
    }
}


template <bool int_optimized, PayloadType payload_type>
inline void Diva<int_optimized, payload_type>::ShrinkInfixStoreInfixSize(InfixStore &store, const uint32_t new_infix_size) {
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t infix_count = store.GetElemCount();
    const uint32_t slot_count = scaled_sizes_[size_grade];

    InfixStore new_store(slot_count, new_infix_size, size_grade, payload_size_);

    // Copy the occupieds and runends bitmaps
    const uint32_t total_bitmap_size = 128 + infix_store_target_size + slot_count;
    memcpy(new_store.ptr, store.ptr, (total_bitmap_size + 7) / 8);
    uint8_t *new_store_byte_ptr = reinterpret_cast<uint8_t *>(new_store.ptr);
    new_store_byte_ptr[(total_bitmap_size + 7) / 8 - 1] &= BITMASK(total_bitmap_size % 8);

    for (int32_t i = 0; i < slot_count; i++) {
        const uint64_t old_slot = GetSlot(store, i);
        if (old_slot) {
            const uint64_t new_slot = (old_slot >> (infix_size_ - new_infix_size))
                                    | (infix_size_ > new_infix_size + lowbit_pos(old_slot) ? 1ULL : 0ULL);
            SetSlot(new_store, i, new_slot, new_infix_size);
        }
    }
    if constexpr (payload_type == PayloadType::FixedLength) {
        const uint32_t old_payloads_pos = 128 + infix_store_target_size + slot_count * (infix_size_ + 1);
        const uint32_t new_payloads_pos = 128 + infix_store_target_size + slot_count * (new_infix_size + 1);
        copy_bitmap_to_bitmap(store.ptr, old_payloads_pos, new_store.ptr, new_payloads_pos, slot_count * payload_size_);
    }
    new_store.ptr[1] = store.ptr[1];    // Transfer the sample payloads
    delete[] store.ptr;
    store.ptr = new_store.ptr;
}


template <bool int_optimized, PayloadType payload_type>
inline void Diva<int_optimized, payload_type>::LoadListToInfixStore(InfixStore &store, const uint64_t *list, const uint32_t list_len,
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
    store.SetElemCount(list_len);
    if (list_len == 0)
        return;

    const bool should_allocate_on_heap = list_len > heap_alloc_threshold;
    int32_t l_contents[should_allocate_on_heap ? 1 : list_len + 1];
    int32_t r_contents[should_allocate_on_heap ? 1 : list_len + 1];
    int32_t *l = l_contents, *r = r_contents;
    if (should_allocate_on_heap) {
        l = new int32_t[list_len + 1];
        r = new int32_t[list_len + 1];
    }
    int32_t ind = 0;

    // Make sure everything is in increasing order
    /*
#ifdef DEBUG
    for (int32_t i = 1; i < list_len; i++)
        assert((list[i - 1] >> infix_size_) <= (list[i] >> infix_size_));
    for (int32_t i = 0; i < list_len; i++)
        assert((list[i] >> infix_size_) < total_implicit);
    for (int32_t i = 1; i < list_len; i++)
        assert(list[i - 1] == list[i] || CompareInfixes(list[i - 1], list[i]));
#endif // DEBUG
    */

    uint64_t *occupieds = store.ptr + 2;
    uint64_t *runends = store.ptr + 2 + infix_store_target_size / 64;
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

    /*
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
    */

    if (should_allocate_on_heap) {
        delete[] l;
        delete[] r;
    }
}


template <bool int_optimized, PayloadType payload_type>
inline typename Diva<int_optimized, payload_type>::InfixStore Diva<int_optimized, payload_type>::AllocateInfixStoreWithList(const uint64_t *list,
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


template <bool int_optimized, PayloadType payload_type>
inline uint32_t Diva<int_optimized, payload_type>::GetInfixList(const InfixStore &store, uint64_t *res,
                                                                uint64_t *res_payload) const {
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t store_size = scaled_sizes_[size_grade];
    const uint64_t *occupieds = store.ptr + 2;
    const uint64_t *runends = store.ptr + 2 + infix_store_target_size / 64;
    uint64_t implicit_part = occupieds[0] & 1ULL ? 0 : NextOccupied(store, 0);
    uint32_t ind = 0;
    for (int32_t i = 0; i < store_size; i++) {
        const uint64_t explicit_part = GetSlot(store, i);
        if (explicit_part) {
#ifdef DEBUG
            assert(implicit_part < infix_store_target_size);
#endif // DEBUG
            res[ind] = (implicit_part << infix_size_) | explicit_part;
            if constexpr (payload_type == PayloadType::FixedLength) {
#ifdef DEBUG
                assert(res_payload != nullptr);
#endif // DEBUG
                const uint32_t slot_and_bitmap_size = 128 + infix_store_target_size + store_size * (infix_size_ + 1);
                const uint32_t pos_in = slot_and_bitmap_size + payload_size_ * i;
                const uint32_t pos_out = payload_size_ * ind;
                copy_bitmap_to_bitmap(store.ptr, pos_in, res_payload, pos_out, payload_size_);
            }
            ind++;
        }
        if (get_bitmap_bit(runends, i))
            implicit_part = NextOccupied(store, implicit_part);
    }

    /*
#ifdef DEBUG
    {
        for (int32_t i = 1; i < ind; i++)
            assert(res[i - 1] == res[i] || CompareInfixes(res[i - 1], res[i]));

        uint32_t occupied_count = 0, runend_count = 0;
        for (int32_t i = 0; i < infix_store_target_size / 64; i++)
            occupied_count += __builtin_popcountll(occupieds[i]);
        for (int32_t i = 0; i < scaled_sizes_[size_grade]; i++)
            runend_count += get_bitmap_bit(runends, i);
        assert(occupied_count == runend_count);

        uint32_t *popcnts = reinterpret_cast<uint32_t *>(store.ptr);
        uint32_t check_popcnts[2] = {};
        for (int32_t i = 0; i < infix_store_target_size / 128; i++) {
            check_popcnts[0] += __builtin_popcountll(occupieds[i]);
            if (static_cast<int32_t>(scaled_sizes_[size_grade]) - i * 64 > 0) {
                const uint64_t mask = BITMASK(std::min(64UL, scaled_sizes_[size_grade] - i * 64));
                check_popcnts[1] += __builtin_popcountll(runends[i] & mask);
            }
        }
        assert(popcnts[0] == check_popcnts[0]);
        assert(popcnts[1] == check_popcnts[1]);
    }
#endif // DEBUG
    */

    return ind;
}


template <bool int_optimized, PayloadType payload_type>
inline Diva<int_optimized, payload_type>::Iterator::Iterator(Diva<int_optimized, payload_type> *parent,
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


template <bool int_optimized, PayloadType payload_type>
inline Diva<int_optimized, payload_type>::Iterator::Iterator(Diva<int_optimized, payload_type> *parent,
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


template <bool int_optimized, PayloadType payload_type>
inline Diva<int_optimized, payload_type>::Iterator::Iterator(Diva<int_optimized, payload_type> *filter,
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


template <bool int_optimized, PayloadType payload_type>
inline typename Diva<int_optimized, payload_type>::Iterator& Diva<int_optimized, payload_type>::Iterator::operator=(const Iterator &other) {
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


template <bool int_optimized, PayloadType payload_type>
inline Diva<int_optimized, payload_type>::Iterator::Iterator(const Iterator& other):
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


template <bool int_optimized, PayloadType payload_type>
inline typename Diva<int_optimized, payload_type>::Iterator& Diva<int_optimized, payload_type>::Iterator::operator++() {
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


template <bool int_optimized, PayloadType payload_type>
inline typename Diva<int_optimized, payload_type>::Iterator Diva<int_optimized, payload_type>::Iterator::operator++(int) {
    auto old = *this;
    operator++();
    return old;
}


template <bool int_optimized, PayloadType payload_type>
inline std::pair<typename Diva<int_optimized, payload_type>::Iterator::KeyType, uint32_t> Diva<int_optimized, payload_type>::Iterator::operator*() {
    if (!IsValid()) {
        if constexpr (int_optimized)
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
    if constexpr (int_optimized) {
        uint64_t res = 0;
        memcpy(&res, current_key_contents_, key_length);
        return {__builtin_bswap64(res), bit_counts_[ind_]};
    }
    else {
        return {std::string(reinterpret_cast<const char *>(current_key_contents_), key_length),
                bit_counts_[ind_]};
    }
}


template <bool int_optimized, PayloadType payload_type>
inline bool Diva<int_optimized, payload_type>::Iterator::operator==(const Iterator& rhs) const {
    // TODO: Ensure this makes sense
    if (filter_ != rhs.filter_ || infixes_ != rhs.infixes_)
        return false;
    if (ind_ == rhs.ind_) {
        auto [fetch_a, bit_count_a] = *this;
        auto [fetch_b, bit_count_b] = *rhs;
        return fetch_a == fetch_b && bit_count_a == bit_count_b;
    }
    return false;
}


template <bool int_optimized, PayloadType payload_type>
inline bool Diva<int_optimized, payload_type>::Iterator::operator!=(const Iterator& rhs) const {
    return !(*this == rhs);
}


template <bool int_optimized, PayloadType payload_type>
inline void Diva<int_optimized, payload_type>::Iterator::GetPayload(uint64_t *out) const {
    static_assert(payload_type == PayloadType::FixedLength);
    assert(ind_ < infixes_.size());
    copy_bitmap_to_bitmap(payloads_.data(), filter_->payload_size_ * ind_,
                          out, 0,
                          filter_->payload_size_);
}


template <bool int_optimized, PayloadType payload_type>
inline void Diva<int_optimized, payload_type>::Iterator::SetDeleteFunction(std::function<bool(const uint64_t *)> should_remove) {
    should_remove_ = should_remove;
    first_store_to_fetched_and_delete_ = (should_remove != nullptr);
}


template <bool int_optimized, PayloadType payload_type>
inline void Diva<int_optimized, payload_type>::Iterator::SetNextToFetchFromExtraction(const InfiniteByteString key, uint64_t extraction) {
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

template <bool int_optimized, PayloadType payload_type>
inline void Diva<int_optimized, payload_type>::Iterator::Fetch() {
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
    if constexpr (int_optimized) {
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
                bit_counts_.push_back(8 * prev_key.length - infix_store.GetInvalidBits());
                payloads_.resize((filter_->payload_size_ * infixes_.size() + 63) / 64 + 1);
                filter_->GetSamplePayload(infix_store, i, payloads_.data(), filter_->payload_size_ * (infixes_.size() - 1));
            }
        }
        else {
            infixes_.push_back(0);
            bit_counts_.push_back(8 * prev_key.length - infix_store.GetInvalidBits());
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
    const uint64_t implicit_scalar = filter_->implicit_scalars_[total_implicit - infix_store_target_size / 2];

    const uint64_t *occupieds = infix_store.ptr + 2;
    const uint64_t *runends = infix_store.ptr + 2 + infix_store_target_size / 64;

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


template <bool int_optimized, PayloadType payload_type>
inline void Diva<int_optimized, payload_type>::Iterator::FetchDelete() {
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
    if constexpr (int_optimized) {
        prev_key_word = *reinterpret_cast<const uint64_t *>(prev_key.str);
        prev_key.str = reinterpret_cast<const uint8_t *>(&prev_key_word);
        next_key_word = *reinterpret_cast<const uint64_t *>(next_key.str);
        next_key.str = reinterpret_cast<const uint8_t *>(&next_key_word);
    }

    InfixStore& infix_store = *infix_store_ptr;
    rwlock_lock_write(infix_store.rwlock);
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
            if (infix_store.GetElemCount() <= threshold)
                filter_->ResizeInfixStore(infix_store, total_implicit);
        }
    }

    rwlock_unlock_write(infix_store.rwlock);
}


template <bool int_optimized, PayloadType payload_type>
inline typename Diva<int_optimized, payload_type>::Iterator Diva<int_optimized, payload_type>::GetIterator(std::string_view start,
                                                                                std::string_view end,
                                                                                std::function<bool(const uint64_t *)> should_remove) {
    return Iterator(this, start, end, should_remove);
}


template <bool int_optimized, PayloadType payload_type>
inline typename Diva<int_optimized, payload_type>::Iterator Diva<int_optimized, payload_type>::GetIterator(const uint8_t *start, const uint32_t start_len,
                                                                                                        const uint8_t *end, const uint32_t end_len,
                                                                                                        std::function<bool(const uint64_t *)> should_remove) {
    return Iterator(this, start, start_len, end, end_len, should_remove);
}


template <bool int_optimized, PayloadType payload_type>
inline typename Diva<int_optimized, payload_type>::Iterator Diva<int_optimized, payload_type>::GetIterator(uint64_t start, uint64_t end,
                                                                            std::function<bool(const uint64_t *)> should_remove) {
    return Iterator(this, start, end, should_remove);
}

template <bool int_optimized, PayloadType payload_type>
inline Diva<int_optimized, payload_type>::Iterator::~Iterator() {
    infixes_.clear();
    bit_counts_.clear();
    payloads_.clear();
}


template <bool int_optimized, PayloadType payload_type>
inline bool Diva<int_optimized, payload_type>::Iterator::IsValid() const {
    return ind_ < infixes_.size() || next_to_fetch_.str;
}

}

