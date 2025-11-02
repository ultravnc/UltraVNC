#include "noise.h"
#include "logger.h"
#include <cstring>
#include <random>
#include <iomanip>
#include <sstream>
#include <stdexcept>

// Include platform-specific headers for cryptographic operations
#ifdef _WIN32
    #include <windows.h>
    #include <bcrypt.h>
    #pragma comment(lib, "bcrypt.lib")
#else
    #include <fcntl.h>
    #include <unistd.h>
#ifndef __IPHONE_OS_VERSION_MIN_REQUIRED
    #include <sys/random.h>
#endif
#endif

#define LOG_NOISE_DEBUG(message) LOG_DEBUG("noise", message)
#define LOG_NOISE_INFO(message)  LOG_INFO("noise", message)
#define LOG_NOISE_WARN(message)  LOG_WARN("noise", message)
#define LOG_NOISE_ERROR(message) LOG_ERROR("noise", message)

namespace librats {

// Noise Protocol constants
constexpr char NOISE_PROTOCOL_NAME[] = "Noise_XX_25519_ChaChaPoly_SHA256";

// Simple implementation of cryptographic primitives
// NOTE: This is a basic implementation for demonstration purposes.
// In production, you should use a well-tested cryptographic library like libsodium.

namespace {

// Simple ChaCha20 implementation
class ChaCha20 {
public:
    static void chacha20_block(uint32_t out[16], const uint32_t in[16]) {
        uint32_t x[16];
        for (int i = 0; i < 16; ++i) x[i] = in[i];

        for (int i = 0; i < 10; ++i) {
            // Column rounds
            quarter_round(x[0], x[4], x[8], x[12]);
            quarter_round(x[1], x[5], x[9], x[13]);
            quarter_round(x[2], x[6], x[10], x[14]);
            quarter_round(x[3], x[7], x[11], x[15]);
            // Diagonal rounds
            quarter_round(x[0], x[5], x[10], x[15]);
            quarter_round(x[1], x[6], x[11], x[12]);
            quarter_round(x[2], x[7], x[8], x[13]);
            quarter_round(x[3], x[4], x[9], x[14]);
        }

        for (int i = 0; i < 16; ++i) out[i] = x[i] + in[i];
    }

private:
    static uint32_t rotl(uint32_t x, int n) {
        return (x << n) | (x >> (32 - n));
    }

