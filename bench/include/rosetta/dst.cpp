#include <vector>
#include <cassert>
#include <memory>
#include <cstring>
#include <cmath>
#include <random>
#include <iostream>
#include <chrono>

#include "dst.h"

bool Helper::flag = false;
std::map<char, DstFilter<BloomFilter<>>*> Helper::mapping = {{' ', NULL}}; // Subarna

// DstFilter<Filter, false>::mapping = {{' ', NULL}};
// template<class FilterClass, bool >::DstFilter<FilterClass> kool = 0;

vector<size_t> calc_dst_origin(vector<size_t> dist, double bpk, vector<size_t> qdist, size_t cutoff) {
    vector<size_t> out(dist.size(), 0);
    size_t bloom_cost = (sizeof(BloomFilter<>) + sizeof(size_t))*8;
    size_t totbits = (size_t)((long double)bpk*dist[dist.size()-1]);

    long double c=0;
    size_t mqdist = 1000000000;
    for (size_t i=0; i<qdist.size(); ++i) {
        if (qdist[i] > 0) {
            totbits = totbits>bloom_cost?totbits-bloom_cost:0;
            mqdist = min(mqdist, qdist[i]);
        }
    }

    long double lo=1/(long double)mqdist, hi=100000, mid;
    size_t bitsused = 0;
    while (fabs(hi-lo) > 1/(long double)mqdist/10) {
        mid = (lo+hi)/2;

        bitsused = 0;
        bool ok = true;
        for (size_t i=0; i<dist.size()-1-cutoff; ++i) {
            if (dist[i] < (1ULL<<(i+1)) and qdist[i] > 0) {
                long double next_fpr = 1/(mid*qdist[i+1]);
                long double this_fpr = 1/(mid*qdist[i]);
                long double bloom_fpr = this_fpr/(2-next_fpr)/next_fpr;
                bloom_fpr = min(bloom_fpr, (long double)1);
                bloom_fpr = max(bloom_fpr, (long double)1e-12);
                bitsused += (size_t)ceil((long double)dist[i]*1.44*log2(1/bloom_fpr));
                if (bitsused > totbits) {
                    ok = false;
                    break;
                }
            }
        }
        bitsused += (size_t)ceil((long double)dist[dist.size()-1]*1.44*log2(mid*qdist[dist.size()-1]));
        if (ok and bitsused <= totbits) {
            lo = mid;
        }
        else {
            hi = mid;
        }
    }
    if (lo < 1e-12) {
        printf("Not enough memory allowance\n");
        exit(0);
    }
    c = lo;

    bitsused = 0;
    for (size_t i=0; i<dist.size()-1-cutoff; ++i) {
        if (dist[i] < (1ULL<<(i+1)) and qdist[i] > 0 and bitsused < totbits) {
            long double next_fpr = 1/(c*qdist[i+1]);
            long double this_fpr = 1/(c*qdist[i]);
            long double bloom_fpr = this_fpr/(2-next_fpr)/next_fpr;
            bloom_fpr = min(bloom_fpr, (long double)1);
            bloom_fpr = max(bloom_fpr, (long double)1e-12);
            out[i] = min((size_t)ceil((long double)dist[i]*1.44*log2(1/bloom_fpr)), totbits-bitsused);
            bitsused += out[i];
        }
    }
    out[dist.size()-1] = bitsused<=totbits?totbits-bitsused:0;
    bitsused += out[dist.size()-1];
    if (bitsused > totbits) {
        printf("Not enough memory allowance, bitsused: %lu, totbits: %lu\n", bitsused, totbits);
        exit(0);
    }
//    for (size_t i=0; i<out.size(); ++i) {
//        if(qdist[i])
//        printf("%lu: %lu, %lu bits, %lu prefixes (%lf bits per key)\n", i, qdist[i], out[i], dist[i], (double)out[i]/dist[out.size()-1]);
//    }
    return out;
}
vector<size_t> calc_dst_largest_level(vector<size_t> dist, double bpk, vector<size_t> qdist, size_t cutoff) {
    vector<size_t> out(dist.size(), 0);
    out[out.size()-1]=bpk*dist[out.size()-1];
   // out[out.size()-2]=1*dist[out.size()-1];
    return out;
}
vector<size_t> calc_dst_math_model(vector<size_t> dist, double bpk, vector<size_t> qdist, size_t cutoff) {
    vector<size_t> out(dist.size(), 0);
    // printf("bpk: %f\n", bpk);
    double valid_levels=0.0;
    for (int i=qdist.size()-1; i>=0; --i) {
        if(qdist[i])
            valid_levels+=1.0;
    }
    double adjust_constant=1.0-0.5/valid_levels;
    // printf("adjust: %f\n", adjust_constant);
    long double multiplies = 1.0/exp(bpk*log(2)*log(2)*adjust_constant);
    
    // printf("qdist size: %d\n", qdist.size());
    for (int i=qdist.size()-1; i>=0; --i) {
        // printf("%d\n", i);
        if(qdist[i]){
            long p=2*qdist[i];
            for(int j=i+1;j<qdist.size();j++){
                qdist[j]+=p;
                p*=2;
            }
        }
    }
    for(int i=0;i< qdist.size();i++){
        if(qdist[i]){
            // printf("multi: %lf\n", multiplies);
            multiplies*=qdist[i];
        }
    }
    
    // printf("multi: %lf\n", multiplies);
    double standard=pow(multiplies, 1.0/valid_levels);
    // printf("standard: %lf\n", standard);
    for(size_t i=0; i<out.size(); ++i) {
        if(qdist[i]){
            // printf("standard/qdist[i]: %lf\n", standard/qdist[i]);
            if(standard/qdist[i]<1){
                out[i]=-log(standard/qdist[i])*dist[out.size()-1]/log(2)/log(2)/adjust_constant;
            }
            else
            {
                qdist[i]=0;
                return calc_dst_math_model(dist, bpk, qdist, cutoff);
                out[i]=0.0;
            }
            
        }
        else
        {
            out[i]=0.0;
        }
        
    }
    double sum_bits=0.0;
    for (size_t i=0; i<out.size(); ++i) {
        sum_bits+=(double)out[i]/dist[out.size()-1];
    }
    // printf("bpk: %f sum_bits: %f\n", bpk, sum_bits);
    // double ratio = bpk/sum_bits;
    // printf("%f \n", ratio);
    // for (size_t i=0; i<out.size(); ++i) {
    //     out[i]*=ratio;
    // }
//    for (size_t i=out.size()-9; i<out.size(); ++i) {
//     //    if(qdist[i])
//        printf("%lu: %lu, %lu bits, %lu prefixes (%lf bits per key)\n", i, qdist[i], out[i], dist[i], (double)out[i]/dist[out.size()-1]);
//    }
    return out;
}

