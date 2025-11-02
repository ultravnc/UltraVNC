#include "bittorrent.h"
#include "fs.h"
#include "network_utils.h"
#include "socket.h"
#include <iostream>
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <cstring>
#include <climits>

#define LOG_BT_DEBUG(message) LOG_DEBUG("bittorrent", message)
#define LOG_BT_INFO(message)  LOG_INFO("bittorrent", message)
#define LOG_BT_WARN(message)  LOG_WARN("bittorrent", message)
#define LOG_BT_ERROR(message) LOG_ERROR("bittorrent", message)

namespace librats {

//=============================================================================
// TorrentInfo Implementation
//=============================================================================

TorrentInfo::TorrentInfo() 
    : total_length_(0), piece_length_(0), private_(false) {
    info_hash_.fill(0);
}

TorrentInfo::~TorrentInfo() = default;

bool TorrentInfo::load_from_file(const std::string& torrent_file) {
    LOG_BT_INFO("Loading torrent from file: " << torrent_file);
    
    std::string content = read_file_text_cpp(torrent_file);
    if (content.empty()) {
        LOG_BT_ERROR("Failed to read torrent file: " << torrent_file);
        return false;
    }
    
    std::vector<uint8_t> data(content.begin(), content.end());
    return load_from_data(data);
}

bool TorrentInfo::load_from_data(const std::vector<uint8_t>& data) {
    LOG_BT_DEBUG("Parsing torrent data (" << data.size() << " bytes)");
    
    try {
        BencodeValue torrent = bencode::decode(data);
        return load_from_bencode(torrent);
    } catch (const std::exception& e) {
        LOG_BT_ERROR("Failed to decode torrent bencode: " << e.what());
        return false;
    }
}

bool TorrentInfo::load_from_bencode(const BencodeValue& torrent_data) {
    try {
        if (!torrent_data.is_dict()) {
            LOG_BT_ERROR("Torrent data is not a dictionary");
            return false;
        }
        
        // Parse announce URL
        if (torrent_data.has_key("announce")) {
            announce_ = torrent_data["announce"].as_string();
        }
        
        // Parse announce-list
        if (torrent_data.has_key("announce-list")) {
            const auto& announce_list = torrent_data["announce-list"];
            if (announce_list.is_list()) {
                for (size_t i = 0; i < announce_list.size(); ++i) {
                    const auto& tier = announce_list[i];
                    if (tier.is_list() && tier.size() > 0) {
                        announce_list_.push_back(tier[0].as_string());
                    }
                }
            }
        }
        
        // Parse info dictionary
        if (!torrent_data.has_key("info")) {
            LOG_BT_ERROR("Torrent data missing 'info' dictionary");
            return false;
        }
        
        const auto& info_dict = torrent_data["info"];
        if (!parse_info_dict(info_dict)) {
            return false;
        }
        
        // Calculate info hash
        calculate_info_hash(info_dict);
        
        LOG_BT_INFO("Successfully parsed torrent: " << name_);
        LOG_BT_INFO("  Info hash: " << info_hash_to_hex(info_hash_));
        LOG_BT_INFO("  Total size: " << total_length_ << " bytes");
        LOG_BT_INFO("  Piece length: " << piece_length_ << " bytes");
        LOG_BT_INFO("  Number of pieces: " << piece_hashes_.size());
        LOG_BT_INFO("  Number of files: " << files_.size());
        
        return true;
        
    } catch (const std::exception& e) {
        LOG_BT_ERROR("Failed to parse torrent: " << e.what());
        return false;
    }
}

bool TorrentInfo::parse_info_dict(const BencodeValue& info_dict) {
    if (!info_dict.is_dict()) {
        LOG_BT_ERROR("Info is not a dictionary");
        return false;
    }
    
    // Parse name
    if (!info_dict.has_key("name")) {
        LOG_BT_ERROR("Info dictionary missing 'name'");
        return false;
    }
    name_ = info_dict["name"].as_string();
    
    // Parse piece length
    if (!info_dict.has_key("piece length")) {
        LOG_BT_ERROR("Info dictionary missing 'piece length'");
        return false;
    }
    piece_length_ = static_cast<uint32_t>(info_dict["piece length"].as_integer());
    
    // Parse pieces hashes
    if (!info_dict.has_key("pieces")) {
        LOG_BT_ERROR("Info dictionary missing 'pieces'");
        return false;
    }
    
    const std::string& pieces_data = info_dict["pieces"].as_string();
    if (pieces_data.length() % 20 != 0) {
        LOG_BT_ERROR("Invalid pieces length: " << pieces_data.length());
        return false;
    }
    
    size_t num_pieces = pieces_data.length() / 20;
    piece_hashes_.reserve(num_pieces);
    
    for (size_t i = 0; i < num_pieces; ++i) {
        std::array<uint8_t, 20> hash;
        std::memcpy(hash.data(), pieces_data.data() + i * 20, 20);
        piece_hashes_.push_back(hash);
    }
    
    // Parse private flag
    if (info_dict.has_key("private")) {
        private_ = info_dict["private"].as_integer() != 0;
    }
    
    // Build file list
    build_file_list(info_dict);
    
    return true;
}

void TorrentInfo::calculate_info_hash(const BencodeValue& info_dict) {
    // Encode the info dictionary and calculate SHA1 hash
    std::vector<uint8_t> encoded = info_dict.encode();
    std::string hash_string = SHA1::hash_bytes(encoded);
    
    // Convert hex string to bytes
    for (size_t i = 0; i < 20; ++i) {
        std::string byte_str = hash_string.substr(i * 2, 2);
        info_hash_[i] = static_cast<uint8_t>(std::stoul(byte_str, nullptr, 16));
    }
}

void TorrentInfo::build_file_list(const BencodeValue& info_dict) {
    files_.clear();
    total_length_ = 0;
    
    if (info_dict.has_key("files")) {
        // Multi-file torrent
        const auto& files_list = info_dict["files"];
        if (!files_list.is_list()) {
            LOG_BT_ERROR("Files is not a list");
            return;
        }
        
        uint64_t offset = 0;
        for (size_t i = 0; i < files_list.size(); ++i) {
            const auto& file_info = files_list[i];
            if (!file_info.is_dict()) {
                continue;
            }
            
            if (!file_info.has_key("length") || !file_info.has_key("path")) {
                continue;
            }
            
            uint64_t length = static_cast<uint64_t>(file_info["length"].as_integer());
            const auto& path_list = file_info["path"];
            
            std::string path;
            if (path_list.is_list()) {
                for (size_t j = 0; j < path_list.size(); ++j) {
                    if (j > 0) path += "/";
                    path += path_list[j].as_string();
                }
            }
            
            files_.emplace_back(path, length, offset);
            offset += length;
            total_length_ += length;
        }
    } else {
        // Single-file torrent
        if (!info_dict.has_key("length")) {
            LOG_BT_ERROR("Single-file torrent missing 'length'");
            return;
        }
        
        uint64_t length = static_cast<uint64_t>(info_dict["length"].as_integer());
        files_.emplace_back(name_, length, 0);
        total_length_ = length;
    }
}

uint32_t TorrentInfo::get_piece_length(PieceIndex piece_index) const {
    if (piece_index >= piece_hashes_.size()) {
        return 0;
    }
    
    if (piece_index == piece_hashes_.size() - 1) {
        // Last piece might be smaller
        uint32_t remainder = static_cast<uint32_t>(total_length_ % piece_length_);
        return remainder > 0 ? remainder : piece_length_;
    }
    
    return piece_length_;
}

bool TorrentInfo::is_valid() const {
    return !name_.empty() && 
           !piece_hashes_.empty() && 
           piece_length_ > 0 && 
           total_length_ > 0 && 
           !files_.empty();
}

//=============================================================================
// PeerMessage Implementation
//=============================================================================

std::vector<uint8_t> PeerMessage::serialize() const {
    std::vector<uint8_t> result;
    
    // Length prefix (4 bytes)
    uint32_t length = 1 + static_cast<uint32_t>(payload.size());  // 1 byte for message type + payload
    result.push_back((length >> 24) & 0xFF);
    result.push_back((length >> 16) & 0xFF);
    result.push_back((length >> 8) & 0xFF);
    result.push_back(length & 0xFF);
    
    // Message type (1 byte)
    result.push_back(static_cast<uint8_t>(type));
    
    // Payload
    result.insert(result.end(), payload.begin(), payload.end());
    
    return result;
}

PeerMessage PeerMessage::create_choke() {
    return PeerMessage(MessageType::CHOKE);
}

PeerMessage PeerMessage::create_unchoke() {
    return PeerMessage(MessageType::UNCHOKE);
}

PeerMessage PeerMessage::create_interested() {
    return PeerMessage(MessageType::INTERESTED);
}

PeerMessage PeerMessage::create_not_interested() {
    return PeerMessage(MessageType::NOT_INTERESTED);
}

PeerMessage PeerMessage::create_have(PieceIndex piece_index) {
    std::vector<uint8_t> payload(4);
    payload[0] = (piece_index >> 24) & 0xFF;
    payload[1] = (piece_index >> 16) & 0xFF;
    payload[2] = (piece_index >> 8) & 0xFF;
    payload[3] = piece_index & 0xFF;
    return PeerMessage(MessageType::HAVE, payload);
}

PeerMessage PeerMessage::create_bitfield(const std::vector<bool>& bitfield) {
    std::vector<uint8_t> payload((bitfield.size() + 7) / 8, 0);
    
    for (size_t i = 0; i < bitfield.size(); ++i) {
        if (bitfield[i]) {
            size_t byte_index = i / 8;
            size_t bit_index = 7 - (i % 8);
            payload[byte_index] |= (1 << bit_index);
        }
    }
    
    return PeerMessage(MessageType::BITFIELD, payload);
}

PeerMessage PeerMessage::create_request(PieceIndex piece_index, uint32_t offset, uint32_t length) {
    std::vector<uint8_t> payload(12);
    
    // Piece index
    payload[0] = (piece_index >> 24) & 0xFF;
    payload[1] = (piece_index >> 16) & 0xFF;
    payload[2] = (piece_index >> 8) & 0xFF;
    payload[3] = piece_index & 0xFF;
    
    // Offset
    payload[4] = (offset >> 24) & 0xFF;
    payload[5] = (offset >> 16) & 0xFF;
    payload[6] = (offset >> 8) & 0xFF;
    payload[7] = offset & 0xFF;
    
    // Length
    payload[8] = (length >> 24) & 0xFF;
    payload[9] = (length >> 16) & 0xFF;
    payload[10] = (length >> 8) & 0xFF;
    payload[11] = length & 0xFF;
    
    return PeerMessage(MessageType::REQUEST, payload);
}

PeerMessage PeerMessage::create_piece(PieceIndex piece_index, uint32_t offset, const std::vector<uint8_t>& data) {
    std::vector<uint8_t> payload(8 + data.size());
    
    // Piece index
    payload[0] = (piece_index >> 24) & 0xFF;
    payload[1] = (piece_index >> 16) & 0xFF;
    payload[2] = (piece_index >> 8) & 0xFF;
    payload[3] = piece_index & 0xFF;
    
    // Offset
    payload[4] = (offset >> 24) & 0xFF;
    payload[5] = (offset >> 16) & 0xFF;
    payload[6] = (offset >> 8) & 0xFF;
    payload[7] = offset & 0xFF;
    
    // Data
    std::copy(data.begin(), data.end(), payload.begin() + 8);
    
    return PeerMessage(MessageType::PIECE, payload);
}

PeerMessage PeerMessage::create_cancel(PieceIndex piece_index, uint32_t offset, uint32_t length) {
    std::vector<uint8_t> payload(12);
    
    // Piece index
    payload[0] = (piece_index >> 24) & 0xFF;
    payload[1] = (piece_index >> 16) & 0xFF;
    payload[2] = (piece_index >> 8) & 0xFF;
    payload[3] = piece_index & 0xFF;
    
    // Offset
    payload[4] = (offset >> 24) & 0xFF;
    payload[5] = (offset >> 16) & 0xFF;
    payload[6] = (offset >> 8) & 0xFF;
    payload[7] = offset & 0xFF;
    
    // Length
    payload[8] = (length >> 24) & 0xFF;
    payload[9] = (length >> 16) & 0xFF;
    payload[10] = (length >> 8) & 0xFF;
    payload[11] = length & 0xFF;
    
    return PeerMessage(MessageType::CANCEL, payload);
}

PeerMessage PeerMessage::create_port(uint16_t port) {
    std::vector<uint8_t> payload(2);
    payload[0] = (port >> 8) & 0xFF;
    payload[1] = port & 0xFF;
    return PeerMessage(MessageType::PORT, payload);
}

//=============================================================================
// PeerConnection Implementation
//=============================================================================

PeerConnection::PeerConnection(TorrentDownload* torrent, const Peer& peer_info, socket_t socket)
    : torrent_(torrent), peer_info_(peer_info), socket_(socket), 
      state_(PeerState::CONNECTING), should_disconnect_(false),
      peer_choked_(true), am_choked_(true), peer_interested_(false), 
      am_interested_(false), am_choking_(true),
      downloaded_bytes_(0), uploaded_bytes_(0), expected_message_length_(0) {
    
    peer_id_.fill(0);
    LOG_BT_DEBUG("Created peer connection to " << peer_info_.ip << ":" << peer_info_.port);
}

PeerConnection::~PeerConnection() {
    disconnect();
}

bool PeerConnection::connect() {
    if (state_ != PeerState::CONNECTING) {
        return false;
    }
    
    LOG_BT_INFO("Connecting to peer " << peer_info_.ip << ":" << peer_info_.port);
    
    if (!is_valid_socket(socket_)) {
        socket_ = create_tcp_client(peer_info_.ip, peer_info_.port, 10000); // 10-second timeout
        if (!is_valid_socket(socket_)) {
            LOG_BT_ERROR("Failed to create connection to " << peer_info_.ip << ":" << peer_info_.port);
            state_ = PeerState::ERROR;
            return false;
        }
    }
    
    // Start connection thread
    connection_thread_ = std::thread(&PeerConnection::connection_loop, this);
    
    return true;
}

void PeerConnection::disconnect() {
    should_disconnect_ = true;
    
    if (is_valid_socket(socket_)) {
        close_socket(socket_);
        socket_ = INVALID_SOCKET_VALUE;
    }
    
    if (connection_thread_.joinable()) {
        connection_thread_.join();
    }
    
    state_ = PeerState::DISCONNECTED;
}

void PeerConnection::connection_loop() {
    LOG_BT_DEBUG("Starting connection loop for peer " << peer_info_.ip << ":" << peer_info_.port);
    
    // Perform handshake
    if (!perform_handshake()) {
        LOG_BT_ERROR("Handshake failed with peer " << peer_info_.ip << ":" << peer_info_.port);
        state_ = PeerState::ERROR;
        return;
    }
    
    state_ = PeerState::CONNECTED;
    LOG_BT_INFO("Successfully connected to peer " << peer_info_.ip << ":" << peer_info_.port);
    
    // Initialize peer bitfield
    const auto& torrent_info = torrent_->get_torrent_info();
    peer_bitfield_.resize(torrent_info.get_num_pieces(), false);
    
    // Main message processing loop
    while (!should_disconnect_ && state_ == PeerState::CONNECTED) {
        process_messages();
        
        // Cleanup expired requests
        cleanup_expired_requests();
        
        // Use conditional variable for responsive shutdown
        {
            std::unique_lock<std::mutex> lock(shutdown_mutex_);
            if (shutdown_cv_.wait_for(lock, std::chrono::milliseconds(10), [this] { return should_disconnect_.load(); })) {
                break;
            }
        }
    }
    
    LOG_BT_DEBUG("Connection loop ended for peer " << peer_info_.ip << ":" << peer_info_.port);
}

bool PeerConnection::perform_handshake() {
    state_ = PeerState::HANDSHAKING;
    
    if (!send_handshake()) {
        return false;
    }
    
    if (!receive_handshake()) {
        return false;
    }
    
    return true;
}

bool PeerConnection::send_handshake() {
    const auto& torrent_info = torrent_->get_torrent_info();
    PeerID our_peer_id = generate_peer_id();
    
    auto handshake_data = create_handshake_message(torrent_info.get_info_hash(), our_peer_id);
    return write_data(handshake_data);
}

bool PeerConnection::receive_handshake() {
    std::vector<uint8_t> handshake_data(68);  // Fixed handshake size
    
    if (!read_data(handshake_data, 68)) {
        LOG_BT_ERROR("Failed to read handshake from peer");
        return false;
    }
    
    InfoHash received_info_hash;
    if (!parse_handshake_message(handshake_data, received_info_hash, peer_id_)) {
        LOG_BT_ERROR("Failed to parse handshake from peer");
        return false;
    }
    
    // Verify info hash matches
    const auto& expected_info_hash = torrent_->get_torrent_info().get_info_hash();
    if (received_info_hash != expected_info_hash) {
        LOG_BT_ERROR("Info hash mismatch in handshake");
        return false;
    }
    
    LOG_BT_DEBUG("Handshake successful with peer " << peer_info_.ip << ":" << peer_info_.port);
    return true;
}

bool PeerConnection::send_message(const PeerMessage& message) {
    auto data = message.serialize();
    return write_data(data);
}

void PeerConnection::process_messages() {
    // Try to read message length first
    if (expected_message_length_ == 0) {
        std::vector<uint8_t> length_buffer(4);
        if (read_data(length_buffer, 4)) {
            expected_message_length_ = (length_buffer[0] << 24) | 
                                     (length_buffer[1] << 16) | 
                                     (length_buffer[2] << 8) | 
                                     length_buffer[3];
            
            if (expected_message_length_ == 0) {
                // Keep-alive message
                expected_message_length_ = 0;
                return;
            }
            
            message_buffer_.clear();
            message_buffer_.reserve(expected_message_length_);
        } else {
            return;  // No data available yet
        }
    }
    
    // Try to read the message payload
    if (expected_message_length_ > 0 && message_buffer_.size() < expected_message_length_) {
        size_t remaining = expected_message_length_ - message_buffer_.size();
        std::vector<uint8_t> temp_buffer(remaining);
        
        if (read_data(temp_buffer, remaining)) {
            message_buffer_.insert(message_buffer_.end(), temp_buffer.begin(), temp_buffer.end());
        }
    }
    
    // Process complete message
    if (message_buffer_.size() >= expected_message_length_) {
        auto message = parse_message(message_buffer_);
        if (message) {
            handle_message(*message);
        }
        
        // Reset for next message
        expected_message_length_ = 0;
        message_buffer_.clear();
    }
}

std::unique_ptr<PeerMessage> PeerConnection::parse_message(const std::vector<uint8_t>& data) {
    if (data.empty()) {
        return nullptr;
    }
    
    MessageType type = static_cast<MessageType>(data[0]);
    std::vector<uint8_t> payload(data.begin() + 1, data.end());
    
    return std::make_unique<PeerMessage>(type, payload);
}

void PeerConnection::handle_message(const PeerMessage& message) {
    LOG_BT_DEBUG("Received message type " << static_cast<int>(message.type) << " from peer " << peer_info_.ip << ":" << peer_info_.port);
    
    switch (message.type) {
        case MessageType::CHOKE:
            handle_choke();
            break;
        case MessageType::UNCHOKE:
            handle_unchoke();
            break;
        case MessageType::INTERESTED:
            handle_interested();
            break;
        case MessageType::NOT_INTERESTED:
            handle_not_interested();
            break;
        case MessageType::HAVE:
            handle_have(message.payload);
            break;
        case MessageType::BITFIELD:
            handle_bitfield(message.payload);
            break;
        case MessageType::REQUEST:
            handle_request(message.payload);
            break;
        case MessageType::PIECE:
            handle_piece(message.payload);
            break;
        case MessageType::CANCEL:
            handle_cancel(message.payload);
            break;
        default:
            LOG_BT_WARN("Unknown message type: " << static_cast<int>(message.type));
            break;
    }
}

void PeerConnection::handle_choke() {
    peer_choked_ = true;
    cancel_all_requests();
    LOG_BT_DEBUG("Peer " << peer_info_.ip << ":" << peer_info_.port << " choked us");
}

void PeerConnection::handle_unchoke() {
    peer_choked_ = false;
    LOG_BT_DEBUG("Peer " << peer_info_.ip << ":" << peer_info_.port << " unchoked us");
}

void PeerConnection::handle_interested() {
    peer_interested_ = true;
    LOG_BT_DEBUG("Peer " << peer_info_.ip << ":" << peer_info_.port << " is interested");
}

void PeerConnection::handle_not_interested() {
    peer_interested_ = false;
    LOG_BT_DEBUG("Peer " << peer_info_.ip << ":" << peer_info_.port << " is not interested");
}

void PeerConnection::handle_have(const std::vector<uint8_t>& payload) {
    if (payload.size() != 4) {
        LOG_BT_WARN("Invalid HAVE message size: " << payload.size());
        return;
    }
    
    PieceIndex piece_index = (payload[0] << 24) | (payload[1] << 16) | (payload[2] << 8) | payload[3];
    
    if (piece_index < peer_bitfield_.size()) {
        peer_bitfield_[piece_index] = true;
        LOG_BT_DEBUG("Peer " << peer_info_.ip << ":" << peer_info_.port << " has piece " << piece_index);
    }
}

void PeerConnection::handle_bitfield(const std::vector<uint8_t>& payload) {
    const auto& torrent_info = torrent_->get_torrent_info();
    uint32_t num_pieces = torrent_info.get_num_pieces();
    
    peer_bitfield_.clear();
    peer_bitfield_.resize(num_pieces, false);
    
    for (uint32_t i = 0; i < num_pieces && i / 8 < payload.size(); ++i) {
        size_t byte_index = i / 8;
        size_t bit_index = 7 - (i % 8);
        peer_bitfield_[i] = (payload[byte_index] & (1 << bit_index)) != 0;
    }
    
    LOG_BT_DEBUG("Received bitfield from peer " << peer_info_.ip << ":" << peer_info_.port);
}

void PeerConnection::handle_request(const std::vector<uint8_t>& payload) {
    if (payload.size() != 12) {
        LOG_BT_WARN("Invalid REQUEST message size: " << payload.size());
        return;
    }
    
    PieceIndex piece_index = (payload[0] << 24) | (payload[1] << 16) | (payload[2] << 8) | payload[3];
    uint32_t offset = (payload[4] << 24) | (payload[5] << 16) | (payload[6] << 8) | payload[7];
    uint32_t length = (payload[8] << 24) | (payload[9] << 16) | (payload[10] << 8) | payload[11];
    
    LOG_BT_DEBUG("Peer " << peer_info_.ip << ":" << peer_info_.port << " requested piece " << piece_index << " offset " << offset << " length " << length);
    
    // TODO: Handle piece requests (seeding functionality)
    // For now, we're just downloading, not seeding
}

void PeerConnection::handle_piece(const std::vector<uint8_t>& payload) {
    if (payload.size() < 8) {
        LOG_BT_WARN("Invalid PIECE message size: " << payload.size());
        return;
    }
    
    PieceIndex piece_index = (payload[0] << 24) | (payload[1] << 16) | (payload[2] << 8) | payload[3];
    uint32_t offset = (payload[4] << 24) | (payload[5] << 16) | (payload[6] << 8) | payload[7];
    
    std::vector<uint8_t> block_data(payload.begin() + 8, payload.end());
    downloaded_bytes_ += block_data.size();
    
    LOG_BT_DEBUG("Received piece " << piece_index << " offset " << offset << " length " << block_data.size() << " from peer " << peer_info_.ip << ":" << peer_info_.port);
    
    // Store the piece block
    if (torrent_->store_piece_block(piece_index, offset, block_data)) {
        // Remove corresponding request
        std::lock_guard<std::mutex> lock(requests_mutex_);
        pending_requests_.erase(
            std::remove_if(pending_requests_.begin(), pending_requests_.end(),
                [piece_index, offset](const PeerRequest& req) {
                    return req.piece_index == piece_index && req.offset == offset;
                }),
            pending_requests_.end());
    }
}

void PeerConnection::handle_cancel(const std::vector<uint8_t>& payload) {
    if (payload.size() != 12) {
        LOG_BT_WARN("Invalid CANCEL message size: " << payload.size());
        return;
    }
    
    PieceIndex piece_index = (payload[0] << 24) | (payload[1] << 16) | (payload[2] << 8) | payload[3];
    uint32_t offset = (payload[4] << 24) | (payload[5] << 16) | (payload[6] << 8) | payload[7];
    uint32_t length = (payload[8] << 24) | (payload[9] << 16) | (payload[10] << 8) | payload[11];
    
    LOG_BT_DEBUG("Peer " << peer_info_.ip << ":" << peer_info_.port << " cancelled request for piece " << piece_index << " offset " << offset << " length " << length);
    
    // TODO: Handle cancel requests (seeding functionality)
}

bool PeerConnection::request_piece_block(PieceIndex piece_index, uint32_t offset, uint32_t length) {
    if (peer_choked_ || state_ != PeerState::CONNECTED) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(requests_mutex_);
    
    if (pending_requests_.size() >= MAX_REQUESTS_PER_PEER) {
        return false;  // Too many pending requests
    }
    
    auto request_msg = PeerMessage::create_request(piece_index, offset, length);
    if (send_message(request_msg)) {
        pending_requests_.emplace_back(piece_index, offset, length);
        LOG_BT_DEBUG("Requested piece " << piece_index << " offset " << offset << " length " << length << " from peer " << peer_info_.ip << ":" << peer_info_.port);
        return true;
    }
    
    return false;
}

void PeerConnection::cancel_request(PieceIndex piece_index, uint32_t offset, uint32_t length) {
    auto cancel_msg = PeerMessage::create_cancel(piece_index, offset, length);
    send_message(cancel_msg);
    
    std::lock_guard<std::mutex> lock(requests_mutex_);
    pending_requests_.erase(
        std::remove_if(pending_requests_.begin(), pending_requests_.end(),
            [piece_index, offset, length](const PeerRequest& req) {
                return req.piece_index == piece_index && req.offset == offset && req.length == length;
            }),
        pending_requests_.end());
}

void PeerConnection::cancel_all_requests() {
    std::lock_guard<std::mutex> lock(requests_mutex_);
    
    for (const auto& request : pending_requests_) {
        auto cancel_msg = PeerMessage::create_cancel(request.piece_index, request.offset, request.length);
        send_message(cancel_msg);
    }
    
    pending_requests_.clear();
}

void PeerConnection::set_interested(bool interested) {
    if (am_interested_ != interested) {
        am_interested_ = interested;
        auto msg = interested ? PeerMessage::create_interested() : PeerMessage::create_not_interested();
        send_message(msg);
    }
}

void PeerConnection::set_choke(bool choke) {
    if (am_choking_ != choke) {
        am_choking_ = choke;
        auto msg = choke ? PeerMessage::create_choke() : PeerMessage::create_unchoke();
        send_message(msg);
    }
}

bool PeerConnection::has_piece(PieceIndex piece_index) const {
    return piece_index < peer_bitfield_.size() && peer_bitfield_[piece_index];
}

void PeerConnection::update_bitfield(const std::vector<bool>& bitfield) {
    peer_bitfield_ = bitfield;
}

void PeerConnection::cleanup_expired_requests() {
    std::lock_guard<std::mutex> lock(requests_mutex_);
    
    auto now = std::chrono::steady_clock::now();
    auto timeout = std::chrono::milliseconds(REQUEST_TIMEOUT_MS);
    
    pending_requests_.erase(
        std::remove_if(pending_requests_.begin(), pending_requests_.end(),
            [now, timeout](const PeerRequest& req) {
                return now - req.requested_at > timeout;
            }),
        pending_requests_.end());
}

bool PeerConnection::read_data(std::vector<uint8_t>& buffer, size_t length) {
    if (!is_valid_socket(socket_)) {
        return false;
    }
    
    // Use existing network utilities
    std::string data = receive_tcp_string(socket_);
    if (data.length() >= length) {
        std::copy(data.begin(), data.begin() + length, buffer.begin());
        return true;
    }
    
    return false;
}

bool PeerConnection::write_data(const std::vector<uint8_t>& data) {
    if (!is_valid_socket(socket_)) {
        return false;
    }
    
    std::string data_str(data.begin(), data.end());
    int sent = send_tcp_string(socket_, data_str);
    return sent > 0;
}

//=============================================================================
// TorrentDownload Implementation
//=============================================================================

TorrentDownload::TorrentDownload(const TorrentInfo& torrent_info, const std::string& download_path)
    : torrent_info_(torrent_info), download_path_(download_path), 
      running_(false), paused_(false), total_downloaded_(0), total_uploaded_(0) {
    
    // Initialize pieces
    uint32_t num_pieces = torrent_info_.get_num_pieces();
    pieces_.reserve(num_pieces);
    piece_completed_.resize(num_pieces, false);
    piece_downloading_.resize(num_pieces, false);
    
    const auto& piece_hashes = torrent_info_.get_piece_hashes();
    for (uint32_t i = 0; i < num_pieces; ++i) {
        uint32_t piece_length = torrent_info_.get_piece_length(i);
        pieces_.push_back(std::make_unique<PieceInfo>(i, piece_hashes[i], piece_length));
    }
    
    LOG_BT_INFO("Created torrent download for: " << torrent_info_.get_name());
    LOG_BT_INFO("  Download path: " << download_path_);
    LOG_BT_INFO("  Total size: " << torrent_info_.get_total_length() << " bytes");
    LOG_BT_INFO("  Number of pieces: " << num_pieces);
}

TorrentDownload::~TorrentDownload() {
    stop();
}

bool TorrentDownload::start() {
    if (running_) {
        return true;
    }
    
    LOG_BT_INFO("Starting torrent download: " << torrent_info_.get_name());
    
    // Create directory structure
    if (!create_directory_structure()) {
        LOG_BT_ERROR("Failed to create directory structure");
        return false;
    }
    
    // Open files
    if (!open_files()) {
        LOG_BT_ERROR("Failed to open files");
        return false;
    }
    
    running_ = true;
    paused_ = false;
    
    // Start download threads
    download_thread_ = std::thread(&TorrentDownload::download_loop, this);
    peer_management_thread_ = std::thread(&TorrentDownload::peer_management_loop, this);
    
    LOG_BT_INFO("Torrent download started: " << torrent_info_.get_name());
    return true;
}

void TorrentDownload::stop() {
    if (!running_) {
        return;
    }
    
    LOG_BT_INFO("Stopping torrent download: " << torrent_info_.get_name());
    
    running_ = false;
    
    // Stop all peer connections
    {
        std::lock_guard<std::mutex> lock(peers_mutex_);
        for (auto& peer : peer_connections_) {
            peer->disconnect();
        }
        peer_connections_.clear();
    }
    
    // Wait for threads to finish
    if (download_thread_.joinable()) {
        download_thread_.join();
    }
    if (peer_management_thread_.joinable()) {
        peer_management_thread_.join();
    }
    
    // Close files
    close_files();
    
    LOG_BT_INFO("Torrent download stopped: " << torrent_info_.get_name());
}

void TorrentDownload::pause() {
    paused_ = true;
    LOG_BT_INFO("Paused torrent download: " << torrent_info_.get_name());
}

void TorrentDownload::resume() {
    paused_ = false;
    LOG_BT_INFO("Resumed torrent download: " << torrent_info_.get_name());
}

bool TorrentDownload::is_complete() const {
    std::lock_guard<std::mutex> lock(pieces_mutex_);
    return std::all_of(piece_completed_.begin(), piece_completed_.end(), [](bool completed) { return completed; });
}

bool TorrentDownload::add_peer(const Peer& peer) {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    
    // Check if peer already exists
    for (const auto& conn : peer_connections_) {
        if (conn->get_peer_info().ip == peer.ip && conn->get_peer_info().port == peer.port) {
            return false;  // Peer already exists
        }
    }
    
    // Check peer limit
    if (peer_connections_.size() >= MAX_PEERS_PER_TORRENT) {
        LOG_BT_DEBUG("Peer limit reached for torrent " << torrent_info_.get_name());
        return false;
    }
    
    // Create new peer connection
    auto peer_conn = std::make_unique<PeerConnection>(this, peer);
    if (peer_conn->connect()) {
        peer_connections_.push_back(std::move(peer_conn));
        
        if (peer_connected_callback_) {
            peer_connected_callback_(peer);
        }
        
        LOG_BT_INFO("Added peer " << peer.ip << ":" << peer.port << " to torrent " << torrent_info_.get_name());
        return true;
    }
    
    return false;
}

void TorrentDownload::remove_peer(const Peer& peer) {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    
    peer_connections_.erase(
        std::remove_if(peer_connections_.begin(), peer_connections_.end(),
            [&peer](const std::unique_ptr<PeerConnection>& conn) {
                return conn->get_peer_info().ip == peer.ip && conn->get_peer_info().port == peer.port;
            }),
        peer_connections_.end());
    
    if (peer_disconnected_callback_) {
        peer_disconnected_callback_(peer);
    }
    
    LOG_BT_DEBUG("Removed peer " << peer.ip << ":" << peer.port << " from torrent " << torrent_info_.get_name());
}

size_t TorrentDownload::get_peer_count() const {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    return peer_connections_.size();
}

std::vector<Peer> TorrentDownload::get_connected_peers() const {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    
    std::vector<Peer> peers;
    for (const auto& conn : peer_connections_) {
        if (conn->is_connected()) {
            peers.push_back(conn->get_peer_info());
        }
    }
    
    return peers;
}

bool TorrentDownload::is_piece_complete(PieceIndex piece_index) const {
    std::lock_guard<std::mutex> lock(pieces_mutex_);
    return piece_index < piece_completed_.size() && piece_completed_[piece_index];
}

bool TorrentDownload::is_piece_downloading(PieceIndex piece_index) const {
    std::lock_guard<std::mutex> lock(pieces_mutex_);
    return piece_index < piece_downloading_.size() && piece_downloading_[piece_index];
}

bool TorrentDownload::store_piece_block(PieceIndex piece_index, uint32_t offset, const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(pieces_mutex_);
    
    if (piece_index >= pieces_.size()) {
        LOG_BT_ERROR("Invalid piece index: " << piece_index);
        return false;
    }
    
    auto& piece = pieces_[piece_index];
    
    // Validate offset and data size
    if (offset + data.size() > piece->length) {
        LOG_BT_ERROR("Block data exceeds piece length for piece " << piece_index);
        return false;
    }
    
    // Calculate block index
    uint32_t block_index = offset / BLOCK_SIZE;
    if (block_index >= piece->get_num_blocks()) {
        LOG_BT_ERROR("Invalid block index: " << block_index << " for piece " << piece_index);
        return false;
    }
    
    // Store the block data
    std::copy(data.begin(), data.end(), piece->data.begin() + offset);
    piece->blocks_downloaded[block_index] = true;
    
    LOG_BT_DEBUG("Stored block " << block_index << " for piece " << piece_index 
                 << " (offset: " << offset << ", size: " << data.size() << ")");
    
    // Check if piece is complete
    if (piece->is_complete() && !piece->verified) {
        LOG_BT_INFO("Piece " << piece_index << " downloaded, verifying...");
        if (verify_piece(piece_index)) {
            piece_completed_[piece_index] = true;
            piece_downloading_[piece_index] = false;
            
            // Write piece to disk
            write_piece_to_disk(piece_index);
            
            // Update statistics
            total_downloaded_ += piece->length;
            
            // Notify completion
            on_piece_completed(piece_index);
            
            LOG_BT_INFO("Piece " << piece_index << " verified and saved");
            return true;
        } else {
            LOG_BT_ERROR("Piece " << piece_index << " verification failed, requesting re-download");
            // Reset piece for re-download
            std::fill(piece->blocks_downloaded.begin(), piece->blocks_downloaded.end(), false);
            piece_downloading_[piece_index] = false;
            return false;
        }
    }
    
    return true;
}

bool TorrentDownload::verify_piece(PieceIndex piece_index) {
    if (piece_index >= pieces_.size()) {
        return false;
    }
    
    auto& piece = pieces_[piece_index];
    
    // Calculate SHA1 hash of piece data
    std::string calculated_hash = SHA1::hash_bytes(piece->data);
    
    // Convert stored hash to hex string for comparison
    std::ostringstream stored_hash_hex;
    for (size_t i = 0; i < piece->hash.size(); ++i) {
        stored_hash_hex << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(piece->hash[i]);
    }
    
    bool verified = (calculated_hash == stored_hash_hex.str());
    piece->verified = verified;
    
    LOG_BT_DEBUG("Piece " << piece_index << " verification: " << (verified ? "PASSED" : "FAILED"));
    return verified;
}

void TorrentDownload::write_piece_to_disk(PieceIndex piece_index) {
    std::lock_guard<std::mutex> lock(files_mutex_);
    
    if (piece_index >= pieces_.size()) {
        LOG_BT_ERROR("Invalid piece index for disk write: " << piece_index);
        return;
    }
    
    auto& piece = pieces_[piece_index];
    if (!piece->verified) {
        LOG_BT_ERROR("Attempting to write unverified piece " << piece_index << " to disk");
        return;
    }
    
    // Calculate piece offset in the torrent
    uint64_t piece_offset = static_cast<uint64_t>(piece_index) * torrent_info_.get_piece_length();
    uint64_t remaining_data = piece->length;
    uint64_t data_offset = 0;
    
    // Write piece data to the appropriate files
    for (const auto& file_info : torrent_info_.get_files()) {
        if (piece_offset >= file_info.offset + file_info.length) {
            continue; // This piece doesn't overlap with this file
        }
        
        if (piece_offset + piece->length <= file_info.offset) {
            break; // No more files will be affected by this piece
        }
        
        // Calculate overlap
        uint64_t file_start_in_piece = (file_info.offset > piece_offset) ? 
                                      file_info.offset - piece_offset : 0;
        uint64_t piece_end = piece_offset + piece->length;
        uint64_t file_end = file_info.offset + file_info.length;
        uint64_t write_end = (std::min)(piece_end, file_end);
        uint64_t write_length = write_end - (piece_offset + file_start_in_piece);
        
        if (write_length == 0) {
            continue;
        }
        
        // Open file for writing
        std::string file_path = download_path_ + "/" + file_info.path;
        std::fstream file(file_path, std::ios::binary | std::ios::in | std::ios::out);
        
        if (!file.is_open()) {
            LOG_BT_ERROR("Failed to open file for writing: " << file_path);
            continue;
        }
        
        // Seek to correct position in file
        uint64_t file_offset = (piece_offset > file_info.offset) ? 
                              piece_offset - file_info.offset : 0;
        file.seekp(file_offset);
        
        // Write data
        file.write(reinterpret_cast<const char*>(piece->data.data() + file_start_in_piece), 
                  write_length);
        
        if (!file.good()) {
            LOG_BT_ERROR("Failed to write data to file: " << file_path);
        }
        
        file.close();
        
        LOG_BT_DEBUG("Wrote " << write_length << " bytes to file " << file_info.path 
                     << " at offset " << file_offset);
    }
}

std::vector<PieceIndex> TorrentDownload::get_available_pieces() const {
    std::lock_guard<std::mutex> lock(pieces_mutex_);
    std::vector<PieceIndex> available_pieces;
    
    for (PieceIndex i = 0; i < piece_completed_.size(); ++i) {
        if (piece_completed_[i]) {
            available_pieces.push_back(i);
        }
    }
    
    return available_pieces;
}

std::vector<PieceIndex> TorrentDownload::get_needed_pieces(const std::vector<bool>& peer_bitfield) const {
    std::lock_guard<std::mutex> lock(pieces_mutex_);
    std::vector<PieceIndex> needed_pieces;
    
    size_t min_size = (std::min)(peer_bitfield.size(), piece_completed_.size());
    for (size_t i = 0; i < min_size; ++i) {
        if (peer_bitfield[i] && !piece_completed_[i] && !piece_downloading_[i]) {
            needed_pieces.push_back(static_cast<PieceIndex>(i));
        }
    }
    
    return needed_pieces;
}

uint64_t TorrentDownload::get_downloaded_bytes() const {
    return total_downloaded_.load();
}

uint64_t TorrentDownload::get_uploaded_bytes() const {
    return total_uploaded_.load();
}

double TorrentDownload::get_progress_percentage() const {
    if (torrent_info_.get_total_length() == 0) {
        return 0.0;
    }
    
    uint64_t downloaded = get_downloaded_bytes();
    return (static_cast<double>(downloaded) / torrent_info_.get_total_length()) * 100.0;
}

uint32_t TorrentDownload::get_completed_pieces() const {
    std::lock_guard<std::mutex> lock(pieces_mutex_);
    return static_cast<uint32_t>(std::count(piece_completed_.begin(), piece_completed_.end(), true));
}

std::vector<bool> TorrentDownload::get_piece_bitfield() const {
    std::lock_guard<std::mutex> lock(pieces_mutex_);
    return piece_completed_;
}

void TorrentDownload::download_loop() {
    LOG_BT_INFO("Download loop started for torrent: " << torrent_info_.get_name());
    
    while (running_ && !is_complete()) {
        if (paused_) {
            // Use conditional variable for responsive shutdown
            {
                std::unique_lock<std::mutex> lock(shutdown_mutex_);
                if (shutdown_cv_.wait_for(lock, std::chrono::milliseconds(100), [this] { return !running_.load() || !paused_.load(); })) {
                    if (!running_) break;
                }
            }
            continue;
        }
        
        // Schedule piece requests to peers
        schedule_piece_requests();
        
        // Update progress
        update_progress();
        
        // Check for completion
        check_torrent_completion();
        
        // Use conditional variable for responsive shutdown
        {
            std::unique_lock<std::mutex> lock(shutdown_mutex_);
            if (shutdown_cv_.wait_for(lock, std::chrono::milliseconds(50), [this] { return !running_.load(); })) {
                break;
            }
        }
    }
    
    LOG_BT_INFO("Download loop ended for torrent: " << torrent_info_.get_name());
}

void TorrentDownload::peer_management_loop() {
    LOG_BT_INFO("Peer management loop started for torrent: " << torrent_info_.get_name());
    
    while (running_) {
        // Clean up disconnected peers
        cleanup_disconnected_peers();
        
        // Use conditional variable for responsive shutdown
        {
            std::unique_lock<std::mutex> lock(shutdown_mutex_);
            if (shutdown_cv_.wait_for(lock, std::chrono::seconds(1), [this] { return !running_.load(); })) {
                break;
            }
        }
    }
    
    LOG_BT_INFO("Peer management loop ended for torrent: " << torrent_info_.get_name());
}

void TorrentDownload::schedule_piece_requests() {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    
    for (auto& peer : peer_connections_) {
        if (!peer->is_connected() || peer->is_choked()) {
            continue;
        }
        
        // Get pieces we need that this peer has
        std::vector<PieceIndex> needed_pieces = get_needed_pieces(peer->get_bitfield());
        
        if (needed_pieces.empty()) {
            continue;
        }
        
        // Select pieces to download using our piece selection strategy
        std::vector<PieceIndex> selected_pieces = select_pieces_for_download();
        
        // Request blocks from selected pieces
        for (PieceIndex piece_index : selected_pieces) {
            if (peer->get_pending_requests() >= MAX_REQUESTS_PER_PEER) {
                break; // Don't overwhelm this peer
            }
            
            if (!peer->has_piece(piece_index)) {
                continue;
            }
            
            // Find blocks we need for this piece
            {
                std::lock_guard<std::mutex> pieces_lock(pieces_mutex_);
                if (piece_index >= pieces_.size() || piece_completed_[piece_index]) {
                    continue;
                }
                
                auto& piece = pieces_[piece_index];
                uint32_t block_size = BLOCK_SIZE;
                
                for (uint32_t block_index = 0; block_index < piece->get_num_blocks(); ++block_index) {
                    if (piece->blocks_downloaded[block_index]) {
                        continue; // Already have this block
                    }
                    
                    uint32_t offset = block_index * BLOCK_SIZE;
                    uint32_t length = (std::min)(block_size, piece->length - offset);
                    
                    if (peer->request_piece_block(piece_index, offset, length)) {
                        piece_downloading_[piece_index] = true;
                        LOG_BT_DEBUG("Requested block " << block_index << " of piece " << piece_index 
                                     << " from peer " << peer->get_peer_info().ip);
                        break; // Request one block at a time per piece per peer
                    }
                }
            }
        }
        
        // Express interest if we need pieces from this peer
        if (!needed_pieces.empty() && !peer->is_interested()) {
            peer->set_interested(true);
        }
    }
}

void TorrentDownload::cleanup_disconnected_peers() {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    
    peer_connections_.erase(
        std::remove_if(peer_connections_.begin(), peer_connections_.end(),
            [](const std::unique_ptr<PeerConnection>& peer) {
                return !peer->is_connected();
            }),
        peer_connections_.end());
}

bool TorrentDownload::open_files() {
    std::lock_guard<std::mutex> lock(files_mutex_);
    
    const auto& files = torrent_info_.get_files();
    file_handles_.clear();
    file_handles_.reserve(files.size());
    
    for (const auto& file_info : files) {
        std::string file_path = download_path_ + "/" + file_info.path;
        
        // Create file with correct size
        std::fstream file(file_path, std::ios::binary | std::ios::in | std::ios::out);
        if (!file.is_open()) {
            // Try to create the file
            file.open(file_path, std::ios::binary | std::ios::out);
            if (!file.is_open()) {
                LOG_BT_ERROR("Failed to create file: " << file_path);
                return false;
            }
            file.close();
            
            // Reopen for reading and writing
            file.open(file_path, std::ios::binary | std::ios::in | std::ios::out);
            if (!file.is_open()) {
                LOG_BT_ERROR("Failed to reopen file: " << file_path);
                return false;
            }
        }
        
        // Resize file to correct length
        file.seekp(file_info.length - 1);
        file.write("\0", 1);
        
        file_handles_.emplace_back(std::move(file));
        LOG_BT_DEBUG("Opened file: " << file_path << " (size: " << file_info.length << ")");
    }
    
    return true;
}

void TorrentDownload::close_files() {
    std::lock_guard<std::mutex> lock(files_mutex_);
    
    for (auto& file : file_handles_) {
        if (file.is_open()) {
            file.close();
        }
    }
    file_handles_.clear();
}

bool TorrentDownload::create_directory_structure() {
    const auto& files = torrent_info_.get_files();
    
    for (const auto& file_info : files) {
        std::string file_path = download_path_ + "/" + file_info.path;
        
        // Extract directory path
        size_t last_slash = file_path.find_last_of('/');
        if (last_slash != std::string::npos) {
            std::string dir_path = file_path.substr(0, last_slash);
            
            // Create directory structure
            if (!create_directories(dir_path.c_str())) {
                LOG_BT_ERROR("Failed to create directory: " << dir_path);
                return false;
            }
        }
    }
    
    return true;
}

std::vector<PieceIndex> TorrentDownload::select_pieces_for_download() {
    std::lock_guard<std::mutex> lock(pieces_mutex_);
    
    std::vector<PieceIndex> needed_pieces;
    
    // Find pieces we need
    for (PieceIndex i = 0; i < piece_completed_.size(); ++i) {
        if (!piece_completed_[i] && !piece_downloading_[i]) {
            needed_pieces.push_back(i);
        }
    }
    
    // Implement rarest-first strategy (simplified)
    std::vector<PieceIndex> selected_pieces;
    
    // For now, just select the first few needed pieces
    // In a more sophisticated implementation, we would count how many peers have each piece
    // and prioritize rarer pieces
    size_t max_pieces = (std::min)(needed_pieces.size(), static_cast<size_t>(10));
    for (size_t i = 0; i < max_pieces; ++i) {
        selected_pieces.push_back(needed_pieces[i]);
    }
    
    return selected_pieces;
}

PieceIndex TorrentDownload::select_rarest_piece(const std::vector<bool>& available_pieces) {
    std::lock_guard<std::mutex> lock(pieces_mutex_);
    
    // Count how many peers have each piece
    std::vector<int> piece_counts(piece_completed_.size(), 0);
    
    {
        std::lock_guard<std::mutex> peers_lock(peers_mutex_);
        for (const auto& peer : peer_connections_) {
            if (!peer->is_connected()) {
                continue;
            }
            
            const auto& peer_bitfield = peer->get_bitfield();
            for (size_t i = 0; i < peer_bitfield.size() && i < piece_counts.size(); ++i) {
                if (peer_bitfield[i]) {
                    piece_counts[i]++;
                }
            }
        }
    }
    
    // Find the rarest piece we need and is available
    int min_count = INT_MAX;
    PieceIndex rarest_piece = static_cast<PieceIndex>(-1);
    
    for (size_t i = 0; i < available_pieces.size() && i < piece_completed_.size(); ++i) {
        if (available_pieces[i] && !piece_completed_[i] && !piece_downloading_[i]) {
            if (piece_counts[i] < min_count) {
                min_count = piece_counts[i];
                rarest_piece = static_cast<PieceIndex>(i);
            }
        }
    }
    
    return rarest_piece;
}

void TorrentDownload::update_progress() {
    if (progress_callback_) {
        uint64_t downloaded = get_downloaded_bytes();
        uint64_t total = torrent_info_.get_total_length();
        double percentage = get_progress_percentage();
        
        progress_callback_(downloaded, total, percentage);
    }
}

void TorrentDownload::on_piece_completed(PieceIndex piece_index) {
    if (piece_complete_callback_) {
        piece_complete_callback_(piece_index);
    }
    
    LOG_BT_INFO("Piece " << piece_index << " completed. Progress: " 
                << get_progress_percentage() << "% (" 
                << get_completed_pieces() << "/" << torrent_info_.get_num_pieces() << " pieces)");
}

void TorrentDownload::check_torrent_completion() {
    if (is_complete() && torrent_complete_callback_) {
        torrent_complete_callback_(torrent_info_.get_name());
        LOG_BT_INFO("Torrent download completed: " << torrent_info_.get_name());
    }
}

void TorrentDownload::announce_to_dht(DhtClient* dht_client) {
    if (!dht_client || !dht_client->is_running()) {
        LOG_BT_WARN("DHT client not available for torrent announcement");
        return;
    }
    
    // Convert info hash to DHT format
    InfoHash dht_info_hash;
    const auto& torrent_hash = torrent_info_.get_info_hash();
    std::copy(torrent_hash.begin(), torrent_hash.end(), dht_info_hash.begin());
    
    // Announce with default BitTorrent port (we don't have a BitTorrent listen port yet)
    uint16_t announce_port = 6881; // Default BitTorrent port
    
    if (dht_client->announce_peer(dht_info_hash, announce_port)) {
        LOG_BT_INFO("Announced torrent to DHT: " << torrent_info_.get_name());
    } else {
        LOG_BT_WARN("Failed to announce torrent to DHT: " << torrent_info_.get_name());
    }
}

void TorrentDownload::request_peers_from_dht(DhtClient* dht_client) {
    if (!dht_client || !dht_client->is_running()) {
        LOG_BT_WARN("DHT client not available for peer discovery");
        return;
    }
    
    // Convert info hash to DHT format
    InfoHash dht_info_hash;
    const auto& torrent_hash = torrent_info_.get_info_hash();
    std::copy(torrent_hash.begin(), torrent_hash.end(), dht_info_hash.begin());
    
    LOG_BT_INFO("Requesting peers from DHT for torrent: " << torrent_info_.get_name());
    
    dht_client->find_peers(dht_info_hash, [this](const std::vector<Peer>& peers, const InfoHash& info_hash) {
        LOG_BT_INFO("DHT discovered " << peers.size() << " peers for torrent: " << torrent_info_.get_name());
        
        for (const auto& peer : peers) {
            if (get_peer_count() >= MAX_PEERS_PER_TORRENT) {
                break; // Don't exceed peer limit
            }
            
            add_peer(peer);
        }
    });
}

//=============================================================================
// BitTorrentClient Implementation
//=============================================================================

BitTorrentClient::BitTorrentClient()
    : running_(false), listen_port_(0), listen_socket_(INVALID_SOCKET_VALUE),
      dht_client_(nullptr), max_connections_per_torrent_(MAX_PEERS_PER_TORRENT),
      download_rate_limit_(0), upload_rate_limit_(0) {
    
    LOG_BT_INFO("BitTorrent client created");
}

BitTorrentClient::~BitTorrentClient() {
    stop();
}

bool BitTorrentClient::start(int listen_port) {
    if (running_.load()) {
        LOG_BT_WARN("BitTorrent client is already running");
        return false;
    }
    
    listen_port_ = listen_port;
    if (listen_port_ == 0) {
        listen_port_ = 6881; // Default BitTorrent port
    }
    
    LOG_BT_INFO("Starting BitTorrent client on port " << listen_port_);
    
    // Initialize socket library (safe to call multiple times)
    if (!init_socket_library()) {
        LOG_BT_ERROR("Failed to initialize socket library");
        return false;
    }
    
    // Create listen socket
    listen_socket_ = create_tcp_server(listen_port_);
    if (!is_valid_socket(listen_socket_)) {
        LOG_BT_ERROR("Failed to create BitTorrent listen socket on port " << listen_port_);
        return false;
    }
    
    running_.store(true);
    
    // Start incoming connections handler
    incoming_connections_thread_ = std::thread(&BitTorrentClient::handle_incoming_connections, this);
    
    LOG_BT_INFO("BitTorrent client started successfully");
    return true;
}

void BitTorrentClient::stop() {
    if (!running_.load()) {
        return;
    }
    
    LOG_BT_INFO("Stopping BitTorrent client");
    running_.store(false);
    
    // Stop all torrents
    {
        std::lock_guard<std::mutex> lock(torrents_mutex_);
        for (auto& pair : torrents_) {
            pair.second->stop();
        }
        torrents_.clear();
    }
    
    // Close listen socket
    if (is_valid_socket(listen_socket_)) {
        close_socket(listen_socket_);
        listen_socket_ = INVALID_SOCKET_VALUE;
    }
    
    // Wait for threads
    if (incoming_connections_thread_.joinable()) {
        incoming_connections_thread_.join();
    }
    
    LOG_BT_INFO("BitTorrent client stopped");
}

std::shared_ptr<TorrentDownload> BitTorrentClient::add_torrent(const std::string& torrent_file, 
                                                              const std::string& download_path) {
    TorrentInfo torrent_info;
    if (!torrent_info.load_from_file(torrent_file)) {
        LOG_BT_ERROR("Failed to load torrent file: " << torrent_file);
        return nullptr;
    }
    
    return add_torrent(torrent_info, download_path);
}

std::shared_ptr<TorrentDownload> BitTorrentClient::add_torrent(const TorrentInfo& torrent_info, 
                                                              const std::string& download_path) {
    if (!running_.load()) {
        LOG_BT_ERROR("BitTorrent client is not running");
        return nullptr;
    }
    
    const InfoHash& info_hash = torrent_info.get_info_hash();
    
    {
        std::lock_guard<std::mutex> lock(torrents_mutex_);
        
        // Check if torrent already exists
        if (torrents_.find(info_hash) != torrents_.end()) {
            LOG_BT_WARN("Torrent already exists: " << torrent_info.get_name());
            return torrents_[info_hash];
        }
        
        // Create new torrent download
        auto torrent_download = std::make_shared<TorrentDownload>(torrent_info, download_path);
        
        // Set up callbacks
        torrent_download->set_progress_callback([this, info_hash](uint64_t downloaded, uint64_t total, double percentage) {
            LOG_BT_DEBUG("Torrent progress: " << percentage << "% (" << downloaded << "/" << total << " bytes)");
        });
        
        torrent_download->set_torrent_complete_callback([this, info_hash](const std::string& torrent_name) {
            LOG_BT_INFO("Torrent completed: " << torrent_name);
            if (torrent_completed_callback_) {
                torrent_completed_callback_(info_hash);
            }
        });
        
        torrents_[info_hash] = torrent_download;
        
        // Start the download
        if (!torrent_download->start()) {
            LOG_BT_ERROR("Failed to start torrent download: " << torrent_info.get_name());
            torrents_.erase(info_hash);
            return nullptr;
        }
        
        // Announce to DHT if available
        if (dht_client_) {
            torrent_download->announce_to_dht(dht_client_);
            torrent_download->request_peers_from_dht(dht_client_);
        }
        
        LOG_BT_INFO("Added torrent: " << torrent_info.get_name());
        
        if (torrent_added_callback_) {
            torrent_added_callback_(info_hash);
        }
        
        return torrent_download;
    }
}

bool BitTorrentClient::remove_torrent(const InfoHash& info_hash) {
    std::lock_guard<std::mutex> lock(torrents_mutex_);
    
    auto it = torrents_.find(info_hash);
    if (it == torrents_.end()) {
        return false;
    }
    
    // Stop the torrent
    it->second->stop();
    torrents_.erase(it);
    
    if (torrent_removed_callback_) {
        torrent_removed_callback_(info_hash);
    }
    
    LOG_BT_INFO("Removed torrent with info hash: " << info_hash_to_hex(info_hash));
    return true;
}

std::shared_ptr<TorrentDownload> BitTorrentClient::get_torrent(const InfoHash& info_hash) {
    std::lock_guard<std::mutex> lock(torrents_mutex_);
    
    auto it = torrents_.find(info_hash);
    return (it != torrents_.end()) ? it->second : nullptr;
}

std::vector<std::shared_ptr<TorrentDownload>> BitTorrentClient::get_all_torrents() {
    std::lock_guard<std::mutex> lock(torrents_mutex_);
    
    std::vector<std::shared_ptr<TorrentDownload>> result;
    result.reserve(torrents_.size());
    
    for (const auto& pair : torrents_) {
        result.push_back(pair.second);
    }
    
    return result;
}

void BitTorrentClient::discover_peers_for_torrent(const InfoHash& info_hash) {
    if (!dht_client_ || !dht_client_->is_running()) {
        LOG_BT_WARN("DHT client not available for peer discovery");
        return;
    }
    
    auto torrent = get_torrent(info_hash);
    if (!torrent) {
        LOG_BT_WARN("Torrent not found for peer discovery");
        return;
    }
    
    torrent->request_peers_from_dht(dht_client_);
}

void BitTorrentClient::announce_torrent_to_dht(const InfoHash& info_hash) {
    if (!dht_client_ || !dht_client_->is_running()) {
        LOG_BT_WARN("DHT client not available for torrent announcement");
        return;
    }
    
    auto torrent = get_torrent(info_hash);
    if (!torrent) {
        LOG_BT_WARN("Torrent not found for DHT announcement");
        return;
    }
    
    torrent->announce_to_dht(dht_client_);
}

size_t BitTorrentClient::get_active_torrents_count() const {
    std::lock_guard<std::mutex> lock(torrents_mutex_);
    return torrents_.size();
}

uint64_t BitTorrentClient::get_total_downloaded() const {
    std::lock_guard<std::mutex> lock(torrents_mutex_);
    
    uint64_t total = 0;
    for (const auto& pair : torrents_) {
        total += pair.second->get_downloaded_bytes();
    }
    
    return total;
}

uint64_t BitTorrentClient::get_total_uploaded() const {
    std::lock_guard<std::mutex> lock(torrents_mutex_);
    
    uint64_t total = 0;
    for (const auto& pair : torrents_) {
        total += pair.second->get_uploaded_bytes();
    }
    
    return total;
}

void BitTorrentClient::handle_incoming_connections() {
    LOG_BT_INFO("BitTorrent incoming connections handler started");
    
    while (running_.load()) {
        socket_t client_socket = accept_client(listen_socket_);
        if (!is_valid_socket(client_socket)) {
            if (running_.load()) {
                LOG_BT_ERROR("Failed to accept BitTorrent client connection");
            }
            continue;
        }
        
        // Handle the connection in a separate thread
        std::thread([this, client_socket]() {
            handle_incoming_connection(client_socket);
        }).detach();
    }
    
    LOG_BT_INFO("BitTorrent incoming connections handler stopped");
}

void BitTorrentClient::handle_incoming_connection(socket_t client_socket) {
    LOG_BT_DEBUG("Handling incoming BitTorrent connection");
    
    InfoHash info_hash;
    PeerID peer_id;
    
    if (!perform_incoming_handshake(client_socket, info_hash, peer_id)) {
        LOG_BT_WARN("Failed to perform incoming handshake");
        close_socket(client_socket);
        return;
    }
    
    // Find the torrent for this info hash
    auto torrent = get_torrent(info_hash);
    if (!torrent) {
        LOG_BT_WARN("Received connection for unknown torrent");
        close_socket(client_socket);
        return;
    }
    
    // Get peer address
    std::string peer_address = get_peer_address(client_socket);
    
    // Parse peer address to get IP and port
    std::string ip;
    int port = 0;
    size_t colon_pos = peer_address.find_last_of(':');
    if (colon_pos != std::string::npos) {
        ip = peer_address.substr(0, colon_pos);
        try {
            port = std::stoi(peer_address.substr(colon_pos + 1));
        } catch (const std::exception&) {
            port = 0;
        }
    }
    
    if (ip.empty() || port == 0) {
        LOG_BT_WARN("Failed to parse peer address: " << peer_address);
        close_socket(client_socket);
        return;
    }
    
    // Create peer object and add to torrent
    Peer peer_info{ip, static_cast<uint16_t>(port)};
    if (torrent->add_peer(peer_info)) {
        LOG_BT_INFO("Added incoming peer: " << peer_address);
    } else {
        LOG_BT_WARN("Failed to add incoming peer: " << peer_address);
        close_socket(client_socket);
    }
}

bool BitTorrentClient::perform_incoming_handshake(socket_t socket, InfoHash& info_hash, PeerID& peer_id) {
    // Receive handshake
    std::vector<uint8_t> handshake_data(68); // Fixed handshake size
    std::string received_data = receive_tcp_string(socket, 68);
    
    if (received_data.length() != 68) {
        LOG_BT_ERROR("Invalid handshake size received: " << received_data.length());
        return false;
    }
    
    std::copy(received_data.begin(), received_data.end(), handshake_data.begin());
    
    if (!parse_handshake_message(handshake_data, info_hash, peer_id)) {
        LOG_BT_ERROR("Failed to parse incoming handshake");
        return false;
    }
    
    // Send our handshake response
    PeerID our_peer_id = generate_peer_id();
    auto response_data = create_handshake_message(info_hash, our_peer_id);
    
    std::string response_str(response_data.begin(), response_data.end());
    if (send_tcp_string(socket, response_str) <= 0) {
        LOG_BT_ERROR("Failed to send handshake response");
        return false;
    }
    
    LOG_BT_DEBUG("Incoming handshake completed successfully");
    return true;
}

//=============================================================================
// Utility Functions Implementation
//=============================================================================

InfoHash calculate_info_hash(const BencodeValue& info_dict) {
    // Encode the info dictionary and calculate SHA1 hash
    std::vector<uint8_t> encoded = info_dict.encode();
    std::string hash_string = SHA1::hash_bytes(encoded);
    
    InfoHash result;
    // Convert hex string to bytes
    for (size_t i = 0; i < 20; ++i) {
        std::string byte_str = hash_string.substr(i * 2, 2);
        result[i] = static_cast<uint8_t>(std::stoul(byte_str, nullptr, 16));
    }
    
    return result;
}

std::string info_hash_to_hex(const InfoHash& hash) {
    std::ostringstream hex_stream;
    for (size_t i = 0; i < hash.size(); ++i) {
        hex_stream << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(hash[i]);
    }
    return hex_stream.str();
}

InfoHash hex_to_info_hash(const std::string& hex) {
    InfoHash result;
    result.fill(0);
    
    if (hex.length() != 40) { // 20 bytes * 2 hex chars per byte
        return result;
    }
    
    for (size_t i = 0; i < 20; ++i) {
        std::string byte_str = hex.substr(i * 2, 2);
        try {
            result[i] = static_cast<uint8_t>(std::stoul(byte_str, nullptr, 16));
        } catch (const std::exception&) {
            result.fill(0);
            return result;
        }
    }
    
    return result;
}

PeerID generate_peer_id() {
    PeerID peer_id;
    
    // Use librats- prefix to identify our client
    std::string prefix = "-LR0001-"; // LR = LibRats, 0001 = version
    std::copy(prefix.begin(), prefix.end(), peer_id.begin());
    
    // Fill the rest with random data
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    for (size_t i = prefix.length(); i < peer_id.size(); ++i) {
        peer_id[i] = static_cast<uint8_t>(dis(gen));
    }
    
    return peer_id;
}

std::vector<uint8_t> create_handshake_message(const InfoHash& info_hash, const PeerID& peer_id) {
    std::vector<uint8_t> handshake(68);
    
    // Protocol identifier length
    handshake[0] = BITTORRENT_PROTOCOL_ID_LENGTH;
    
    // Protocol identifier
    std::copy_n(BITTORRENT_PROTOCOL_ID, BITTORRENT_PROTOCOL_ID_LENGTH, handshake.begin() + 1);
    
    // Reserved bytes (8 bytes of zeros)
    std::fill_n(handshake.begin() + 20, 8, 0);
    
    // Info hash (20 bytes)
    std::copy(info_hash.begin(), info_hash.end(), handshake.begin() + 28);
    
    // Peer ID (20 bytes)
    std::copy(peer_id.begin(), peer_id.end(), handshake.begin() + 48);
    
    return handshake;
}

bool parse_handshake_message(const std::vector<uint8_t>& data, InfoHash& info_hash, PeerID& peer_id) {
    if (data.size() != 68) {
        return false;
    }
    
    // Check protocol identifier length
    if (data[0] != BITTORRENT_PROTOCOL_ID_LENGTH) {
        return false;
    }
    
    // Check protocol identifier
    std::string protocol_id(data.begin() + 1, data.begin() + 1 + BITTORRENT_PROTOCOL_ID_LENGTH);
    if (protocol_id != std::string(reinterpret_cast<const char*>(BITTORRENT_PROTOCOL_ID), BITTORRENT_PROTOCOL_ID_LENGTH)) {
        return false;
    }
    
    // Extract info hash
    std::copy(data.begin() + 28, data.begin() + 48, info_hash.begin());
    
    // Extract peer ID
    std::copy(data.begin() + 48, data.begin() + 68, peer_id.begin());
    
    return true;
}

} // namespace librats
