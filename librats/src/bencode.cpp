#include "bencode.h"
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <cstdint>

namespace librats {

// BencodeValue implementation
BencodeValue::BencodeValue() : type_(Type::String), value_(std::string()) {}

BencodeValue::BencodeValue(int64_t value) : type_(Type::Integer), value_(value) {}

BencodeValue::BencodeValue(const std::string& value) : type_(Type::String), value_(value) {}

BencodeValue::BencodeValue(const char* value) : type_(Type::String), value_(std::string(value)) {}

#if LIBRATS_USE_SHARED_PTR_VARIANT
BencodeValue::BencodeValue(const BencodeList& value) : type_(Type::List), value_(std::make_shared<BencodeList>(value)) {}
#else
BencodeValue::BencodeValue(const BencodeList& value) : type_(Type::List), value_(value) {}
#endif

#if LIBRATS_USE_SHARED_PTR_VARIANT
BencodeValue::BencodeValue(const BencodeDict& value) : type_(Type::Dictionary), value_(std::make_shared<BencodeDict>(value)) {}
#else
BencodeValue::BencodeValue(const BencodeDict& value) : type_(Type::Dictionary), value_(value) {}
#endif

BencodeValue::BencodeValue(const BencodeValue& other) : type_(other.type_), value_(other.value_) {}

BencodeValue::BencodeValue(BencodeValue&& other) noexcept : type_(other.type_), value_(std::move(other.value_)) {}

BencodeValue& BencodeValue::operator=(const BencodeValue& other) {
    if (this != &other) {
        type_ = other.type_;
        value_ = other.value_;
    }
    return *this;
}

BencodeValue& BencodeValue::operator=(BencodeValue&& other) noexcept {
    if (this != &other) {
        type_ = other.type_;
        value_ = std::move(other.value_);
    }
    return *this;
}

BencodeValue::~BencodeValue() = default;

int64_t BencodeValue::as_integer() const {
    if (type_ != Type::Integer) {
        throw std::runtime_error("BencodeValue is not an integer");
    }
    return std::get<int64_t>(value_);
}

const std::string& BencodeValue::as_string() const {
    if (type_ != Type::String) {
        throw std::runtime_error("BencodeValue is not a string");
    }
    return std::get<std::string>(value_);
}

const BencodeList& BencodeValue::as_list() const {
    if (type_ != Type::List) {
        throw std::runtime_error("BencodeValue is not a list");
    }
#if LIBRATS_USE_SHARED_PTR_VARIANT
    return *std::get<std::shared_ptr<BencodeList>>(value_);
#else
    return std::get<BencodeList>(value_);
#endif
}

const BencodeDict& BencodeValue::as_dict() const {
    if (type_ != Type::Dictionary) {
        throw std::runtime_error("BencodeValue is not a dictionary");
    }
#if LIBRATS_USE_SHARED_PTR_VARIANT
    return *std::get<std::shared_ptr<BencodeDict>>(value_);
#else
    return std::get<BencodeDict>(value_);
#endif
}

BencodeList& BencodeValue::as_list() {
    if (type_ != Type::List) {
        throw std::runtime_error("BencodeValue is not a list");
    }
#if LIBRATS_USE_SHARED_PTR_VARIANT
    return *std::get<std::shared_ptr<BencodeList>>(value_);
#else
    return std::get<BencodeList>(value_);
#endif
}

BencodeDict& BencodeValue::as_dict() {
    if (type_ != Type::Dictionary) {
        throw std::runtime_error("BencodeValue is not a dictionary");
    }
#if LIBRATS_USE_SHARED_PTR_VARIANT
    return *std::get<std::shared_ptr<BencodeDict>>(value_);
#else
    return std::get<BencodeDict>(value_);
#endif
}

bool BencodeValue::has_key(const std::string& key) const {
    if (type_ != Type::Dictionary) {
        return false;
    }
#if LIBRATS_USE_SHARED_PTR_VARIANT
    const auto& dict = *std::get<std::shared_ptr<BencodeDict>>(value_);
#else
    const auto& dict = std::get<BencodeDict>(value_);
#endif
    return dict.find(key) != dict.end();
}

const BencodeValue& BencodeValue::operator[](const std::string& key) const {
    if (type_ != Type::Dictionary) {
        throw std::runtime_error("BencodeValue is not a dictionary");
    }
#if LIBRATS_USE_SHARED_PTR_VARIANT
    const auto& dict = *std::get<std::shared_ptr<BencodeDict>>(value_);
#else
    const auto& dict = std::get<BencodeDict>(value_);
#endif
    auto it = dict.find(key);
    if (it == dict.end()) {
        throw std::runtime_error("Key not found in dictionary: " + key);
    }
    return it->second;
}

BencodeValue& BencodeValue::operator[](const std::string& key) {
    if (type_ != Type::Dictionary) {
        throw std::runtime_error("BencodeValue is not a dictionary");
    }
#if LIBRATS_USE_SHARED_PTR_VARIANT
    auto& dict = *std::get<std::shared_ptr<BencodeDict>>(value_);
#else
    auto& dict = std::get<BencodeDict>(value_);
#endif
    return dict[key];
}

const BencodeValue& BencodeValue::operator[](size_t index) const {
    if (type_ != Type::List) {
        throw std::runtime_error("BencodeValue is not a list");
    }
#if LIBRATS_USE_SHARED_PTR_VARIANT
    const auto& list = *std::get<std::shared_ptr<BencodeList>>(value_);
#else
    const auto& list = std::get<BencodeList>(value_);
#endif
    if (index >= list.size()) {
        throw std::runtime_error("Index out of bounds");
    }
    return list[index];
}

BencodeValue& BencodeValue::operator[](size_t index) {
    if (type_ != Type::List) {
        throw std::runtime_error("BencodeValue is not a list");
    }
#if LIBRATS_USE_SHARED_PTR_VARIANT
    auto& list = *std::get<std::shared_ptr<BencodeList>>(value_);
#else
    auto& list = std::get<BencodeList>(value_);
#endif
    if (index >= list.size()) {
        throw std::runtime_error("Index out of bounds");
    }
    return list[index];
}

void BencodeValue::push_back(const BencodeValue& value) {
    if (type_ != Type::List) {
        throw std::runtime_error("BencodeValue is not a list");
    }
#if LIBRATS_USE_SHARED_PTR_VARIANT
    auto& list = *std::get<std::shared_ptr<BencodeList>>(value_);
#else
    auto& list = std::get<BencodeList>(value_);
#endif
    list.push_back(value);
}

size_t BencodeValue::size() const {
    switch (type_) {
        case Type::String:
            return std::get<std::string>(value_).size();
        case Type::List:
#if LIBRATS_USE_SHARED_PTR_VARIANT
            return std::get<std::shared_ptr<BencodeList>>(value_)->size();
#else
            return std::get<BencodeList>(value_).size();
#endif
        case Type::Dictionary:
#if LIBRATS_USE_SHARED_PTR_VARIANT
            return std::get<std::shared_ptr<BencodeDict>>(value_)->size();
#else
            return std::get<BencodeDict>(value_).size();
#endif
        default:
            throw std::runtime_error("Size not applicable to this type");
    }
}

std::vector<uint8_t> BencodeValue::encode() const {
    std::vector<uint8_t> buffer;
    encode_to_buffer(buffer);
    return buffer;
}

std::string BencodeValue::encode_string() const {
    auto buffer = encode();
    return std::string(buffer.begin(), buffer.end());
}

void BencodeValue::encode_to_buffer(std::vector<uint8_t>& buffer) const {
    switch (type_) {
        case Type::Integer: {
            std::string str = "i" + std::to_string(std::get<int64_t>(value_)) + "e";
            buffer.insert(buffer.end(), str.begin(), str.end());
            break;
        }
        case Type::String: {
            const auto& str = std::get<std::string>(value_);
            std::string len_str = std::to_string(str.size()) + ":";
            buffer.insert(buffer.end(), len_str.begin(), len_str.end());
            buffer.insert(buffer.end(), str.begin(), str.end());
            break;
        }
        case Type::List: {
            buffer.push_back('l');
#if LIBRATS_USE_SHARED_PTR_VARIANT
            const auto& list = *std::get<std::shared_ptr<BencodeList>>(value_);
#else
            const auto& list = std::get<BencodeList>(value_);
#endif
            for (const auto& item : list) {
                item.encode_to_buffer(buffer);
            }
            buffer.push_back('e');
            break;
        }
        case Type::Dictionary: {
            buffer.push_back('d');
#if LIBRATS_USE_SHARED_PTR_VARIANT
            const auto& dict = *std::get<std::shared_ptr<BencodeDict>>(value_);
#else
            const auto& dict = std::get<BencodeDict>(value_);
#endif
            
            // Sort keys for consistent encoding
            std::vector<std::string> keys;
            for (const auto& pair : dict) {
                keys.push_back(pair.first);
            }
            std::sort(keys.begin(), keys.end());
            
            for (const auto& key : keys) {
                // Encode key
                std::string len_str = std::to_string(key.size()) + ":";
                buffer.insert(buffer.end(), len_str.begin(), len_str.end());
                buffer.insert(buffer.end(), key.begin(), key.end());
                
                // Encode value
                dict.at(key).encode_to_buffer(buffer);
            }
            buffer.push_back('e');
            break;
        }
    }
}

BencodeValue BencodeValue::create_integer(int64_t value) {
    return BencodeValue(value);
}

BencodeValue BencodeValue::create_string(const std::string& value) {
    return BencodeValue(value);
}

BencodeValue BencodeValue::create_list() {
    BencodeValue result;
    result.type_ = Type::List;
#if LIBRATS_USE_SHARED_PTR_VARIANT
    result.value_ = std::make_shared<BencodeList>();
#else
    result.value_ = BencodeList();
#endif
    return result;
}

BencodeValue BencodeValue::create_dict() {
    BencodeValue result;
    result.type_ = Type::Dictionary;
#if LIBRATS_USE_SHARED_PTR_VARIANT
    result.value_ = std::make_shared<BencodeDict>();
#else
    result.value_ = BencodeDict();
#endif
    return result;
}

// BencodeDecoder implementation
BencodeDecoder::BencodeDecoder(const uint8_t* data, size_t size) : data_(data), size_(size), pos_(0) {}

BencodeValue BencodeDecoder::decode(const std::vector<uint8_t>& data) {
    return decode(data.data(), data.size());
}

BencodeValue BencodeDecoder::decode(const std::string& data) {
    return decode(reinterpret_cast<const uint8_t*>(data.data()), data.size());
}

BencodeValue BencodeDecoder::decode(const uint8_t* data, size_t size) {
    BencodeDecoder decoder(data, size);
    return decoder.decode_value();
}

BencodeValue BencodeDecoder::decode_value() {
    if (!has_more()) {
        throw std::runtime_error("Unexpected end of data");
    }
    
    uint8_t first_byte = current_byte();
    
    if (first_byte == 'i') {
        return decode_integer();
    } else if (first_byte == 'l') {
        return decode_list();
    } else if (first_byte == 'd') {
        return decode_dict();
    } else if (first_byte >= '0' && first_byte <= '9') {
        return decode_string();
    } else {
        throw std::runtime_error("Invalid bencode data");
    }
}

BencodeValue BencodeDecoder::decode_integer() {
    if (consume_byte() != 'i') {
        throw std::runtime_error("Expected 'i' for integer");
    }
    
    std::string num_str;
    while (has_more() && current_byte() != 'e') {
        num_str += consume_byte();
    }
    
    if (!has_more() || consume_byte() != 'e') {
        throw std::runtime_error("Expected 'e' to end integer");
    }
    
    if (num_str.empty()) {
        throw std::runtime_error("Empty integer");
    }
    
    try {
        return BencodeValue(std::stoll(num_str));
    } catch (const std::exception&) {
        throw std::runtime_error("Invalid integer: " + num_str);
    }
}

BencodeValue BencodeDecoder::decode_string() {
    std::string len_str;
    while (has_more() && current_byte() != ':') {
        len_str += consume_byte();
    }
    
    if (!has_more() || consume_byte() != ':') {
        throw std::runtime_error("Expected ':' in string");
    }
    
    if (len_str.empty()) {
        throw std::runtime_error("Empty string length");
    }
    
    size_t length;
    try {
        length = std::stoull(len_str);
    } catch (const std::exception&) {
        throw std::runtime_error("Invalid string length: " + len_str);
    }
    
    if (pos_ + length > size_) {
        throw std::runtime_error("String length exceeds data size");
    }
    
    std::string str = consume_string(length);
    return BencodeValue(str);
}

BencodeValue BencodeDecoder::decode_list() {
    if (consume_byte() != 'l') {
        throw std::runtime_error("Expected 'l' for list");
    }
    
    BencodeValue list = BencodeValue::create_list();
    
    while (has_more() && current_byte() != 'e') {
        list.push_back(decode_value());
    }
    
    if (!has_more() || consume_byte() != 'e') {
        throw std::runtime_error("Expected 'e' to end list");
    }
    
    return list;
}

BencodeValue BencodeDecoder::decode_dict() {
    if (consume_byte() != 'd') {
        throw std::runtime_error("Expected 'd' for dictionary");
    }
    
    BencodeValue dict = BencodeValue::create_dict();
    
    while (has_more() && current_byte() != 'e') {
        // Decode key (must be string)
        BencodeValue key = decode_string();
        
        // Decode value
        BencodeValue value = decode_value();
        
        dict[key.as_string()] = value;
    }
    
    if (!has_more() || consume_byte() != 'e') {
        throw std::runtime_error("Expected 'e' to end dictionary");
    }
    
    return dict;
}

uint8_t BencodeDecoder::current_byte() const {
    if (pos_ >= size_) {
        throw std::runtime_error("Unexpected end of data");
    }
    return data_[pos_];
}

uint8_t BencodeDecoder::consume_byte() {
    if (pos_ >= size_) {
        throw std::runtime_error("Unexpected end of data");
    }
    return data_[pos_++];
}

std::string BencodeDecoder::consume_string(size_t length) {
    if (pos_ + length > size_) {
        throw std::runtime_error("String length exceeds data size");
    }
    std::string str(reinterpret_cast<const char*>(data_ + pos_), length);
    pos_ += length;
    return str;
}

// Utility functions
namespace bencode {
    BencodeValue decode(const std::vector<uint8_t>& data) {
        return BencodeDecoder::decode(data);
    }
    
    BencodeValue decode(const std::string& data) {
        return BencodeDecoder::decode(data);
    }
    
    std::vector<uint8_t> encode(const BencodeValue& value) {
        return value.encode();
    }
    
    std::string encode_string(const BencodeValue& value) {
        return value.encode_string();
    }
}

} // namespace librats 