    static void quarter_round(uint32_t& a, uint32_t& b, uint32_t& c, uint32_t& d) {
        a += b; d ^= a; d = rotl(d, 16);
        c += d; b ^= c; b = rotl(b, 12);
        a += b; d ^= a; d = rotl(d, 8);
        c += d; b ^= c; b = rotl(b, 7);
    }
};

// Simple Poly1305 implementation
class Poly1305 {
public:
    static void poly1305_mac(uint8_t out[16], const uint8_t* m, size_t inlen, const uint8_t key[32]) {
        uint32_t r0, r1, r2, r3, r4;
        uint32_t s1, s2, s3, s4;
        uint32_t h0, h1, h2, h3, h4;
        uint64_t d0, d1, d2, d3, d4;
        uint32_t c;

        // r &= 0xffffffc0ffffffc0ffffffc0fffffff
        r0 = (get_u32le(key + 0)) & 0x3ffffff;
        r1 = (get_u32le(key + 3) >> 2) & 0x3ffff03;
        r2 = (get_u32le(key + 6) >> 4) & 0x3ffc0ff;
        r3 = (get_u32le(key + 9) >> 6) & 0x3f03fff;
        r4 = (get_u32le(key + 12) >> 8) & 0x00fffff;

        s1 = r1 * 5;
        s2 = r2 * 5;
        s3 = r3 * 5;
        s4 = r4 * 5;

        h0 = h1 = h2 = h3 = h4 = 0;

        while (inlen > 0) {
            // h += m[i]
            if (inlen >= 16) {
                h0 += (get_u32le(m + 0)) & 0x3ffffff;
                h1 += (get_u32le(m + 3) >> 2) & 0x3ffffff;
                h2 += (get_u32le(m + 6) >> 4) & 0x3ffffff;
                h3 += (get_u32le(m + 9) >> 6) & 0x3ffffff;
                h4 += (get_u32le(m + 12) >> 8) | (1 << 24);
                m += 16;
                inlen -= 16;
            } else {
                uint8_t mp[16];
                size_t i;
                for (i = 0; i < inlen; i++) mp[i] = m[i];
                mp[i++] = 1;
                for (; i < 16; i++) mp[i] = 0;
                inlen = 0;
                h0 += (get_u32le(mp + 0)) & 0x3ffffff;
                h1 += (get_u32le(mp + 3) >> 2) & 0x3ffffff;
                h2 += (get_u32le(mp + 6) >> 4) & 0x3ffffff;
                h3 += (get_u32le(mp + 9) >> 6) & 0x3ffffff;
                h4 += (get_u32le(mp + 12) >> 8);
            }

            // h *= r
            d0 = ((uint64_t)h0 * r0) + ((uint64_t)h1 * s4) + ((uint64_t)h2 * s3) + ((uint64_t)h3 * s2) + ((uint64_t)h4 * s1);
            d1 = ((uint64_t)h0 * r1) + ((uint64_t)h1 * r0) + ((uint64_t)h2 * s4) + ((uint64_t)h3 * s3) + ((uint64_t)h4 * s2);
            d2 = ((uint64_t)h0 * r2) + ((uint64_t)h1 * r1) + ((uint64_t)h2 * r0) + ((uint64_t)h3 * s4) + ((uint64_t)h4 * s3);
            d3 = ((uint64_t)h0 * r3) + ((uint64_t)h1 * r2) + ((uint64_t)h2 * r1) + ((uint64_t)h3 * r0) + ((uint64_t)h4 * s4);
            d4 = ((uint64_t)h0 * r4) + ((uint64_t)h1 * r3) + ((uint64_t)h2 * r2) + ((uint64_t)h3 * r1) + ((uint64_t)h4 * r0);

            // (partial) h %= p
            c = (uint32_t)(d0 >> 26); h0 = (uint32_t)d0 & 0x3ffffff;
            d1 += c; c = (uint32_t)(d1 >> 26); h1 = (uint32_t)d1 & 0x3ffffff;
            d2 += c; c = (uint32_t)(d2 >> 26); h2 = (uint32_t)d2 & 0x3ffffff;
            d3 += c; c = (uint32_t)(d3 >> 26); h3 = (uint32_t)d3 & 0x3ffffff;
            d4 += c; c = (uint32_t)(d4 >> 26); h4 = (uint32_t)d4 & 0x3ffffff;
            h0 += c * 5; c = h0 >> 26; h0 = h0 & 0x3ffffff;
            h1 += c;
        }

        // fully carry h
        c = h1 >> 26; h1 = h1 & 0x3ffffff;
        h2 += c; c = h2 >> 26; h2 = h2 & 0x3ffffff;
        h3 += c; c = h3 >> 26; h3 = h3 & 0x3ffffff;
        h4 += c; c = h4 >> 26; h4 = h4 & 0x3ffffff;
        h0 += c * 5; c = h0 >> 26; h0 = h0 & 0x3ffffff;
        h1 += c;

        // compute h + -p
        uint32_t g0 = h0 + 5; c = g0 >> 26; g0 &= 0x3ffffff;
        uint32_t g1 = h1 + c; c = g1 >> 26; g1 &= 0x3ffffff;
        uint32_t g2 = h2 + c; c = g2 >> 26; g2 &= 0x3ffffff;
        uint32_t g3 = h3 + c; c = g3 >> 26; g3 &= 0x3ffffff;
        uint32_t g4 = h4 + c - (1 << 26);

        // select h if h < p, or h + -p if h >= p
        uint32_t mask = (g4 >> ((sizeof(uint32_t) * 8) - 1)) - 1;
        g0 &= mask;
        g1 &= mask;
        g2 &= mask;
        g3 &= mask;
        g4 &= mask;
        mask = ~mask;
        h0 = (h0 & mask) | g0;
        h1 = (h1 & mask) | g1;
        h2 = (h2 & mask) | g2;
        h3 = (h3 & mask) | g3;
        h4 = (h4 & mask) | g4;

        // h = h % (2^128)
        h0 = ((h0) | (h1 << 26)) & 0xffffffff;
        h1 = ((h1 >> 6) | (h2 << 20)) & 0xffffffff;
        h2 = ((h2 >> 12) | (h3 << 14)) & 0xffffffff;
        h3 = ((h3 >> 18) | (h4 << 8)) & 0xffffffff;

        // mac = (h + s) % (2^128)
        d0 = (uint64_t)h0 + get_u32le(key + 16); h0 = (uint32_t)d0;
        d1 = (uint64_t)h1 + get_u32le(key + 20) + (d0 >> 32); h1 = (uint32_t)d1;
        d2 = (uint64_t)h2 + get_u32le(key + 24) + (d1 >> 32); h2 = (uint32_t)d2;
        d3 = (uint64_t)h3 + get_u32le(key + 28) + (d2 >> 32); h3 = (uint32_t)d3;

        put_u32le(out + 0, h0);
        put_u32le(out + 4, h1);
        put_u32le(out + 8, h2);
        put_u32le(out + 12, h3);
    }

private:
    static uint32_t get_u32le(const uint8_t* p) {
        return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
    }

    static void put_u32le(uint8_t* p, uint32_t v) {
        p[0] = (uint8_t)(v);
        p[1] = (uint8_t)(v >> 8);
        p[2] = (uint8_t)(v >> 16);
        p[3] = (uint8_t)(v >> 24);
    }
};

// Simple Curve25519 implementation
class Curve25519 {
public:
    static void scalarmult(uint8_t out[32], const uint8_t scalar[32], const uint8_t point[32]) {
        // This is a simplified implementation for demonstration
        // In production, use a proper curve25519 implementation
        std::memcpy(out, point, 32);
        for (int i = 0; i < 32; ++i) {
            out[i] ^= scalar[i];  // Simplified operation - NOT cryptographically secure
        }
    }

