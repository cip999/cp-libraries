#include <cassert>

#include "../../src/validation.hpp"

using namespace cplib;

constexpr int MINRC = 1;
constexpr int MAXRC = 1000;

void validate(const char* input_file) {
    auto r = io::Reader(input_file, /* strict */ true);

    int R = r.read_integer<int>(MINRC, MAXRC);
    r.must_be_space();
    int C = r.read_integer<int>(MINRC, MAXRC);
    r.must_be_newline();

    std::vector<std::vector<unsigned short>> M(R, std::vector<unsigned short>(C));
    r >> M;

    ASSERT(val::all(M, [](auto const& v) { return val::all_between(v, 0, 1); }))

    r.must_be_newline();
    r.must_be_eof();
}

int main(int argc, char** argv) {
    assert(argc >= 2);
    const char* input_file = argv[1];

    validate(input_file);
}
