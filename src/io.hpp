#include <cstdio>
#include <cstring>
#include <exception>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <numeric>
#include <sstream>
#include <string>

#include "common.hpp"

namespace cplib::io {

class IOException : public CplibException {
   private:
    inline std::string prefix() const noexcept override { return "I/O ERROR"; }

   public:
    IOException(std::string const& msg) : CplibException(msg) {}
};

class OpenFailureException : public IOException {
   public:
    OpenFailureException(std::string const& file_name)
        : IOException("Couldn't open " + file_name) {}
};

class EOFException : public IOException {
   public:
    EOFException() : IOException("Reached EOF") {}
};

class UnexpectedReadException : public IOException {
   private:
    inline std::string prefix() const noexcept override {
        return "UNEXPECTED READ";
    }

   public:
    UnexpectedReadException(char c)
        : IOException("Encountered character '" + std::string(1, c) + "'") {}
    UnexpectedReadException(std::string const& s)
        : IOException("Expected " + s) {}
};

class OverflowException : public IOException {
   private:
    inline std::string prefix() const noexcept override {
        return "INTEGER OVERFLOW";
    }

   public:
    template <class T>
    explicit OverflowException(T max_integer)
        : IOException("Exceeded limit " + std::to_string(max_integer)) {}
};

class Reader {
   private:
    std::unique_ptr<std::istream> source;

    bool strict = false;
    bool leading_zeros = false;
    char decimal_separator = '.';

    static inline bool is_space(char c) {
        return c == ' ' || c == '\t' || c == '\r' || c == '\n';
    }
    static inline bool is_numeric(char c) { return '0' <= c && c <= '9'; }

    template <class T>
    T read_unsigned_strict();

    template <class T>
    T read_signed_strict();

    template <class T>
    T read_integer_strict();

    template <class T>
    T read_floating_point_strict();

    template <class T>
    std::vector<T> read_n(std::size_t n, std::function<T()> read_single,
                          std::string const& sep = "");

    std::string read_string_strict(
        std::function<bool(std::size_t, char)> const& check_char,
        std::size_t min_length = 0, std::size_t max_length = std::string::npos);

   public:
    Reader() = default;
    Reader(bool strict) : strict(strict) {}
    Reader(const char* file_name) : source(new std::ifstream(file_name)) {
        if (source->fail()) {
            throw OpenFailureException(std::string(file_name));
        }
    }
    Reader(const char* file_name, bool strict) : Reader(file_name) {
        this->strict = strict;
    }
    Reader(std::istream& source) : source(&source) {}
    Reader(std::istream& source, bool strict) : Reader(source) {
        this->strict = strict;
    }

    ~Reader() { source.get_deleter()(source.release()); }

    Reader& with_string_stream(std::string const& s) {
        source.reset(new std::istringstream(s));
        return *this;
    }

    Reader& make_strict() {
        strict = true;
        return *this;
    }
    Reader& make_non_strict() {
        strict = false;
        return *this;
    }

    Reader& with_leading_zeros() {
        leading_zeros = true;
        return *this;
    }
    Reader& without_leading_zeros() {
        leading_zeros = false;
        return *this;
    }

    Reader& with_comma_as_decimal_separator() {
        decimal_separator = ',';
        return *this;
    }
    Reader& with_dot_as_decimal_separator() {
        decimal_separator = '.';
        return *this;
    }

    void must_be_space();
    void must_be_newline();
    void must_be_eof();

    void skip_spaces() noexcept;
    void skip_non_numeric() noexcept;

    char read_char();

    std::string read_constant(std::string const& token);
    std::string read_any_of(std::vector<std::string> const& tokens);

    template <class T>
    T read_integer();

    template <class T>
    T read_integer(T min_value, T max_value);

    template <class T>
    T read_floating_point();

    template <class T>
    std::vector<T> read_n_integers(std::size_t n, std::string const& sep = "");

    template <class T>
    std::vector<T> read_n_integers(std::size_t n, T min_value, T max_value,
                                   std::string const& sep = "");

    template <class T>
    std::vector<T> read_n_floating_point(std::size_t n,
                                         std::string const& sep = "");

    std::string read_string(std::size_t exact_length = 0);
    std::string read_string(std::size_t min_length, std::size_t max_length);
    std::string read_string(std::string const& allowed_chars,
                            std::size_t exact_length = 0);
    std::string read_string(std::string const& allowed_chars,
                            std::size_t min_length, std::size_t max_length);
    std::string read_string(
        std::function<bool(std::size_t, char)> const& check_char,
        std::size_t min_length = 0, std::size_t max_length = std::string::npos);