    static void scalarmult_base(uint8_t out[32], const uint8_t scalar[32]) {
        uint8_t basepoint[32] = {9};  // Standard base point
        scalarmult(out, scalar, basepoint);
    }
};

// Simple SHA256 implementation
class SHA256 {
public:
    static void hash(uint8_t out[32], const uint8_t* data, size_t len) {
        // This is a placeholder implementation
        // In production, use a proper SHA256 implementation
        std::memset(out, 0, 32);
        for (size_t i = 0; i < len && i < 32; ++i) {
            out[i] = data[i] ^ (uint8_t)(len & 0xFF);
        }
    }
};

} // anonymous namespace

//=============================================================================
// NoiseCrypto Implementation
//=============================================================================

NoiseKey NoiseCrypto::generate_keypair(NoiseKey& private_key) {
    random_bytes(private_key.data(), NOISE_KEY_SIZE);
    
    NoiseKey public_key;
    Curve25519::scalarmult_base(public_key.data(), private_key.data());
    
    return public_key;
}

NoiseKey NoiseCrypto::dh(const NoiseKey& private_key, const NoiseKey& public_key) {
    NoiseKey shared_secret;
    Curve25519::scalarmult(shared_secret.data(), private_key.data(), public_key.data());
    return shared_secret;
}

std::vector<uint8_t> NoiseCrypto::encrypt(const NoiseKey& key, uint64_t nonce, 
                                         const std::vector<uint8_t>& plaintext,
                                         const std::vector<uint8_t>& ad) {
    std::vector<uint8_t> ciphertext(plaintext.size() + NOISE_TAG_SIZE);
    
    // Setup ChaCha20 state
    uint32_t state[16];
    std::memcpy(state, "expand 32-byte k", 16);
    std::memcpy(state + 4, key.data(), 32);
    state[12] = 0;  // Counter
    state[13] = (uint32_t)nonce;
    state[14] = (uint32_t)(nonce >> 32);
    state[15] = 0;
    
    // Generate Poly1305 key (use counter 0)
    uint32_t poly_key[16];
    ChaCha20::chacha20_block(poly_key, state);
    
    // Generate keystream and encrypt (start from counter 1)
    state[12] = 1;
    for (size_t i = 0; i < plaintext.size(); i += 64) {
        uint32_t keystream[16];
        ChaCha20::chacha20_block(keystream, state);
        state[12]++;
        
        size_t chunk_size = (std::min)(size_t(64), plaintext.size() - i);
        for (size_t j = 0; j < chunk_size; ++j) {
            ciphertext[i + j] = plaintext[i + j] ^ ((uint8_t*)keystream)[j];
        }
    }
    
    // Calculate MAC
    std::vector<uint8_t> mac_data;
    mac_data.insert(mac_data.end(), ad.begin(), ad.end());
    mac_data.insert(mac_data.end(), ciphertext.begin(), ciphertext.begin() + plaintext.size());
    
    uint8_t mac[16];
    Poly1305::poly1305_mac(mac, mac_data.data(), mac_data.size(), (uint8_t*)poly_key);
    
    // Append MAC
    std::memcpy(ciphertext.data() + plaintext.size(), mac, 16);
    
    return ciphertext;
}

std::vector<uint8_t> NoiseCrypto::decrypt(const NoiseKey& key, uint64_t nonce,
                                         const std::vector<uint8_t>& ciphertext,
                                         const std::vector<uint8_t>& ad) {
    if (ciphertext.size() < NOISE_TAG_SIZE) {
        return {};
    }
    
    size_t plaintext_size = ciphertext.size() - NOISE_TAG_SIZE;
    
    // Setup ChaCha20 state
    uint32_t state[16];
    std::memcpy(state, "expand 32-byte k", 16);
    std::memcpy(state + 4, key.data(), 32);
    state[12] = 0;
    state[13] = (uint32_t)nonce;
    state[14] = (uint32_t)(nonce >> 32);
    state[15] = 0;
    
    // Generate Poly1305 key and verify MAC
    uint32_t poly_state[16];
    std::memcpy(poly_state, state, sizeof(state));
    poly_state[12] = 0;
    uint32_t poly_key[16];
    ChaCha20::chacha20_block(poly_key, poly_state);
    
    std::vector<uint8_t> mac_data;
    mac_data.insert(mac_data.end(), ad.begin(), ad.end());
    mac_data.insert(mac_data.end(), ciphertext.begin(), ciphertext.begin() + plaintext_size);
    
    uint8_t computed_mac[16];
    Poly1305::poly1305_mac(computed_mac, mac_data.data(), mac_data.size(), (uint8_t*)poly_key);
    
    // Verify MAC
    if (std::memcmp(computed_mac, ciphertext.data() + plaintext_size, 16) != 0) {
        return {};  // MAC verification failed
    }
    
    // Decrypt
    std::vector<uint8_t> plaintext(plaintext_size);
    state[12] = 1;  // Start from counter 1 (0 was used for Poly1305 key)
    
    for (size_t i = 0; i < plaintext_size; i += 64) {
        uint32_t keystream[16];
        ChaCha20::chacha20_block(keystream, state);
        state[12]++;
        
        size_t chunk_size = (std::min)(size_t(64), plaintext_size - i);
        for (size_t j = 0; j < chunk_size; ++j) {
            plaintext[i + j] = ciphertext[i + j] ^ ((uint8_t*)keystream)[j];
        }
    }
    
    return plaintext;
}

NoiseHash NoiseCrypto::hash(const std::vector<uint8_t>& data) {
    NoiseHash result;
    SHA256::hash(result.data(), data.data(), data.size());
    return result;
}

void NoiseCrypto::hkdf(const std::vector<uint8_t>& salt, const std::vector<uint8_t>& ikm,
                      const std::vector<uint8_t>& info, uint8_t* okm, size_t okm_len) {
    // Simplified HKDF implementation
    std::vector<uint8_t> prk_data = salt;
    prk_data.insert(prk_data.end(), ikm.begin(), ikm.end());
    NoiseHash prk = hash(prk_data);
    
    std::vector<uint8_t> okm_data = info;
    okm_data.insert(okm_data.end(), prk.begin(), prk.end());
    NoiseHash result = hash(okm_data);
    
    std::memcpy(okm, result.data(), (std::min)(okm_len, size_t(NOISE_HASH_SIZE)));
}

void NoiseCrypto::secure_memzero(void* ptr, size_t size) {
    volatile uint8_t* p = static_cast<volatile uint8_t*>(ptr);
    for (size_t i = 0; i < size; ++i) {
        p[i] = 0;
    }
}

void NoiseCrypto::random_bytes(uint8_t* buffer, size_t size) {
#ifdef _WIN32
    BCRYPT_ALG_HANDLE hAlg;
    if (BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_RNG_ALGORITHM, nullptr, 0) == 0) {
        BCryptGenRandom(hAlg, buffer, (ULONG)size, 0);
        BCryptCloseAlgorithmProvider(hAlg, 0);
    } else {
        // Fallback to less secure method
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        for (size_t i = 0; i < size; ++i) {
            buffer[i] = static_cast<uint8_t>(dis(gen));
        }
    }
#else
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd >= 0) {
        read(fd, buffer, size);
        close(fd);
    } else {
        // Fallback to less secure method
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        for (size_t i = 0; i < size; ++i) {
            buffer[i] = static_cast<uint8_t>(dis(gen));
        }
    }
