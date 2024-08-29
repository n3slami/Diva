#ifndef DST_H_
#define DST_H_

#include <vector>
#include <memory>
#include <cassert>
#include <functional>
#include <arpa/inet.h>
#include <cstring>
#include <map>
#include <vector>
//#include "../SuRF_standalone/SuRF/include/surf.hpp"
#include "MurmurHash3.h"
//#include <surf.hpp>

#ifdef USE_DTL
#include <dtl/filter/blocked_bloomfilter/blocked_bloomfilter.hpp>
#include <dtl/filter/blocked_bloomfilter/blocked_bloomfilter_tune.hpp>
#include <dtl/filter/bbf_64.hpp>
#endif

using namespace std;

static char global_name='A';

vector<size_t> calc_dst(vector<size_t> dist, double bpk, vector<size_t> qdist, size_t cutoff = 0);

template<typename T>
class FastModulo {
    private:
        T num_;

    public:
        FastModulo(T num): num_(num) { }
        friend T operator%(const T &v, const FastModulo &u) {
            return v%u.num_;
        }
        T value() {
            return num_;
        }
};

class Bitwise {
    private:
        uint8_t *data_ = 0;
        size_t len_;
        bool is_mut_; // Create immutable views when copying data that will
                      // last less than the mutable version. Much faster than shared_ptr. A
                      // smart person would have done this through inheritance, but I am not
                      // a smart person.

    public:
        Bitwise(const string &key): len_(8*(key.size()+1)), is_mut_(true) {
            if (len_ == 0) { return; }
            data_ = new uint8_t[len_/8];
            assert(data_);
            memcpy(data_, key.c_str(), len_/8);
        }

        Bitwise(const uint32_t &key): len_(8*sizeof(uint32_t)), is_mut_(true) {
            if (len_ == 0) { return; }
            data_ = new uint8_t[len_/8];
            assert(data_);
            uint32_t tmp = htonl(key);
            memcpy(data_, &tmp, len_/8);
        }

        Bitwise(const uint64_t &key): len_(8*sizeof(uint64_t)), is_mut_(true) {
            if (len_ == 0) { return; }
            data_ = new uint8_t[len_/8];
            assert(data_);
            uint64_t tmp = __builtin_bswap64(key);
            memcpy(data_, &tmp, len_/8);
        }

        Bitwise(bool init, size_t len): len_(len), is_mut_(true) {
            if (len_ == 0) { return; }
            data_ = new uint8_t[(len_+7)/8]();
            assert(data_);
            if (init == 1) {
                assert(false);
                memset(data_, 255, (len_+7)/8);
            }
        }

        Bitwise(const uint8_t *data, size_t nbytes): len_(8*nbytes), is_mut_(true) {
            if (len_ == 0) { return; }
            data_ = new uint8_t[len_/8];
            assert(data_);
            memcpy(data_, data, len_/8);
        }

        Bitwise(uint8_t *data, size_t nbytes, bool is_mut): data_(data), len_(8*nbytes), is_mut_(is_mut) { }

        // Copy constructor should only be used when adding new levels to a non-empty DST
        Bitwise(const Bitwise &other): len_(other.size()), is_mut_(other.is_mut_) {
            assert(false);
            if (is_mut_) {
                data_ = new uint8_t[len_/8];
                assert(data_);
                memcpy(data_, other.data(), len_/8);
            }
            else {
                data_ = other.data();
            }
        }

        Bitwise(Bitwise &&other) noexcept {
            len_ = other.size();
            data_ = other.data();
            if (other.is_mut_) {
                other.is_mut_ = false;
                is_mut_ = true;
            }
            else {
                is_mut_ = false;
            }
        }

        // Needs to exist for some reason, but should never be called.
        Bitwise &operator=(const Bitwise &other) {
            assert(false);
            len_ = other.size();
            data_ = other.data();
            is_mut_ = false;
            return *this;
        }

        Bitwise(const Bitwise &other, size_t len): len_(len) {
            if (len <= other.size()) {
                data_ = other.data();
                is_mut_ = false;
            }
            else {
                data_ = new uint8_t[(len_+7)/8]();
                memcpy(data_, other.data(), other.size()/8);
                if (other.size()%8 != 0) {
                    assert(false);
                }
                is_mut_ = true;
            }
        }

        ~Bitwise() {
            if (is_mut_ and data_) {
                delete[] data_;
            }
        }