    std::vector<std::string> read_n_strings(std::size_t n,
                                            std::size_t exact_length = 0,
                                            std::string const& sep = "");

    template <class T, std::enable_if_t<std::is_same_v<T, char>, bool> = true>
    char read();

    template <class T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
    T read();

    template <class T,
              std::enable_if_t<std::is_floating_point_v<T>, bool> = true>
    T read();

    template <class T,
              std::enable_if_t<std::is_same_v<T, std::string>, bool> = true>
    std::string read();

    template <class T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
    std::vector<T> read(std::size_t n);

    template <class T,
              std::enable_if_t<std::is_floating_point_v<T>, bool> = true>
    std::vector<T> read(std::size_t n);

    template <class T,
              std::enable_if_t<std::is_same_v<T, std::string>, bool> = true>
    std::vector<std::string> read(std::size_t n);

    template <class T>
    std::vector<std::vector<T>> read(std::size_t n, std::size_t m);

    template <class T, class = decltype(std::declval<Reader>().read<T>())>
    friend Reader& operator>>(Reader& r, T& x) {
        x = r.read<T>();
        return r;
    }

    template <class T, class = decltype(std::declval<Reader>().read<T>())>
    friend Reader& operator>>(Reader& r, std::vector<T>& v) {
        v = r.read<T>(v.size());
        return r;
    }