#endif
}

//=============================================================================
// NoiseCipherState Implementation
//=============================================================================

NoiseCipherState::NoiseCipherState() : nonce_(0), has_key_(false) {
    key_.fill(0);
}

NoiseCipherState::~NoiseCipherState() {
    NoiseCrypto::secure_memzero(key_.data(), key_.size());
}

void NoiseCipherState::initialize_key(const NoiseKey& key) {
    key_ = key;
    nonce_ = 0;
    has_key_ = true;
}

std::vector<uint8_t> NoiseCipherState::encrypt_with_ad(const std::vector<uint8_t>& plaintext,
                                                      const std::vector<uint8_t>& ad) {
    if (!has_key_) {
        return plaintext;  // No encryption if no key
    }
    
    auto result = NoiseCrypto::encrypt(key_, nonce_, plaintext, ad);
    nonce_++;
    return result;
}

std::vector<uint8_t> NoiseCipherState::decrypt_with_ad(const std::vector<uint8_t>& ciphertext,
                                                      const std::vector<uint8_t>& ad) {
    if (!has_key_) {
        return ciphertext;  // No decryption if no key
    }
    
    auto result = NoiseCrypto::decrypt(key_, nonce_, ciphertext, ad);
    if (!result.empty()) {
        nonce_++;
    }
    return result;
}

//=============================================================================
// NoiseSymmetricState Implementation
//=============================================================================

NoiseSymmetricState::NoiseSymmetricState() {
    ck_.fill(0);
    h_.fill(0);
}

NoiseSymmetricState::~NoiseSymmetricState() {
    NoiseCrypto::secure_memzero(ck_.data(), ck_.size());
    NoiseCrypto::secure_memzero(h_.data(), h_.size());
}

void NoiseSymmetricState::initialize(const std::string& protocol_name) {
    if (protocol_name.length() <= NOISE_HASH_SIZE) {
        std::memcpy(h_.data(), protocol_name.c_str(), protocol_name.length());
        std::memset(h_.data() + protocol_name.length(), 0, NOISE_HASH_SIZE - protocol_name.length());
    } else {
        h_ = NoiseCrypto::hash(std::vector<uint8_t>(protocol_name.begin(), protocol_name.end()));
    }
    ck_ = h_;
}

