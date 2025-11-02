#include "encrypted_socket.h"
#include "logger.h"
#include <algorithm>
#include <cstring>
#include <thread>
#include <chrono>

#define LOG_ENCRYPT_DEBUG(message) LOG_DEBUG("encrypt", message)
#define LOG_ENCRYPT_INFO(message)  LOG_INFO("encrypt", message)
#define LOG_ENCRYPT_WARN(message)  LOG_WARN("encrypt", message)
#define LOG_ENCRYPT_ERROR(message) LOG_ERROR("encrypt", message)

namespace librats {

// Message framing constants
constexpr uint32_t NOISE_MESSAGE_MAGIC = 0x4E4F4953; // "NOIS" in little endian
constexpr uint32_t HANDSHAKE_MESSAGE_MAGIC = 0x48534B48; // "HSKH" in little endian
constexpr size_t MESSAGE_HEADER_SIZE = 8; // 4 bytes magic + 4 bytes length

//=============================================================================
// EncryptedSocket Implementation
//=============================================================================

EncryptedSocket::EncryptedSocket() = default;

EncryptedSocket::~EncryptedSocket() {
    clear_all_sockets();
}

bool EncryptedSocket::initialize_as_initiator(socket_t socket, const NoiseKey& static_private_key) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    auto session = std::make_unique<SocketSession>(socket);
    if (!session->session->initialize_as_initiator(static_private_key)) {
        LOG_ENCRYPT_ERROR("Failed to initialize noise session as initiator for socket " << socket);
        return false;
    }
    
    session->is_encrypted = true;
    sessions_[socket] = std::move(session);
    
    LOG_ENCRYPT_INFO("Initialized encrypted socket " << socket << " as initiator");
    return true;
}

bool EncryptedSocket::initialize_as_responder(socket_t socket, const NoiseKey& static_private_key) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    auto session = std::make_unique<SocketSession>(socket);
    if (!session->session->initialize_as_responder(static_private_key)) {
        LOG_ENCRYPT_ERROR("Failed to initialize noise session as responder for socket " << socket);
        return false;
    }
    
    session->is_encrypted = true;
    sessions_[socket] = std::move(session);
    
    LOG_ENCRYPT_INFO("Initialized encrypted socket " << socket << " as responder");
    return true;
}

bool EncryptedSocket::is_encrypted(socket_t socket) const {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    const auto* session = get_session(socket);
    return session && session->is_encrypted;
}

bool EncryptedSocket::is_handshake_completed(socket_t socket) const {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    const auto* session = get_session(socket);
    return session && session->session->is_handshake_completed();
}

bool EncryptedSocket::has_handshake_failed(socket_t socket) const {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    const auto* session = get_session(socket);
    return session && session->session->has_handshake_failed();
}