    template <class T, class = decltype(std::declval<Reader>().read<T>())>
    friend Reader& operator>>(Reader& r, std::vector<std::vector<T>>& v) {
        if (v.size() == 0) {
            throw InvalidArgumentException(
                "Both dimensions of the matrix must have positive size");
        }
        v = r.read<T>(v.size(), v[0].size());
        return r;
    }
};

void Reader::must_be_space() {
    char c = read_char();
    if (c != ' ') {
        throw UnexpectedReadException("space");
    }
}

void Reader::must_be_newline() {
    char c = read_char();
    if (c == '\r') {
        c = read_char();
    }
    if (c != '\n') {
        throw UnexpectedReadException("newline");
    }
}

void Reader::must_be_eof() {
    source->peek();
    if (source->eof()) {
        return;
    }
    throw UnexpectedReadException("EOF");
}

void Reader::skip_spaces() noexcept {
    char c;
    try {
        do {
            c = read_char();
        } while (is_space(c));
    } catch (EOFException const&) {
        return;
    }
    source->unget();
}

void Reader::skip_non_numeric() noexcept {
    char c;
    try {
        do {
            c = read_char();
        } while (!is_numeric(c) && c != '-');
    } catch (EOFException const&) {
        return;
    }
    source->unget();
};

char Reader::read_char() {
    char c;
    source->get(c);
    if (source->eof()) {
        throw EOFException();
    }
    return c;
}

std::string Reader::read_constant(std::string const& token) {
    if (token.empty()) {
        throw InvalidArgumentException(
            "Argument 'token' must not be the empty string");
    }
    char* s = (char*)malloc((token.size() + 1) * sizeof(char));
    source->read(s, token.size());
    if (source->eof()) {
        throw EOFException();
    }
    s[token.size()] = '\0';
    if (std::string(s) != token) {
        throw UnexpectedReadException("'" + token + "'");
    }
    return std::string(s);
}

std::string Reader::read_any_of(std::vector<std::string> const& tokens) {
    if (tokens.empty()) {
        throw InvalidArgumentException("Argument 'tokens' must not be empty");
    }
    std::size_t min_length =
        std::accumulate(tokens.begin(), tokens.end(), std::string::npos,
                        [](std::size_t a, std::string const& b) {
                            return std::min(a, b.size());
                        });
    std::size_t max_length =
        std::accumulate(tokens.begin(), tokens.end(), 0,
                        [](std::size_t a, std::string const& b) {
                            return std::max(a, b.size());
                        });
    if (min_length == 0) {
        throw InvalidArgumentException(
            "Elements of 'tokens' must not be the empty string");
    }
    std::string s = read_string(min_length, max_length);
    if (std::find(tokens.begin(), tokens.end(), s) == tokens.end()) {
        std::string concat =
            static_cast<std::string>(
                std::accumulate(tokens.begin(), tokens.end(), std::string("'"),
                                [](std::string const& a, std::string const& b) {
                                    return a + "', '" + b;
                                })) +
            "'";
        throw UnexpectedReadException("one of " + concat);
    }
    return s;
}

template <class T>
T Reader::read_unsigned_strict() {
    T limit = std::numeric_limits<T>::max();
    T n = 0;
    bool start = true;
    char c;
    try {
        do {
            c = read_char();
            if (!is_numeric(c)) {
                if (start) {
                    throw UnexpectedReadException(c);
                }
                break;
            }
            if (n == 0 && !start && !leading_zeros) {
                throw UnexpectedReadException('0');
            }
            start = false;
            long long units = static_cast<T>(c - '0');
            if (n > (limit - units) / T(10)) {
                throw OverflowException(limit);
            }
            n = T(10) * n + units;
        } while (true);
    } catch (EOFException const& e) {
        if (start) {
            throw e;
        }
        return n;
    }
    source->unget();
    return n;
}

template <class T>
T Reader::read_signed_strict() {
    bool negative = false;
    char c = read_char();
    if (c == '-') {
        negative = true;
    } else if (is_numeric(c)) {
        source->unget();
    } else {
        throw UnexpectedReadException(c);
    }
    using unsigned_T = std::make_unsigned_t<T>;
    unsigned_T n = read_unsigned_strict<unsigned_T>();
    unsigned_T limit =
        negative ? static_cast<unsigned_T>(0) - std::numeric_limits<T>::min()
                 : static_cast<unsigned_T>(std::numeric_limits<T>::max());
    if (n > limit) throw OverflowException(limit);
    if (negative) {
        // This should work because of two's complement.
        return static_cast<T>(static_cast<unsigned_T>(0) - n);
    };
    return static_cast<T>(n);
}

template <class T>
T Reader::read_integer_strict() {
    if (std::is_signed_v<T>) {
        return read_signed_strict<T>();
    }
    return read_unsigned_strict<T>();
}

template <class T>
T Reader::read_integer() {
    static_assert(std::is_integral_v<T>, "Type must be integral");
    if (!strict) skip_non_numeric();
    return read_integer_strict<T>();
}

template <class T>
T Reader::read_integer(T min_value, T max_value) {
    T n = read_integer<T>();
    if (n < min_value || n > max_value) {
        throw FailedValidationException::interval_constraint("n", min_value,
                                                             max_value);
    }
    return n;
}

template <class T>
T Reader::read_floating_point_strict() {
    std::string x_string;
    char c;
    bool is_zero = true;
    bool after_decimal_separator = false;
    try {
        do {
            c = read_char();
            if (!is_numeric(c) && c != '-' && c != decimal_separator) {
                if (x_string.empty()) {
                    throw UnexpectedReadException(c);
                }
                source->unget();
                break;
            }
            if (c == '-' && !x_string.empty()) {
                throw UnexpectedReadException('-');
            }
            if (c == decimal_separator) {
                if (x_string.empty() || x_string == "-" ||
                    after_decimal_separator) {
                    throw UnexpectedReadException(decimal_separator);
                }
                after_decimal_separator = true;
                x_string.push_back('.');
                continue;
            }
            if (is_zero && !leading_zeros && !after_decimal_separator &&
                !x_string.empty() && x_string != "-") {
                throw UnexpectedReadException('0');
            }
            if (is_numeric(c) && c != '0') {
                is_zero = false;
            }
            x_string.push_back(c);
        } while (true);
    } catch (EOFException const& e) {
        if (x_string.empty() || !is_numeric(x_string.back())) {
            throw e;
        }
    }
    T x;
    std::istringstream(x_string) >> x;
    return x;
}

template <class T>
T Reader::read_floating_point() {
    static_assert(std::is_floating_point_v<T>, "Type must be floating point");
    if (!strict) skip_non_numeric();
    return read_floating_point_strict<T>();
}

template <class T>
std::vector<T> Reader::read_n(std::size_t n, std::function<T()> read_single,
                              std::string const& sep) {
    if (n == 0) {
        throw InvalidArgumentException("n must be strictly positive");
    }
    if (!strict) {
        skip_spaces();
    }
    std::vector<T> v(n);
    for (std::size_t i = 0; i < n; ++i) {
        v[i] = read_single();
        if (sep.size() > 0 && i + 1 < n) {
            read_constant(sep);
        }
    }
    return v;
}

template <class T>
std::vector<T> Reader::read_n_integers(std::size_t n, std::string const& sep) {
    return sep.size() == 0
               ? read_n<T>(
                     n, [this]() { return read_integer<T>(); }, sep)
               : read_n<T>(
                     n, [this]() { return read_integer_strict<T>(); }, sep);
}

template <class T>
std::vector<T> Reader::read_n_integers(std::size_t n, T min_value, T max_value,
                                       std::string const& sep) {
    return sep.size() == 0
               ? read_n<T>(
                     n,
                     [this, min_value, max_value]() {
                         return read_integer<T>(min_value, max_value);
                     },
                     sep)
               : read_n<T>(
                     n,
                     [this, min_value, max_value]() {
                         T x = read_integer_strict<T>();
                         if (x < min_value || x > max_value) {
                             throw FailedValidationException::
                                 interval_constraint("x", min_value, max_value);
                         }
                         return x;
                     },
                     sep);
}

template <class T>
std::vector<T> Reader::read_n_floating_point(std::size_t n,
                                             std::string const& sep) {
    return sep.size() == 0
               ? read_n<T>(
                     n, [this]() { return read_floating_point<T>(); }, sep)
               : read_n<T>(
                     n, [this]() { return read_floating_point_strict<T>(); },
                     sep);
}

std::string Reader::read_string_strict(
    std::function<bool(std::size_t, char)> const& check_char,
    std::size_t min_length, std::size_t max_length) {
    std::string s;
    s.reserve(min_length);
    char c;
    try {
        for (std::size_t i = 0;; ++i) {
            c = read_char();
            if (is_space(c)) {
                if (i == 0) {
                    throw UnexpectedReadException("non-space character");
                }
                source->unget();
                break;
            }
            if (i >= max_length) {
                throw FailedValidationException::interval_constraint(
                    "len(string)", min_length, max_length);
            }
            if (!check_char(i, c)) {
                throw FailedValidationException(
                    "Invalid character '" + to_string(c) + "' at position " +
                    std::to_string(i));
            }
            s.push_back(c);
        }
    } catch (EOFException const&) {
        if (s.empty()) {
            throw EOFException();
        }
    }
    if (s.size() < min_length) {
        throw FailedValidationException::interval_constraint(
            "len(string)", min_length, max_length);
    }
    return s;
}

std::string Reader::read_string(std::size_t exact_length) {
    return exact_length > 0 ? read_string(exact_length, exact_length)
                            : read_string(0, std::string::npos);
}

std::string Reader::read_string(std::size_t min_length,
                                std::size_t max_length) {
    return read_string([](std::size_t i, char c) { return true; }, min_length,
                       max_length);
}

std::string Reader::read_string(std::string const& allowed_chars,
                                std::size_t exact_length) {
    return exact_length > 0
               ? read_string(allowed_chars, exact_length, exact_length)
               : read_string(allowed_chars, 0, std::string::npos);
}

std::string Reader::read_string(std::string const& allowed_chars,
                                std::size_t min_length,
                                std::size_t max_length) {
    return read_string(
        [&allowed_chars](std::size_t i, char c) {
            return allowed_chars.find(c) != std::string::npos;
        },
        min_length, max_length);
}

std::string Reader::read_string(
    std::function<bool(std::size_t, char)> const& check_char,
    std::size_t min_length, std::size_t max_length) {
    if (!strict) skip_spaces();
    return read_string_strict(check_char, min_length, max_length);
}

std::vector<std::string> Reader::read_n_strings(std::size_t n,
                                                std::size_t exact_length,
                                                std::string const& sep) {
    return sep.size() == 0
               ? read_n<std::string>(
                     n,
                     [this, exact_length]() {
                         return read_string(exact_length);
                     },
                     sep)
               : read_n<std::string>(
                     n,
                     [this, exact_length]() {
                         auto check_char = [](std::size_t, char) {
                             return true;
                         };
                         if (exact_length == 0) {
                             return read_string_strict(check_char);
                         }
                         return read_string_strict(check_char, exact_length,
                                                   exact_length);
                     },
                     sep);
}

template <class T, std::enable_if_t<std::is_same_v<T, char>, bool>>
char Reader::read() {
    return read_char();
}

template <class T, std::enable_if_t<std::is_integral_v<T>, bool>>
T Reader::read() {
    return read_integer<T>();
}

template <class T, std::enable_if_t<std::is_floating_point_v<T>, bool>>
T Reader::read() {
    return read_floating_point<T>();
}

template <class T, std::enable_if_t<std::is_same_v<T, std::string>, bool>>
std::string Reader::read() {
    return read_string();
}

template <class T, std::enable_if_t<std::is_integral_v<T>, bool>>
std::vector<T> Reader::read(std::size_t n) {
    return read_n_integers<T>(n, strict ? " " : "");
}

template <class T, std::enable_if_t<std::is_floating_point_v<T>, bool>>
std::vector<T> Reader::read(std::size_t n) {
    return read_n_floating_point<T>(n, strict ? " " : "");
}

template <class T, std::enable_if_t<std::is_same_v<T, std::string>, bool>>
std::vector<std::string> Reader::read(std::size_t n) {
    return read_n_strings(n, 0, strict ? " " : "");
}

template <class T>
std::vector<std::vector<T>> Reader::read(std::size_t n, std::size_t m) {
    return read_n<std::vector<T>>(
        n, [this, m]() { return read<T>(m); }, "\n");
}

class Writer {
   private:
    std::unique_ptr<std::ostream> dest;