void NoiseSymmetricState::mix_key(const std::vector<uint8_t>& input_key_material) {
    std::vector<uint8_t> temp_k(NOISE_KEY_SIZE);
    std::vector<uint8_t> salt(ck_.begin(), ck_.end());
    
    NoiseCrypto::hkdf(salt, input_key_material, {}, temp_k.data(), NOISE_KEY_SIZE);
    NoiseCrypto::hkdf(salt, input_key_material, {1}, ck_.data(), NOISE_KEY_SIZE);
    
    NoiseKey key;
    std::memcpy(key.data(), temp_k.data(), NOISE_KEY_SIZE);
    cipher_state_.initialize_key(key);
    
    NoiseCrypto::secure_memzero(temp_k.data(), temp_k.size());
}

void NoiseSymmetricState::mix_hash(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> hash_input(h_.begin(), h_.end());
    hash_input.insert(hash_input.end(), data.begin(), data.end());
    h_ = NoiseCrypto::hash(hash_input);
}

void NoiseSymmetricState::mix_key_and_hash(const std::vector<uint8_t>& input_key_material) {
    std::vector<uint8_t> temp_h(NOISE_HASH_SIZE);
    std::vector<uint8_t> temp_k(NOISE_KEY_SIZE);
    std::vector<uint8_t> salt(ck_.begin(), ck_.end());
    
    NoiseCrypto::hkdf(salt, input_key_material, {}, temp_h.data(), NOISE_HASH_SIZE);
    NoiseCrypto::hkdf(salt, input_key_material, {1}, temp_k.data(), NOISE_KEY_SIZE);
    NoiseCrypto::hkdf(salt, input_key_material, {2}, ck_.data(), NOISE_KEY_SIZE);
    
    std::memcpy(h_.data(), temp_h.data(), NOISE_HASH_SIZE);
    
    NoiseKey key;
    std::memcpy(key.data(), temp_k.data(), NOISE_KEY_SIZE);
    cipher_state_.initialize_key(key);
    
    NoiseCrypto::secure_memzero(temp_h.data(), temp_h.size());
    NoiseCrypto::secure_memzero(temp_k.data(), temp_k.size());
}

std::vector<uint8_t> NoiseSymmetricState::encrypt_and_hash(const std::vector<uint8_t>& plaintext) {
    auto ciphertext = cipher_state_.encrypt_with_ad(plaintext, std::vector<uint8_t>(h_.begin(), h_.end()));
    mix_hash(ciphertext);
    return ciphertext;
}

std::vector<uint8_t> NoiseSymmetricState::decrypt_and_hash(const std::vector<uint8_t>& ciphertext) {
    auto plaintext = cipher_state_.decrypt_with_ad(ciphertext, std::vector<uint8_t>(h_.begin(), h_.end()));
    if (!plaintext.empty()) {
        mix_hash(ciphertext);
    }
    return plaintext;
}

std::pair<NoiseCipherState, NoiseCipherState> NoiseSymmetricState::split() {
    std::vector<uint8_t> temp_k1(NOISE_KEY_SIZE);
    std::vector<uint8_t> temp_k2(NOISE_KEY_SIZE);
    std::vector<uint8_t> salt(ck_.begin(), ck_.end());
    
    NoiseCrypto::hkdf(salt, {}, {}, temp_k1.data(), NOISE_KEY_SIZE);
    NoiseCrypto::hkdf(salt, {}, {1}, temp_k2.data(), NOISE_KEY_SIZE);
    
    NoiseCipherState c1, c2;
    NoiseKey key1, key2;
    std::memcpy(key1.data(), temp_k1.data(), NOISE_KEY_SIZE);
    std::memcpy(key2.data(), temp_k2.data(), NOISE_KEY_SIZE);
    
    c1.initialize_key(key1);
    c2.initialize_key(key2);
    
    NoiseCrypto::secure_memzero(temp_k1.data(), temp_k1.size());
    NoiseCrypto::secure_memzero(temp_k2.data(), temp_k2.size());
    
    return std::make_pair(std::move(c1), std::move(c2));
}

//=============================================================================
// NoiseHandshake Implementation
//=============================================================================

NoiseHandshake::NoiseHandshake() : role_(NoiseRole::INITIATOR), state_(NoiseHandshakeState::UNINITIALIZED) {
    s_.fill(0);
    e_.fill(0);
    rs_.fill(0);
    re_.fill(0);
}

NoiseHandshake::~NoiseHandshake() {
    NoiseCrypto::secure_memzero(s_.data(), s_.size());
    NoiseCrypto::secure_memzero(e_.data(), e_.size());
}

bool NoiseHandshake::initialize(NoiseRole role, const NoiseKey& static_private_key) {
    role_ = role;
    s_ = static_private_key;
    
    symmetric_state_.initialize(NOISE_PROTOCOL_NAME);
    
    if (role == NoiseRole::INITIATOR) {
        state_ = NoiseHandshakeState::WRITE_MESSAGE_1;
    } else {
        state_ = NoiseHandshakeState::READ_MESSAGE_1;
    }
    
    LOG_NOISE_INFO("Initialized Noise handshake as " << (role == NoiseRole::INITIATOR ? "initiator" : "responder"));
    return true;
}

