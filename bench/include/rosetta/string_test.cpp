#include <bits/stdc++.h>
#include "dst.h"

int main() {
    vector<Bitwise> v;
    v.push_back(Bitwise("hello"));
//    v.push_back(Bitwise("world"));
//    v.push_back(Bitwise((uint32_t)1));
//    v.push_back(Bitwise((uint32_t)2));
//    v.push_back(Bitwise((uint32_t)3));
//    v.push_back(Bitwise((uint32_t)4));

    DstFilter<BloomFilter<>> dst(100, 32, [](vector<size_t> distribution) -> vector<size_t> {for(auto &i: distribution) i = ceil(i*2.1); return distribution;});
    printf("Adding keys..\n");
    clock_t begin, end;
    begin = clock();
    dst.AddKeys(v);
    end = clock();
    printf("Done adding keys (%lfus per key)\n", (double)1000000*(end-begin)/CLOCKS_PER_SEC/v.size());

    printf("%d\n", dst.Query(Bitwise((const uint8_t *)"ha\0\0\0", 6), Bitwise((const uint8_t *)"you\0\0", 6)));
    printf("%d\n", dst.Query(Bitwise((const uint8_t *)"he\0\0\0", 6), Bitwise((const uint8_t *)"you\0\0", 6)));
    printf("%d\n", dst.Query(Bitwise((const uint8_t *)"hi\0\0\0", 6), Bitwise((const uint8_t *)"you\0\0", 6)));

    printf("%d\n", dst.Query(string("ha"), string("you")));
    printf("%d\n", dst.Query(string("he"), string("you")));
    printf("%d\n", dst.Query(string("hi"), string("you")));

    return 0;
}