    char decimal_separator = '.';

   public:
    Writer() = default;
    Writer(const char* file_name) : dest(new std::ofstream(file_name)) {
        if (dest->fail()) {
            throw OpenFailureException(std::string(file_name));
        }
    }
    Writer(std::ostream& dest) : dest(&dest) {}

    ~Writer() { dest.get_deleter()(dest.release()); }

    Writer& with_dest(std::ostream& dest) {
        this->dest.get_deleter()(this->dest.release());
        this->dest = std::unique_ptr<std::ostream>(&dest);
        return *this;
    }

    Writer& with_comma_as_decimal_separator() {
        decimal_separator = ',';
        return *this;
    }
    Writer& with_dot_as_decimal_separator() {
        decimal_separator = '.';
        return *this;
    }

    void write_space();
    void write_newline(bool with_cr);

    void write_char(char c);

    void write_string(const char* s, std::size_t n = 0);
    void write_string(std::string const& s);

    template <class T>
    void write_integer(T x);

    template <class T>
    void write_floating_point(T x, int fixed_decimals = -1);

    template <class It>
    void write_iter(It const& begin, It const& end,
                    std::string const& separator = " ");

    template <class V>
    void write_iter(V const& v, std::string const& separator = " ");

    template <class M>
    void write_matrix(M const& m);