std::vector<uint8_t> NoiseHandshake::write_message(const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> message;
    
    try {
        switch (state_) {
            case NoiseHandshakeState::WRITE_MESSAGE_1: {
                // -> e
                NoiseKey e_public = NoiseCrypto::generate_keypair(e_);
                message.insert(message.end(), e_public.begin(), e_public.end());
                symmetric_state_.mix_hash(std::vector<uint8_t>(e_public.begin(), e_public.end()));
                break;
            }
            
            case NoiseHandshakeState::WRITE_MESSAGE_2: {
                // <- e, ee, s, es
                NoiseKey e_public = NoiseCrypto::generate_keypair(e_);
                message.insert(message.end(), e_public.begin(), e_public.end());
                symmetric_state_.mix_hash(std::vector<uint8_t>(e_public.begin(), e_public.end()));
                
                // ee
                NoiseKey ee = NoiseCrypto::dh(e_, re_);
                symmetric_state_.mix_key(std::vector<uint8_t>(ee.begin(), ee.end()));
                
                // s
                NoiseKey s_public = NoiseCrypto::generate_keypair(s_);  // Get public from private
                auto encrypted_s = symmetric_state_.encrypt_and_hash(std::vector<uint8_t>(s_public.begin(), s_public.end()));
                message.insert(message.end(), encrypted_s.begin(), encrypted_s.end());
                
                // es
                NoiseKey es = NoiseCrypto::dh(s_, re_);
                symmetric_state_.mix_key(std::vector<uint8_t>(es.begin(), es.end()));
                break;
            }
            
            case NoiseHandshakeState::WRITE_MESSAGE_3: {
                // -> s, se
                NoiseKey s_public = NoiseCrypto::generate_keypair(s_);  // Get public from private
                auto encrypted_s = symmetric_state_.encrypt_and_hash(std::vector<uint8_t>(s_public.begin(), s_public.end()));
                message.insert(message.end(), encrypted_s.begin(), encrypted_s.end());
                
                // se
                NoiseKey se = NoiseCrypto::dh(s_, re_);
                symmetric_state_.mix_key(std::vector<uint8_t>(se.begin(), se.end()));
                break;
            }
            
            default:
                LOG_NOISE_ERROR("Invalid state for write_message: " << static_cast<int>(state_));
                fail_handshake();
                return {};
        }
        
        // Encrypt payload
        if (!payload.empty()) {
            auto encrypted_payload = symmetric_state_.encrypt_and_hash(payload);
            message.insert(message.end(), encrypted_payload.begin(), encrypted_payload.end());
        }
        
        advance_state();
        LOG_NOISE_DEBUG("Wrote handshake message, new state: " << static_cast<int>(state_));
        
    } catch (const std::exception& e) {
        LOG_NOISE_ERROR("Exception in write_message: " << e.what());
        fail_handshake();
        return {};
    }
    
    return message;
}

