#include <algorithm>
#include <cstdio>
#include <functional>
#include <iostream>
#include <regex>
#include <set>
#include <string>
#include <variant>
#include <vector>

#include "io.hpp"

#define ASSERT(f)                                                           \
    {                                                                       \
        auto x = f;                                                         \
        if (x.failed()) {                                                   \
            auto e = x.get_failure();                                       \
            std::cerr << e.what_with_line(__FILE__, __LINE__) << std::endl; \
            throw e;                                                        \
        }                                                                   \
    }

namespace cplib::val {

class ValidationResult {
   private:
    std::variant<std::string, FailedValidationException> const outcome;

    static std::string indent(std::string s) {
        static const std::string indent = "  ";
        return indent + std::regex_replace(s, std::regex("\\n"), "\n" + indent);
    }

   public:
    explicit ValidationResult() = default;
    ValidationResult(std::string const& success) : outcome(success) {}
    ValidationResult(FailedValidationException const& failure)
        : outcome(failure) {}

    inline bool success() const noexcept {
        return std::holds_alternative<std::string>(outcome);
    }
    inline bool failed() const noexcept {
        return std::holds_alternative<FailedValidationException>(outcome);
    }

    std::string get_success() const { return std::get<std::string>(outcome); }
    FailedValidationException get_failure() const {
        return std::get<FailedValidationException>(outcome);
    }
    std::string message() const {
        return success() ? get_success() : std::string(get_failure().what());
    }

    operator bool() const { return success(); }

    ValidationResult operator!() const;
    friend ValidationResult operator&&(ValidationResult const& a,
                                       ValidationResult const& b);
    friend ValidationResult operator||(ValidationResult const& a,
                                       ValidationResult const& b);
};

ValidationResult ValidationResult::operator!() const {
    if (success()) {
        return FailedValidationException("NOT\n" + indent(message()));
    }
    return "NOT\n" + indent(message());
}

ValidationResult operator&&(ValidationResult const& a,
                            ValidationResult const& b) {
    std::string new_message = ValidationResult::indent(a.message()) +
                              "\nAND\n" + ValidationResult::indent(b.message());
    if (a.success() && b.success()) {
        return new_message;
    }
    return FailedValidationException(new_message);
}

ValidationResult operator||(ValidationResult const& a,
                            ValidationResult const& b) {
    std::string new_message = ValidationResult::indent(a.message()) + "\nOR\n" +
                              ValidationResult::indent(b.message());
    if (a.success() || b.success()) {
        return new_message;
    }
    return FailedValidationException(new_message);
}

template <class T>
ValidationResult eq(T const& a, T const& b) {
    return a == b ? ValidationResult("Elements are equal")
                  : ValidationResult(
                        FailedValidationException("Elements are not equal"));
}

template <class T>
ValidationResult neq(T const& a, T const& b) {
    return !(a == b) ? ValidationResult("Elements are unequal")
                     : ValidationResult(FailedValidationException(
                           "Elements are not unequal: " + to_string(a) +
                           " != " + to_string(b)));
}

template <class T>
ValidationResult lt(T const& a, T const& b) {
    return a < b ? ValidationResult("Comparison satisfied")
                 : ValidationResult(FailedValidationException(
                       "Comparison failed: " + to_string(a) +
                       " >= " + to_string(b)));
}

template <class T>
ValidationResult lte(T const& a, T const& b) {
    return !(b < a) ? ValidationResult("Comparison satisfied")
                    : ValidationResult(FailedValidationException(
                          "Comparison failed: " + to_string(a) + " > " +
                          to_string(b)));
}

template <class T>
ValidationResult gt(T const& a, T const& b) {
    return b < a ? ValidationResult("Comparison satisfied")
                 : ValidationResult(FailedValidationException(
                       "Comparison failed: " + to_string(a) +
                       " <= " + to_string(b)));
}

template <class T>
ValidationResult gte(T const& a, T const& b) {
    return !(a < b) ? ValidationResult("Comparison satisfied")
                    : ValidationResult(FailedValidationException(
                          "Comparison failed: " + to_string(a) + " < " +
                          to_string(b)));
}

template <class T>
ValidationResult between(T const& x, T const& low, T const& high) {
    std::string interval = "[" + to_string(low) + ", " + to_string(high) + "]";
    if (x < low) {
        return FailedValidationException("Value does not lie in " + interval +
                                         ": " + to_string(x) + " < " +
                                         to_string(low));
    } else if (high < x) {
        return FailedValidationException("Value does not lie in " + interval +
                                         ": " + to_string(x) + " > " +
                                         to_string(high));
    }
    return "Value (x = " + to_string(x) + ") lies in " + interval;
}

template <class It, class P>
ValidationResult all(It const& begin, It const& end, P const& predicate) {
    for (It it = begin; it != end; it = std::next(it)) {
        ValidationResult res = predicate(*it);
        if (res.failed()) {
            return FailedValidationException(
                "Failed check for element " +
                to_string(std::distance(begin, it)) + ": " + res.message());
        }
    }
    return std::string("Property satisfied by all elements");
}

template <class V, class P>
ValidationResult all(V const& v, P const& predicate) {
    return all(v.begin(), v.end(), predicate);
}

template <class It, class T>
ValidationResult all_between(It const& begin, It const& end, T const& low,
                             T const& high) {
    return all(begin, end,
               [low, high](T const& x) { return between(x, low, high); });
}

template <class V, class T>
ValidationResult all_between(V const& v, T const& low, T const& high) {
    return all_between(v.begin(), v.end(), low, high);
}

template <class It, class T = std::decay_t<decltype(*std::declval<It>())>>
ValidationResult distinct(It const& begin, It const& end) {
    std::vector<T> v(begin, end);
    std::sort(v.begin(), v.end());
    for (auto it = v.begin(); std::next(it) != v.end(); it = std::next(it)) {
        if (*it == *std::next(it)) {
            return FailedValidationException(
                "Elements are not distinct: Multiple occurrences of " +
                to_string(*it));
        }
    }
    return std::string("Elements are distinct");
}

template <class V>
ValidationResult distinct(V const& v) {
    return distinct(v.begin(), v.end());
}

template <class It, class T = std::decay_t<decltype(*std::declval<It>())>>
ValidationResult sorted(It const& begin, It const& end, bool strict = true,
                        bool decreasing = false) {
    auto compare = [strict, decreasing](T const& a, T const& b) {
        bool check_increasing = strict ? a < b : !(b < a);
        return decreasing ? !check_increasing : check_increasing;
    };
    return sorted(begin, end, compare);
}

template <class It, class C>
ValidationResult sorted(It const& begin, It const& end, C const& compare) {
    for (It it = begin; std::next(it) != end; it = std::next(it)) {
        if (!compare(*it, *std::next(it))) {
            int pos = std::distance(begin, it);
            return FailedValidationException(
                "Array is not sorted: Wrong order at positions " +
                to_string(pos) + " and " + to_string(pos + 1));
        }
    }
    return std::string("Array is sorted");
}

template <class T>
ValidationResult sorted(std::vector<T> const& v, bool strict = true,
                        bool decreasing = false) {
    return sorted(v.begin(), v.end(), strict, decreasing);
}

template <class T, class C>
ValidationResult sorted(std::vector<T> const& v, C const& compare) {
    return sorted(v.begin(), v.end(), compare);
}

}  // namespace cplib::val
