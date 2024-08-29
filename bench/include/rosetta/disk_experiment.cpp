#include <bits/stdc++.h>
#include "dst.h"

#define MAX_BUFFER_SIZE 512

vector<size_t> calc_dst(vector<size_t> dist, size_t logmaxrange, size_t totbits, vector<size_t> qdist) {
    (void)logmaxrange;
    vector<size_t> out(dist.size(), 0);

    long double a=0, b=0, c=0;
    size_t mqdist = 0;
    for (size_t i=0; i<qdist.size(); ++i) {
        if (qdist[i] > 0) {
            a += 1.44*log2(qdist[i])*dist[i];
            b += 1.44*dist[i];
            mqdist = max(mqdist, qdist[i]);
        }
    }
    c = pow(2,((long double)totbits-a)/b);
    //printf("a: %Lf, b: %Lf, c: %Lf\n", a, b, c);

    long double lo=1e-12, hi=1000, mid;
    size_t bitsused;
    while (fabs(hi-lo) > 1e-12) {
        mid = (lo+hi)/2;
        //printf("%Lf %Lf %Lf\n", lo, hi, mid);

        bitsused = 0;
        bool ok = true;
        for (size_t i=0; i<dist.size()-1; ++i) {
            if (dist[i] < (1ULL<<(i+1)) and qdist[i] > 0) {
                long double next_fpr = 1/(mid*qdist[i+1]);
                long double this_fpr = 1/(mid*qdist[i]);
                long double bloom_fpr = this_fpr/(2-next_fpr)/next_fpr;
                bloom_fpr = min(bloom_fpr, (long double)1.0);
                bitsused += ceil((long double)dist[i]*1.44*log2(1/bloom_fpr));
                if (bitsused > totbits) {
                    ok = false;
                    break;
                }
            }
        }
        bitsused += ceil((long double)dist[dist.size()-1]*1.44*log2(mid*qdist[dist.size()-1]));
        //printf("mid: %Lf, bitsused: %lu, totbits: %lu\n", mid, bitsused, totbits);
        if (ok and bitsused <= totbits) {
            lo = mid;
        }
        else {
            hi = mid;
        }
    }
    if (lo < 1e-9) {
        printf("Not enough memory allowance\n");
        exit(0);
    }
    c = lo;
    //printf("best c: %Lf, bits used: %lu, totbits: %lu\n", c, bitsused, totbits);

    bitsused = 0;
    for (size_t i=0; i<dist.size()-1; ++i) {
        if (dist[i] < (1ULL<<(i+1)) and qdist[i] > 0 and bitsused < totbits) {
            long double next_fpr = 1/(c*qdist[i+1]);
            long double this_fpr = 1/(c*qdist[i]);
            long double bloom_fpr = this_fpr/(2-next_fpr)/next_fpr;
            bloom_fpr = min(bloom_fpr, (long double)1.0);
            out[i] = ceil((long double)dist[i]*1.44*log2(1/bloom_fpr));
            bitsused += out[i];
        }
    }
    out[dist.size()-1] = ceil((long double)dist[dist.size()-1]*1.44*log2(c*qdist[dist.size()-1]));
    bitsused += out[dist.size()-1];
    if (bitsused > totbits) {
        printf("Not enough memory allowance\n");
        exit(0);
    }
    for (size_t i=0; i<out.size(); ++i) {
        if (out[i] == 0) continue;
        //printf("%lu: %lu bits, %lu prefixes (%lf bits per key)\n", i, out[i], dist[i], (double)out[i]/dist[out.size()-1]);
    }
    return out;
}

struct DiskBlock {
    string filename_;
    size_t num_keys_;
    uint64_t min_key_, max_key_;

    DiskBlock(vector<uint64_t> keys, string filename): filename_(filename) {
        min_key_ = keys[0];
        max_key_ = keys[keys.size()-1];
        num_keys_ = keys.size();
        FILE *f = fopen(filename_.c_str(), "wb");
        fwrite(keys.data(), sizeof(uint64_t), keys.size(), f);
        fclose(f);
    }

    bool IsRangeFull(uint64_t from, uint64_t to) {
        if (to <= min_key_ or from > max_key_) {
            return false;
        }
        from = max(from, min_key_);
        to = min(to, max_key_+1);
        uint64_t *buffer = new uint64_t[num_keys_];
        FILE *f = fopen(filename_.c_str(), "rb");
        fread(buffer, sizeof(uint64_t), num_keys_, f);
        fclose(f);
        bool out = false;
        for (size_t i=0; i<num_keys_; ++i) {
            if (buffer[i] >= from && buffer[i] < to) {
                out = true;
                break;
            }

        }
        delete[] buffer;
        return out;
    }
};