vector<size_t> calc_dst(vector<size_t> dist, double bpk, vector<size_t> qdist, size_t cutoff){
    double valid_levels = 0.0;
    for(int i=0;i<qdist.size();i++){
        if(qdist[i])
            valid_levels+=1.0;
    }
    // printf("valid levels: %d\n", valid_levels);
    if(valid_levels<=4.0){// approximately estimate that the max range is at most 16.
        return calc_dst_largest_level(dist, bpk, qdist, cutoff);
    }
    else{
        return calc_dst_math_model(dist, bpk, qdist, cutoff);
    }
}

template<bool keep_stats>
void BloomFilter<keep_stats>::AddKeys(const vector<Bitwise> &keys) {
    if (data_.size() == 0) return;
    nkeys_ = keys.size();
    nhf_ = (size_t)(round(log(2)*data_.size()/nkeys_));
    nhf_ = (nhf_==0?1:nhf_);
    seeds_.resize(nhf_);
    mt19937 gen(1337);
    for (size_t i=0; i<nhf_; ++i) {
        seeds_[i] = gen();
    }

    for (auto &key: keys) {
        //printf("Insert key ");
        //key.print();
        //printf(" (%s)\n", key.data());
        for (size_t i=0; i<nhf_; ++i) {
            data_.set(key.hash(seeds_[i], nmod_), 1);
        }
    }
}

