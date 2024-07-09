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

        uint64_t value;
        char buf[9];
        memset(buf, 0, sizeof(buf));

        value = to_big_endian_order(static_cast<uint64_t>(0x0000000000111111));
        s.AddTreeKeySplitInfixStore(reinterpret_cast<char *>(&value), sizeof(value));

        value = to_big_endian_order(static_cast<uint64_t>(0x0000000000222222));
        s.AddTreeKeySplitInfixStore(reinterpret_cast<char *>(&value), sizeof(value));

        value = to_big_endian_order(static_cast<uint64_t>(0x0000000000333333));
        s.AddTreeKeySplitInfixStore(reinterpret_cast<char *>(&value), sizeof(value));

        value = to_big_endian_order(static_cast<uint64_t>(0x0000000000444444));
        s.AddTreeKeySplitInfixStore(reinterpret_cast<char *>(&value), sizeof(value));

        value = to_big_endian_order(static_cast<uint64_t>(0x0000000000200000));
        s.Insert(reinterpret_cast<char *>(&value), sizeof(value));
    }

private:
};

int main(int argc, char **argv) {
    std::cerr << ansi_green << "===== [ Running Tests ]" << ansi_white << std::endl;
    SteroidsTests::InsertionTest();
    std::cerr << ansi_green << "===== [ Done ]" << ansi_white << std::endl;

    return 0;
}