bool EncryptedSocket::send_handshake_message(socket_t socket, const std::vector<uint8_t>& payload) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    auto* session = get_session(socket);
    if (!session) {
        LOG_ENCRYPT_ERROR("No session found for socket " << socket);
        return false;
    }
    
    try {
        auto handshake_data = session->session->create_handshake_message(payload);
        if (handshake_data.empty() && !session->session->is_handshake_completed()) {
            LOG_ENCRYPT_ERROR("Failed to create handshake message for socket " << socket);
            return false;
        }
        
        if (!handshake_data.empty()) {
            auto framed_message = frame_message(handshake_data);
            
            LOG_ENCRYPT_DEBUG("Created framed message with " << framed_message.size() << " bytes, noise data was " << handshake_data.size() << " bytes");
            LOG_ENCRYPT_DEBUG("First 12 bytes of framed message: " << std::hex
                             << "0x" << std::setfill('0') << std::setw(2) << (unsigned)framed_message[0]
                             << std::setfill('0') << std::setw(2) << (unsigned)framed_message[1]
                             << std::setfill('0') << std::setw(2) << (unsigned)framed_message[2]
                             << std::setfill('0') << std::setw(2) << (unsigned)framed_message[3] << " "
                             << "0x" << std::setfill('0') << std::setw(2) << (unsigned)framed_message[4]
                             << std::setfill('0') << std::setw(2) << (unsigned)framed_message[5]
                             << std::setfill('0') << std::setw(2) << (unsigned)framed_message[6]
                             << std::setfill('0') << std::setw(2) << (unsigned)framed_message[7] << " "
                             << "0x" << std::setfill('0') << std::setw(2) << (unsigned)framed_message[8]
                             << std::setfill('0') << std::setw(2) << (unsigned)framed_message[9]
                             << std::setfill('0') << std::setw(2) << (unsigned)framed_message[10]
                             << std::setfill('0') << std::setw(2) << (unsigned)framed_message[11]);
            
            // Add handshake magic to distinguish from regular messages (convert to network byte order)
            std::vector<uint8_t> handshake_message;
            handshake_message.resize(4);
            uint32_t handshake_magic_be = htonl(HANDSHAKE_MESSAGE_MAGIC);
            std::memcpy(handshake_message.data(), &handshake_magic_be, 4);
            handshake_message.insert(handshake_message.end(), framed_message.begin(), framed_message.end());
            
            std::string data_str(handshake_message.begin(), handshake_message.end());
            LOG_ENCRYPT_DEBUG("Sending total handshake message: " << handshake_message.size() << " bytes");
            LOG_ENCRYPT_DEBUG("Complete message first 16 bytes: " << std::hex
                             << "0x" << std::setfill('0') << std::setw(2) << (unsigned)handshake_message[0]
                             << std::setfill('0') << std::setw(2) << (unsigned)handshake_message[1]
                             << std::setfill('0') << std::setw(2) << (unsigned)handshake_message[2]
                             << std::setfill('0') << std::setw(2) << (unsigned)handshake_message[3] << " "
                             << "0x" << std::setfill('0') << std::setw(2) << (unsigned)handshake_message[4]
                             << std::setfill('0') << std::setw(2) << (unsigned)handshake_message[5]
                             << std::setfill('0') << std::setw(2) << (unsigned)handshake_message[6]
                             << std::setfill('0') << std::setw(2) << (unsigned)handshake_message[7] << " "
                             << "0x" << std::setfill('0') << std::setw(2) << (unsigned)handshake_message[8]
                             << std::setfill('0') << std::setw(2) << (unsigned)handshake_message[9]
                             << std::setfill('0') << std::setw(2) << (unsigned)handshake_message[10]
                             << std::setfill('0') << std::setw(2) << (unsigned)handshake_message[11] << " "
                             << "0x" << std::setfill('0') << std::setw(2) << (unsigned)handshake_message[12]
                             << std::setfill('0') << std::setw(2) << (unsigned)handshake_message[13]
                             << std::setfill('0') << std::setw(2) << (unsigned)handshake_message[14]
                             << std::setfill('0') << std::setw(2) << (unsigned)handshake_message[15]);
            int sent = send_tcp_string(socket, data_str);
            
            if (sent <= 0) {
                LOG_ENCRYPT_ERROR("Failed to send handshake message to socket " << socket);
                return false;
            }
            
            LOG_ENCRYPT_DEBUG("Sent handshake message (" << handshake_data.size() << " bytes) to socket " << socket);
        }
        
        return true;
        
    } catch (const std::exception& e) {
        LOG_ENCRYPT_ERROR("Exception in send_handshake_message: " << e.what());
        return false;
    }
}

