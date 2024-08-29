#include <string>
#include <cstddef>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

#define MAX_BUFFER_SIZE (256*1024*1024/128)
template<typename T>
struct DiskBlock {
    string filename_;
    size_t num_keys_, key_length_;
    T min_key_, max_key_;

    DiskBlock(vector<T> keys, string filename, size_t key_length);
    bool IsRangeFull(T from, T to, int& cnt_reads);
    void putKeyToBuffer(uint8_t *buf, T &key);
    void getKeyFromBuffer(uint8_t *buf, T &key);
};

template<typename T> void DiskBlock<T>::putKeyToBuffer(uint8_t *buf, T &key) { memcpy(buf, &key, sizeof(T)); }
template<typename T> void DiskBlock<T>::getKeyFromBuffer(uint8_t *buf, T &key) { key = *(T*)(buf); }
template<> void DiskBlock<string>::putKeyToBuffer(uint8_t *buf, string &key) { memcpy(buf, key.c_str(), key.size()); }
template<> void DiskBlock<string>::getKeyFromBuffer(uint8_t *buf, string &key) { key = string((char*)(buf)); }

template<typename T>
DiskBlock<T>::DiskBlock(vector<T> keys, string filename, size_t key_length): filename_(filename), num_keys_(keys.size()), key_length_(key_length), min_key_(keys[0]), max_key_(keys[keys.size()-1]) {
    FILE *f = fopen(filename_.c_str(), "wb");
    uint8_t *buffer = new uint8_t[num_keys_ * key_length_]();

    for (size_t i=0; i<num_keys_; ++i) {
        putKeyToBuffer(buffer + i * key_length_, keys[i]);
    }

    fwrite(buffer, key_length_, num_keys_, f);
    fclose(f);
    delete[] buffer;
}

template<typename T>
bool DiskBlock<T>::IsRangeFull(T from, T to, int& cnt_reads) {
    if (to <= min_key_ or from > max_key_) {
        return false;
    }
    clock_t begin=clock();
    FILE *f = fopen(filename_.c_str(), "rb");
    int c =8192*2;
//    printf("%d\n", num_keys_);
    getchar();
    uint8_t *buffer = new uint8_t[key_length_*c];
    bool out = false;
    for (size_t i=0; i<num_keys_/c; ++i) {
        fread(buffer, key_length_*c, 1, f);
        T tmp;
	for(int j=0;j<c;j++){
		cnt_reads++;
        	getKeyFromBuffer(buffer+j*key_length_, tmp);
        	if (tmp >= from && tmp < to) {
            		out = true;
//            		break;
        	}
	}
    }
    fclose(f);
//	printf("%d %d\n", from, to);
//    printf("%f\n", (double)1000000*(clock()-begin)/CLOCKS_PER_SEC);
    delete[] buffer;
    return out;
}
