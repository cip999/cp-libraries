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
    static FailedValidationException const& interval_constraint(std::string var,
                                                                T low, T high) {
        return FailedValidationException("Expected " + std::to_string(low) +
                                         " <= " + var +
                                         " <= " + std::to_string(high));
    }
};

}  // namespace cplib