        uint64_t hash(uint32_t seed, FastModulo<uint64_t> modulo) const {
            if (modulo.value() < (1ULL<<32)) {
                uint32_t h;
                MurmurHash3_x86_32(data(), size()/8, seed, &h);
                if (size()%8 != 0) {
                    uint32_t h2;
                    uint8_t tmp = data()[size()/8];
                    tmp >>= (8-(size()%8));
//                    tmp &= (((1<<8)-1) ^ ((1<<(7-(size()%8)))-1));
                    MurmurHash3_x86_32(&tmp, 1, seed, &h2);
                    h ^= h2;
                }
                return h % modulo;
            }
            uint64_t h[2];
            MurmurHash3_x64_128(data(), size()/8, seed, h);
            if (size()%8 != 0) {
                uint64_t h2[2];
                uint8_t tmp = data()[size()/8];
                tmp >>= (8-(size()%8));
//                tmp &= (((1<<8)-1) ^ ((1<<(7-(size()%8)))-1));
                MurmurHash3_x64_128(&tmp, 1, seed, h2);
                h[0] ^= h2[0];
            }
            return h[0] % modulo;
        }

        uint64_t to_uint64() const {
            if (len_<=64) {
                uint64_t out = 0;
                memcpy(&out, data_, (len_+7)/8);
                out = __builtin_bswap64(out);
                out >>= (64-len_);
                out <<= (64-len_);
                //out = __builtin_bswap64(out);
                return out;
            }
            return 0; // TODO
        }

        void print() const {
            for (size_t i=0; i<len_; ++i) {
                printf("%d", get(i));
            }
        }

        bool get(size_t i) const {
            return (data_[i/8] >> (7-(i%8))) & 1;
        }

        void flip(size_t i) {
            assert(is_mut_);
            data_[i/8] ^= (1<<(7-(i%8)));
        }

        void set(size_t i, bool v) {
            assert(is_mut_);
            if (get(i) != v) {
                flip(i);
            }
        }

        size_t lcp(const Bitwise &other) const {
            size_t out = 0;
            while (out+64 <= other.size() && out+64 <= size() && ((uint64_t*)data_)[out/64] == ((uint64_t*)other.data())[out/64]) {
                out += 64;
            }
            while (out+32 <= other.size() && out+32 <= size() && ((uint32_t*)data_)[out/32] == ((uint32_t*)other.data())[out/32]) {
                out += 32;
            }
            while (out+16 <= other.size() && out+16 <= size() && ((uint16_t*)data_)[out/16] == ((uint16_t*)other.data())[out/16]) {
                out += 16;
            }
            while (out+8 <= other.size() && out+8 <= size() && data_[out/8] == other.data()[out/8]) {
                out += 8;
            }
            while (out+1 <= other.size() && out+1 <= size() && get(out) == other.get(out)) {
                out += 1;
            }
            return out;
        }

        size_t size() const {
            return len_;
        }

        uint8_t *data() const {
            return data_;
        }
};

class Filter {
    public:
        Filter(){};
        virtual ~Filter(){};
        virtual void AddKeys(const vector<Bitwise> &keys) = 0;
        virtual void AddKey(const Bitwise key) = 0;
        virtual bool Query(const Bitwise &key) = 0;
        virtual pair<uint8_t*, size_t> serialize() const = 0;
        static pair<Filter*, size_t> deserialize(uint8_t* ser);
};

template<bool keep_stats=false>
class BloomFilter final : public Filter {
    private:
        Bitwise data_;
        size_t nhf_;
        size_t nkeys_;
        vector<size_t> seeds_;
        FastModulo<uint64_t> nmod_;

    public:
        size_t nqueries_ = 0;
        size_t npositives_ = 0;

        BloomFilter(size_t nbits): data_(Bitwise(false, ((nbits+7)/8)*8)), nhf_(0), nkeys_(0), nmod_(((nbits+7)/8*8)) {}
        BloomFilter(size_t nbits, uint8_t* data, size_t nhf, size_t nkeys, vector<size_t> seeds): data_(Bitwise(data, nbits/8, false)), nhf_(nhf), nkeys_(nkeys), seeds_(seeds), nmod_(nbits) {}

        void AddKeys(const vector<Bitwise> &keys);
        void AddKey(const Bitwise key);
        bool Query(const Bitwise &key);
        pair<uint8_t*, size_t> serialize() const;
        static pair<BloomFilter<keep_stats>*, size_t> deserialize(uint8_t* ser);
        void printStats() const {
            assert(keep_stats);
            printf("#queries: %lu, #positives: %lu\n", nqueries_, npositives_);
        }
        void printMyStats() const {
            printf("Inside BF printMyStats()\n");
            printf("#queries: %lu, #positives: %lu\n", nqueries_, npositives_);
        }
};

#ifdef USE_DTL
class DtlBlockedBloomFilter final: public Filter {
    private:
        Bitwise data_;
        size_t nkeys_;
//        dtl::blocked_bloomfilter<uint64_t> *b_;
        dtl::bbf_64 *b_;

    public:
        DtlBlockedBloomFilter(size_t nbits): data_(Bitwise(false, ((nbits+7)/8)*8)) {}
        //~DtlBlockedBloomFilter() { delete b_; }

