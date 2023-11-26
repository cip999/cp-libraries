#include "../src/io.hpp"

#include <gtest/gtest.h>

#include <iostream>
#include <string>
#include <vector>

using namespace cplib;

class ReaderTestNonStrict : public testing::Test {
   protected:
    io::Reader reader;

    void TearDown() override {
        reader.~Reader();
    }
};

TEST_F(ReaderTestNonStrict, ReadIntegers_AllCorrect_ShouldNotThrow) {
    std::string input =
        "1 2  \t 0 123000000000 -2147483648\n"
        " abc-42\r\n";
    reader.with_string_stream(input);

    EXPECT_EQ(reader.read_integer<long long>(), 1);
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

TEST_F(ReaderTestNonStrict, ReadIntegers_NegativeUnsigned_ShouldThrow) {
    reader.with_string_stream("-42");
    EXPECT_THROW(reader.read_integer<unsigned int>(), io::UnexpectedReadException);
}

TEST_F(ReaderTestNonStrict, ReadIntegers_WithLeadingZeros_BehavesAsExpected) {
    std::vector<std::string> inputs({"0042", "000", "-0042"});

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

TEST_F(ReaderTestNonStrict, ReadIntegers_WhenNoIntegers_ShouldThrow) {
    reader.with_string_stream("some text with no numbers");
    EXPECT_THROW(reader.read_integer<int>(), io::EOFException);
}

TEST_F(ReaderTestNonStrict, ReadIntegers_WithOverflow_ShouldThrow) {
    reader.with_string_stream("2147483648");
    EXPECT_THROW(reader.read_integer<int>(), io::IntegerOverflowException);

    reader.with_string_stream("-2147483649");
    EXPECT_THROW(reader.read_integer<int>(), io::IntegerOverflowException);

    reader.with_string_stream("4294967296");
    EXPECT_THROW(reader.read_integer<unsigned int>(), io::IntegerOverflowException);
}
