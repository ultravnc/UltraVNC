#include "ice.h"
#include "socket.h"
#include "stun.h"
#include "network_utils.h"
#include "logger.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstring>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
#endif

// ICE module logging macros
#define LOG_ICE_DEBUG(message) LOG_DEBUG("ice", message)
#define LOG_ICE_INFO(message)  LOG_INFO("ice", message)
#define LOG_ICE_WARN(message)  LOG_WARN("ice", message)
#define LOG_ICE_ERROR(message) LOG_ERROR("ice", message)

namespace librats {

//=============================================================================
// IceCandidate Implementation
//=============================================================================

IceCandidate::IceCandidate() 
    : component_id(1), transport(IceTransport::UDP), priority(0), port(0), 
      type(IceCandidateType::HOST), related_port(0), turn_port(0) {
}

std::string IceCandidate::to_sdp() const {
    std::ostringstream sdp;
    sdp << "candidate:" << foundation << " " << component_id << " " 
        << ice_transport_to_string(transport) << " " << priority << " " 
        << ip << " " << port << " typ " << ice_candidate_type_to_string(type);
    
    if (!related_ip.empty() && related_port > 0) {
        sdp << " raddr " << related_ip << " rport " << related_port;
    }
    
    return sdp.str();
}

IceCandidate IceCandidate::from_sdp(const std::string& sdp_line) {
    IceCandidate candidate;
    std::istringstream iss(sdp_line);
    std::string token;
    
    // Parse: candidate:foundation component transport priority ip port typ type [raddr related_ip rport related_port]
    if (iss >> token && token.substr(0, 10) == "candidate:") {
        candidate.foundation = token.substr(10);
        iss >> candidate.component_id;
        
        iss >> token;
        candidate.transport = string_to_ice_transport(token);
        
        iss >> candidate.priority;
        iss >> candidate.ip;
        iss >> candidate.port;
        
        iss >> token; // "typ"
        iss >> token;
        candidate.type = string_to_ice_candidate_type(token);
        
        // Parse optional related address
        while (iss >> token) {
            if (token == "raddr") {
                iss >> candidate.related_ip;
            } else if (token == "rport") {
                iss >> candidate.related_port;
            }
        }
    }
    
    return candidate;
}

nlohmann::json IceCandidate::to_json() const {
    nlohmann::json json;
    json["foundation"] = foundation;
    json["component_id"] = component_id;
    json["transport"] = ice_transport_to_string(transport);
    json["priority"] = priority;
    json["ip"] = ip;
    json["port"] = port;
    json["type"] = ice_candidate_type_to_string(type);
    json["related_ip"] = related_ip;
    json["related_port"] = related_port;
    json["ufrag"] = ufrag;
    json["pwd"] = pwd;
    json["turn_server"] = turn_server;
    json["turn_port"] = turn_port;
    return json;
}

IceCandidate IceCandidate::from_json(const nlohmann::json& json) {
    IceCandidate candidate;
    candidate.foundation = json.value("foundation", "");
    candidate.component_id = json.value("component_id", 1);
    candidate.transport = string_to_ice_transport(json.value("transport", "udp"));
    candidate.priority = json.value("priority", 0);
    candidate.ip = json.value("ip", "");
    candidate.port = json.value("port", 0);
    candidate.type = string_to_ice_candidate_type(json.value("type", "host"));
    candidate.related_ip = json.value("related_ip", "");
    candidate.related_port = json.value("related_port", 0);
    candidate.ufrag = json.value("ufrag", "");
    candidate.pwd = json.value("pwd", "");
    candidate.turn_server = json.value("turn_server", "");
    candidate.turn_port = json.value("turn_port", 0);
    return candidate;
}

//=============================================================================
// IceCandidatePair Implementation
//=============================================================================

IceCandidatePair::IceCandidatePair(const IceCandidate& local, const IceCandidate& remote)
    : local(local), remote(remote), nominated(false), check_count(0), succeeded(false) {
    priority = calculate_priority();
    last_check_time = std::chrono::steady_clock::now();
}

uint64_t IceCandidatePair::calculate_priority() const {
    // RFC 8445 - Use the higher priority as the controlling agent priority
    uint32_t controlling = (std::max)(local.priority, remote.priority);
    uint32_t controlled = (std::min)(local.priority, remote.priority);
    
    return (static_cast<uint64_t>(controlling) << 32) | controlled;
}

//=============================================================================
// IceConfig Implementation
//=============================================================================

IceConfig::IceConfig() 
    : enable_host_candidates(true),
      enable_server_reflexive_candidates(true),
      enable_relay_candidates(true),
      enable_tcp_candidates(false),
      stun_timeout_ms(5000),
      turn_timeout_ms(10000),
      connectivity_check_timeout_ms(30000),
      max_connectivity_checks(100) {
    
    // Default STUN servers
    stun_servers.push_back("stun.l.google.com:19302");
    stun_servers.push_back("stun1.l.google.com:19302");
    stun_servers.push_back("stun.stunprotocol.org:3478");
}

//=============================================================================
// TurnClient Implementation
//=============================================================================

TurnClient::TurnClient(const std::string& server, uint16_t port, 
                      const std::string& username, const std::string& password)
    : server_(server), port_(port), username_(username), password_(password),
      socket_(INVALID_SOCKET_VALUE), allocated_(false), allocated_port_(0) {
    
    last_refresh_ = std::chrono::steady_clock::now();
}

TurnClient::~TurnClient() {
    deallocate();
}

bool TurnClient::allocate_relay(std::string& allocated_ip, uint16_t& allocated_port) {
    LOG_ICE_INFO("Allocating TURN relay on " << server_ << ":" << port_);
    
    // Initialize socket library (safe to call multiple times)
    if (!init_socket_library()) {
        LOG_ICE_ERROR("Failed to initialize socket library");
        return false;
    }
    
    // Create UDP socket for TURN
    socket_ = create_udp_socket_v4(0);
    if (!is_valid_socket(socket_)) {
        LOG_ICE_ERROR("Failed to create socket for TURN client");
        return false;
    }
    
    if (!send_allocate_request()) {
        LOG_ICE_ERROR("Failed to send TURN allocate request");
        close_socket(socket_);
        socket_ = INVALID_SOCKET_VALUE;
        return false;
    }
    
    // Wait for response
    std::vector<uint8_t> response;
    Peer sender;
    response = receive_udp_data(socket_, 1500, sender);
    
    if (response.empty()) {
        LOG_ICE_ERROR("No response from TURN server");
        close_socket(socket_);
        socket_ = INVALID_SOCKET_VALUE;
        return false;
    }
    
    if (!handle_allocate_response(response)) {
        LOG_ICE_ERROR("Failed to handle TURN allocate response");
        close_socket(socket_);
        socket_ = INVALID_SOCKET_VALUE;
        return false;
    }
    
    allocated_ = true;
    allocated_ip = allocated_ip_;
    allocated_port = allocated_port_;
    
    LOG_ICE_INFO("TURN relay allocated: " << allocated_ip_ << ":" << allocated_port_);
    return true;
}

bool TurnClient::create_permission(const std::string& peer_ip) {
    if (!allocated_) {
        LOG_ICE_ERROR("TURN relay not allocated");
        return false;
    }
    
    LOG_ICE_DEBUG("Creating TURN permission for " << peer_ip);
    
    // Create permission request - simplified implementation
    // In a full implementation, this would send a proper TURN CreatePermission request
    return true;
}

bool TurnClient::send_data(const std::vector<uint8_t>& data, const std::string& peer_ip, uint16_t peer_port) {
    if (!allocated_) {
        LOG_ICE_ERROR("TURN relay not allocated");
        return false;
    }
    
    // Send data through TURN relay - simplified implementation
    // In a full implementation, this would use TURN Send indication or ChannelData
    return send_udp_data_to(socket_, data, peer_ip, peer_port) > 0;
}

std::vector<uint8_t> TurnClient::receive_data(std::string& from_ip, uint16_t& from_port, int timeout_ms) {
    if (!allocated_) {
        return {};
    }
    
    // Receive data through TURN relay - simplified implementation
    std::vector<uint8_t> response = receive_udp_data_with_timeout(socket_, 1500, timeout_ms, &from_ip, reinterpret_cast<int*>(&from_port));
    return response;
}

void TurnClient::refresh_allocation() {
    if (!allocated_) {
        return;
    }
    
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::minutes>(now - last_refresh_);
    
    if (duration.count() >= 5) { // Refresh every 5 minutes
        send_refresh_request();
        last_refresh_ = now;
    }
}

void TurnClient::deallocate() {
    if (allocated_) {
        LOG_ICE_DEBUG("Deallocating TURN relay");
        // Send deallocate request - simplified implementation
        allocated_ = false;
    }
    
    if (is_valid_socket(socket_)) {
        close_socket(socket_);
        socket_ = INVALID_SOCKET_VALUE;
    }
}

bool TurnClient::send_allocate_request() {
    // Simplified TURN allocate request
    // In a full implementation, this would follow RFC 5766
    std::vector<uint8_t> request(20); // Basic STUN header
    
    // STUN message type: Allocate Request (0x003)
    request[0] = 0x00;
    request[1] = 0x03;
    
    // Message length (will be filled)
    request[2] = 0x00;
    request[3] = 0x00;
    
    // Magic cookie
    request[4] = 0x21;
    request[5] = 0x12;
    request[6] = 0xA4;
    request[7] = 0x42;
    
    // Transaction ID (random)
    std::random_device rd;
    std::mt19937 gen(rd());
    for (int i = 8; i < 20; ++i) {
        request[i] = static_cast<uint8_t>(gen() % 256);
    }
    
    return send_udp_data_to(socket_, request, server_, port_) > 0;
}

bool TurnClient::send_refresh_request() {
    // Simplified refresh request
    return true;
}

bool TurnClient::handle_allocate_response(const std::vector<uint8_t>& response) {
    if (response.size() < 20) {
        return false;
    }
    
    // Check if it's a success response (0x0103)
    if (response[0] == 0x01 && response[1] == 0x03) {
        // Parse XOR-RELAYED-ADDRESS attribute (simplified)
        allocated_ip_ = "127.0.0.1"; // Placeholder
        allocated_port_ = 12345;     // Placeholder
        return true;
    }
    
    return false;
}

bool TurnClient::authenticate_with_server() {
    // Simplified authentication
    return true;
}

//=============================================================================
// NatTypeDetector Implementation
//=============================================================================

NatTypeDetector::NatTypeDetector() {
    LOG_ICE_DEBUG("NAT type detector created");
}

NatTypeDetector::~NatTypeDetector() {
    LOG_ICE_DEBUG("NAT type detector destroyed");
}

NatType NatTypeDetector::detect_nat_type(const std::vector<std::string>& stun_servers, int timeout_ms) {
    LOG_ICE_INFO("Starting NAT type detection");
    
    if (stun_servers.empty()) {
        LOG_ICE_ERROR("No STUN servers provided for NAT detection");
        return NatType::UNKNOWN;
    }
    
    std::string server1 = stun_servers[0];
    std::string server2 = stun_servers.size() > 1 ? stun_servers[1] : server1;
    
    // Test 1: Check if UDP is blocked
    if (test_udp_blocked(server1, timeout_ms)) {
        LOG_ICE_INFO("NAT type detected: UDP BLOCKED");
        return NatType::BLOCKED;
    }
    
    // Test 2: Check for open internet (no NAT)
    if (test_open_internet(server1, timeout_ms)) {
        LOG_ICE_INFO("NAT type detected: OPEN INTERNET");
        return NatType::OPEN_INTERNET;
    }
    
    // Test 3: Check for symmetric NAT
    if (test_symmetric_nat(server1, server2, timeout_ms)) {
        LOG_ICE_INFO("NAT type detected: SYMMETRIC NAT");
        return NatType::SYMMETRIC;
    }
    
    // Test 4: Check for full cone NAT
    if (test_full_cone(server1, server2, timeout_ms)) {
        LOG_ICE_INFO("NAT type detected: FULL CONE NAT");
        return NatType::FULL_CONE;
    }
    
    // Default to port restricted cone NAT
    LOG_ICE_INFO("NAT type detected: PORT RESTRICTED NAT");
    return NatType::PORT_RESTRICTED;
}

std::string NatTypeDetector::nat_type_to_string(NatType type) const {
    switch (type) {
        case NatType::UNKNOWN: return "Unknown";
        case NatType::OPEN_INTERNET: return "Open Internet";
        case NatType::FULL_CONE: return "Full Cone NAT";
        case NatType::RESTRICTED_CONE: return "Restricted Cone NAT";
        case NatType::PORT_RESTRICTED: return "Port Restricted Cone NAT";
        case NatType::SYMMETRIC: return "Symmetric NAT";
        case NatType::BLOCKED: return "UDP Blocked";
        default: return "Unknown";
    }
}

bool NatTypeDetector::test_udp_blocked(const std::string& stun_server, int timeout_ms) {
    size_t colon_pos = stun_server.find(':');
    std::string host = stun_server.substr(0, colon_pos);
    int port = colon_pos != std::string::npos ? std::stoi(stun_server.substr(colon_pos + 1)) : 3478;
    
    StunClient stun_client;
    StunAddress public_address;
    
    return !stun_client.get_public_address(host, port, public_address, timeout_ms);
}

bool NatTypeDetector::test_open_internet(const std::string& stun_server, int timeout_ms) {
    StunAddress mapped_addr = get_mapped_address(stun_server, timeout_ms);
    if (mapped_addr.ip.empty()) {
        return false;
    }
    
    // Get local addresses
    auto local_addrs = network_utils::get_local_interface_addresses();
    
    // Check if mapped address matches any local address
    return std::find(local_addrs.begin(), local_addrs.end(), mapped_addr.ip) != local_addrs.end();
}

bool NatTypeDetector::test_full_cone(const std::string& stun_server1, const std::string& stun_server2, int timeout_ms) {
    StunAddress addr1 = get_mapped_address(stun_server1, timeout_ms);
    StunAddress addr2 = get_mapped_address(stun_server2, timeout_ms);
    
    if (addr1.ip.empty() || addr2.ip.empty()) {
        return false;
    }
    
    // In full cone NAT, the mapped address should be the same from different servers
    return addr1.ip == addr2.ip && addr1.port == addr2.port;
}

bool NatTypeDetector::test_symmetric_nat(const std::string& stun_server1, const std::string& stun_server2, int timeout_ms) {
    StunAddress addr1 = get_mapped_address(stun_server1, timeout_ms);
    StunAddress addr2 = get_mapped_address(stun_server2, timeout_ms);
    
    if (addr1.ip.empty() || addr2.ip.empty()) {
        return false;
    }
    
    // In symmetric NAT, the mapped address changes for different destinations
    return addr1.ip != addr2.ip || addr1.port != addr2.port;
}

StunAddress NatTypeDetector::get_mapped_address(const std::string& stun_server, int timeout_ms) {
    size_t colon_pos = stun_server.find(':');
    std::string host = stun_server.substr(0, colon_pos);
    int port = colon_pos != std::string::npos ? std::stoi(stun_server.substr(colon_pos + 1)) : 3478;
    
    StunClient stun_client;
    StunAddress public_address;
    
    if (stun_client.get_public_address(host, port, public_address, timeout_ms)) {
        return public_address;
    }
    
    return StunAddress{}; // Empty address on failure
}

StunAddress NatTypeDetector::get_mapped_address_different_port(const std::string& stun_server, int timeout_ms) {
    // This would use a different port on the same server for testing
    return get_mapped_address(stun_server, timeout_ms);
}

//=============================================================================
// IceAgent Implementation
//=============================================================================

IceAgent::IceAgent(IceRole role, const IceConfig& config)
    : role_(role), config_(config), running_(false), state_(IceConnectionState::NEW),
      udp_socket_(INVALID_SOCKET_VALUE), tcp_socket_(INVALID_SOCKET_VALUE),
      selected_pair_(IceCandidate(), IceCandidate()) {
    
    // Initialize NAT detector
    nat_detector_ = std::make_unique<NatTypeDetector>();
    
    // Generate local credentials
    local_ufrag_ = generate_ufrag();
    local_pwd_ = generate_password();
    
    LOG_ICE_INFO("ICE Agent created with role: " << (role_ == IceRole::CONTROLLING ? "CONTROLLING" : "CONTROLLED"));
}

IceAgent::~IceAgent() {
    stop();
}

bool IceAgent::start() {
    if (running_.load()) {
        LOG_ICE_WARN("ICE Agent is already running");
        return false;
    }
    
    LOG_ICE_INFO("Starting ICE Agent");
    
    // Initialize socket library (safe to call multiple times)
    if (!init_socket_library()) {
        LOG_ICE_ERROR("Failed to initialize socket library");
        return false;
    }
    
    // Create UDP socket for ICE
    udp_socket_ = create_udp_socket_v4(0);
    if (!is_valid_socket(udp_socket_)) {
        LOG_ICE_ERROR("Failed to create UDP socket for ICE");
        return false;
    }
    
    // Set socket to non-blocking mode for receive loop
    if (!set_socket_nonblocking(udp_socket_)) {
        LOG_ICE_WARN("Failed to set ICE UDP socket to non-blocking mode");
    }
    
    running_.store(true);
    set_state(IceConnectionState::NEW);
    
    // Start receive thread
    receive_thread_ = std::thread(&IceAgent::receive_loop, this);
    
    LOG_ICE_INFO("ICE Agent started successfully");
    return true;
}

void IceAgent::stop() {
    if (!running_.load()) {
        return;
    }
    
    LOG_ICE_INFO("Stopping ICE Agent");
    
    // Trigger immediate shutdown of all background threads
    shutdown_immediate();
    
    // Close sockets
    if (is_valid_socket(udp_socket_)) {
        close_socket(udp_socket_);
        udp_socket_ = INVALID_SOCKET_VALUE;
    }
    
    if (is_valid_socket(tcp_socket_)) {
        close_socket(tcp_socket_);
        tcp_socket_ = INVALID_SOCKET_VALUE;
    }
    
    // Wait for threads to finish
    if (gather_thread_.joinable()) {
        gather_thread_.join();
    }
    
    if (check_thread_.joinable()) {
        check_thread_.join();
    }
    
    if (receive_thread_.joinable()) {
        receive_thread_.join();
    }
    
    // Clean up TURN client
    turn_client_.reset();
    
    LOG_ICE_INFO("ICE Agent stopped");
}

void IceAgent::shutdown_immediate() {
    LOG_ICE_INFO("Triggering immediate shutdown of ICE background threads");
    
    running_.store(false);
    set_state(IceConnectionState::CLOSED);
    
    // Notify all waiting threads to wake up immediately
    shutdown_cv_.notify_all();
}

void IceAgent::set_local_credentials(const std::string& ufrag, const std::string& pwd) {
    local_ufrag_ = ufrag;
    local_pwd_ = pwd;
    LOG_ICE_DEBUG("Set local credentials: ufrag=" << ufrag);
}

void IceAgent::set_remote_credentials(const std::string& ufrag, const std::string& pwd) {
    remote_ufrag_ = ufrag;
    remote_pwd_ = pwd;
    LOG_ICE_DEBUG("Set remote credentials: ufrag=" << ufrag);
}

std::pair<std::string, std::string> IceAgent::get_local_credentials() const {
    return {local_ufrag_, local_pwd_};
}

void IceAgent::gather_candidates() {
    if (!running_.load()) {
        LOG_ICE_ERROR("ICE Agent not running");
        return;
    }
    
    LOG_ICE_INFO("Starting candidate gathering");
    set_state(IceConnectionState::GATHERING);
    
    // Start gathering in a separate thread
    gather_thread_ = std::thread([this]() {
        std::lock_guard<std::mutex> lock(candidates_mutex_);
        local_candidates_.clear();
        
        if (config_.enable_host_candidates) {
            gather_host_candidates();
        }
        
        if (config_.enable_server_reflexive_candidates) {
            gather_server_reflexive_candidates();
        }
        
        if (config_.enable_relay_candidates) {
            gather_relay_candidates();
        }
        
        if (config_.enable_tcp_candidates) {
            gather_tcp_candidates();
        }
        
        LOG_ICE_INFO("Candidate gathering completed. Found " << local_candidates_.size() << " candidates");
        
        // Notify about gathered candidates
        if (candidate_callback_) {
            for (const auto& candidate : local_candidates_) {
                candidate_callback_(candidate);
            }
        }
    });
}

std::vector<IceCandidate> IceAgent::get_local_candidates() const {
    std::lock_guard<std::mutex> lock(candidates_mutex_);
    return local_candidates_;
}

void IceAgent::add_remote_candidate(const IceCandidate& candidate) {
    std::lock_guard<std::mutex> lock(candidates_mutex_);
    remote_candidates_.push_back(candidate);
    LOG_ICE_DEBUG("Added remote candidate: " << candidate.ip << ":" << candidate.port);
    
    // Form new candidate pairs
    form_candidate_pairs();
}

void IceAgent::add_remote_candidates(const std::vector<IceCandidate>& candidates) {
    std::lock_guard<std::mutex> lock(candidates_mutex_);
    for (const auto& candidate : candidates) {
        remote_candidates_.push_back(candidate);
        LOG_ICE_DEBUG("Added remote candidate: " << candidate.ip << ":" << candidate.port);
    }
    
    // Form new candidate pairs
    form_candidate_pairs();
}

void IceAgent::start_connectivity_checks() {
    if (!running_.load()) {
        LOG_ICE_ERROR("ICE Agent not running");
        return;
    }
    
    // Check if connectivity checks are already running
    if (check_thread_.joinable()) {
        LOG_ICE_WARN("Connectivity checks already running, skipping duplicate start");
        return;
    }
    
    LOG_ICE_INFO("Starting connectivity checks");
    set_state(IceConnectionState::CHECKING);
    
    // Start connectivity checks in a separate thread
    check_thread_ = std::thread(&IceAgent::connectivity_check_loop, this);
}

void IceAgent::restart_ice() {
    LOG_ICE_INFO("Restarting ICE");
    
    // Clear previous state
    {
        std::lock_guard<std::mutex> lock(candidates_mutex_);
        local_candidates_.clear();
        remote_candidates_.clear();
    }
    
    {
        std::lock_guard<std::mutex> lock(pairs_mutex_);
        candidate_pairs_.clear();
    }
    
    // Generate new credentials
    local_ufrag_ = generate_ufrag();
    local_pwd_ = generate_password();
    
    // Restart gathering
    set_state(IceConnectionState::NEW);
    gather_candidates();
}

bool IceAgent::send_data(const std::vector<uint8_t>& data) {
    if (!is_connected()) {
        LOG_ICE_ERROR("ICE not connected, cannot send data");
        return false;
    }
    
    // Send using selected pair
    IceCandidatePair selected = [this]() {
        std::lock_guard<std::mutex> lock(pairs_mutex_);
        return selected_pair_;
    }();
    
    if (selected.local.type == IceCandidateType::RELAY && turn_client_) {
        return turn_client_->send_data(data, selected.remote.ip, selected.remote.port);
    } else {
        return send_udp_data_to(udp_socket_, data, selected.remote.ip, selected.remote.port) > 0;
    }
}

bool IceAgent::send_data_to(const std::vector<uint8_t>& data, const std::string& addr) {
    // Parse address
    size_t colon_pos = addr.find(':');
    if (colon_pos == std::string::npos) {
        return false;
    }
    
    std::string ip = addr.substr(0, colon_pos);
    uint16_t port = static_cast<uint16_t>(std::stoi(addr.substr(colon_pos + 1)));
    
    return send_udp_data_to(udp_socket_, data, ip, port) > 0;
}

NatType IceAgent::detect_nat_type() {
    if (!nat_detector_) {
        return NatType::UNKNOWN;
    }
    
    return nat_detector_->detect_nat_type(config_.stun_servers, config_.stun_timeout_ms);
}

bool IceAgent::perform_hole_punching(const std::string& peer_ip, uint16_t peer_port) {
    LOG_ICE_INFO("Performing hole punching to " << peer_ip << ":" << peer_port);
    
    // Try UDP hole punching first
    if (udp_hole_punch(peer_ip, peer_port)) {
        return true;
    }
    
    // Try TCP hole punching if enabled
    if (config_.enable_tcp_candidates) {
        return tcp_hole_punch(peer_ip, peer_port);
    }
    
    return false;
}

bool IceAgent::coordinate_connection(const nlohmann::json& signaling_data) {
    LOG_ICE_INFO("Coordinating connection with signaling data");
    
    try {
        // Extract remote candidates from signaling data
        if (signaling_data.contains("candidates")) {
            std::vector<IceCandidate> remote_candidates;
            for (const auto& candidate_json : signaling_data["candidates"]) {
                remote_candidates.push_back(IceCandidate::from_json(candidate_json));
            }
            add_remote_candidates(remote_candidates);
        }
        
        // Extract remote credentials
        if (signaling_data.contains("ufrag") && signaling_data.contains("pwd")) {
            set_remote_credentials(signaling_data["ufrag"], signaling_data["pwd"]);
        }
        
        // Start connectivity checks
        start_connectivity_checks();
        
        return true;
    } catch (const std::exception& e) {
        LOG_ICE_ERROR("Failed to coordinate connection: " << e.what());
        return false;
    }
}

nlohmann::json IceAgent::get_local_description() const {
    nlohmann::json desc;
    desc["ufrag"] = local_ufrag_;
    desc["pwd"] = local_pwd_;
    
    std::lock_guard<std::mutex> lock(candidates_mutex_);
    nlohmann::json candidates = nlohmann::json::array();
    for (const auto& candidate : local_candidates_) {
        candidates.push_back(candidate.to_json());
    }
    desc["candidates"] = candidates;
    
    return desc;
}

bool IceAgent::set_remote_description(const nlohmann::json& remote_desc) {
    try {
        if (remote_desc.contains("ufrag") && remote_desc.contains("pwd")) {
            set_remote_credentials(remote_desc["ufrag"], remote_desc["pwd"]);
        }
        
        if (remote_desc.contains("candidates")) {
            std::vector<IceCandidate> candidates;
            for (const auto& candidate_json : remote_desc["candidates"]) {
                candidates.push_back(IceCandidate::from_json(candidate_json));
            }
            add_remote_candidates(candidates);
        }
        
        return true;
    } catch (const std::exception& e) {
        LOG_ICE_ERROR("Failed to set remote description: " << e.what());
        return false;
    }
}

nlohmann::json IceAgent::create_connection_offer() const {
    nlohmann::json offer;
    offer["type"] = "offer";
    offer["ice"] = get_local_description();
    offer["role"] = (role_ == IceRole::CONTROLLING) ? "controlling" : "controlled";
    return offer;
}

nlohmann::json IceAgent::create_connection_answer(const nlohmann::json& offer) const {
    nlohmann::json answer;
    answer["type"] = "answer";
    answer["ice"] = get_local_description();
    answer["role"] = (role_ == IceRole::CONTROLLING) ? "controlling" : "controlled";
    return answer;
}

std::vector<IceCandidatePair> IceAgent::get_candidate_pairs() const {
    std::lock_guard<std::mutex> lock(pairs_mutex_);
    return candidate_pairs_;
}

IceCandidatePair IceAgent::get_selected_pair() const {
    std::lock_guard<std::mutex> lock(pairs_mutex_);
    return selected_pair_;
}

nlohmann::json IceAgent::get_statistics() const {
    nlohmann::json stats;
    stats["state"] = ice_connection_state_to_string(state_.load());
    stats["role"] = (role_ == IceRole::CONTROLLING) ? "controlling" : "controlled";
    
    {
        std::lock_guard<std::mutex> lock(candidates_mutex_);
        stats["local_candidates"] = local_candidates_.size();
        stats["remote_candidates"] = remote_candidates_.size();
    }
    
    {
        std::lock_guard<std::mutex> lock(pairs_mutex_);
        stats["candidate_pairs"] = candidate_pairs_.size();
        if (selected_pair_.succeeded) {
            stats["selected_pair"] = {
                {"local", selected_pair_.local.ip + ":" + std::to_string(selected_pair_.local.port)},
                {"remote", selected_pair_.remote.ip + ":" + std::to_string(selected_pair_.remote.port)},
                {"priority", selected_pair_.priority}
            };
        }
    }
    
    return stats;
}

// Private methods implementation

void IceAgent::set_state(IceConnectionState new_state) {
    IceConnectionState old_state = state_.exchange(new_state);
    if (old_state != new_state) {
        LOG_ICE_INFO("ICE state changed: " << ice_connection_state_to_string(old_state) 
                     << " -> " << ice_connection_state_to_string(new_state));
        
        if (state_change_callback_) {
            state_change_callback_(new_state);
        }
    }
}

void IceAgent::gather_host_candidates() {
    LOG_ICE_DEBUG("Gathering host candidates");
    
    auto local_addresses = network_utils::get_local_interface_addresses();
    
    // Get actual port from socket
    uint16_t actual_port = 0;
    if (is_valid_socket(udp_socket_)) {
        sockaddr_storage addr;
        socklen_t addr_len = sizeof(addr);
        if (getsockname(udp_socket_, reinterpret_cast<sockaddr*>(&addr), &addr_len) == 0) {
            if (addr.ss_family == AF_INET) {
                sockaddr_in* addr_in = reinterpret_cast<sockaddr_in*>(&addr);
                actual_port = ntohs(addr_in->sin_port);
            } else if (addr.ss_family == AF_INET6) {
                sockaddr_in6* addr_in6 = reinterpret_cast<sockaddr_in6*>(&addr);
                actual_port = ntohs(addr_in6->sin6_port);
            }
        }
    }
    
    for (const auto& ip : local_addresses) {
        if (ip == "127.0.0.1" || ip == "::1") {
            continue; // Skip loopback
        }
        
        IceCandidate candidate;
        candidate.foundation = generate_foundation(candidate);
        candidate.component_id = 1;
        candidate.transport = IceTransport::UDP;
        candidate.priority = calculate_candidate_priority(IceCandidateType::HOST, 65535, 1);
        candidate.ip = ip;
        candidate.port = actual_port;
        candidate.type = IceCandidateType::HOST;
        candidate.ufrag = local_ufrag_;
        candidate.pwd = local_pwd_;
        
        local_candidates_.push_back(candidate);
        LOG_ICE_DEBUG("Added host candidate: " << candidate.ip << ":" << candidate.port);
    }
}

void IceAgent::gather_server_reflexive_candidates() {
    LOG_ICE_DEBUG("Gathering server reflexive candidates");
    
    for (const auto& stun_server : config_.stun_servers) {
        size_t colon_pos = stun_server.find(':');
        std::string host = stun_server.substr(0, colon_pos);
        int port = colon_pos != std::string::npos ? std::stoi(stun_server.substr(colon_pos + 1)) : 3478;
        
        StunClient stun_client;
        StunAddress public_address;
        
        if (stun_client.get_public_address(host, port, public_address, config_.stun_timeout_ms)) {
            IceCandidate candidate;
            candidate.foundation = generate_foundation(candidate);
            candidate.component_id = 1;
            candidate.transport = IceTransport::UDP;
            candidate.priority = calculate_candidate_priority(IceCandidateType::SERVER_REFLEXIVE, 65534, 1);
            candidate.ip = public_address.ip;
            candidate.port = public_address.port;
            candidate.type = IceCandidateType::SERVER_REFLEXIVE;
            candidate.ufrag = local_ufrag_;
            candidate.pwd = local_pwd_;
            
            // Set related address (base)
            if (!local_candidates_.empty()) {
                candidate.related_ip = local_candidates_[0].ip;
                candidate.related_port = local_candidates_[0].port;
            }
            
            local_candidates_.push_back(candidate);
            LOG_ICE_DEBUG("Added server reflexive candidate: " << candidate.ip << ":" << candidate.port);
            break; // Only need one reflexive candidate
        }
    }
}

void IceAgent::gather_relay_candidates() {
    LOG_ICE_DEBUG("Gathering relay candidates");
    
    if (config_.turn_servers.empty()) {
        LOG_ICE_DEBUG("No TURN servers configured");
        return;
    }
    
    // Use first TURN server
    std::string turn_server = config_.turn_servers[0];
    size_t colon_pos = turn_server.find(':');
    std::string host = turn_server.substr(0, colon_pos);
    uint16_t port = colon_pos != std::string::npos ? static_cast<uint16_t>(std::stoi(turn_server.substr(colon_pos + 1))) : 3478;
    
    std::string username = config_.turn_usernames.empty() ? "" : config_.turn_usernames[0];
    std::string password = config_.turn_passwords.empty() ? "" : config_.turn_passwords[0];
    
    turn_client_ = std::make_unique<TurnClient>(host, port, username, password);
    
    std::string allocated_ip;
    uint16_t allocated_port;
    
    if (turn_client_->allocate_relay(allocated_ip, allocated_port)) {
        IceCandidate candidate;
        candidate.foundation = generate_foundation(candidate);
        candidate.component_id = 1;
        candidate.transport = IceTransport::UDP;
        candidate.priority = calculate_candidate_priority(IceCandidateType::RELAY, 65533, 1);
        candidate.ip = allocated_ip;
        candidate.port = allocated_port;
        candidate.type = IceCandidateType::RELAY;
        candidate.ufrag = local_ufrag_;
        candidate.pwd = local_pwd_;
        candidate.turn_server = host;
        candidate.turn_port = port;
        
        // Set related address (TURN server)
        candidate.related_ip = host;
        candidate.related_port = port;
        
        local_candidates_.push_back(candidate);
        LOG_ICE_DEBUG("Added relay candidate: " << candidate.ip << ":" << candidate.port);
    }
}

void IceAgent::gather_tcp_candidates() {
    LOG_ICE_DEBUG("Gathering TCP candidates");
    
    // TCP candidate gathering would be implemented here
    // This is more complex and less commonly used
}

void IceAgent::connectivity_check_loop() {
    LOG_ICE_INFO("Starting connectivity check loop");
    
    auto start_time = std::chrono::steady_clock::now();
    auto timeout = std::chrono::milliseconds(config_.connectivity_check_timeout_ms);
    
    while (running_.load() && state_.load() == IceConnectionState::CHECKING) {
        auto now = std::chrono::steady_clock::now();
        if (now - start_time > timeout) {
            LOG_ICE_WARN("Connectivity checks timed out");
            set_state(IceConnectionState::FAILED);
            break;
        }
        
        std::vector<IceCandidatePair> pairs_to_check;
        {
            std::lock_guard<std::mutex> lock(pairs_mutex_);
            pairs_to_check = candidate_pairs_;
        }
        
        bool any_succeeded = false;
        for (auto& pair : pairs_to_check) {
            if (pair.check_count >= config_.max_connectivity_checks) {
                continue;
            }
            
            if (perform_connectivity_check(pair)) {
                pair.succeeded = true;
                any_succeeded = true;
                
                // Update the pair in the main list
                {
                    std::lock_guard<std::mutex> lock(pairs_mutex_);
                    for (auto& stored_pair : candidate_pairs_) {
                        if (stored_pair.local.ip == pair.local.ip && 
                            stored_pair.local.port == pair.local.port &&
                            stored_pair.remote.ip == pair.remote.ip && 
                            stored_pair.remote.port == pair.remote.port) {
                            stored_pair = pair;
                            break;
                        }
                    }
                }
                
                // Select this pair if it's the first successful one
                {
                    std::lock_guard<std::mutex> lock(pairs_mutex_);
                    if (!selected_pair_.succeeded) {
                        selected_pair_ = pair;
                        nominate_pair(pair);
                        
                        set_state(IceConnectionState::CONNECTED);
                        
                        std::string local_addr = pair.local.ip + ":" + std::to_string(pair.local.port);
                        std::string remote_addr = pair.remote.ip + ":" + std::to_string(pair.remote.port);
                        
                        if (connected_callback_) {
                            connected_callback_(local_addr, remote_addr);
                        }
                        
                        LOG_ICE_INFO("ICE connection established using pair: " << local_addr << " <-> " << remote_addr);
                        return;
                    }
                }
            }
            
            pair.check_count++;
            pair.last_check_time = now;
        }
        
        if (!any_succeeded && now - start_time > std::chrono::seconds(10)) {
            // Try hole punching for remaining pairs
            for (const auto& pair : pairs_to_check) {
                if (!pair.succeeded) {
                    perform_hole_punching(pair.remote.ip, pair.remote.port);
                }
            }
        }
        
        // Use conditional variable for responsive shutdown
        {
            std::unique_lock<std::mutex> lock(shutdown_mutex_);
            if (shutdown_cv_.wait_for(lock, std::chrono::milliseconds(100), [this] { return !running_.load(); })) {
                break;
            }
        }
    }
    
    if (state_.load() == IceConnectionState::CHECKING) {
        set_state(IceConnectionState::FAILED);
    }
}

bool IceAgent::perform_connectivity_check(IceCandidatePair& pair) {
    LOG_ICE_DEBUG("Performing connectivity check: " 
                  << pair.local.ip << ":" << pair.local.port << " -> " 
                  << pair.remote.ip << ":" << pair.remote.port);
    
    // Create STUN binding request for connectivity check
    std::vector<uint8_t> request = StunClient::create_binding_request();
    
    // Send request
    int sent = 0;
    if (pair.local.type == IceCandidateType::RELAY && turn_client_) {
        sent = turn_client_->send_data(request, pair.remote.ip, pair.remote.port) ? request.size() : 0;
    } else {
        sent = send_udp_data_to(udp_socket_, request, pair.remote.ip, pair.remote.port);
    }
    
    if (sent <= 0) {
        LOG_ICE_DEBUG("Failed to send connectivity check");
        return false;
    }
    
    // Wait for response (simplified)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // In a real implementation, we would wait for and validate the STUN response
    // For now, we'll consider the check successful if we could send the request
    return true;
}

void IceAgent::form_candidate_pairs() {
    std::lock_guard<std::mutex> lock(pairs_mutex_);
    
    candidate_pairs_.clear();
    
    for (const auto& local : local_candidates_) {
        for (const auto& remote : remote_candidates_) {
            // Only pair candidates of the same component and transport
            if (local.component_id == remote.component_id && 
                local.transport == remote.transport) {
                candidate_pairs_.emplace_back(local, remote);
            }
        }
    }
    
    // Sort pairs by priority (highest first)
    std::sort(candidate_pairs_.begin(), candidate_pairs_.end(),
              [](const IceCandidatePair& a, const IceCandidatePair& b) {
                  return a.priority > b.priority;
              });
    
    LOG_ICE_DEBUG("Formed " << candidate_pairs_.size() << " candidate pairs");
}

void IceAgent::prioritize_candidate_pairs() {
    std::lock_guard<std::mutex> lock(pairs_mutex_);
    
    // Recalculate priorities and sort
    for (auto& pair : candidate_pairs_) {
        pair.priority = pair.calculate_priority();
    }
    
    std::sort(candidate_pairs_.begin(), candidate_pairs_.end(),
              [](const IceCandidatePair& a, const IceCandidatePair& b) {
                  return a.priority > b.priority;
              });
}

void IceAgent::nominate_pair(IceCandidatePair& pair) {
    pair.nominated = true;
    LOG_ICE_INFO("Nominated candidate pair: " 
                 << pair.local.ip << ":" << pair.local.port << " <-> " 
                 << pair.remote.ip << ":" << pair.remote.port);
}

void IceAgent::receive_loop() {
    LOG_ICE_DEBUG("Starting ICE receive loop");
    
    while (running_.load()) {
        Peer sender;
        std::vector<uint8_t> data = receive_udp_data(udp_socket_, 1500, sender);
        
        if (!data.empty()) {
            std::string from_addr = sender.ip + ":" + std::to_string(sender.port);
            handle_incoming_data(data, from_addr);
        }
        
        // Refresh TURN allocation if needed
        if (turn_client_) {
            turn_client_->refresh_allocation();
        }
        
        // Use conditional variable for responsive shutdown
        {
            std::unique_lock<std::mutex> lock(shutdown_mutex_);
            if (shutdown_cv_.wait_for(lock, std::chrono::milliseconds(50), [this] { return !running_.load(); })) {
                break;
            }
        }
    }
    
    LOG_ICE_DEBUG("ICE receive loop ended");
}

void IceAgent::handle_incoming_data(const std::vector<uint8_t>& data, const std::string& from_addr) {
    if (is_stun_message(data)) {
        handle_stun_message(data, from_addr);
    } else if (data_callback_) {
        data_callback_(data, from_addr);
    }
}

bool IceAgent::is_stun_message(const std::vector<uint8_t>& data) {
    if (data.size() < 20) {
        return false;
    }
    
    // Check STUN magic cookie
    uint32_t magic_cookie = (static_cast<uint32_t>(data[4]) << 24) |
                           (static_cast<uint32_t>(data[5]) << 16) |
                           (static_cast<uint32_t>(data[6]) << 8) |
                           static_cast<uint32_t>(data[7]);
    
    return magic_cookie == 0x2112A442;
}

void IceAgent::handle_stun_message(const std::vector<uint8_t>& data, const std::string& from_addr) {
    LOG_ICE_DEBUG("Received STUN message from " << from_addr);
    
    // Parse and handle STUN message
    // This would include handling binding requests, responses, and ICE-specific attributes
    
    // For connectivity checks, we need to handle binding requests and send responses
    // For now, this is a simplified implementation
}

uint32_t IceAgent::calculate_candidate_priority(IceCandidateType type, uint16_t local_pref, uint16_t component_id) {
    uint8_t type_pref = 0;
    switch (type) {
        case IceCandidateType::HOST: type_pref = 126; break;
        case IceCandidateType::PEER_REFLEXIVE: type_pref = 110; break;
        case IceCandidateType::SERVER_REFLEXIVE: type_pref = 100; break;
        case IceCandidateType::RELAY: type_pref = 0; break;
    }
    
    return (static_cast<uint32_t>(type_pref) << 24) |
           (static_cast<uint32_t>(local_pref) << 8) |
           static_cast<uint32_t>(256 - component_id);
}

std::string IceAgent::generate_foundation(const IceCandidate& candidate) {
    // Simple foundation generation based on type and base address
    std::string base = ice_candidate_type_to_string(candidate.type) + "_" + candidate.ip;
    std::hash<std::string> hasher;
    return std::to_string(hasher(base) % 1000000);
}

std::string IceAgent::generate_ufrag() {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, sizeof(charset) - 2);
    
