#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace librats {

class SHA1 {
public:
    SHA1();
    
    // Process a single byte
    void update(uint8_t byte);
    
    // Process a buffer
    void update(const uint8_t* data, size_t length);
    
    // Process a string
    void update(const std::string& str);
    
    // Get the final hash as a hex string
    std::string finalize();
    
    // Convenience function to hash a string directly
    static std::string hash(const std::string& input);
    
    // Convenience function to hash a vector of bytes directly
    static std::string hash_bytes(const std::vector<uint8_t>& input);

private:
    void process_block();
    void reset();
    
    uint32_t h0, h1, h2, h3, h4;
    uint8_t buffer[64];
    size_t buffer_length;
    uint64_t total_length;
    bool finalized;
};

} // namespace librats 