#include <bits/stdc++.h>
#include "dst.h"

int main() {
    BloomFilter<> bf(100);
    vector<Bitwise> v;
    v.push_back(Bitwise("hello"));
    v.push_back(Bitwise("world"));
    v.push_back(Bitwise((uint32_t)1));
    v.push_back(Bitwise((uint32_t)2));
    v.push_back(Bitwise((uint32_t)3));
    v.push_back(Bitwise((uint32_t)4));
    bf.AddKeys(v);
    printf("%d\n", bf.Query(Bitwise("hi")));
    printf("%d\n", bf.Query(Bitwise("hello")));
    printf("%d\n", bf.Query(Bitwise("world")));
    printf("%d\n", bf.Query(Bitwise((uint32_t)1)));
    printf("%d\n", bf.Query(Bitwise((uint32_t)2)));
    printf("%d\n", bf.Query(Bitwise((uint32_t)3)));
    printf("%d\n", bf.Query(Bitwise((uint32_t)4)));
    printf("%d\n", bf.Query(Bitwise((uint32_t)5)));
    printf("%d\n", bf.Query(Bitwise((uint32_t)6)));
    printf("%d\n", bf.Query(Bitwise((uint32_t)7)));
    printf("%d\n", bf.Query(Bitwise((uint32_t)8)));
    printf("%d\n", bf.Query(Bitwise((uint32_t)9)));

    vector<uint32_t> v2;
    for (size_t i=0; i<1000000; ++i) {
        v2.push_back(rand());
    }
    sort(v2.begin(), v2.end());
    v.clear();
    for (size_t i=0; i<v2.size(); ++i) {
        v.push_back(v2[i]);
    }
    DstFilter<BloomFilter<true>, true> dst(100, 32, [](vector<size_t> distribution) -> vector<size_t> {for(auto &i: distribution) i = ceil(i*2.1); return distribution;});
    printf("Adding keys..\n");
    clock_t begin, end;
    begin = clock();
    dst.AddKeys(v);
    end = clock();
    printf("Done adding keys (%lfus per key)\n", (double)1000000*(end-begin)/CLOCKS_PER_SEC/v.size());
    set<uint32_t> s(v2.begin(), v2.end());
    size_t fp = 0, empty = 0;
    for (size_t i=0; i<100000; ++i) {
        uint32_t from = rand();
        uint32_t to = from+rand()%10+1;
        auto it = s.lower_bound(from);
        bool fullrange = false,
             positive = false;
        if (it != s.end() && (*it) < to) {
            fullrange = true;
        }
        if (dst.Query(Bitwise(from), Bitwise(to))) {
            positive = true;
        }
        if (not fullrange) {
            ++empty;
            if (positive) {
                ++fp;
            }
        }
        if (fullrange and not positive) {
            printf("false negative\n");
            printf("%u %u %u\n", from, to, *it);
            Bitwise(from).print();
            printf("\n");
            Bitwise(to).print();
            printf("\n");
            Bitwise(*it).print();
            printf("\n");
            return 0;
        }
    }
    printf("empty: %lu, fp: %lu\n", empty, fp);
    dst.printStats();
}
