#pragma once

#include <array>
#include <vector>
#include <string>
#include <memory>
#include <cstdint>

namespace librats {

// Noise Protocol constants
constexpr size_t NOISE_KEY_SIZE = 32;      // 32 bytes for Curve25519 keys
constexpr size_t NOISE_HASH_SIZE = 32;     // 32 bytes for SHA256
constexpr size_t NOISE_TAG_SIZE = 16;      // 16 bytes for ChaCha20-Poly1305 tag
constexpr size_t NOISE_MAX_MESSAGE_SIZE = 65535; // Maximum noise message size

// Noise key types
using NoiseKey = std::array<uint8_t, NOISE_KEY_SIZE>;
using NoiseHash = std::array<uint8_t, NOISE_HASH_SIZE>;

/**
 * Noise Protocol handshake state enumeration
 */
enum class NoiseHandshakeState {
    UNINITIALIZED,
    WRITE_MESSAGE_1,    // Initiator sends first message
    READ_MESSAGE_1,     // Responder receives first message
    WRITE_MESSAGE_2,    // Responder sends second message
    READ_MESSAGE_2,     // Initiator receives second message
    WRITE_MESSAGE_3,    // Initiator sends third message
    READ_MESSAGE_3,     // Responder receives third message
    COMPLETED,          // Handshake completed successfully
    FAILED              // Handshake failed
};

/**
 * Noise Protocol role enumeration
 */
enum class NoiseRole {
    INITIATOR,          // The peer that initiates the handshake
    RESPONDER           // The peer that responds to the handshake
};

/**
 * Cryptographic functions interface for Noise Protocol
 */
class NoiseCrypto {
public:
    // ECDH operations using Curve25519
    static NoiseKey generate_keypair(NoiseKey& private_key);
    static NoiseKey dh(const NoiseKey& private_key, const NoiseKey& public_key);
    
    // AEAD encryption/decryption using ChaCha20-Poly1305
    static std::vector<uint8_t> encrypt(const NoiseKey& key, uint64_t nonce, 
                                       const std::vector<uint8_t>& plaintext,
                                       const std::vector<uint8_t>& ad = {});
    static std::vector<uint8_t> decrypt(const NoiseKey& key, uint64_t nonce,
                                       const std::vector<uint8_t>& ciphertext,
                                       const std::vector<uint8_t>& ad = {});
    
    // Hash functions using SHA256
    static NoiseHash hash(const std::vector<uint8_t>& data);
    static void hkdf(const std::vector<uint8_t>& salt, const std::vector<uint8_t>& ikm,
                     const std::vector<uint8_t>& info, uint8_t* okm, size_t okm_len);
    
    // Utility functions
    static void secure_memzero(void* ptr, size_t size);
    static void random_bytes(uint8_t* buffer, size_t size);
};

/**
 * Noise Protocol cipher state for managing encryption/decryption
 */
class NoiseCipherState {
public:
    NoiseCipherState();
    ~NoiseCipherState();
    
    void initialize_key(const NoiseKey& key);
    bool has_key() const { return has_key_; }
    
    std::vector<uint8_t> encrypt_with_ad(const std::vector<uint8_t>& plaintext,
                                        const std::vector<uint8_t>& ad = {});
    std::vector<uint8_t> decrypt_with_ad(const std::vector<uint8_t>& ciphertext,
                                        const std::vector<uint8_t>& ad = {});
    
    void set_nonce(uint64_t nonce) { nonce_ = nonce; }
    uint64_t get_nonce() const { return nonce_; }
    
private:
    NoiseKey key_;
    uint64_t nonce_;
    bool has_key_;
};

/**
 * Noise Protocol symmetric state for managing handshake state
 */
class NoiseSymmetricState {
public:
    NoiseSymmetricState();
    ~NoiseSymmetricState();
    
    void initialize(const std::string& protocol_name);
    void mix_key(const std::vector<uint8_t>& input_key_material);
    void mix_hash(const std::vector<uint8_t>& data);
    void mix_key_and_hash(const std::vector<uint8_t>& input_key_material);
    