template<bool keep_stats>
void BloomFilter<keep_stats>::AddKey(const Bitwise key) {
    for (size_t i=0; i<nhf_; ++i) {
        data_.set(key.hash(seeds_[i], nmod_), 1);
    }
}

template<bool keep_stats>
bool BloomFilter<keep_stats>::Query(const Bitwise &key) {
    if (data_.size() == 0) return true;
    bool out=true;
    for (size_t i=0; i<nhf_ && out; ++i) {
        out &= data_.get(key.hash(seeds_[i], nmod_));
    }

    if (keep_stats) {
        nqueries_ += 1;
        npositives_ += out;
    }
    return out;
}

template<bool keep_stats>
pair<uint8_t*, size_t> BloomFilter<keep_stats>::serialize() const {
    //printf("HERE1\n"); // Subarna
    size_t len = sizeof(size_t) + sizeof(size_t) + sizeof(size_t) + seeds_.size()*sizeof(size_t) + (data_.size()+7)/8;
    uint8_t *out = new uint8_t[len];
    uint8_t *pos = out;

    memcpy(pos, &nkeys_, sizeof(size_t));
    pos += sizeof(size_t);
    memcpy(pos, &nhf_, sizeof(size_t));
    pos += sizeof(size_t);
    size_t tmp = data_.size();
    memcpy(pos, &tmp, sizeof(size_t));
    pos += sizeof(size_t);
    memcpy(pos, seeds_.data(), seeds_.size()*sizeof(size_t));
    pos += seeds_.size()*sizeof(size_t);
    memcpy(pos, data_.data(), (data_.size()+7)/8);
    return {out, len};
}

template<bool keep_stats>
pair<BloomFilter<keep_stats>*, size_t> BloomFilter<keep_stats>::deserialize(uint8_t* ser) {
    uint8_t *pos = ser;

    size_t nkeys = ((size_t*)pos)[0];
    size_t nhf = ((size_t*)pos)[1];
    size_t nbits = ((size_t*)pos)[2];
    pos += 3*sizeof(size_t);
    vector<size_t> seeds((size_t*)pos, (size_t*)pos+nhf);
    pos += nhf*sizeof(size_t);

    size_t len = pos-ser + nbits/8;
    return {new BloomFilter(nbits, pos, nhf, nkeys, seeds), len};

//    BloomFilter *out = new BloomFilter(nbits);
//    out->nkeys_ = ((size_t*)pos)[0];
//    out->nhf_ = ((size_t*)pos)[1];
//    pos += 3*sizeof(size_t);
//    out->seeds_.insert(out->seeds_.end(), (size_t*)pos, (size_t*)(pos+sizeof(size_t)*out->nhf_));
//    pos += out->nhf_*sizeof(size_t);
//    memcpy(out->data_.data(), pos, nbits/8);
//    pos += nbits/8;
//    size_t len = 3*sizeof(size_t)+out->seeds_.size()*sizeof(size_t) + (out->data_.size()+7)/8;
//    return {out, len};
}

#ifdef USE_DTL
void DtlBlockedBloomFilter::AddKeys(const vector<Bitwise> &keys) {
    if (data_.size() == 0) return;
    nkeys_ = keys.size();
    size_t k = data_.size()/nkeys_;
    if (k == 0) {
        k = 1;
    }
    dtl::bbf_64::force_unroll_factor(0);
    b_ = new dtl::bbf_64(data_.size(), k, 1, 1);
//    b_ = new dtl::blocked_bloomfilter<uint64_t>(data_.size(), k, 4, 1, dtl::blocked_bloomfilter_tune());
    for (auto &key: keys) {
        b_->insert((uint64_t*)data_.data(), key.to_uint64());
    }
}

bool DtlBlockedBloomFilter::Query(const Bitwise &key) {
    if (data_.size() == 0) return true;
    return b_->contains((uint64_t*)data_.data(), key.to_uint64());
}