std::vector<uint8_t> EncryptedSocket::receive_handshake_message(socket_t socket) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    auto* session = get_session(socket);
    if (!session) {
        LOG_ENCRYPT_ERROR("No session found for socket " << socket);
        return {};
    }
    
    try {
        // First, read the handshake magic (4 bytes)
        LOG_ENCRYPT_DEBUG("Attempting to receive handshake magic on socket " << socket);
        std::string magic_data = receive_exact_bytes(socket, 4);
        if (magic_data.size() < 4) {
            LOG_ENCRYPT_DEBUG("Failed to receive complete handshake magic, got " << magic_data.size() << " bytes");
            return {};
        }
        
        // Check handshake magic
        uint32_t received_magic;
        std::memcpy(&received_magic, magic_data.data(), 4);
        received_magic = ntohl(received_magic); // Convert from network byte order
        if (received_magic != HANDSHAKE_MESSAGE_MAGIC) {
            LOG_ENCRYPT_WARN("Received data is not a handshake message (magic: 0x" << std::hex << received_magic << ")");
            return {};
        }
        
        // Now read the noise frame header (8 bytes)
        LOG_ENCRYPT_DEBUG("Reading noise frame header on socket " << socket);
        std::string frame_header = receive_exact_bytes(socket, 8);
        LOG_ENCRYPT_DEBUG("Received frame header: " << frame_header.size() << " bytes (expected 8)");
        if (frame_header.size() < 8) {
            LOG_ENCRYPT_ERROR("Failed to receive complete frame header, got " << frame_header.size() << " bytes");
            return {};
        }
        
        // Parse the noise message length from the frame header
        uint32_t noise_magic;
        uint32_t noise_length;
        std::memcpy(&noise_magic, frame_header.data(), 4);
        std::memcpy(&noise_length, frame_header.data() + 4, 4);
        
        LOG_ENCRYPT_DEBUG("Raw frame header bytes: " << std::hex 
                         << "0x" << std::setfill('0') << std::setw(2) << (unsigned)(uint8_t)frame_header[0]
                         << std::setfill('0') << std::setw(2) << (unsigned)(uint8_t)frame_header[1]
                         << std::setfill('0') << std::setw(2) << (unsigned)(uint8_t)frame_header[2]
                         << std::setfill('0') << std::setw(2) << (unsigned)(uint8_t)frame_header[3] << " "
                         << "0x" << std::setfill('0') << std::setw(2) << (unsigned)(uint8_t)frame_header[4]
                         << std::setfill('0') << std::setw(2) << (unsigned)(uint8_t)frame_header[5]
                         << std::setfill('0') << std::setw(2) << (unsigned)(uint8_t)frame_header[6]
                         << std::setfill('0') << std::setw(2) << (unsigned)(uint8_t)frame_header[7]);
        
        // Convert from network byte order (big endian) to host byte order
        noise_magic = ntohl(noise_magic);
        noise_length = ntohl(noise_length);
        
        LOG_ENCRYPT_DEBUG("Parsed noise magic: 0x" << std::hex << noise_magic << ", length: " << std::dec << noise_length);
        
        if (noise_magic != NOISE_MESSAGE_MAGIC) {
            LOG_ENCRYPT_ERROR("Invalid noise message magic in frame header: 0x" << std::hex << noise_magic << " (expected 0x" << NOISE_MESSAGE_MAGIC << ")");
            return {};
        }
        
        if (noise_length > NOISE_MAX_MESSAGE_SIZE) {
            LOG_ENCRYPT_ERROR("Noise message length too large: " << noise_length);
            return {};
        }
        
        // Read the actual noise message data
        LOG_ENCRYPT_DEBUG("Reading " << noise_length << " bytes of noise message data on socket " << socket);
        std::string noise_data = receive_exact_bytes(socket, noise_length);
        if (noise_data.size() < noise_length) {
            LOG_ENCRYPT_ERROR("Failed to receive complete noise message, got " << noise_data.size() << " of " << noise_length << " bytes");
            return {};
        }
        
        // Reconstruct the complete framed message
        std::vector<uint8_t> framed_data;
        framed_data.insert(framed_data.end(), frame_header.begin(), frame_header.end());
        framed_data.insert(framed_data.end(), noise_data.begin(), noise_data.end());
        
        auto handshake_data = unframe_message(framed_data);
        LOG_ENCRYPT_DEBUG("Successfully received and unframed " << handshake_data.size() << " bytes of handshake data");
        
        if (handshake_data.empty()) {
            LOG_ENCRYPT_ERROR("Failed to unframe handshake message");
            return {};
        }
        
        // Process handshake message
        auto payload = session->session->process_handshake_message(handshake_data);
        
        LOG_ENCRYPT_DEBUG("Received and processed handshake message (" << handshake_data.size() << " bytes) from socket " << socket);
        
        return payload;
        
    } catch (const std::exception& e) {
        LOG_ENCRYPT_ERROR("Exception in receive_handshake_message: " << e.what());
        return {};
    }
}

