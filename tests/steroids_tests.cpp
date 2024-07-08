#include <cstdint>
#include <iostream>
#include <assert.h>
#include <string>

#include "steroids.hpp"

const std::string ansi_green = "\033[0;32m";
const std::string ansi_white = "\033[0;97m";

class SteroidsTests {
public:
    static void Test() {
        std::cerr << "TEST" << std::endl;
    }

private:
};

int main(int argc, char **argv) {
    std::cerr << ansi_green << "===== [ Running Tests ]" << ansi_white << std::endl;
    SteroidsTests::Test();
    std::cerr << ansi_green << "===== [ Done ]" << ansi_white << std::endl;

    return 0;
}

