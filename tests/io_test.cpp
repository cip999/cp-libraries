#include "../src/io.hpp"

#include <gtest/gtest.h>

#include <iostream>
#include <string>
#include <vector>
#include <set>

using namespace cplib;

class ReaderTestNonStrict : public testing::Test {
   protected:
    io::Reader reader;

    void TearDown() override { reader.~Reader(); }
};

TEST_F(ReaderTestNonStrict, ReadInteger_WhenAllCorrect_ShouldSucceed) {
    std::string input =
        "1 2  \t 0 123000000000 -2147483648\n"
        " abc-42\r\n";
    reader.with_string_stream(input);

    EXPECT_EQ(reader.read_integer<long long>(), 1);

    EXPECT_NO_THROW(reader.must_be_space());

    EXPECT_EQ(reader.read_integer<unsigned int>(), 2);
    EXPECT_EQ(reader.read_integer<unsigned int>(), 0);
    EXPECT_EQ(reader.read_integer<unsigned long long>(), 123000000000);
    EXPECT_EQ(reader.read_integer<int>(), -2147483648);

    EXPECT_NO_THROW(reader.must_be_newline());
    EXPECT_NO_THROW(reader.must_be_space());

    EXPECT_EQ(reader.read_integer<int>(), -42);

    EXPECT_NO_THROW(reader.must_be_newline());
    EXPECT_NO_THROW(reader.must_be_eof());
}

TEST_F(ReaderTestNonStrict, ReadInteger_WithNegativeUnsigned_ShouldThrow) {
    reader.with_string_stream("-42");
    EXPECT_THROW(reader.read_integer<unsigned int>(),
                 io::UnexpectedReadException);
}

TEST_F(ReaderTestNonStrict, ReadInteger_WithLeadingZeros_BehavesAsExpected) {
    std::vector<std::string> inputs({"042", "000", "-0042"});

    for (std::string const& s : inputs) {
        reader.with_string_stream(s);
        EXPECT_THROW(reader.read_integer<int>(), io::UnexpectedReadException);
    }

    reader.with_leading_zeros();
    for (std::string const& s : inputs) {
        reader.with_string_stream(s);
        EXPECT_NO_THROW(reader.read_integer<int>());
    }
}

TEST_F(ReaderTestNonStrict, ReadInteger_WhenNoIntegers_ShouldThrow) {
    reader.with_string_stream("some text with no numbers");
    EXPECT_THROW(reader.read_integer<int>(), io::EOFException);

    reader.with_string_stream("-");
    EXPECT_THROW(reader.read_integer<int>(), io::EOFException);
}

TEST_F(ReaderTestNonStrict, ReadInteger_WithOverflow_ShouldThrow) {
    reader.with_string_stream("2147483648");
    EXPECT_THROW(reader.read_integer<int>(), io::OverflowException);

    reader.with_string_stream("-2147483649");
    EXPECT_THROW(reader.read_integer<int>(), io::OverflowException);

    reader.with_string_stream("4294967296");
    EXPECT_THROW(reader.read_integer<unsigned int>(), io::OverflowException);
}

TEST_F(ReaderTestNonStrict, ReadFloatingPoint_WhenAllCorrect_ShouldSucceed) {
    std::string input =
        "1.20 7     -1200.3944383\n"
        "\t0.000001 123.456  hello!10.0\n";
    reader.with_string_stream(input);

    EXPECT_FLOAT_EQ(reader.read_floating_point<float>(), 1.20);

    EXPECT_NO_THROW(reader.must_be_space());

    EXPECT_DOUBLE_EQ(reader.read_floating_point<double>(), 7.0);

    EXPECT_NO_THROW(reader.must_be_space());
    EXPECT_NO_THROW(reader.must_be_space());

    EXPECT_FLOAT_EQ(reader.read_floating_point<float>(), -1200.3944383);
    EXPECT_FLOAT_EQ(reader.read_floating_point<float>(), 0.000001);
    EXPECT_DOUBLE_EQ(reader.read_floating_point<double>(), 123.456);
    EXPECT_DOUBLE_EQ(reader.read_floating_point<double>(), 10.0);

    EXPECT_NO_THROW(reader.must_be_newline());
    EXPECT_NO_THROW(reader.must_be_eof());
}

