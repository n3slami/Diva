#include <iostream>

#include "steroids.hpp"

int main(int argc, char *argv[]) {
    std::cerr << "KALLE KACHAL" << std::endl;

    Steroids ster;
    ster.insert("a");
    ster.insert("b");
    ster.insert("c");
    std::cerr << +ster.get("a") << ' ' << +ster.get("b") << ' ' << +ster.get("c") << std::endl;
}
