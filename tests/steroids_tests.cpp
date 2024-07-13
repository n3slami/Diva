#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <assert.h>
#include <string>

#include "steroids.hpp"
#include "util.hpp"

const std::string ansi_green = "\033[0;32m";
const std::string ansi_white = "\033[0;97m";

class SteroidsTests {
public:
    static void InsertionTest() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        Steroids s(infix_size, seed, load_factor);
        Steroids::InfixStore res;

        uint64_t value;
        char buf[9];
        memset(buf, 0, sizeof(buf));

        value = to_big_endian_order(static_cast<uint64_t>(0x0000000000111111));
        s.AddTreeKey(reinterpret_cast<char *>(&value), sizeof(value));

        value = to_big_endian_order(static_cast<uint64_t>(0x0000000000222222));
        s.AddTreeKey(reinterpret_cast<char *>(&value), sizeof(value));
        res = s.tree_.get(reinterpret_cast<char *>(&value), sizeof(value));

        value = to_big_endian_order(static_cast<uint64_t>(0x0000000000333333));
        s.AddTreeKey(reinterpret_cast<char *>(&value), sizeof(value));

        value = to_big_endian_order(static_cast<uint64_t>(0x0000000000444444));
        s.AddTreeKey(reinterpret_cast<char *>(&value), sizeof(value));

        value = to_big_endian_order(static_cast<uint64_t>(0x0000000000200000));
        s.Insert(reinterpret_cast<char *>(&value), sizeof(value));

        value = to_big_endian_order(static_cast<uint64_t>(0x0000000000407777));
        s.Insert(reinterpret_cast<char *>(&value), sizeof(value));

        for (int32_t i = 1; i < 300; i++) {
            const uint64_t l = 0x0000000000111111, r = 0x0000000000222222;
            const uint64_t interp = (l * i + r * (300 - i)) / 300;
            value = to_big_endian_order(interp);
            s.Insert(reinterpret_cast<char *>(&value), sizeof(value));
        }

        value = (0x0000000000111111 + 0x0000000000222222) / 2;
        value = to_big_endian_order(value);
        s.AddTreeKeySplitInfixStore(reinterpret_cast<char *>(&value), sizeof(value));
        res = s.tree_.get(reinterpret_cast<char *>(&value), sizeof(value));
        std::cerr << "LT" << std::endl;
        PrintStore(s, res);
        value = to_big_endian_order(static_cast<uint64_t>(0x0000000000222222));
        res = s.tree_.get(reinterpret_cast<char *>(&value), sizeof(value));
        std::cerr << "GT" << std::endl;
        PrintStore(s, res);
    }

private:
    static void PrintStore(Steroids &s, Steroids::InfixStore &store) {
        const uint32_t size_grade = store.size_grade_elem_count >> s.infix_size_;
        const uint64_t *occupieds = store.ptr;
        const uint64_t *runends = store.ptr + Steroids::infix_store_target_size / 64;

        std::cerr << "occupieds: ";
        for (int32_t i = 0; i < Steroids::infix_store_target_size; i++) {
            if ((store.ptr[i / 64] >> (i % 64)) & 1ULL)
                std::cerr << i << ' ';
        }
        std::cerr << std::endl << "runends + slots:" << std::endl;;
        int32_t cnt = 0;
        for (int32_t i = 0; i < s.scaled_sizes_[size_grade]; i++) {
            const uint64_t value = s.GetSlot(store, i);
            if (value == 0)
                continue;
            std::cerr << std::setfill(' ') << std::setw(3) << i;
            std::cerr << ',' << ((runends[i / 64] >> (i % 64)) & 1ULL) << ',';
            for (int32_t j = s.infix_size_ - 1; j >= 0; j--)
                std::cerr << ((value >> (j % 64)) & 1ULL);
            std::cerr << "   ";
            if (cnt % 8 == 7)
                std::cerr << std::endl;
            cnt++;
        }
        std::cerr << std::endl;
    }
};

int main(int argc, char **argv) {
    std::cerr << ansi_green << "===== [ Running Tests ]" << ansi_white << std::endl;
    SteroidsTests::InsertionTest();
    std::cerr << ansi_green << "===== [ Done ]" << ansi_white << std::endl;

    return 0;
}

