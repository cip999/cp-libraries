#include "../../src/io.hpp"

using namespace cplib;

void read_ok_1() {
    auto r = io::Reader("data/oii2022_bus_input.txt", /* strict */ true);

    int N = r.read<int>();
    r.must_be_space();
    int L = r.read<int>();
    r.must_be_newline();

    for (int i = 0; i < L; ++i) {
        int K = r.read<int>();
        r.must_be_space();
        // Reads K space-separated integers and ensures they lie in [0, N).
        auto F = r.read_n_integers(K, 0, N, " ");
        r.must_be_newline();
    }

    r.must_be_eof();
}

void read_ok_2() {
    auto r = io::Reader("data/oii2022_bus_input.txt"); // 'strict' defaults to false.

    int N, L, K;
    r >> N >> L;

    for (int i = 0; i < L; ++i) {
        r >> K;
        std::vector<int> F(K);
        // Reads F.size() integers (equivalent to F = r.read<int>(n)).
        // Note that a strict reader would enforce that the integers are single-space-separated.
        r >> F;
    }
}

void read_fail_1() {
    auto r = io::Reader("data/oii2022_bus_input.txt", /* strict */ true);

    int N, L;
    r >> N >> L; // Doesn't check for whitespace.
}

void read_fail_2() {
    auto r = io::Reader("data/oii2022_bus_input.txt", /* strict */ true);

    int N = r.read_integer(2, 5); // Actual value (8) is out of bounds.
}

int main() {
    read_ok_1();
    read_ok_2();

    try {
        read_fail_1();
    } catch (io::UnexpectedReadException const& e) {
        std::cerr << e.what() << std::endl;
    }

    try {
        read_fail_2();
    } catch (FailedValidationException const& e) {
        std::cerr << e.what() << std::endl;
    }
}