bool EncryptedSocket::send_encrypted_data(socket_t socket, const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    auto* session = get_session(socket);
    if (!session) {
        LOG_ENCRYPT_ERROR("No session found for socket " << socket);
        return false;
    }
    
    if (!session->session->is_handshake_completed()) {
        LOG_ENCRYPT_ERROR("Cannot send encrypted data: handshake not completed for socket " << socket);
        return false;
    }
    
    try {
        auto encrypted_data = session->session->encrypt_transport_message(data);
        
        if (encrypted_data.empty()) {
            LOG_ENCRYPT_ERROR("Failed to encrypt data for socket " << socket);
            return false;
        }
        
        auto framed_message = frame_message(encrypted_data);
        
        int sent = send_tcp_data(socket, framed_message);
        if (sent <= 0) {
            LOG_ENCRYPT_ERROR("Failed to send encrypted data to socket " << socket);
            return false;
        }
        
        LOG_ENCRYPT_DEBUG("Sent encrypted data (" << data.size() << " bytes plaintext, " << encrypted_data.size() << " bytes encrypted) to socket " << socket);
        return true;
        
    } catch (const std::exception& e) {
        LOG_ENCRYPT_ERROR("Exception in send_encrypted_data: " << e.what());
        return false;
    }
}

bool EncryptedSocket::send_encrypted_data(socket_t socket, const std::string& data) {
    std::vector<uint8_t> binary_data(data.begin(), data.end());
    return send_encrypted_data(socket, binary_data);
}

std::vector<uint8_t> EncryptedSocket::receive_encrypted_data(socket_t socket) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    auto* session = get_session(socket);
    if (!session) {
        LOG_ENCRYPT_ERROR("No session found for socket " << socket);
        return std::vector<uint8_t>();
    }
    
    if (!session->session->is_handshake_completed()) {
        LOG_ENCRYPT_ERROR("Cannot receive encrypted data: handshake not completed for socket " << socket);
        return std::vector<uint8_t>();
    }
    
    try {
        std::vector<uint8_t> raw_data = receive_tcp_data(socket, 4096);
        if (raw_data.empty()) {
            return std::vector<uint8_t>();
        }
        
        auto encrypted_data = unframe_message(raw_data);
        
        if (encrypted_data.empty()) {
            LOG_ENCRYPT_ERROR("Failed to unframe encrypted message");
            return std::vector<uint8_t>();
        }
        
        auto decrypted_data = session->session->decrypt_transport_message(encrypted_data);
        
        if (decrypted_data.empty()) {
            LOG_ENCRYPT_ERROR("Failed to decrypt data from socket " << socket);
            return std::vector<uint8_t>();
        }
        
        LOG_ENCRYPT_DEBUG("Received encrypted data (" << encrypted_data.size() << " bytes encrypted, " << decrypted_data.size() << " bytes plaintext) from socket " << socket);
        
        return decrypted_data;
        
    } catch (const std::exception& e) {
        LOG_ENCRYPT_ERROR("Exception in receive_encrypted_data: " << e.what());
        return std::vector<uint8_t>();
    }
}

std::string EncryptedSocket::receive_encrypted_data_string(socket_t socket) {
    std::vector<uint8_t> binary_data = receive_encrypted_data(socket);
    if (binary_data.empty()) {
        return "";
    }
    return std::string(binary_data.begin(), binary_data.end());
}

bool EncryptedSocket::send_unencrypted_data(socket_t socket, const std::vector<uint8_t>& data) {
    int sent = send_tcp_data(socket, data);
    return sent > 0;
}

bool EncryptedSocket::send_unencrypted_data(socket_t socket, const std::string& data) {
    std::vector<uint8_t> binary_data(data.begin(), data.end());
    return send_unencrypted_data(socket, binary_data);
}

std::vector<uint8_t> EncryptedSocket::receive_unencrypted_data(socket_t socket) {
    return receive_tcp_data(socket);
}

std::string EncryptedSocket::receive_unencrypted_data_string(socket_t socket) {
    return receive_tcp_string(socket);
}

void EncryptedSocket::remove_socket(socket_t socket) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    auto it = sessions_.find(socket);
    if (it != sessions_.end()) {
        LOG_ENCRYPT_DEBUG("Removing encrypted socket session for socket " << socket);
        sessions_.erase(it);
    }
}

void EncryptedSocket::clear_all_sockets() {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    LOG_ENCRYPT_INFO("Clearing all encrypted socket sessions (" << sessions_.size() << " sessions)");
    sessions_.clear();
}

