/**
 * @file diva example
 * @author Navid Eslami
 */
#include <cstdint>
#include <cstring>
#include <random>
#include <string>
#include <vector>

#include "diva.hpp"


const uint32_t rng_seed = 10;
const uint32_t min_string_key_length = 5;
const uint32_t max_string_key_length = 10;
std::mt19937_64 rng(rng_seed);

uint64_t new_int_key() {
    return rng();
}

std::string new_string_key() {
    std::string res = "";
    const uint32_t new_string_key_length = rng() % (max_string_key_length - min_string_key_length + 1) + min_string_key_length;
    for (uint32_t j = 0; j < new_string_key_length; j++)
        res += static_cast<char>(std::max<uint8_t>(1, rng() & 0xFFULL));
    return res;
}


int main() {
    const uint32_t infix_size = 5;  // Includes the unary counter
    const uint32_t seed = 1;
    const float load_factor = 0.95; // Determines how much each Infix Store is filled Up

    // Normal Allocation
    {   
        diva::Diva<false> normal_diva(infix_size, seed, load_factor);       // Can also be instantiated as `diva::Diva<>`.
        diva::Diva<true> int_optimized_diva(infix_size, seed, load_factor);
    }

    const uint32_t n_keys = 10000;
    std::vector<std::string> string_keys;
    std::vector<uint64_t> int_keys;
    for (uint32_t i = 0; i < n_keys; i++) {
        string_keys.push_back(new_string_key());
        int_keys.push_back(new_int_key());
    }
    std::sort(string_keys.begin(), string_keys.end());
    std::sort(int_keys.begin(), int_keys.end());

    // Allocation with Bulk Loading
    diva::Diva<false> normal_diva(infix_size, string_keys.begin(), string_keys.end(), 
                                  seed, load_factor);
    diva::Diva<true> int_optimized_diva(infix_size, int_keys.begin(), int_keys.end(), 
                                        sizeof(uint64_t), seed, load_factor);

    // Insertions
    const uint32_t n_inserts = 10;
    std::vector<uint64_t> new_int_keys;
    std::vector<std::string> new_string_keys;
    for (uint32_t i = 0; i < n_inserts; i++) {
        const std::string string_key = new_string_key();
        new_string_keys.push_back(string_key);
        normal_diva.Insert(string_key);

        const uint64_t int_key = new_int_key();
        new_int_keys.push_back(int_key);
        int_optimized_diva.Insert(int_key);
    }
    // Insertion of Byte String with Null Characters
    uint8_t byte_string[10] = {0x01, 0x02, 0x03, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    normal_diva.Insert(byte_string, 10);

    // Point Queries
    normal_diva.PointQuery("diva");
    int_optimized_diva.PointQuery(100);
    // Point Query for Byte String with Null Characters
    normal_diva.PointQuery(byte_string, 10);

    // Range Queries
    normal_diva.RangeQuery("abc", "efg");
    int_optimized_diva.RangeQuery(10, 20);
    // Range Query with Byte String Endpoints with Null Characters
    uint8_t byte_string_l[10] = {0x01, 0x02, 0x02, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    uint8_t byte_string_r[6] = {0xFF, 0xFF, 0x01, 0x00, 0x01, 0x02};
    normal_diva.RangeQuery(byte_string_l, 10, byte_string_r, 6);

    // Deletions
    for (uint32_t i = 0; i < n_inserts; i++) {
        const std::string string_key = new_string_keys[i];
        normal_diva.Delete(string_key);

        const uint64_t int_key = new_int_keys[i];
        int_optimized_diva.Delete(int_key);
    }
    // Deletion of Byte String with Null Characters
    normal_diva.Delete(byte_string, 10);

    // Shrink Infix Size (to save memory)
    normal_diva.ShrinkInfixSize(infix_size - 1);
    int_optimized_diva.ShrinkInfixSize(infix_size - 1);

    // Get Size (in bytes)
    normal_diva.Size();
    int_optimized_diva.Size();
}