    std::string ufrag;
    for (int i = 0; i < 8; ++i) {
        ufrag += charset[dis(gen)];
    }
    return ufrag;
}

std::string IceAgent::generate_password() {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789+/";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, sizeof(charset) - 2);
    
    std::string password;
    for (int i = 0; i < 24; ++i) {
        password += charset[dis(gen)];
    }
    return password;
}

bool IceAgent::udp_hole_punch(const std::string& peer_ip, uint16_t peer_port) {
    LOG_ICE_DEBUG("Performing UDP hole punch to " << peer_ip << ":" << peer_port);
    
    // Send multiple packets to open the NAT hole
    std::vector<uint8_t> punch_data = {'P', 'U', 'N', 'C', 'H'};
    
    for (int i = 0; i < 5; ++i) {
        send_udp_data_to(udp_socket_, punch_data, peer_ip, peer_port);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    return true;
}

bool IceAgent::tcp_hole_punch(const std::string& peer_ip, uint16_t peer_port) {
    LOG_ICE_DEBUG("Performing TCP hole punch to " << peer_ip << ":" << peer_port);
    
    // TCP hole punching is more complex and requires coordination
    // This is a simplified implementation
    
    return false;
}

bool IceAgent::coordinate_hole_punch(const nlohmann::json& punch_data) {
    try {
        std::string peer_ip = punch_data.value("ip", "");
        uint16_t peer_port = punch_data.value("port", 0);
        
        if (!peer_ip.empty() && peer_port > 0) {
            return perform_hole_punching(peer_ip, peer_port);
        }
    } catch (const std::exception& e) {
        LOG_ICE_ERROR("Failed to coordinate hole punch: " << e.what());
    }
    
    return false;
}

//=============================================================================
// Utility Functions
//=============================================================================

std::string ice_candidate_type_to_string(IceCandidateType type) {
    switch (type) {
        case IceCandidateType::HOST: return "host";
        case IceCandidateType::SERVER_REFLEXIVE: return "srflx";
        case IceCandidateType::PEER_REFLEXIVE: return "prflx";
        case IceCandidateType::RELAY: return "relay";
        default: return "unknown";
    }
}

IceCandidateType string_to_ice_candidate_type(const std::string& type_str) {
    if (type_str == "host") return IceCandidateType::HOST;
    if (type_str == "srflx") return IceCandidateType::SERVER_REFLEXIVE;
    if (type_str == "prflx") return IceCandidateType::PEER_REFLEXIVE;
    if (type_str == "relay") return IceCandidateType::RELAY;
    return IceCandidateType::HOST;
}

std::string ice_transport_to_string(IceTransport transport) {
    switch (transport) {
        case IceTransport::UDP: return "udp";
        case IceTransport::TCP: return "tcp";
        default: return "udp";
    }
}

IceTransport string_to_ice_transport(const std::string& transport_str) {
    if (transport_str == "tcp") return IceTransport::TCP;
    return IceTransport::UDP;
}

std::string ice_connection_state_to_string(IceConnectionState state) {
    switch (state) {
        case IceConnectionState::NEW: return "new";
        case IceConnectionState::GATHERING: return "gathering";
        case IceConnectionState::CHECKING: return "checking";
        case IceConnectionState::CONNECTED: return "connected";
        case IceConnectionState::COMPLETED: return "completed";
        case IceConnectionState::FAILED: return "failed";
        case IceConnectionState::DISCONNECTED: return "disconnected";
        case IceConnectionState::CLOSED: return "closed";
        default: return "unknown";
    }
}

} // namespace librats 