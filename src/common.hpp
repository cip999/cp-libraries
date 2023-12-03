#include <cstdio>
#include <exception>
#include <limits>
#include <stdexcept>
#include <string>

namespace cplib {

template <class T>
struct Limits {
    static const T MIN = std::numeric_limits<T>::min();
    static const T MAX = std::numeric_limits<T>::max();
};

class CplibException : public std::exception {
   private:
    const std::string msg_with_prefix;

    inline virtual std::string prefix() const noexcept { return "GENERIC"; }
    inline std::string add_prefix(std::string const& msg) const noexcept {
        return prefix() + ": " + msg;
    }

   protected:
    const std::string msg;

   public:
    explicit CplibException() = default;
    explicit CplibException(std::string const& msg)
        : msg(msg), msg_with_prefix(add_prefix(msg)) {}

    const char* what() const noexcept override { return msg.c_str(); }
};

class InvalidArgumentException : public CplibException {
   private:
    inline std::string prefix() const noexcept override {
        return "INVALID ARGUMENT";
    }

   public:
    InvalidArgumentException(std::string const& msg) : CplibException(msg) {}
};

class FailedValidationException : public CplibException {
   private:
    inline std::string prefix() const noexcept override {
        return "FAILED VALIDATION";
    }

   public:
    template <class T>
    static FailedValidationException interval_constraint(std::string var, T low,
                                                         T high) noexcept {
        return FailedValidationException("Expected " + std::to_string(low) +
                                         " <= " + var +
                                         " <= " + std::to_string(high));
    }

    FailedValidationException(std::string const& msg) : CplibException(msg) {}

    std::string what_with_line(const char* file,
                               unsigned int line) const noexcept {
        return "FAILED VALIDATION AT " + std::string(file) +
               "::" + std::to_string(line) + "\n---\n" + msg + "\n---";
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