pair<uint8_t*, size_t> DtlBlockedBloomFilter::serialize() const {
    size_t len = sizeof(size_t) + sizeof(size_t) + (data_.size()+7)/8;
    uint8_t *out = new uint8_t[len];
    uint8_t *pos = out;

    memcpy(pos, &nkeys_, sizeof(size_t));
    pos += sizeof(size_t);
    size_t tmp = data_.size();
    memcpy(pos, &tmp, sizeof(size_t));
    pos += sizeof(size_t);
    memcpy(pos, data_.data(), (data_.size()+7)/8);
    return {out, len};
}
#endif

template<class FilterClass, bool keep_stats>
void DstFilter<FilterClass, keep_stats>::AddKeys(const vector<Bitwise> &keys) {
    nkeys_ = keys.size();
    vector<size_t> distribution;
    vector<vector<Bitwise>> bloom_keys;
    maxlen_ = 0;
    for (size_t i=0; i<nkeys_; ++i) {
        maxlen_ = max(maxlen_, keys[i].size());
    }
    distribution.resize(maxlen_, 0);
    bloom_keys.resize(maxlen_);
    for (size_t i=0; i<nkeys_; ++i) {
        size_t lcp = i>0?keys[i-1].lcp(keys[i]):0;
        for (size_t j=lcp; j<maxlen_; ++j) {
            ++distribution[j];
        }
    }
    vector<size_t> nbits = get_nbits_(distribution);

    for (size_t j=0; j<maxlen_; ++j) {
        if (nbits[j] > 0) {
            bloom_keys[j].reserve(distribution[j]);
        }
    }
    for (size_t i=0; i<nkeys_; ++i) {
        size_t lcp = i>0?keys[i-1].lcp(keys[i]):0;
        for (size_t j=lcp; j<maxlen_; ++j) {
            if (nbits[j] > 0) {
                bloom_keys[j].emplace_back(Bitwise(keys[i], j+1));
            }
        }
    }


    if (keep_stats) {
        qdist_.resize(maxlen_, 0);
    }

    bfs_.reserve(maxlen_);
    for (size_t i=0; i<maxlen_; ++i) {
        bfs_.emplace_back(FilterClass(nbits[i]));
        bfs_[i].AddKeys(bloom_keys[i]);
    }
}

template<class FilterClass, bool keep_stats>
void DstFilter<FilterClass, keep_stats>::AddKey(const Bitwise key) {
    for (size_t i=0; i<maxlen_; ++i) {
        bfs_[i].AddKey(key);
    }
}

template<class FilterClass, bool keep_stats>
bool DstFilter<FilterClass, keep_stats>::Doubt(Bitwise *idx, size_t &C, size_t level, size_t maxlevel) {
//    printf("Doubt ");
//    Bitwise(*idx, level+1).print();
//    printf(" (%s)\n", idx->data());
    if (level >= maxlen_) {
        return false;
    }
    if (not bfs_[level].Query(Bitwise(*idx, level+1))) {
        return false;
    }
    --C;
    if (level == maxlevel-1 or C == 0) {
        C = 0;
//        printf("diffidence true\n");
        return true;
    }
    bool old = idx->get(level+1);
    idx->set(level+1, 0);
    if (Doubt(idx, C, level+1, maxlevel)) {
        return true;
    }
    idx->set(level+1, 1);
    if (Doubt(idx, C, level+1, maxlevel)) {
        return true;
    }
    idx->set(level+1, old);
    return false;
}