TEST_F(ReaderTestNonStrict,
       ReadFloatingPoint_WithCommaSeparator_ShouldSucceed) {
    reader.with_string_stream("123,456 0,0").with_comma_as_decimal_separator();

    EXPECT_FLOAT_EQ(reader.read_floating_point<float>(), 123.456);
    EXPECT_DOUBLE_EQ(reader.read_floating_point<double>(), 0.0);
}

TEST_F(ReaderTestNonStrict,
       ReadFloatingPoint_WithLeadingZeros_BehavesAsExpected) {
    std::vector<std::string> inputs({"00.123", "-042", "01.1000", "00"});

    for (std::string const& s : inputs) {
        reader.with_string_stream(s);
        EXPECT_THROW(reader.read_floating_point<float>(),
                     io::UnexpectedReadException);
    }

    reader.with_leading_zeros();
    for (std::string const& s : inputs) {
        reader.with_string_stream(s);
        EXPECT_NO_THROW(reader.read_floating_point<float>());
    }
}

TEST_F(ReaderTestNonStrict,
       ReadFloatingPoint_WithMalformedNumbers_ShouldThrow) {
    reader.with_string_stream("-");
    EXPECT_THROW(reader.read_floating_point<float>(), io::EOFException);

    reader.with_string_stream(".");
    EXPECT_THROW(reader.read_floating_point<float>(), io::EOFException);

    reader.with_string_stream("-.");
    EXPECT_THROW(reader.read_floating_point<float>(),
                 io::UnexpectedReadException);

    reader.with_string_stream("--42");
    EXPECT_THROW(reader.read_floating_point<float>(),
                 io::UnexpectedReadException);

    reader.with_string_stream("1.");
    EXPECT_THROW(reader.read_floating_point<float>(), io::EOFException);

    reader.with_string_stream("1..1");
    EXPECT_THROW(reader.read_floating_point<float>(),
                 io::UnexpectedReadException);

    reader.with_string_stream("1-0.0");
    EXPECT_THROW(reader.read_floating_point<float>(),
                 io::UnexpectedReadException);
}

TEST_F(ReaderTestNonStrict, ReadString_WhenAllCorrect_ShouldSucceed) {
    std::string input =
        "  \t hello world! xxx_123_lol\n\n"
        "something-with-hyphens_and_underscores";
    reader.with_string_stream(input);

    EXPECT_EQ(reader.read_string(), "hello");
    EXPECT_EQ(reader.read_string(6), "world!");
    EXPECT_EQ(reader.read_string(5, 13), "xxx_123_lol");
    EXPECT_EQ(reader.read_string(
                  [](std::size_t i, char c) { return i > 20 || c != '_'; }),
              "something-with-hyphens_and_underscores");
}

TEST_F(ReaderTestNonStrict, ReadString_WhenIncorrect_ShouldThrow) {
    std::string input = "a_test_string";

    reader.with_string_stream(input);
    EXPECT_THROW(reader.read_string(10), FailedValidationException);

    reader.with_string_stream(input);
    EXPECT_THROW(reader.read_string(15, 20), FailedValidationException);

    reader.with_string_stream(input);
    EXPECT_THROW(reader.read_string("abcdefghijklmnopqrstuvwxyz"),
                 FailedValidationException);

    reader.with_string_stream(input);
    EXPECT_THROW(reader.read_string("_aeginrst", 20),
                 FailedValidationException);

    reader.with_string_stream(input);
    EXPECT_THROW(reader.read_string([](std::size_t i, char c) {
        if (c == '_') return i == 1;
        return true;
    }),
                 FailedValidationException);
}

TEST_F(ReaderTestNonStrict, ReadConstant) {
    std::string input = "hello world";

    reader.with_string_stream(input);
    EXPECT_EQ(reader.read_constant("hello"), "hello");
    EXPECT_THROW(reader.read_constant("world"), io::UnexpectedReadException);

    reader.with_string_stream(input);
    EXPECT_EQ(reader.read_constant("hello world"), "hello world");

    reader.with_string_stream(input);
    EXPECT_THROW(reader.read_constant(""), InvalidArgumentException);

    reader.with_string_stream(input);
    EXPECT_THROW(reader.read_constant("hello world!"), io::EOFException);
}

