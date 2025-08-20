#include <gtest/gtest.h>
#include <print>

#include "scan.hpp"

// --- Single Type Tests ---

TEST(ScanTest, EmptyFormatSpecifier) {
    auto result = stdx::scan<std::string>("string", "{}");
    ASSERT_TRUE(result);
}

TEST(ScanTest, WrongFormatSpecifier) {
    auto result = stdx::scan<std::string>("wrong format specifier", "{s}");
    EXPECT_FALSE(result);
}

TEST(ScanTest, CStringTypeTest) {
    const char* s = "lovely string";
    auto result   = stdx::scan<const char*>(s, "{%s}");
    EXPECT_TRUE(result);
    EXPECT_TRUE(std::get<0>(result.value().result) == s);
}

TEST(ScanTest, StringSpecifierTest) {
    auto result = stdx::scan<std::string>("lovely string", "{%s}");
    EXPECT_TRUE(result);
}

TEST(ScanTest, DoubleStringSpecifierTest) {
    auto result = stdx::scan<std::string, std::string>("lovely string makes love", "{} string makes {%s}");
    EXPECT_TRUE(result);
}

TEST(ScanTest, ParseSingleString_EmptySpecifier) {
    auto result = stdx::scan<std::string>("hello world", "{}");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(std::get<0>(result.value().result), "hello world");
}

TEST(ScanTest, ParseSingleInt_EmptySpecifier) {
    auto result = stdx::scan<int>("42", "{}");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(std::get<0>(result.value().result), 42);
}

TEST(ScanTest, ParseSingleInt_D_Specifier) {
    auto result = stdx::scan<int>("-123", "{%d}");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(std::get<0>(result.value().result), -123);
}

TEST(ScanTest, ParseSingleUnsignedInt_U_Specifier) {
    auto result = stdx::scan<unsigned int>("456", "{%u}");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(std::get<0>(result.value().result), 456u);
}

TEST(ScanTest, ParseSingleDouble_F_Specifier) {
    auto result = stdx::scan<double>("3.14159", "{%f}");
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(std::get<0>(result.value().result), 3.14159);
}

TEST(ScanTest, ParseSingleString_S_Specifier) {
    auto result = stdx::scan<std::string>("test_string", "{%s}");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(std::get<0>(result.value().result), "test_string");
}

// --- Multiple Type Tests ---

TEST(ScanTest, ParseMultipleTypes_MixedSpecifiers) {
    auto result = stdx::scan<int, std::string, double>("100 hello 2.5", "{%d} {%s} {%f}");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(std::get<0>(result.value().result), 100);
    EXPECT_EQ(std::get<1>(result.value().result), "hello");
    EXPECT_DOUBLE_EQ(std::get<2>(result.value().result), 2.5);
}

TEST(ScanTest, ParseMultipleTypes_EmptyAndSpecified) {
    auto result = stdx::scan<std::string, int>("start 99", "{} {%d}");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(std::get<0>(result.value().result), "start");
    EXPECT_EQ(std::get<1>(result.value().result), 99);
}

TEST(ScanTest, ParseMultipleStrings_EmptySpecifiers) {
    auto result = stdx::scan<std::string, std::string>("word1 word2", "{} {}");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(std::get<0>(result.value().result), "word1");
    EXPECT_EQ(std::get<1>(result.value().result), "word2");
}

// --- Format and Input Matching Tests ---

TEST(ScanTest, ParseWithLiteralText) {
    auto result = stdx::scan<int, std::string>("ID: 123 Name: Smith", "ID: {%d} Name: {%s}");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(std::get<0>(result.value().result), 123);
    EXPECT_EQ(std::get<1>(result.value().result), "Smith");
}

TEST(ScanTest, ParseWithLeadingAndTrailingText) {
    auto result = stdx::scan<int>("[Value=50]", "[Value={%d}]");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(std::get<0>(result.value().result), 50);
}

// --- Error Handling Tests ---

TEST(ScanTest, FailWrongSpecifierForType) {
    auto result = stdx::scan<int>("not_a_number", "{%d}");
    EXPECT_FALSE(result.has_value()) << "Scan should fail due to type mismatch";
    EXPECT_NE(result.error().message.find("Failed to convert into <int>."), std::string::npos);
}

TEST(ScanTest, FailMismatchedSpecifierCount) {
    auto result = stdx::scan<int, std::string>("100", "{%d}");
    EXPECT_FALSE(result.has_value()) << "Scan should fail due to mismatched type's count";
    EXPECT_NE(result.error().message.find(" Mismatched number of format specifiers and target types"),
              std::string::npos);
}

TEST(ScanTest, FailInvalidFormat_UnmatchedBrace) {
    auto result = stdx::scan<int>("100", "{%d");  // Не хватает закрывающей placeholder.
    EXPECT_FALSE(result.has_value()) << "Scan should fail due to invalid format";
    EXPECT_NE(result.error().message.find("Unformatted text in input and format string are different"),
              std::string::npos);
}

TEST(ScanTest, FailLiteralTextMismatch) {
    auto result =
        stdx::scan<int>("ID 123", "ID: {%d}");  // Форматная строка ожидает 'ID: ', а строка ввода имеет вид 'ID '
    EXPECT_FALSE(result.has_value()) << "Scan should fail due to literal text mismatch";
    EXPECT_NE(result.error().message.find("Unformatted text in input and format string are different"),
              std::string::npos);
}

TEST(ScanTest, FailEmptyInputNonEmptyFormat) {
    auto result = stdx::scan<int>("", "{%d}");
    EXPECT_FALSE(result.has_value()) << "Scan should fail for empty input";
    EXPECT_NE(result.error().message.find("Failed to convert into <int>"), std::string::npos);
}