template<class FilterClass, bool keep_stats>
Bitwise *DstFilter<FilterClass, keep_stats>::GetFirst(const Bitwise &from, const Bitwise &to) {
    Bitwise tfrom(from, min(from.size(), maxlen_)),
            tto(to, min(to.size(), maxlen_));
    assert(tfrom.size() <= maxlen_);
    assert(tto.size() <= maxlen_);
    size_t lcp = tfrom.lcp(tto);
    size_t C = diffidence_;
//    printf("GetFirst ");
//    tfrom.print();
//    printf(" (%s), ", tfrom.data());
//    tto.print();
//    printf(" (%s)\n", tto.data());

//    printf("lcp: %lu\n", lcp);

    if (!keep_stats) {
        Bitwise *tmp = new Bitwise(tto.data(), tto.size()/8);
        for (size_t i=0; i<lcp; ++i) {
            if (!Doubt(tmp, C, i, i+1)) {
                return tmp;
            }
            memcpy(tmp->data(), tto.data(), tto.size()/8);
        }
        delete tmp;
    }

    bool carry = false;
    Bitwise *out;
    if (tfrom.size() > tto.size()) {
        out = new Bitwise(tfrom.data(), tfrom.size()/8);
    }
    else {
        out = new Bitwise(false, tto.size());
        memcpy(out->data(), tfrom.data(), tfrom.size()/8);
    }
    if (lcp == maxlen_) {
        return out;
    }
    for (size_t i=tfrom.size()-1; i>lcp; --i) {
        if (carry) {
            if (out->get(i) == 0) {
                carry = false;
            }
            out->flip(i);
        }

        if (out->get(i) == 1) {
            if (keep_stats) {
                ++qdist_[i];
            }

            if (Doubt(out, C, i, min(min(maxlen_, out->size()), i+diffidence_level_))) {
                return out;
            }
            out->set(i, 0);
            carry = true;
        }
    }

    if (keep_stats and !carry and lcp < maxlen_) {
        ++qdist_[lcp];
    }

    if (!carry and Doubt(out, C, lcp, min(min(maxlen_, out->size()), lcp+diffidence_level_))) {
        return out;
    }
    out->set(lcp, 1);

    for (size_t i=lcp+1; i<tto.size(); ++i) {
        if (tto.get(i) == 1) {
            if (keep_stats) {
                ++qdist_[i];
            }

            if (Doubt(out, C, i, min(min(maxlen_, out->size()), i+diffidence_level_))) {
                return out;
            }
            out->set(i, 1);
        }
    }
//    printf("Got to the end of GetFirst\n");
    return out;
}

template<class FilterClass, bool keep_stats>
bool DstFilter<FilterClass, keep_stats>::Query(const Bitwise &from, const Bitwise &to) {
    Bitwise *qry = GetFirst(from, to);
    size_t lcp = to.lcp(*qry);
//    printf("GetFirst output: ");
    //qry->print();
//    printf(", lcp: %lu\n", lcp);
//    printf("\n");
    delete qry;
    if (keep_stats) {
        nqueries_ += 1;
    }
//    printf("Done Query\n");
    if (lcp < min(to.size(), maxlen_)) {
        if (keep_stats) {
            npositives_ += 1;
        }
        return true;
    }
    //printf("####FALSE!\n");
    return false;
}

template<class FilterClass, bool keep_stats>
bool DstFilter<FilterClass, keep_stats>::Query(const Bitwise &key){
    if (keep_stats) {
        ++qdist_[maxlen_-1];
    }
    return bfs_[maxlen_-1].Query(Bitwise(key, maxlen_));
}

template<class FilterClass, bool keep_stats>
pair<uint8_t*, size_t> DstFilter<FilterClass, keep_stats>::serialize1() const {
    vector<pair<uint8_t*, size_t>> serial_Bloom;
    size_t len = 4*sizeof(size_t);
    for (auto &bf: bfs_) {
        auto ser = bf.serialize();
        len += ser.second;
        serial_Bloom.emplace_back(ser);
    }

    uint8_t *out = new uint8_t[len];
    uint8_t *pos = out;

    memcpy(pos, &diffidence_, sizeof(size_t));
    pos += sizeof(size_t);
    memcpy(pos, &diffidence_level_, sizeof(size_t));
    pos += sizeof(size_t);
    memcpy(pos, &maxlen_, sizeof(size_t));
    pos += sizeof(size_t);
    memcpy(pos, &nkeys_, sizeof(size_t));
    pos += sizeof(size_t);
//    memcpy(pos, fprs_.data(), fprs_.size()*sizeof(double));
//    pos += fprs_.size()*sizeof(double);

    for (auto &ser: serial_Bloom) {
        memcpy(pos, ser.first, ser.second);
        delete[] ser.first;
        pos += ser.second;
    }
//    printf("DST serialized size: %lu\n", len);
    return {out, len};
}