NoiseKey EncryptedSocket::generate_static_key() {
    return noise_utils::generate_static_keypair();
}

std::string EncryptedSocket::key_to_string(const NoiseKey& key) {
    return noise_utils::key_to_hex(key);
}

NoiseKey EncryptedSocket::string_to_key(const std::string& key_str) {
    return noise_utils::hex_to_key(key_str);
}

NoiseRole EncryptedSocket::get_socket_role(socket_t socket) const {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    const auto* session = get_session(socket);
    if (session) {
        return session->session->get_role();
    }
    return NoiseRole::INITIATOR; // Default
}

const NoiseKey& EncryptedSocket::get_remote_static_key(socket_t socket) const {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    const auto* session = get_session(socket);
    if (session) {
        return session->session->get_remote_static_public_key();
    }
    static NoiseKey empty_key;
    empty_key.fill(0);
    return empty_key;
}

EncryptedSocket::SocketSession* EncryptedSocket::get_session(socket_t socket) {
    auto it = sessions_.find(socket);
    return (it != sessions_.end()) ? it->second.get() : nullptr;
}

const EncryptedSocket::SocketSession* EncryptedSocket::get_session(socket_t socket) const {
    auto it = sessions_.find(socket);
    return (it != sessions_.end()) ? it->second.get() : nullptr;
}

std::string EncryptedSocket::receive_exact_bytes(socket_t socket, size_t byte_count) {
    std::string buffer;
    buffer.reserve(byte_count);
    
    while (buffer.size() < byte_count) {
        size_t remaining = byte_count - buffer.size();
        std::string chunk = receive_tcp_string(socket, remaining);
        
        if (chunk.empty()) {
            // Connection closed or error
            LOG_ENCRYPT_DEBUG("Connection closed or error while reading " << remaining << " remaining bytes");
            
            // Mark the session as failed if connection is closed during handshake
            auto* session = get_session(socket);
            if (session && !session->session->is_handshake_completed()) {
                LOG_ENCRYPT_DEBUG("Marking handshake as failed due to connection closure");
                // The session will be cleaned up by the caller
            }
            break;
        }
        
        buffer.append(chunk);
        LOG_ENCRYPT_DEBUG("Read " << chunk.size() << " bytes, total: " << buffer.size() << "/" << byte_count);
        if (chunk.size() <= 8) {  // Log small chunks in detail
            LOG_ENCRYPT_DEBUG("Chunk content: " << std::hex
                             << "0x" << std::setfill('0') << std::setw(2) << (chunk.size() > 0 ? (unsigned)(uint8_t)chunk[0] : 0)
                             << std::setfill('0') << std::setw(2) << (chunk.size() > 1 ? (unsigned)(uint8_t)chunk[1] : 0)
                             << std::setfill('0') << std::setw(2) << (chunk.size() > 2 ? (unsigned)(uint8_t)chunk[2] : 0)
                             << std::setfill('0') << std::setw(2) << (chunk.size() > 3 ? (unsigned)(uint8_t)chunk[3] : 0) << " "
                             << "0x" << std::setfill('0') << std::setw(2) << (chunk.size() > 4 ? (unsigned)(uint8_t)chunk[4] : 0)
                             << std::setfill('0') << std::setw(2) << (chunk.size() > 5 ? (unsigned)(uint8_t)chunk[5] : 0)
                             << std::setfill('0') << std::setw(2) << (chunk.size() > 6 ? (unsigned)(uint8_t)chunk[6] : 0)
                             << std::setfill('0') << std::setw(2) << (chunk.size() > 7 ? (unsigned)(uint8_t)chunk[7] : 0));
        }
    }
    
    return buffer;
}

std::vector<uint8_t> EncryptedSocket::frame_message(const std::vector<uint8_t>& message) {
    std::vector<uint8_t> framed(MESSAGE_HEADER_SIZE + message.size());
    
    // Add magic number (convert to network byte order)
    uint32_t magic_be = htonl(NOISE_MESSAGE_MAGIC);
    std::memcpy(framed.data(), &magic_be, 4);
    
    // Add message length (convert to network byte order)
    uint32_t length = static_cast<uint32_t>(message.size());
    uint32_t length_be = htonl(length);
    std::memcpy(framed.data() + 4, &length_be, 4);
    
    // Add message data
    std::memcpy(framed.data() + MESSAGE_HEADER_SIZE, message.data(), message.size());
    
    return framed;
}

