#include <cassert>

#include "../../src/validation.hpp"

using namespace cplib;

constexpr int MINN = 2;
constexpr int MAXN = 100'000;
constexpr int MINL = 1;
constexpr int MAXL = 100'000;
constexpr int MINK = 2;
constexpr int MAX_SUMK = 300'000;

void validate(const char* input_file) {
    auto r = io::Reader(input_file, /* strict */ true);

    int N = r.read<int>();
    ASSERT(val::between(N, MINN, MAXN));
    r.must_be_space();

    int L = r.read<int>();
    ASSERT(val::between(L, MINL, MAXL));
    r.must_be_newline();

    int sumK = 0;
    for (int i = 0; i < L; ++i) {
        // Do the check K >= MINK at read time.
        // Upon failure, this will return a FailedValidationException but won't
        // show the details, unless the what() is explicitely printed out.
        int K = r.read_integer<int>(MINK, Limits<int>::MAX);
        sumK += K;

        r.must_be_space();

        auto F = r.read<int>(K);
        ASSERT(val::all_between(F, 0, N - 1));

        // Subtask 4: Check that F is strictly increasing.
        // ASSERT(val::sorted(F));

        int cur = F[0];
        ASSERT(val::all(std::next(F.begin()), F.end(), [&cur](int x) {
            auto res = val::neq(cur, x);
            cur = x;
            return res;
        }))

        r.must_be_newline();
    }

    ASSERT(val::lte(sumK, MAX_SUMK));
    r.must_be_eof();
}

int main(int argc, char** argv) {
    assert(argc >= 2);
    const char* input_file = argv[1];

    validate(input_file);
}