template<class FilterClass, bool keep_stats>
pair<uint8_t*, size_t> DstFilter<FilterClass, keep_stats>::serialize() const {
    vector<pair<uint8_t*, size_t>> serial_Bloom;
    size_t len = 5*sizeof(size_t); // Subarna
    for (auto &bf: bfs_) {
        auto ser = bf.serialize();
        len += ser.second;
        serial_Bloom.emplace_back(ser);
    }

    uint8_t *out = new uint8_t[len];
    uint8_t *pos = out;

    memcpy(pos, &diffidence_, sizeof(size_t));
    pos += sizeof(size_t);
    memcpy(pos, &diffidence_level_, sizeof(size_t));
    pos += sizeof(size_t);
    memcpy(pos, &maxlen_, sizeof(size_t));
    pos += sizeof(size_t);
    memcpy(pos, &nkeys_, sizeof(size_t));
    pos += sizeof(size_t);
    memcpy(pos, &name, sizeof(size_t)); // Subarna
    pos += sizeof(size_t); // Subarna
//    memcpy(pos, fprs_.data(), fprs_.size()*sizeof(double));
//    pos += fprs_.size()*sizeof(double);

    for (auto &ser: serial_Bloom) {
        memcpy(pos, ser.first, ser.second);
        delete[] ser.first;
        pos += ser.second;
    }
//    printf("DST serialized size: %lu\n", len);
    return {out, len};
}

template<class FilterClass, bool keep_stats>
pair<DstFilter<FilterClass, keep_stats>*, size_t> DstFilter<FilterClass, keep_stats>::deserialize2(uint8_t* ser) {
    uint8_t* pos = ser;
    size_t diffidence = ((size_t*)pos)[0];
    size_t diffidence_level = ((size_t*)pos)[1];

    DstFilter<FilterClass, keep_stats>* out = new DstFilter<FilterClass, keep_stats>(diffidence, diffidence_level, [](vector<size_t> distribution) -> vector<size_t> { assert(false); return distribution;});

    out->maxlen_ = ((size_t*)pos)[2];
    if (keep_stats) {
        out->qdist_.resize(out->maxlen_, 0);
    }
    out->nkeys_ = ((size_t*)pos)[3];
    pos += 4*sizeof(size_t);
//    out->fprs_.insert(out->fprs_.end(), (double*)pos, (double*)pos+out->maxlen_);
//    pos += out->maxlen_*sizeof(double);

    out->bfs_.reserve(out->maxlen_);
    //auto begin = std::chrono::system_clock::now(); // Subarna
    for (size_t i=0; i<out->maxlen_; ++i) {
        pair<FilterClass*,size_t> tmp = FilterClass::deserialize(pos);
        out->bfs_.emplace_back(move(*tmp.first));
        delete tmp.first;
        pos += tmp.second;
    }
    // auto end = std::chrono::system_clock::now(); // Subarna
    // auto time_check = end - begin; // Subarna
    // std::cout << "deserialize\t" << time_check/std::chrono::nanoseconds(1) << std::endl; // Subarna
    return {out, pos-ser};
}