TEST_F(ReaderTestNonStrict, ReadAnyOf) {
    std::string input = "hello world";

    reader.with_string_stream(input);
    EXPECT_EQ(reader.read_any_of({"Say", "hello", "to", "your", "friend"}),
              "hello");
    EXPECT_EQ(reader.read_any_of({"The", "world", "was", "wide", "enough"}),
              "world");
    EXPECT_THROW(reader.read_any_of({"a"}), io::EOFException);

    reader.with_string_stream(input);
    EXPECT_THROW(reader.read_any_of({"Say", "no", "to", "this"}),
                 FailedValidationException);

    reader.with_string_stream(input);
    EXPECT_THROW(reader.read_any_of({"Alexander", "", "Hamilton"}),
                 InvalidArgumentException);
}

TEST_F(ReaderTestNonStrict, ReadNIntegers_WithoutSeparator) {
    reader.with_string_stream("   1 2  -42 7");
    EXPECT_EQ(reader.read_n_integers<int>(3), std::vector<int>({1, 2, -42}));
    EXPECT_THROW(reader.read_n_integers<int>(2), io::EOFException);
}

TEST_F(ReaderTestNonStrict, ReadNIntegers_WithSeparator) {
    reader.with_string_stream("   1 2 -42 7  0");
    EXPECT_EQ(reader.read_n_integers<int>(3, " "),
              std::vector<int>({1, 2, -42}));
    EXPECT_THROW(reader.read_n_integers<int>(2, " "),
                 io::UnexpectedReadException);
}

TEST_F(ReaderTestNonStrict, ReadNFloatingPoint_WithoutSeparator) {
    reader.with_string_stream("   1.23 2  -42.000 7.7");

    std::vector<float> v;
    EXPECT_NO_THROW(v = reader.read_n_floating_point<float>(3));
    EXPECT_EQ(v.size(), 3);
    EXPECT_FLOAT_EQ(v[0], 1.23);
    EXPECT_FLOAT_EQ(v[1], 2);
    EXPECT_FLOAT_EQ(v[2], -42);

    EXPECT_THROW(reader.read_n_floating_point<float>(2), io::EOFException);
}

TEST_F(ReaderTestNonStrict, ReadNFloatingPoint_WithSeparator) {
    reader.with_string_stream("   1.23 2 -42.000 7.7   0");

    std::vector<float> v;
    EXPECT_NO_THROW(v = reader.read_n_floating_point<float>(3, " "));
    EXPECT_EQ(v.size(), 3);
    EXPECT_FLOAT_EQ(v[0], 1.23);
    EXPECT_FLOAT_EQ(v[1], 2);
    EXPECT_FLOAT_EQ(v[2], -42);

    EXPECT_THROW(reader.read_n_floating_point<float>(2, " "),
                 io::UnexpectedReadException);
}

TEST_F(ReaderTestNonStrict, ReadGeneric) {
    std::string input =
        "1 -42.0 hello\n"
        " 3, 5, -6, 0\n"
        "what doesn't kill you makes you stronger\n";
    reader.with_string_stream(input);

    EXPECT_EQ(reader.read<unsigned int>(), 1);
    EXPECT_DOUBLE_EQ(reader.read<double>(), -42);
    EXPECT_EQ(reader.read<std::string>(), "hello");
    EXPECT_EQ(reader.read<int>(4), std::vector<int>({3, 5, -6, 0}));
    EXPECT_EQ(reader.read<std::set<std::string>>(7), std::set<std::string>({
        "doesn't",
        "kill",
        "makes",
        "stronger",
        "what",
        "you",
    }));

    reader.with_string_stream("1 2 3");
    EXPECT_THROW(reader.read<std::set<int>>(0), InvalidArgumentException);
}

class ReaderTestStrict : public testing::Test {
   protected:
    io::Reader reader;

    void SetUp() override { reader.make_strict(); }
    void TearDown() override { reader.~Reader(); }
};

TEST_F(ReaderTestStrict, ReadIntegers) {
    std::string input =
        "1 2  \t 0 123000000000 -2147483648\n"
        " abc-42\r\n";
    reader.with_string_stream(input);

    EXPECT_EQ(reader.read_integer<int>(), 1);
    EXPECT_NO_THROW(reader.must_be_space());
    EXPECT_EQ(reader.read_integer<unsigned int>(), 2);
    reader.skip_spaces();
    EXPECT_EQ(reader.read_integer<long long>(), 0);
    EXPECT_THROW(reader.read_integer<long long>(),
                 io::UnexpectedReadException);

    reader.with_string_stream(input);
    EXPECT_NO_THROW(reader.read_n_integers<int>(2, " "));
    reader.skip_non_numeric();
    EXPECT_THROW(reader.read_n_integers<long long>(3),
                 io::UnexpectedReadException);
}