std::vector<uint8_t> EncryptedSocket::unframe_message(const std::vector<uint8_t>& framed_message) {
    if (framed_message.size() < MESSAGE_HEADER_SIZE) {
        return {};
    }
    
    // Check magic number (convert from network byte order)
    uint32_t magic;
    std::memcpy(&magic, framed_message.data(), 4);
    magic = ntohl(magic);
    if (magic != NOISE_MESSAGE_MAGIC) {
        return {};
    }
    
    // Get message length (convert from network byte order)
    uint32_t length;
    std::memcpy(&length, framed_message.data() + 4, 4);
    length = ntohl(length);
    
    if (length > NOISE_MAX_MESSAGE_SIZE || framed_message.size() < MESSAGE_HEADER_SIZE + length) {
        return {};
    }
    
    // Extract message
    std::vector<uint8_t> message(length);
    std::memcpy(message.data(), framed_message.data() + MESSAGE_HEADER_SIZE, length);
    
    return message;
}

bool EncryptedSocket::is_noise_handshake_message(const std::vector<uint8_t>& data) {
    if (data.size() < 4) {
        return false;
    }
    
    uint32_t magic;
    std::memcpy(&magic, data.data(), 4);
    magic = ntohl(magic); // Convert from network byte order
    return magic == HANDSHAKE_MESSAGE_MAGIC;
}

//=============================================================================
// EncryptedSocketManager Implementation
//=============================================================================

EncryptedSocketManager::EncryptedSocketManager() : encryption_enabled_(true) {
    // Generate a default static key
    static_key_ = noise_utils::generate_static_keypair();
    LOG_ENCRYPT_INFO("Generated default static key for encrypted socket manager");
}

EncryptedSocketManager::~EncryptedSocketManager() {
    cleanup_all_sockets();
}

EncryptedSocketManager& EncryptedSocketManager::getInstance() {
    static EncryptedSocketManager instance;
    return instance;
}

bool EncryptedSocketManager::initialize_socket_as_initiator(socket_t socket, const NoiseKey& static_private_key) {
    return encrypted_socket_.initialize_as_initiator(socket, static_private_key);
}

bool EncryptedSocketManager::initialize_socket_as_responder(socket_t socket, const NoiseKey& static_private_key) {
    return encrypted_socket_.initialize_as_responder(socket, static_private_key);
}

bool EncryptedSocketManager::send_data(socket_t socket, const std::vector<uint8_t>& data) {
    if (!encryption_enabled_) {
        return encrypted_socket_.send_unencrypted_data(socket, data);
    }
    
    if (encrypted_socket_.is_handshake_completed(socket)) {
        return encrypted_socket_.send_encrypted_data(socket, data);
    } else {
        LOG_ENCRYPT_WARN("Attempting to send binary data on socket " << socket << " before handshake completion");
        return false;
    }
}

bool EncryptedSocketManager::send_data(socket_t socket, const std::string& data) {
    // Convert string to binary and use primary binary method
    std::vector<uint8_t> binary_data(data.begin(), data.end());
    return send_data(socket, binary_data);
}

std::vector<uint8_t> EncryptedSocketManager::receive_data(socket_t socket) {
    if (!encryption_enabled_) {
        return encrypted_socket_.receive_unencrypted_data(socket);
    }
    
    if (encrypted_socket_.is_handshake_completed(socket)) {
        return encrypted_socket_.receive_encrypted_data(socket);
    } else {
        LOG_ENCRYPT_WARN("Attempting to receive binary data on socket " << socket << " before handshake completion");
        return std::vector<uint8_t>();
    }
}

std::string EncryptedSocketManager::receive_data_string(socket_t socket) {
    // Use primary binary method and convert to string
    std::vector<uint8_t> binary_data = receive_data(socket);
    if (binary_data.empty()) {
        return "";
    }
    return std::string(binary_data.begin(), binary_data.end());
}