template<class FilterClass, bool keep_stats>
pair<DstFilter<FilterClass, keep_stats>*, size_t> DstFilter<FilterClass, keep_stats>::deserialize(uint8_t* ser) {
    uint8_t* pos = ser;
    size_t diffidence = ((size_t*)pos)[0];
    size_t diffidence_level = ((size_t*)pos)[1];

    DstFilter<FilterClass, keep_stats>* out = new DstFilter<FilterClass, keep_stats>(diffidence, diffidence_level, [](vector<size_t> distribution) -> vector<size_t> { assert(false); return distribution;});

    out->maxlen_ = ((size_t*)pos)[2];
    if (keep_stats) {
        out->qdist_.resize(out->maxlen_, 0);
    }
    out->nkeys_ = ((size_t*)pos)[3];
    out->name = ((size_t*)pos)[4];
    pos += 5*sizeof(size_t);

    // map<char, DstFilter<BloomFilter<>>*>::iterator it = Helper::mapping.find(out->name);
    // if(it != Helper::mapping.end())
    // {
    //     DstFilter<FilterClass, keep_stats>* out_me = (DstFilter<FilterClass, keep_stats>*)(it->second);
    //     printf("V2\n");
    //     out_me->printMyStats();
    //     printf("V1\n");
    //     it->second->printMyStats();
    //     return {out_me, pos-ser};
    // }

    out->bfs_.reserve(out->maxlen_);
    //auto begin = std::chrono::system_clock::now(); // Subarna
    for (size_t i=0; i<out->maxlen_; ++i) {
        pair<FilterClass*,size_t> tmp = FilterClass::deserialize(pos);
        out->bfs_.emplace_back(move(*tmp.first));
        delete tmp.first;
        pos += tmp.second;
    }
    // auto end = std::chrono::system_clock::now(); // Subarna
    // auto time_check = end - begin; // Subarna
    // std::cout << "deserialize\t" << time_check/std::chrono::nanoseconds(1) << std::endl; // Subarna
    return {(DstFilter<FilterClass, keep_stats>*)out, pos-ser};
}


template<class FilterClass, bool keep_stats>
pair<DstFilter<FilterClass, keep_stats>*, size_t> DstFilter<FilterClass, keep_stats>::deserialize1(uint8_t* ser) {
    uint8_t* pos = ser;
    size_t diffidence = ((size_t*)pos)[0];
    size_t diffidence_level = ((size_t*)pos)[1];
    //printf("Redirection to addToMap() for Rosetta %c\n", ((size_t*)pos)[4]);
    DstFilter<FilterClass, keep_stats>* out = (DstFilter<FilterClass, keep_stats>*)Helper::addToMap(diffidence, diffidence_level, ((size_t*)pos)[4]);
    if(Helper::flag)
    {
        return {out, pos-ser};
    }
    //printf("First time for Rosetta %c\n", out->name);
    out->maxlen_ = ((size_t*)pos)[2];
    if (keep_stats) {
        out->qdist_.resize(out->maxlen_, 0);
    }
    out->nkeys_ = ((size_t*)pos)[3];
    pos += 5*sizeof(size_t); // Subarna

    /** Subarna **/
    // map<char, DstFilter<BloomFilter<>>*>::iterator it = Helper::mapping.find(out->name);
    // printf("For Rosetta:%c\n", out->name);
    // printf("Map size: %ld\n", Helper::mapping.size());
    // if(it != Helper::mapping.end())
    // {
    //     DstFilter<FilterClass, keep_stats>* out_me = (DstFilter<FilterClass, keep_stats>*)(it->second);
    //     printf("Got Rosetta:%c\n", out_me->name);
    //     //delete out;
    //     //Helper::printRosetta((DstFilter<BloomFilter<>>*)out_me);
    //     //return {out_me, pos-ser};
    // }
    // /** Subarna **/

    for (size_t i=0; i<out->maxlen_; ++i) {
        pair<FilterClass*,size_t> tmp = FilterClass::deserialize(pos);
        out->bfs_.emplace_back(move(*tmp.first));
        delete tmp.first;
        pos += tmp.second;
    }

    printf("deserialized length for Rose %c:\t%ld\n", out->name, pos-ser);
    Helper::updateMap((DstFilter<BloomFilter<>>*)out, pos, ser);

    map<char, DstFilter<BloomFilter<>>*>::iterator itr;
        for (itr = Helper::mapping.begin(); itr != Helper::mapping.end(); ++itr) { 
        std::cout << '\t' << itr->first << '\t' << itr->second << "\t" << '\n'; 
    }
    //Helper::printRosetta((DstFilter<BloomFilter<>>*)out);
    return {out, pos-ser};
}


template class DstFilter<BloomFilter<false>, false>;
template class DstFilter<BloomFilter<true>, true>;
#ifdef USE_DTL
template class DstFilter<DtlBlockedBloomFilter>;
#endif
