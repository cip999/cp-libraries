#include <cstdio>
#include <exception>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace cplib {

namespace io {

class IOException : public std::ios_base::failure {
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

class IntegerOverflowException : public IOException {
   public:
    template <class T>
    explicit IntegerOverflowException(T max_integer)
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

    ~Reader() {
        source.get_deleter()(source.release());
    }

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

    template <class T>
    T read_integer();

    template <class T>
    friend Reader& operator >>(Reader& r, T& v);
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
            if (c == '0' && n == 0 && !start && !leading_zeros) {
                throw UnexpectedReadException(c);
            }
            start = false;
            long long units = static_cast<T>(c - '0');
            if (n > (limit - units) / 10) {
                throw IntegerOverflowException(limit);
            }
            n = 10 * n + units;
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
    unsigned_T limit = negative
        ? static_cast<unsigned_T>(0) - std::numeric_limits<T>::min()
        : static_cast<unsigned_T>(std::numeric_limits<T>::max());
    if (n > limit) throw IntegerOverflowException(limit);
    if (negative) {
        // This should work because of two's complement.
        return static_cast<T>(static_cast<unsigned_T>(0) - n);
    };
    return static_cast<T>(n);
}

template <>
unsigned int Reader::read_integer_strict() {
    return read_unsigned_strict<unsigned int>();
}

template <>
int Reader::read_integer_strict() {
    return read_signed_strict<int>();
}

template <>
unsigned long long Reader::read_integer_strict() {
    return read_unsigned_strict<unsigned long long>();
}

template <>
long long Reader::read_integer_strict() {
    return read_signed_strict<long long>();
}

template <class T>
T Reader::read_integer() {
    if (!strict) skip_non_numeric();
    return read_integer_strict<T>();
}

template<class T>
Reader& operator >>(Reader& r, T& v) {
    v = r.read_integer<T>();
    return r;
}

}  // namespace io

}  // namespace cplib
