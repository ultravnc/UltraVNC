#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <variant>
#include <cstdint>

// Use shared_ptr variant for GCC 11 and lower compatibility
#ifndef LIBRATS_USE_SHARED_PTR_VARIANT
#if defined(__GNUC__) && (__GNUC__ <= 11)
#define LIBRATS_USE_SHARED_PTR_VARIANT 1
#else
#define LIBRATS_USE_SHARED_PTR_VARIANT 0
#endif
#endif

namespace librats {

// Forward declarations
class BencodeValue;
using BencodeDict = std::unordered_map<std::string, BencodeValue>;
using BencodeList = std::vector<BencodeValue>;

/**
 * Represents a bencoded value which can be:
 * - Integer (signed 64-bit)
 * - String (byte string)
 * - List (array of bencoded values)
 * - Dictionary (map of string keys to bencoded values)
 */
class BencodeValue {
public:
    enum class Type {
        Integer,
        String,
        List,
        Dictionary
    };

    // Constructors
    BencodeValue();
    BencodeValue(int64_t value);
    BencodeValue(const std::string& value);
    BencodeValue(const char* value);
    BencodeValue(const BencodeList& value);
    BencodeValue(const BencodeDict& value);
    
    // Copy and move constructors
    BencodeValue(const BencodeValue& other);
    BencodeValue(BencodeValue&& other) noexcept;
    BencodeValue& operator=(const BencodeValue& other);
    BencodeValue& operator=(BencodeValue&& other) noexcept;
    
    ~BencodeValue();

    // Type checking
    Type get_type() const { return type_; }
    bool is_integer() const { return type_ == Type::Integer; }
    bool is_string() const { return type_ == Type::String; }
    bool is_list() const { return type_ == Type::List; }
    bool is_dict() const { return type_ == Type::Dictionary; }

    // Value access
    int64_t as_integer() const;
    const std::string& as_string() const;
    const BencodeList& as_list() const;
    const BencodeDict& as_dict() const;

    // Mutable access
    BencodeList& as_list();
    BencodeDict& as_dict();

    // Dictionary operations
    bool has_key(const std::string& key) const;
    const BencodeValue& operator[](const std::string& key) const;
    BencodeValue& operator[](const std::string& key);

    // List operations
    const BencodeValue& operator[](size_t index) const;
    BencodeValue& operator[](size_t index);
    void push_back(const BencodeValue& value);
    size_t size() const;

    // Encoding
    std::vector<uint8_t> encode() const;
    std::string encode_string() const;

    // Static creation methods
    static BencodeValue create_integer(int64_t value);
    static BencodeValue create_string(const std::string& value);
    static BencodeValue create_list();
    static BencodeValue create_dict();

private:
    Type type_;
#if LIBRATS_USE_SHARED_PTR_VARIANT
    std::variant<int64_t, std::string, std::shared_ptr<BencodeList>, std::shared_ptr<BencodeDict>> value_;
#else
    std::variant<int64_t, std::string, BencodeList, BencodeDict> value_;
#endif
    
    void encode_to_buffer(std::vector<uint8_t>& buffer) const;
};

/**
 * Bencode decoder
 */
class BencodeDecoder {
public:
    static BencodeValue decode(const std::vector<uint8_t>& data);
    static BencodeValue decode(const std::string& data);
    static BencodeValue decode(const uint8_t* data, size_t size);

private:
    const uint8_t* data_;
    size_t size_;
    size_t pos_;

    BencodeDecoder(const uint8_t* data, size_t size);
    
    BencodeValue decode_value();
    BencodeValue decode_integer();
    BencodeValue decode_string();
    BencodeValue decode_list();
    BencodeValue decode_dict();
    
    bool has_more() const { return pos_ < size_; }
    uint8_t current_byte() const;
    uint8_t consume_byte();
    std::string consume_string(size_t length);
};

/**
 * Utility functions
 */
namespace bencode {
    BencodeValue decode(const std::vector<uint8_t>& data);
    BencodeValue decode(const std::string& data);
    std::vector<uint8_t> encode(const BencodeValue& value);
    std::string encode_string(const BencodeValue& value);
}

} // namespace librats 