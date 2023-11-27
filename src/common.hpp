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

}  // namespace cplib