    template <class T, class = decltype(std::declval<std::ostream>()
                                        << std::declval<T>())>
    void write(T const& x);

    template <class V,
              class = decltype(std::declval<std::ostream>()
                               << *std::declval<V>().begin()),
              std::enable_if_t<!std::is_same_v<std::decay_t<V>, std::string>,
                               bool> = true>
    void write(V const& v);

    template <
        class M,
        class = decltype(std::declval<std::ostream>()
                         << *std::declval<M>().begin()->begin()),
        std::enable_if_t<
            !std::is_same_v<std::decay_t<decltype(*std::declval<M>().begin())>,
                            std::string>,
            bool> = true>
    void write(M const& m);

    template <class T>
    friend Writer& operator<<(Writer& w, T const& x);
};

void Writer::write_space() { dest->put(' '); }

void Writer::write_newline(bool with_cr = false) {
    if (with_cr) {
        dest->put('\r');
    }
    dest->put('\n');
}

void Writer::write_char(char c) { dest->put(c); }

void Writer::write_string(const char* s, std::size_t n) {
    if (n == 0) {
        n = strlen(s);
    }
    dest->write(s, n);
}

void Writer::write_string(std::string const& s) { *dest << s; }

template <class T>
void Writer::write_integer(T x) {
    static_assert(std::is_integral_v<T>);
    *dest << x;
}

template <class T>
void Writer::write_floating_point(T x, int fixed_decimals) {
    static_assert(std::is_floating_point_v<T>);
    std::ostringstream ss;
    if (fixed_decimals >= 0) {
        ss << std::fixed;
        ss.precision(fixed_decimals);
    }
    ss << x;
    write_string(ss.str());
}

template <class It>
void Writer::write_iter(It const& begin, It const& end,
                        std::string const& separator) {
    for (It it = begin; it != end; it = std::next(it)) {
        if (it != begin) write_string(separator);
        write(*it);
    }
}

template <class V>
void Writer::write_iter(V const& v, std::string const& separator) {
    write_iter(v.begin(), v.end(), separator);
}

template <class M>
void Writer::write_matrix(M const& m) {
    for (auto it = m.begin(); it != m.end(); it = next(it)) {
        if (it != m.begin()) write_string("\n");
        write_iter(*it);
    }
}

template <class T, class>
void Writer::write(T const& x) {
    *dest << x;
}

template <class V, class,
          std::enable_if_t<!std::is_same_v<std::decay_t<V>, std::string>, bool>>
void Writer::write(V const& v) {
    write_iter(v.begin(), v.end());
}

template <
    class M, class,
    std::enable_if_t<
        !std::is_same_v<std::decay_t<decltype(*std::declval<M>().begin())>,
                        std::string>,
        bool>>
void Writer::write(M const& m) {
    write_matrix(m);
}

template <class T>
Writer& operator<<(Writer& w, T const& x) {
    w.write(x);
    return w;
}

}  // namespace cplib::io