    std::vector<uint8_t> encrypt_and_hash(const std::vector<uint8_t>& plaintext);
    std::vector<uint8_t> decrypt_and_hash(const std::vector<uint8_t>& ciphertext);
    
    std::pair<NoiseCipherState, NoiseCipherState> split();
    
    const NoiseHash& get_handshake_hash() const { return h_; }
    
private:
    NoiseCipherState cipher_state_;
    NoiseKey ck_;  // Chaining key
    NoiseHash h_;  // Handshake hash
};

/**
 * Noise Protocol handshake state for Noise_XX pattern
 */
class NoiseHandshake {
public:
    NoiseHandshake();
    ~NoiseHandshake();
    
    // Initialize for either initiator or responder role
    bool initialize(NoiseRole role, const NoiseKey& static_private_key);
    
    // Handshake message processing
    std::vector<uint8_t> write_message(const std::vector<uint8_t>& payload = {});
    std::vector<uint8_t> read_message(const std::vector<uint8_t>& message);
    
    // State queries
    NoiseHandshakeState get_state() const { return state_; }
    NoiseRole get_role() const { return role_; }
    bool is_completed() const { return state_ == NoiseHandshakeState::COMPLETED; }
    bool has_failed() const { return state_ == NoiseHandshakeState::FAILED; }
    
    // Get cipher states after handshake completion
    std::pair<NoiseCipherState, NoiseCipherState> get_cipher_states();
    
    // Get remote static public key (available after handshake)
    const NoiseKey& get_remote_static_public_key() const { return rs_; }
    
private:
    NoiseRole role_;
    NoiseHandshakeState state_;
    NoiseSymmetricState symmetric_state_;
    
    // Local keys
    NoiseKey s_;   // Local static private key
    NoiseKey e_;   // Local ephemeral private key
    
    // Remote keys
    NoiseKey rs_;  // Remote static public key
    NoiseKey re_;  // Remote ephemeral public key
    
    void advance_state();
    void fail_handshake();
};

/**
 * High-level Noise Protocol session manager
 */
class NoiseSession {
public:
    NoiseSession();
    ~NoiseSession();
    
    // Initialize session for client (initiator) or server (responder)
    bool initialize_as_initiator(const NoiseKey& static_private_key);
    bool initialize_as_responder(const NoiseKey& static_private_key);
    
    // Handshake operations
    bool is_handshake_completed() const;
    bool has_handshake_failed() const;
    
    std::vector<uint8_t> create_handshake_message(const std::vector<uint8_t>& payload = {});
    std::vector<uint8_t> process_handshake_message(const std::vector<uint8_t>& message);
    
    // Transport operations (available after handshake completion)
    std::vector<uint8_t> encrypt_transport_message(const std::vector<uint8_t>& plaintext);
    std::vector<uint8_t> decrypt_transport_message(const std::vector<uint8_t>& ciphertext);
    
    // Utility functions
    NoiseRole get_role() const;
    NoiseHandshakeState get_handshake_state() const;
    const NoiseKey& get_remote_static_public_key() const;
    
private:
    std::unique_ptr<NoiseHandshake> handshake_state_;
    std::unique_ptr<NoiseCipherState> send_cipher_;
    std::unique_ptr<NoiseCipherState> receive_cipher_;
    bool handshake_completed_;
};

/**
 * Utility functions for Noise Protocol
 */
namespace noise_utils {
    // Key management
    NoiseKey generate_static_keypair();
    std::string key_to_hex(const NoiseKey& key);
    NoiseKey hex_to_key(const std::string& hex);
    
    // Protocol utilities
    std::string get_protocol_name();
    bool validate_message_size(size_t size);
    
    // Error handling
    enum class NoiseError {
        SUCCESS,
        INVALID_STATE,
        HANDSHAKE_FAILED,
        DECRYPTION_FAILED,
        INVALID_MESSAGE_SIZE,
        CRYPTO_ERROR
    };
    
    std::string noise_error_to_string(NoiseError error);
}

} // namespace librats 