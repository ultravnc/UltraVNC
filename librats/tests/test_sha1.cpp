#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "sha1.h"
#include <string>
#include <vector>

using namespace librats;

class SHA1Test : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup if needed
    }
    
    void TearDown() override {
        // Cleanup if needed
    }
};

// Test empty string hashing
TEST_F(SHA1Test, EmptyStringTest) {
    SHA1 sha1;
    std::string result = sha1.finalize();
    
    // SHA1 of empty string should be da39a3ee5e6b4b0d3255bfef95601890afd80709
    EXPECT_EQ(result, "da39a3ee5e6b4b0d3255bfef95601890afd80709");
}

// Test single character hashing
TEST_F(SHA1Test, SingleCharacterTest) {
    SHA1 sha1;
    sha1.update('a');
    std::string result = sha1.finalize();
    
    // SHA1 of "a" should be 86f7e437faa5a7fce15d1ddcb9eaeaea377667b8
    EXPECT_EQ(result, "86f7e437faa5a7fce15d1ddcb9eaeaea377667b8");
}

// Test short string hashing
TEST_F(SHA1Test, ShortStringTest) {
    SHA1 sha1;
    sha1.update("hello");
    std::string result = sha1.finalize();
    
    // SHA1 of "hello" should be aaf4c61ddcc5e8a2dabede0f3b482cd9aea9434d
    EXPECT_EQ(result, "aaf4c61ddcc5e8a2dabede0f3b482cd9aea9434d");
}

// Test longer string hashing
TEST_F(SHA1Test, LongStringTest) {
    SHA1 sha1;
    sha1.update("The quick brown fox jumps over the lazy dog");
    std::string result = sha1.finalize();
    
    // SHA1 of "The quick brown fox jumps over the lazy dog" should be 2fd4e1c67a2d28fced849ee1bb76e7391b93eb12
    EXPECT_EQ(result, "2fd4e1c67a2d28fced849ee1bb76e7391b93eb12");
}

// Test very long string hashing (>64 bytes to test multiple blocks)
TEST_F(SHA1Test, VeryLongStringTest) {
    SHA1 sha1;
    std::string long_string = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    sha1.update(long_string);
    std::string result = sha1.finalize();
    
    // SHA1 of "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq" should be 84983e441c3bd26ebaae4aa1f95129e5e54670f1
    EXPECT_EQ(result, "84983e441c3bd26ebaae4aa1f95129e5e54670f1");
}

// Test incremental hashing (byte by byte)
TEST_F(SHA1Test, IncrementalByteTest) {
    SHA1 sha1;
    std::string input = "hello";
    
    for (char c : input) {
        sha1.update(static_cast<uint8_t>(c));
    }
    
    std::string result = sha1.finalize();
    
    // Should be same as "hello" hash
    EXPECT_EQ(result, "aaf4c61ddcc5e8a2dabede0f3b482cd9aea9434d");
}

// Test incremental hashing (buffer by buffer)
TEST_F(SHA1Test, IncrementalBufferTest) {
    SHA1 sha1;
    std::string part1 = "hello";
    std::string part2 = " world";
    
    sha1.update(reinterpret_cast<const uint8_t*>(part1.c_str()), part1.length());
    sha1.update(reinterpret_cast<const uint8_t*>(part2.c_str()), part2.length());
    
    std::string result = sha1.finalize();
    
    // Should be same as "hello world" hash
    SHA1 reference;
    reference.update("hello world");
    std::string expected = reference.finalize();
    
    EXPECT_EQ(result, expected);
}

// Test string update method
TEST_F(SHA1Test, StringUpdateTest) {
    SHA1 sha1;
    sha1.update("hello");
    sha1.update(" ");
    sha1.update("world");
    std::string result = sha1.finalize();
    
    // Should be same as "hello world" hash
    SHA1 reference;
    reference.update("hello world");
    std::string expected = reference.finalize();
    
    EXPECT_EQ(result, expected);
}

// Test static hash function
TEST_F(SHA1Test, StaticHashTest) {
    std::string result = SHA1::hash("hello");
    
    // Should be same as instance-based hash
    SHA1 sha1;
    sha1.update("hello");
    std::string expected = sha1.finalize();
    
    EXPECT_EQ(result, expected);
    EXPECT_EQ(result, "aaf4c61ddcc5e8a2dabede0f3b482cd9aea9434d");
}

// Test binary data hashing
TEST_F(SHA1Test, BinaryDataTest) {
    std::vector<uint8_t> binary_data = {0x00, 0x01, 0x02, 0x03, 0xFF, 0xFE, 0xFD, 0xFC};
    
    SHA1 sha1;
    sha1.update(binary_data.data(), binary_data.size());
    std::string result = sha1.finalize();
    
    // Result should be consistent
    SHA1 reference;
    reference.update(binary_data.data(), binary_data.size());
    std::string expected = reference.finalize();
    
    EXPECT_EQ(result, expected);
    EXPECT_EQ(result.length(), 40);  // SHA1 hash is 40 hex characters
}

// Test zero-length buffer
TEST_F(SHA1Test, ZeroLengthBufferTest) {
    SHA1 sha1;
    uint8_t* null_buffer = nullptr;
    sha1.update(null_buffer, 0);
    std::string result = sha1.finalize();
    
    // Should be same as empty string hash
    EXPECT_EQ(result, "da39a3ee5e6b4b0d3255bfef95601890afd80709");
}