bool EncryptedSocketManager::perform_handshake_step(socket_t socket, const std::vector<uint8_t>& received_data) {
    LOG_ENCRYPT_DEBUG("perform_handshake_step called for socket " << socket << " with " << received_data.size() << " bytes of received data");
    
    if (!encryption_enabled_) {
        LOG_ENCRYPT_DEBUG("Encryption not enabled, returning true");
        return true; // No handshake needed when encryption is disabled
    }
    
    if (encrypted_socket_.is_handshake_completed(socket)) {
        LOG_ENCRYPT_DEBUG("Handshake already completed for socket " << socket);
        return true; // Already completed
    }
    
    if (encrypted_socket_.has_handshake_failed(socket)) {
        LOG_ENCRYPT_DEBUG("Handshake already failed for socket " << socket);
        return false; // Already failed
    }
    
    try {
        NoiseRole role = encrypted_socket_.get_socket_role(socket);
        
                if (received_data.empty()) {
            // For initiators, only send the initial handshake message if we haven't started yet
            if (role == NoiseRole::INITIATOR) {
                auto* session = encrypted_socket_.get_session(socket);
                if (session && session->session->get_handshake_state() == NoiseHandshakeState::WRITE_MESSAGE_1) {
                    LOG_ENCRYPT_DEBUG("Initiator sending initial handshake message on socket " << socket);
                    return encrypted_socket_.send_handshake_message(socket);
                } else {
                    LOG_ENCRYPT_DEBUG("Initiator waiting for responder message (state: " << static_cast<int>(session ? session->session->get_handshake_state() : NoiseHandshakeState::FAILED) << ")");
                    // Initiator should wait for responder's message, not keep sending
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    return !encrypted_socket_.has_handshake_failed(socket);
                }
            } else {
                // For responders, try to receive and process incoming handshake data
                auto* session = encrypted_socket_.get_session(socket);
                if (session && session->session->get_handshake_state() == NoiseHandshakeState::READ_MESSAGE_1) {
                    LOG_ENCRYPT_DEBUG("Responder trying to receive first handshake message on socket " << socket);
                    auto payload = encrypted_socket_.receive_handshake_message(socket);
                    
                    // If we received a handshake message, we need to send a response
                    if (!payload.empty()) {
                        LOG_ENCRYPT_DEBUG("Responder received handshake message (" << payload.size() << " bytes) on socket " << socket);
                        if (!encrypted_socket_.is_handshake_completed(socket)) {
                            LOG_ENCRYPT_DEBUG("Responder sending handshake response on socket " << socket);
                            return encrypted_socket_.send_handshake_message(socket);
                        }
                    } else {
                        LOG_ENCRYPT_DEBUG("Responder received empty handshake message on socket " << socket);
                        if (encrypted_socket_.has_handshake_failed(socket)) {
                            LOG_ENCRYPT_DEBUG("Handshake failed for socket " << socket);
                            return false;
                        }
                    }
                } else {
                    LOG_ENCRYPT_DEBUG("Responder waiting for initiator or already processed initial message (state: " << static_cast<int>(session ? session->session->get_handshake_state() : NoiseHandshakeState::FAILED) << ")");
                    // Responder should wait for the next message or handshake completion
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
                
                return !encrypted_socket_.has_handshake_failed(socket);
            }
        } else {
            // Process received handshake message
            auto payload = encrypted_socket_.receive_handshake_message(socket);
            
            // If we received a handshake message and we're the responder, 
            // we might need to send a response
            if (!encrypted_socket_.is_handshake_completed(socket)) {
                if (role == NoiseRole::RESPONDER) {
                    return encrypted_socket_.send_handshake_message(socket);
                }
            }
            
            return !encrypted_socket_.has_handshake_failed(socket);
        }
        
    } catch (const std::exception& e) {
        LOG_ENCRYPT_ERROR("Exception in perform_handshake_step: " << e.what());
        return false;
    }
}

bool EncryptedSocketManager::is_handshake_completed(socket_t socket) const {
    if (!encryption_enabled_) {
        return true; // No handshake needed when encryption is disabled
    }
    
    return encrypted_socket_.is_handshake_completed(socket);
}

bool EncryptedSocketManager::has_handshake_failed(socket_t socket) const {
    if (!encryption_enabled_) {
        return false; // No handshake to fail when encryption is disabled
    }
    
    return encrypted_socket_.has_handshake_failed(socket);
}

void EncryptedSocketManager::remove_socket(socket_t socket) {
    encrypted_socket_.remove_socket(socket);
}

void EncryptedSocketManager::cleanup_all_sockets() {
    encrypted_socket_.clear_all_sockets();
}

//=============================================================================
// High-level encrypted communication functions
//=============================================================================

namespace encrypted_communication {

bool initialize_encryption(const NoiseKey& static_key) {
    auto& manager = EncryptedSocketManager::getInstance();
    manager.set_static_key(static_key);
    manager.set_encryption_enabled(true);
    
    LOG_ENCRYPT_INFO("Initialized encryption with provided static key");
    return true;
}

NoiseKey generate_node_key() {
    NoiseKey key = EncryptedSocket::generate_static_key();
    LOG_ENCRYPT_INFO("Generated new node static key: " << EncryptedSocket::key_to_string(key));
    return key;
}

void set_encryption_enabled(bool enabled) {
    auto& manager = EncryptedSocketManager::getInstance();
    manager.set_encryption_enabled(enabled);
    
    LOG_ENCRYPT_INFO("Encryption " << (enabled ? "enabled" : "disabled"));
}

bool is_encryption_enabled() {
    auto& manager = EncryptedSocketManager::getInstance();
    return manager.is_encryption_enabled();
}

int send_tcp_data_encrypted(socket_t socket, const std::vector<uint8_t>& data) {
    auto& manager = EncryptedSocketManager::getInstance();
    
    if (manager.send_data(socket, data)) {
        return static_cast<int>(data.size());
    }
    return -1;
}

int send_tcp_data_encrypted(socket_t socket, const std::string& data) {
    auto& manager = EncryptedSocketManager::getInstance();
    
    if (manager.send_data(socket, data)) {
        return static_cast<int>(data.size());
    }
    return -1;
}

std::vector<uint8_t> receive_tcp_data_encrypted(socket_t socket, size_t buffer_size) {
    auto& manager = EncryptedSocketManager::getInstance();
    return manager.receive_data(socket);
}

std::string receive_tcp_data_encrypted_string(socket_t socket, size_t buffer_size) {
    auto& manager = EncryptedSocketManager::getInstance();
    return manager.receive_data_string(socket);
}

bool initialize_outgoing_connection(socket_t socket) {
    auto& manager = EncryptedSocketManager::getInstance();
    
    if (!manager.is_encryption_enabled()) {
        return true; // No initialization needed when encryption is disabled
    }
    
    bool success = manager.initialize_socket_as_initiator(socket, manager.get_static_key());
    if (success) {
        LOG_ENCRYPT_INFO("Initialized outgoing encrypted connection for socket " << socket);
        
        // Perform initial handshake step
        success = manager.perform_handshake_step(socket);
        if (!success) {
            LOG_ENCRYPT_ERROR("Failed initial handshake step for outgoing connection on socket " << socket);
        }
    }
    
    return success;
}

bool initialize_incoming_connection(socket_t socket) {
    auto& manager = EncryptedSocketManager::getInstance();
    
    if (!manager.is_encryption_enabled()) {
        return true; // No initialization needed when encryption is disabled
    }
    
    bool success = manager.initialize_socket_as_responder(socket, manager.get_static_key());
    if (success) {
        LOG_ENCRYPT_INFO("Initialized incoming encrypted connection for socket " << socket);
    }
    
    return success;
}

bool perform_handshake(socket_t socket) {
    auto& manager = EncryptedSocketManager::getInstance();
    LOG_ENCRYPT_DEBUG("perform_handshake called for socket " << socket);
    bool result = manager.perform_handshake_step(socket);
    LOG_ENCRYPT_DEBUG("perform_handshake_step returned " << result << " for socket " << socket);
    return result;
}

bool is_handshake_completed(socket_t socket) {
    auto& manager = EncryptedSocketManager::getInstance();
    return manager.is_handshake_completed(socket);
}

void cleanup_socket(socket_t socket) {
    auto& manager = EncryptedSocketManager::getInstance();
    manager.remove_socket(socket);
}

} // namespace encrypted_communication

} // namespace librats 