int main(int argc, char **argv) {
    if (argc < 9) {
        printf("Usage:\n\t%s [keys] [queries-left] [queries-right] [max-range] [dfs-diffidence] [bfs-diffidence] [bits-per-key] [SuRF or DST] (surf hash length) (surf real length)\n", argv[0]);
        return 1;
    }

    FILE *key_file = fopen(argv[1], "r"),
         *lft_file = fopen(argv[2], "r"),
         *rgt_file = fopen(argv[3], "r");

    size_t maxrange = atoi(argv[4]),
           dfs_diff = atoi(argv[5]),
           bfs_diff = atoi(argv[6]),
           mem_budg = atoi(argv[7]);

    bool use_DST = (strcmp(argv[8], "DST") == 0);


    size_t surf_hlen, surf_rlen;
    if (!use_DST) {
        assert(argc == 11);
        surf_hlen = atoi(argv[9]);
        surf_rlen = atoi(argv[10]);
    }

    vector<DiskBlock> files;
    vector<uint64_t> keybuffer;
    vector<Bitwise> keysbwise;
    vector<pair<uint64_t, uint64_t>> queries;

    uint64_t key;
    while (fscanf(key_file,"%lu\n", &key) != EOF) {
        keybuffer.push_back(key);
        keysbwise.push_back(key);

        if (keybuffer.size() == MAX_BUFFER_SIZE) {
            files.emplace_back(keybuffer, "./disk_experiment_binary."+to_string(files.size()));
            keybuffer.clear();
        }
    }
    files.emplace_back(keybuffer, "./disk_experiment_binary."+to_string(files.size()));
    keybuffer.clear();
    fclose(key_file);
    size_t nkeys = keysbwise.size();
    size_t logmaxrange = ceil(log2(maxrange));
    uint64_t upper;
    vector<pair<uint64_t, uint64_t>> range_queries;
    vector<size_t> qdist(64, 0);
    for (size_t i=64-logmaxrange; i<64; ++i) {
        qdist[i] = i+logmaxrange-64;//64-i;
    }

    if (use_DST) {
        volatile bool res;
        DstFilter<BloomFilter> dst_stat(dfs_diff, bfs_diff, [logmaxrange, mem_budg, qdist](vector<size_t> x) -> vector<size_t> {return calc_dst(x, logmaxrange, mem_budg*3, qdist);});
        vector<Bitwise> tmp;
        tmp.push_back(Bitwise((uint64_t)0));
        tmp.push_back(Bitwise((uint64_t)1));
        tmp.push_back(Bitwise((uint64_t)2));
        dst_stat.AddKeys(tmp);
        //printf("Done adding fake keys\n");
        while (fscanf(lft_file, "%lu\n", &key) != EOF) {
            fscanf(rgt_file, "%lu\n", &upper);
            range_queries.push_back({key, upper});
            res = dst_stat.Query(key, upper);
        }
        //printf("Making vector...\n");
        qdist = vector<size_t>(dst_stat.qdist_.begin(), dst_stat.qdist_.end());
        //printf("Done collecting stats\n");
    }
    else {
        while (fscanf(lft_file, "%lu\n", &key) != EOF) {
            fscanf(rgt_file, "%lu\n", &upper);
            range_queries.push_back({key, upper});
        }
    }
    //printf("Qdist:\n");
    //for (size_t i=0; i<64; ++i) {
    //    printf("\t%lu\n", qdist[i]);
    //}

    DstFilter<BloomFilter> *dst;
    SurfFilter *surf;
    if (use_DST) {
        dst = new DstFilter<BloomFilter>(dfs_diff, bfs_diff, [logmaxrange, mem_budg, nkeys, qdist](vector<size_t> x) -> vector<size_t> {return calc_dst(x, logmaxrange, mem_budg*nkeys, qdist);});
    }
    else {
        surf = new SurfFilter(surf_hlen, surf_rlen, (surf::SuffixType)2);
    }
    clock_t begin, end;
    begin = clock();
    if (use_DST) {
        dst->AddKeys(keysbwise);
    }
    else {
        surf->AddKeys(keysbwise);
    }
    end = clock();
    printf("Inserted keys to filter, %lfus per key\n", (double)1000000*(end-begin)/CLOCKS_PER_SEC/keysbwise.size());

    fclose(lft_file);
    fclose(rgt_file);

    begin = clock();
    size_t empty=0, fp=0;
    for (auto &q: range_queries) {
        key = q.first;
        upper = q.second;

        bool filterAns=false, diskAns=false;
        if (use_DST) {
            filterAns = dst->Query(key, upper);
        }
        else {
            filterAns = surf->Query(key, upper);
        }

        if (filterAns) {
            for (auto &file: files) {
                if (file.IsRangeFull(key, upper)) {
                    diskAns = true;
                    break;
                }
            }
        }

        if (filterAns && !diskAns) {
            ++fp;
        }

        if (!diskAns) {
            ++empty;
        }
    }
    end = clock();
    printf("%lfus per query\n", (double)1000000*(end-begin)/CLOCKS_PER_SEC/range_queries.size());
    printf("fpr: %lf\n", (double)fp/empty);

    return 0;
}