std::vector<uint8_t> NoiseHandshake::read_message(const std::vector<uint8_t>& message) {
    std::vector<uint8_t> payload;
    size_t offset = 0;
    
    try {
        switch (state_) {
            case NoiseHandshakeState::READ_MESSAGE_1: {
                // -> e
                if (message.size() < NOISE_KEY_SIZE) {
                    LOG_NOISE_ERROR("Message too short for e");
                    fail_handshake();
                    return {};
                }
                
                std::memcpy(re_.data(), message.data() + offset, NOISE_KEY_SIZE);
                offset += NOISE_KEY_SIZE;
                symmetric_state_.mix_hash(std::vector<uint8_t>(re_.begin(), re_.end()));
                break;
            }
            
            case NoiseHandshakeState::READ_MESSAGE_2: {
                // <- e, ee, s, es
                if (message.size() < NOISE_KEY_SIZE) {
                    LOG_NOISE_ERROR("Message too short for e");
                    fail_handshake();
                    return {};
                }
                
                std::memcpy(re_.data(), message.data() + offset, NOISE_KEY_SIZE);
                offset += NOISE_KEY_SIZE;
                symmetric_state_.mix_hash(std::vector<uint8_t>(re_.begin(), re_.end()));
                
                // ee
                NoiseKey ee = NoiseCrypto::dh(e_, re_);
                symmetric_state_.mix_key(std::vector<uint8_t>(ee.begin(), ee.end()));
                
                // s
                size_t s_encrypted_size = NOISE_KEY_SIZE + NOISE_TAG_SIZE;
                if (message.size() < offset + s_encrypted_size) {
                    LOG_NOISE_ERROR("Message too short for encrypted s");
                    fail_handshake();
                    return {};
                }
                
                std::vector<uint8_t> encrypted_s(message.begin() + offset, message.begin() + offset + s_encrypted_size);
                offset += s_encrypted_size;
                
                auto decrypted_s = symmetric_state_.decrypt_and_hash(encrypted_s);
                if (decrypted_s.size() != NOISE_KEY_SIZE) {
                    LOG_NOISE_ERROR("Failed to decrypt s");
                    fail_handshake();
                    return {};
                }
                std::memcpy(rs_.data(), decrypted_s.data(), NOISE_KEY_SIZE);
                
                // es
                NoiseKey es = NoiseCrypto::dh(e_, rs_);
                symmetric_state_.mix_key(std::vector<uint8_t>(es.begin(), es.end()));
                break;
            }
            
            case NoiseHandshakeState::READ_MESSAGE_3: {
                // -> s, se
                size_t s_encrypted_size = NOISE_KEY_SIZE + NOISE_TAG_SIZE;
                if (message.size() < s_encrypted_size) {
                    LOG_NOISE_ERROR("Message too short for encrypted s");
                    fail_handshake();
                    return {};
                }
                
                std::vector<uint8_t> encrypted_s(message.begin() + offset, message.begin() + offset + s_encrypted_size);
                offset += s_encrypted_size;
                
                auto decrypted_s = symmetric_state_.decrypt_and_hash(encrypted_s);
                if (decrypted_s.size() != NOISE_KEY_SIZE) {
                    LOG_NOISE_ERROR("Failed to decrypt s");
                    fail_handshake();
                    return {};
                }
                std::memcpy(rs_.data(), decrypted_s.data(), NOISE_KEY_SIZE);
                
                // se
                NoiseKey se = NoiseCrypto::dh(e_, rs_);
                symmetric_state_.mix_key(std::vector<uint8_t>(se.begin(), se.end()));
                break;
            }
            
            default:
                LOG_NOISE_ERROR("Invalid state for read_message: " << static_cast<int>(state_));
                fail_handshake();
                return {};
        }
        
        // Decrypt payload
        if (offset < message.size()) {
            std::vector<uint8_t> encrypted_payload(message.begin() + offset, message.end());
            payload = symmetric_state_.decrypt_and_hash(encrypted_payload);
            if (payload.empty() && !encrypted_payload.empty()) {
                LOG_NOISE_ERROR("Failed to decrypt payload");
                fail_handshake();
                return {};
            }
        }
        
        advance_state();
        LOG_NOISE_DEBUG("Read handshake message, new state: " << static_cast<int>(state_));
        
    } catch (const std::exception& e) {
        LOG_NOISE_ERROR("Exception in read_message: " << e.what());
        fail_handshake();
        return {};
    }
    
    return payload;
}

std::pair<NoiseCipherState, NoiseCipherState> NoiseHandshake::get_cipher_states() {
    if (state_ != NoiseHandshakeState::COMPLETED) {
        return std::make_pair(NoiseCipherState(), NoiseCipherState());
    }
    
    return symmetric_state_.split();
}

void NoiseHandshake::advance_state() {
    switch (state_) {
        case NoiseHandshakeState::WRITE_MESSAGE_1:
            state_ = NoiseHandshakeState::READ_MESSAGE_2;
            break;
        case NoiseHandshakeState::READ_MESSAGE_1:
            state_ = NoiseHandshakeState::WRITE_MESSAGE_2;
            break;
        case NoiseHandshakeState::WRITE_MESSAGE_2:
            state_ = NoiseHandshakeState::READ_MESSAGE_3;
            break;
        case NoiseHandshakeState::READ_MESSAGE_2:
            state_ = NoiseHandshakeState::WRITE_MESSAGE_3;
            break;
        case NoiseHandshakeState::WRITE_MESSAGE_3:
        case NoiseHandshakeState::READ_MESSAGE_3:
            state_ = NoiseHandshakeState::COMPLETED;
            LOG_NOISE_INFO("Noise handshake completed successfully");
            break;
        default:
            break;
    }
}

void NoiseHandshake::fail_handshake() {
    state_ = NoiseHandshakeState::FAILED;
    LOG_NOISE_ERROR("Noise handshake failed");
}

//=============================================================================
// NoiseSession Implementation
//=============================================================================

NoiseSession::NoiseSession() : handshake_completed_(false) {
    handshake_state_ = std::make_unique<NoiseHandshake>();
}

NoiseSession::~NoiseSession() = default;

bool NoiseSession::initialize_as_initiator(const NoiseKey& static_private_key) {
    return handshake_state_->initialize(NoiseRole::INITIATOR, static_private_key);
}

bool NoiseSession::initialize_as_responder(const NoiseKey& static_private_key) {
    return handshake_state_->initialize(NoiseRole::RESPONDER, static_private_key);
}

bool NoiseSession::is_handshake_completed() const {
    return handshake_completed_;
}

bool NoiseSession::has_handshake_failed() const {
    return handshake_state_->has_failed();
}