        void AddKeys(const vector<Bitwise> &keys);
        bool Query(const Bitwise &key);
        pair<uint8_t*, size_t> serialize() const;
};
#endif


template<class FilterClass, bool keep_stats=false>
class DstFilter final: public Filter {
    private:
        size_t diffidence_, maxlen_, nkeys_, diffidence_level_;
        vector<FilterClass> bfs_;
        function<vector<size_t> (vector<size_t>)> get_nbits_;

    public:
        char name;
        size_t nqueries_ = 0;
        size_t npositives_ = 0;
        vector<size_t> qdist_;
        //static std::map<char, DstFilter<BloomFilter<>>*> mapping; // Subarna

        DstFilter(size_t diffidence, size_t diffidence_level, function<vector<size_t> (vector<size_t>)> get_nbits): diffidence_(diffidence), maxlen_(0), nkeys_(0), diffidence_level_(diffidence_level), get_nbits_(get_nbits), name(global_name) {
            global_name++;
            static_assert(is_base_of<Filter, FilterClass>::value, "DST template argument must be a filter");
        }

        void AddKeys(const vector<Bitwise> &keys);
        void AddKey(const Bitwise key);
        bool Doubt(Bitwise *idx, size_t &C, size_t level, size_t maxlevel);
        Bitwise *GetFirst(const Bitwise &from, const Bitwise &to);
        bool Query(const Bitwise &key);
        bool Query(const Bitwise &from, const Bitwise &to);
        pair<uint8_t*, size_t> serialize() const;
        pair<uint8_t*, size_t> serialize1() const;
        static pair<DstFilter*, size_t> deserialize(uint8_t* ser);
        static pair<DstFilter*, size_t> deserialize1(uint8_t* ser);
        static pair<DstFilter*, size_t> deserialize2(uint8_t* ser);
        void printStats() const {
            assert(keep_stats);
            printf("DST total stats: #queries: %lu, #positives: %lu\n", nqueries_, npositives_);
            printf("Stats for each bf:\n");
            for (auto &bf: bfs_) {
                printf("\t");
                bf.printStats();
            }
            printf("Query distribution:\n");
            for (auto &i: qdist_) {
                printf("\t%lu\n", i);
            }
        }

        // void printMyStats() const {
        //     std::cout << "Rosetta: " << name << std::endl;
        //     std::cout << "diffidence_: " << diffidence_ << " maxlen_: " << maxlen_ << " nkeys_: " << nkeys_ << " diffidence_level_: " << diffidence_level_ << std::endl;
        //     printf("Rosetta total stats: #queries: %lu, #positives: %lu\n", nqueries_, npositives_);
        //     printf("Stats for each bf:\n");
        //     for (auto &bf: bfs_) {
        //         printf("\t");
        //         bf.printMyStats();
        //     }
        //     printf("Query distribution:\n");
        //     for (auto &i: qdist_) {
        //         printf("\t%lu\n", i);
        //     }
        // }
};


class Helper
{
    public:
        static std::map< char, DstFilter<BloomFilter<>>* > mapping; // Subarna
        static bool flag;

        static void printRosetta(DstFilter<BloomFilter<>>* dst_obj)
        {
            //dst_obj->printMyStats();
            dst_obj->printStats();
        }
        static DstFilter<BloomFilter<>>* addToMap(size_t diffidence, size_t diffidence_level, size_t name)
        {
            //std::cout << "Inside addToMap(): Looking for Rosetta " << (char)name << std::endl;
            flag = false;
            map<char, DstFilter<BloomFilter<>>*>::iterator it = mapping.find((char)name);
            if(it != Helper::mapping.end())
            {
                printf("Rosetta %c Found\n", (char)name);
                flag = true;
                map<char, DstFilter<BloomFilter<>>*>::iterator itr;
              //  for (itr = mapping.begin(); itr != mapping.end(); ++itr) { 
                //    cout << '\t' << itr->first << '\t' << itr->second << '\n'; 
               // } 
                return it->second;
            }
            //std::cout << "Rosetta " << (char)name << " not found - TBA\n";
            // Rosetta instance not in mapping table
            DstFilter<BloomFilter<>>* out = new DstFilter<BloomFilter<>>(diffidence, diffidence_level, [](vector<size_t> distribution) -> vector<size_t> { assert(false); return distribution;});
            out->name = name;
            mapping.insert(pair<char, DstFilter<BloomFilter<>>*>(name, out)); // Subarna
            //std::cout << "Rosetta " << out->name << " added to map\n";
            return out;
        }
        static void updateMap(DstFilter<BloomFilter<>>* out, uint8_t* pos, uint8_t* ser)
        {
            map<char, DstFilter<BloomFilter<>>*>::iterator it = mapping.find(out->name);
            memcpy(it->second, out, pos-ser);
        }
};


#endif