// Test multiple finalize calls (should throw or return empty)
TEST_F(SHA1Test, MultipleFinalizeTest) {
    SHA1 sha1;
    sha1.update("test");
    std::string result1 = sha1.finalize();
    
    // Second finalize should return empty string or throw
    std::string result2 = sha1.finalize();
    EXPECT_TRUE(result2.empty());
    
    EXPECT_EQ(result1.length(), 40);  // First result should be valid
}

// Test case sensitivity
TEST_F(SHA1Test, CaseSensitivityTest) {
    std::string hash1 = SHA1::hash("Hello");
    std::string hash2 = SHA1::hash("hello");
    
    EXPECT_NE(hash1, hash2);
}

// Test numeric string hashing
TEST_F(SHA1Test, NumericStringTest) {
    std::string result = SHA1::hash("123456789");
    
    // Should produce consistent hash
    EXPECT_EQ(result.length(), 40);
    EXPECT_EQ(result, "f7c3bc1d808e04732adf679965ccc34ca7ae3441");
}

// Test special characters
TEST_F(SHA1Test, SpecialCharactersTest) {
    std::string result = SHA1::hash("!@#$%^&*()");
    
    // Should produce consistent hash
    EXPECT_EQ(result.length(), 40);
    EXPECT_EQ(result, "bf24d65c9bb05b9b814a966940bcfa50767c8a8d");
}

// Test whitespace handling
TEST_F(SHA1Test, WhitespaceTest) {
    std::string hash1 = SHA1::hash("hello world");
    std::string hash2 = SHA1::hash("hello  world");  // Two spaces
    std::string hash3 = SHA1::hash("hello\tworld");   // Tab
    std::string hash4 = SHA1::hash("hello\nworld");   // Newline
    
    // All should be different
    EXPECT_NE(hash1, hash2);
    EXPECT_NE(hash1, hash3);
    EXPECT_NE(hash1, hash4);
    EXPECT_NE(hash2, hash3);
    EXPECT_NE(hash2, hash4);
    EXPECT_NE(hash3, hash4);
}

// Test Unicode/UTF-8 handling
TEST_F(SHA1Test, UnicodeTest) {
    std::string unicode_str = "Hello 世界";  // "Hello World" in Chinese
    std::string result = SHA1::hash(unicode_str);
    
    // Should produce consistent hash
    EXPECT_EQ(result.length(), 40);
    
    // Should be different from ASCII version
    std::string ascii_result = SHA1::hash("Hello World");
    EXPECT_NE(result, ascii_result);
}

// Test large data (stress test)
TEST_F(SHA1Test, LargeDataTest) {
    std::string large_data;
    large_data.reserve(10000);
    
    // Create 10KB of data
    for (int i = 0; i < 10000; ++i) {
        large_data += static_cast<char>('a' + (i % 26));
    }
    
    SHA1 sha1;
    sha1.update(large_data);
    std::string result = sha1.finalize();
    
    // Should produce valid hash
    EXPECT_EQ(result.length(), 40);
    
    // Should be consistent
    std::string result2 = SHA1::hash(large_data);
    EXPECT_EQ(result, result2);
}

// Test that hash is deterministic
TEST_F(SHA1Test, DeterministicTest) {
    std::string input = "deterministic test";
    
    std::string hash1 = SHA1::hash(input);
    std::string hash2 = SHA1::hash(input);
    std::string hash3 = SHA1::hash(input);
    
    EXPECT_EQ(hash1, hash2);
    EXPECT_EQ(hash2, hash3);
    EXPECT_EQ(hash1, hash3);
}

// Test hash format (should be lowercase hex)
TEST_F(SHA1Test, HashFormatTest) {
    std::string result = SHA1::hash("test");
    
    // Should be 40 characters long
    EXPECT_EQ(result.length(), 40);
    
    // Should contain only lowercase hex digits
    for (char c : result) {
        EXPECT_TRUE((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'));
    }
}

// Test boundary conditions
TEST_F(SHA1Test, BoundaryConditionsTest) {
    // Test exactly 55 bytes (boundary for single block)
    std::string boundary55(55, 'a');
    std::string hash55 = SHA1::hash(boundary55);
    EXPECT_EQ(hash55.length(), 40);
    
    // Test exactly 56 bytes (boundary for two blocks)
    std::string boundary56(56, 'a');
    std::string hash56 = SHA1::hash(boundary56);
    EXPECT_EQ(hash56.length(), 40);
    
    // Test exactly 64 bytes (one full block)
    std::string boundary64(64, 'a');
    std::string hash64 = SHA1::hash(boundary64);
    EXPECT_EQ(hash64.length(), 40);
    
    // All should be different
    EXPECT_NE(hash55, hash56);
    EXPECT_NE(hash56, hash64);
    EXPECT_NE(hash55, hash64);
}

// Test known test vectors
TEST_F(SHA1Test, KnownVectorsTest) {
    // Test vector from RFC 3174
    std::string result = SHA1::hash("abc");
    EXPECT_EQ(result, "a9993e364706816aba3e25717850c26c9cd0d89d");
    
    // Another test vector
    std::string result2 = SHA1::hash("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq");
    EXPECT_EQ(result2, "84983e441c3bd26ebaae4aa1f95129e5e54670f1");
} 