std::vector<uint8_t> NoiseSession::create_handshake_message(const std::vector<uint8_t>& payload) {
    if (handshake_completed_) {
        LOG_NOISE_WARN("Handshake already completed");
        return {};
    }
    
    auto message = handshake_state_->write_message(payload);
    
    if (handshake_state_->is_completed()) {
        auto cipher_states = handshake_state_->get_cipher_states();
        
        // According to Noise protocol spec: initiator gets (send=first, receive=second)
        // responder gets (send=second, receive=first)
        if (handshake_state_->get_role() == NoiseRole::INITIATOR) {
            send_cipher_ = std::make_unique<NoiseCipherState>(std::move(cipher_states.first));
            receive_cipher_ = std::make_unique<NoiseCipherState>(std::move(cipher_states.second));
        } else {
            send_cipher_ = std::make_unique<NoiseCipherState>(std::move(cipher_states.second));
            receive_cipher_ = std::make_unique<NoiseCipherState>(std::move(cipher_states.first));
        }
        
        handshake_completed_ = true;
        LOG_NOISE_INFO("Handshake completed, transport encryption enabled");
    }
    
    return message;
}

std::vector<uint8_t> NoiseSession::process_handshake_message(const std::vector<uint8_t>& message) {
    if (handshake_completed_) {
        LOG_NOISE_WARN("Handshake already completed");
        return {};
    }
    
    auto payload = handshake_state_->read_message(message);
    
    if (handshake_state_->is_completed()) {
        auto cipher_states = handshake_state_->get_cipher_states();
        
        // According to Noise protocol spec: initiator gets (send=first, receive=second)
        // responder gets (send=second, receive=first)
        if (handshake_state_->get_role() == NoiseRole::INITIATOR) {
            send_cipher_ = std::make_unique<NoiseCipherState>(std::move(cipher_states.first));
            receive_cipher_ = std::make_unique<NoiseCipherState>(std::move(cipher_states.second));
        } else {
            send_cipher_ = std::make_unique<NoiseCipherState>(std::move(cipher_states.second));
            receive_cipher_ = std::make_unique<NoiseCipherState>(std::move(cipher_states.first));
        }
        
        handshake_completed_ = true;
        LOG_NOISE_INFO("Handshake completed, transport encryption enabled");
    }
    
    return payload;
}

std::vector<uint8_t> NoiseSession::encrypt_transport_message(const std::vector<uint8_t>& plaintext) {
    if (!handshake_completed_ || !send_cipher_) {
        LOG_NOISE_ERROR("Cannot encrypt: handshake not completed");
        return {};
    }
    
    return send_cipher_->encrypt_with_ad(plaintext);
}

std::vector<uint8_t> NoiseSession::decrypt_transport_message(const std::vector<uint8_t>& ciphertext) {
    if (!handshake_completed_ || !receive_cipher_) {
        LOG_NOISE_ERROR("Cannot decrypt: handshake not completed");
        return {};
    }
    
    return receive_cipher_->decrypt_with_ad(ciphertext);
}

NoiseRole NoiseSession::get_role() const {
    return handshake_state_->get_role();
}

NoiseHandshakeState NoiseSession::get_handshake_state() const {
    return handshake_state_->get_state();
}

const NoiseKey& NoiseSession::get_remote_static_public_key() const {
    return handshake_state_->get_remote_static_public_key();
}

//=============================================================================
// Utility Functions Implementation
//=============================================================================

namespace noise_utils {

NoiseKey generate_static_keypair() {
    NoiseKey private_key;
    NoiseCrypto::generate_keypair(private_key);
    return private_key;
}

std::string key_to_hex(const NoiseKey& key) {
    std::ostringstream hex_stream;
    for (uint8_t byte : key) {
        hex_stream << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(byte);
    }
    return hex_stream.str();
}

NoiseKey hex_to_key(const std::string& hex) {
    NoiseKey key;
    key.fill(0);
    
    if (hex.length() != NOISE_KEY_SIZE * 2) {
        return key;
    }
    
    for (size_t i = 0; i < NOISE_KEY_SIZE; ++i) {
        std::string byte_str = hex.substr(i * 2, 2);
        try {
            key[i] = static_cast<uint8_t>(std::stoul(byte_str, nullptr, 16));
        } catch (const std::exception&) {
            key.fill(0);
            return key;
        }
    }
    
    return key;
}

std::string get_protocol_name() {
    return NOISE_PROTOCOL_NAME;
}

bool validate_message_size(size_t size) {
    return size <= NOISE_MAX_MESSAGE_SIZE;
}

std::string noise_error_to_string(NoiseError error) {
    switch (error) {
        case NoiseError::SUCCESS: return "Success";
        case NoiseError::INVALID_STATE: return "Invalid state";
        case NoiseError::HANDSHAKE_FAILED: return "Handshake failed";
        case NoiseError::DECRYPTION_FAILED: return "Decryption failed";
        case NoiseError::INVALID_MESSAGE_SIZE: return "Invalid message size";
        case NoiseError::CRYPTO_ERROR: return "Cryptographic error";
        default: return "Unknown error";
    }
}

} // namespace noise_utils

} // namespace librats 
