#include <cstdio>
#include <exception>
#include <stdexcept>
#include <string>

namespace cplib {

class CplibException : public std::exception {
   public:
    explicit CplibException() = default;
};

class InvalidArgumentException : public std::runtime_error,
                                 public CplibException {
   public:
    explicit InvalidArgumentException(std::string const& msg)
        : std::runtime_error(msg) {}
};

class FailedValidationException : public std::logic_error,
                                  public CplibException {
   public:
    explicit FailedValidationException(std::string const& msg)
        : std::logic_error(msg) {}

    template <class T>
    static FailedValidationException interval_constraint(std::string var, T low,
                                                         T high) noexcept {
        return FailedValidationException("Expected " + std::to_string(low) +
                                         " <= " + var +
                                         " <= " + std::to_string(high));
    }

    const char* what() const noexcept override {
        return std::logic_error::what();
    }

    std::string what_with_line(const char* file, unsigned int line) noexcept {
        return "FAILED VALIDATION AT " + std::string(file) +
               "::" + std::to_string(line) + "\n---\n" + what() + "\n---";
    }
};

template <class T,
          std::enable_if_t<std::is_convertible_v<std::decay_t<T>, std::string>,
                           bool> = true>
std::string to_string(T const& x) {
    return "\"" + static_cast<std::string>(x) + "\"";
}

template <class T, class = decltype(std::to_string(std::declval<T>()))>
std::string to_string(T const& x) {
    return std::to_string(x);
}

template <class V, class = decltype(*std::declval<V>().begin()),
          std::enable_if_t<!std::is_convertible_v<std::decay_t<V>, std::string>,
                         bool> = true>
std::string to_string(V const& v) {
    return "[iterable]";
}

}  // namespace cplib
