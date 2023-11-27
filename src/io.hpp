#include <cstdio>
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

namespace cplib {

namespace io {

class IOException : public std::ios_base::failure, public CplibException {
   public:
    explicit IOException(std::string const& msg)
        : std::ios_base::failure(msg) {}
};

class OpenFailureException : public IOException {
   public:
    explicit OpenFailureException(std::string const& file_name)
        : IOException("Couldn't open " + file_name) {}
    explicit OpenFailureException(char* const file_name)
        : OpenFailureException(std::string(file_name)) {}
};

class EOFException : public IOException {
   public:
    explicit EOFException() : IOException("Reached EOF") {}
};

class UnexpectedReadException : public IOException {
   public:
    explicit UnexpectedReadException()
        : IOException("Encountered an unexpected character") {}
    explicit UnexpectedReadException(char c)
        : IOException("Encountered an unexpected character (got '" +
                      std::string(1, c) + "')") {}
    explicit UnexpectedReadException(std::string const& s)
        : IOException("Encounterd an unexpected characted (expected " + s +
                      ")") {}
};

class OverflowException : public IOException {
   public:
    template <class T>
    explicit OverflowException(T max_integer)
        : IOException("Integer read overflow: exceeded limit " +
                      std::to_string(max_integer)) {}
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

    std::string read_string_strict(
        std::function<bool(std::size_t, char)> const& check_char,
        std::size_t min_length, std::size_t max_length);

   public:
    Reader() = default;
    Reader(bool strict) : strict(strict) {}
    Reader(char* const file_name) : source(new std::ifstream(file_name)) {
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
    T read_floating_point();

    std::string read_string(std::size_t exact_length);
    std::string read_string(std::size_t min_length, std::size_t max_length);
    std::string read_string(std::string const& allowed_chars,
                            std::size_t min_length, std::size_t max_length);
    std::string read_string(
        std::function<bool(std::size_t, char)> const& check_char,
        std::size_t min_length, std::size_t max_length);

    template <class T>
    friend Reader& operator>>(Reader& r, T& v);
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
    char *s = (char *) malloc((token.size() + 1) * sizeof(char));
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

std::string Reader::read_string_strict(
    std::function<bool(std::size_t, char)> const& check_char,
    std::size_t min_length = 0, std::size_t max_length = std::string::npos) {
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
                throw UnexpectedReadException("string of length <= " +
                                              std::to_string(max_length));
            }
            if (!check_char(i, c)) {
                throw UnexpectedReadException(c);
            }
            s.push_back(c);
        }
    } catch (EOFException const&) {
        if (s.empty()) {
            throw EOFException();
        }
    }
    if (s.size() < min_length) {
        throw UnexpectedReadException("string of length >= " +
                                      std::to_string(min_length));
    }
    return s;
}

std::string Reader::read_string(std::size_t exact_length = 0) {
    return exact_length > 0 ? read_string(exact_length, exact_length)
                            : read_string(0, std::string::npos);
}

std::string Reader::read_string(std::size_t min_length,
                                std::size_t max_length) {
    return read_string([](std::size_t i, char c) { return true; }, min_length,
                       max_length);
}

std::string Reader::read_string(std::string const& allowed_chars,
                                std::size_t min_length = 0,
                                std::size_t max_length = std::string::npos) {
    return read_string(
        [&allowed_chars](std::size_t i, char c) {
            return allowed_chars.find(c) != std::string::npos;
        },
        min_length, max_length);
}

std::string Reader::read_string(
    std::function<bool(std::size_t, char)> const& check_char,
    std::size_t min_length = 0, std::size_t max_length = std::string::npos) {
    if (!strict) skip_spaces();
    return read_string_strict(check_char, min_length, max_length);
}

template <class T>
Reader& operator>>(Reader& r, T& v) {
    throw std::runtime_error("Unimplemented");
    return r;
}

}  // namespace io

}  // namespace cplib
