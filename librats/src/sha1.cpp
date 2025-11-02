#include "sha1.h"
#include <iomanip>
#include <sstream>
#include <cstring>

namespace librats {

// SHA1 constants
static const uint32_t K[] = {
    0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6
};

// Left rotate function
static uint32_t left_rotate(uint32_t value, int amount) {
    return (value << amount) | (value >> (32 - amount));
}

SHA1::SHA1() {
    reset();
}

void SHA1::reset() {
    // SHA1 initialization constants
    h0 = 0x67452301;
    h1 = 0xEFCDAB89;
    h2 = 0x98BADCFE;
    h3 = 0x10325476;
    h4 = 0xC3D2E1F0;
    
    buffer_length = 0;
    total_length = 0;
    finalized = false;
}

void SHA1::update(uint8_t byte) {
    if (finalized) {
        return;
    }
    
    buffer[buffer_length++] = byte;
    total_length++;
    
    if (buffer_length == 64) {
        process_block();
        buffer_length = 0;
    }
}

void SHA1::update(const uint8_t* data, size_t length) {
    for (size_t i = 0; i < length; i++) {
        update(data[i]);
    }
}

void SHA1::update(const std::string& str) {
    update(reinterpret_cast<const uint8_t*>(str.c_str()), str.length());
}

void SHA1::process_block() {
    uint32_t w[80];
    
    // Break chunk into sixteen 32-bit big-endian words
    for (int i = 0; i < 16; i++) {
        w[i] = (buffer[i * 4] << 24) | 
               (buffer[i * 4 + 1] << 16) | 
               (buffer[i * 4 + 2] << 8) | 
               (buffer[i * 4 + 3]);
    }
    
    // Extend the sixteen 32-bit words into eighty 32-bit words
    for (int i = 16; i < 80; i++) {
        w[i] = left_rotate(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
    }
    
    // Initialize hash value for this chunk
    uint32_t a = h0;
    uint32_t b = h1;
    uint32_t c = h2;
    uint32_t d = h3;
    uint32_t e = h4;
    
    // Main loop
    for (int i = 0; i < 80; i++) {
        uint32_t f, k;
        
        if (i < 20) {
            f = (b & c) | (~b & d);
            k = K[0];
        } else if (i < 40) {
            f = b ^ c ^ d;
            k = K[1];
        } else if (i < 60) {
            f = (b & c) | (b & d) | (c & d);
            k = K[2];
        } else {
            f = b ^ c ^ d;
            k = K[3];
        }
        
        uint32_t temp = left_rotate(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = left_rotate(b, 30);
        b = a;
        a = temp;
    }
    
    // Add this chunk's hash to result so far
    h0 += a;
    h1 += b;
    h2 += c;
    h3 += d;
    h4 += e;
}

std::string SHA1::finalize() {
    if (finalized) {
        // Return empty string for subsequent calls
        return "";
    }
    
    // Pre-processing: adding padding bits
    uint64_t bit_length = total_length * 8;
    
    // Append the '1' bit
    update(0x80);
    
    // Append zeros until message length ≡ 448 (mod 512)
    while (buffer_length != 56) {
        update(0x00);
    }
    
    // Append length in bits as 64-bit big-endian integer
    for (int i = 7; i >= 0; i--) {
        update(static_cast<uint8_t>(bit_length >> (i * 8)));
    }
    
    // Produce the final hash value as a 160-bit number (hex string)
    std::ostringstream result;
    result << std::hex << std::setfill('0');
    result << std::setw(8) << h0;
    result << std::setw(8) << h1;
    result << std::setw(8) << h2;
    result << std::setw(8) << h3;
    result << std::setw(8) << h4;
    
    finalized = true;
    return result.str();
}

std::string SHA1::hash(const std::string& input) {
    SHA1 hasher;
    hasher.update(input);
    return hasher.finalize();
}

std::string SHA1::hash_bytes(const std::vector<uint8_t>& input) {
    SHA1 hasher;
    hasher.update(input.data(), input.size());
    return hasher.finalize();
}

} // namespace librats 