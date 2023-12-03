#include <cassert>

#include "../../src/io.hpp"

using namespace cplib;

constexpr int MINN = 1;
constexpr int MAXN = 300'000;

void validate(const char* input_file) {
    auto r = io::Reader(input_file, /* strict */ true);

    int N = r.read_integer<int>(MINN, MAXN);
    r.must_be_newline();

    // Read a string of length N and ensure every character is one of =, #, <, >.
    std::string S = r.read_string("=#<>", N);
    r.must_be_newline();

    r.must_be_eof();
}

int main(int argc, char** argv) {
    assert(argc >= 2);
    const char* input_file = argv[1];

    validate(input_file);
}
