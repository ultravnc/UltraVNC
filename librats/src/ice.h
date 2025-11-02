#ifndef LIBRATS_ICE_H
#define LIBRATS_ICE_H

#include "socket.h"
#include "stun.h"
#include "json.hpp"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <atomic>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <condition_variable>

namespace librats {

// ICE Candidate Types
enum class IceCandidateType {
    HOST,               // Local interface address
    SERVER_REFLEXIVE,   // Public address discovered via STUN
    PEER_REFLEXIVE,     // Address discovered during connectivity checks
    RELAY               // Address allocated on TURN server
};

// ICE Candidate Transport Protocol
enum class IceTransport {
    UDP,
    TCP
};

// ICE Candidate Structure
struct IceCandidate {
    std::string foundation;     // Foundation for grouping candidates
    uint32_t component_id;      // Component identifier (1 for RTP, 2 for RTCP, etc.)
    IceTransport transport;     // Transport protocol
    uint32_t priority;          // Candidate priority
    std::string ip;             // IP address
    uint16_t port;              // Port number
    IceCandidateType type;      // Candidate type
    std::string related_ip;     // Related address (for reflexive/relay candidates)
    uint16_t related_port;      // Related port
    std::string ufrag;          // Username fragment
    std::string pwd;            // Password
    
    // Optional TURN server info (for relay candidates)
    std::string turn_server;
    uint16_t turn_port;
    
    IceCandidate();
    std::string to_sdp() const;
    static IceCandidate from_sdp(const std::string& sdp_line);
    nlohmann::json to_json() const;
    static IceCandidate from_json(const nlohmann::json& json);
};

// ICE Candidate Pair for connectivity checks
struct IceCandidatePair {
    IceCandidate local;
    IceCandidate remote;
    uint64_t priority;
    bool nominated;
    std::chrono::steady_clock::time_point last_check_time;
    int check_count;
    bool succeeded;
    
    IceCandidatePair(const IceCandidate& local, const IceCandidate& remote);
    uint64_t calculate_priority() const;
};

// ICE Connection State
enum class IceConnectionState {
    NEW,
    GATHERING,
    CHECKING,
    CONNECTED,
    COMPLETED,
    FAILED,
    DISCONNECTED,
    CLOSED
};

// ICE Role
enum class IceRole {
    CONTROLLING,
    CONTROLLED
};

// NAT Type Detection Results
enum class NatType {
    UNKNOWN,
    OPEN_INTERNET,      // No NAT
    FULL_CONE,          // Full cone NAT
    RESTRICTED_CONE,    // Restricted cone NAT
    PORT_RESTRICTED,    // Port restricted cone NAT
    SYMMETRIC,          // Symmetric NAT
    BLOCKED             // UDP blocked
};

// ICE Configuration
struct IceConfig {
    std::vector<std::string> stun_servers;
    std::vector<std::string> turn_servers;
    std::vector<std::string> turn_usernames;
    std::vector<std::string> turn_passwords;
    bool enable_host_candidates;
    bool enable_server_reflexive_candidates;
    bool enable_relay_candidates;
    bool enable_tcp_candidates;
    int stun_timeout_ms;
    int turn_timeout_ms;
    int connectivity_check_timeout_ms;
    int max_connectivity_checks;
    
    IceConfig();
};

// ICE Events
class IceAgent;
using IceCandidateCallback = std::function<void(const IceCandidate&)>;
using IceStateChangeCallback = std::function<void(IceConnectionState)>;
using IceConnectedCallback = std::function<void(const std::string& local_addr, const std::string& remote_addr)>;
using IceDataCallback = std::function<void(const std::vector<uint8_t>&, const std::string& from_addr)>;

// TURN Client for relay candidates
class TurnClient {
public:
    TurnClient(const std::string& server, uint16_t port, 
               const std::string& username, const std::string& password);
    ~TurnClient();
    
    bool allocate_relay(std::string& allocated_ip, uint16_t& allocated_port);
    bool create_permission(const std::string& peer_ip);
    bool send_data(const std::vector<uint8_t>& data, const std::string& peer_ip, uint16_t peer_port);
    std::vector<uint8_t> receive_data(std::string& from_ip, uint16_t& from_port, int timeout_ms = 1000);
    void refresh_allocation();
    void deallocate();
    
    bool is_allocated() const { return allocated_; }
    std::string get_allocated_ip() const { return allocated_ip_; }
    uint16_t get_allocated_port() const { return allocated_port_; }

private:
    std::string server_;
    uint16_t port_;
    std::string username_;
    std::string password_;
    socket_t socket_;
    bool allocated_;
    std::string allocated_ip_;
    uint16_t allocated_port_;
    std::string realm_;
    std::string nonce_;
    std::chrono::steady_clock::time_point last_refresh_;
    
    bool send_allocate_request();
    bool send_refresh_request();
    bool handle_allocate_response(const std::vector<uint8_t>& response);
    bool authenticate_with_server();
};

// NAT Type Detector
class NatTypeDetector {
public:
    NatTypeDetector();
    ~NatTypeDetector();
    
    NatType detect_nat_type(const std::vector<std::string>& stun_servers, int timeout_ms = 5000);
    std::string nat_type_to_string(NatType type) const;
    
private:
    bool test_udp_blocked(const std::string& stun_server, int timeout_ms);
    bool test_open_internet(const std::string& stun_server, int timeout_ms);
    bool test_full_cone(const std::string& stun_server1, const std::string& stun_server2, int timeout_ms);
    bool test_symmetric_nat(const std::string& stun_server1, const std::string& stun_server2, int timeout_ms);
    
    StunAddress get_mapped_address(const std::string& stun_server, int timeout_ms);
    StunAddress get_mapped_address_different_port(const std::string& stun_server, int timeout_ms);
};

// Main ICE Agent
class IceAgent {
public:
    IceAgent(IceRole role, const IceConfig& config = IceConfig());
    ~IceAgent();
    
    // Lifecycle
    bool start();
    void stop();
    void shutdown_immediate();
    bool is_running() const { return running_.load(); }
    
    // Configuration
    void set_config(const IceConfig& config) { config_ = config; }
    IceConfig get_config() const { return config_; }
    void set_role(IceRole role) { role_ = role; }
    IceRole get_role() const { return role_; }
    
    // Credentials
    void set_local_credentials(const std::string& ufrag, const std::string& pwd);
    void set_remote_credentials(const std::string& ufrag, const std::string& pwd);
    std::pair<std::string, std::string> get_local_credentials() const;
    
    // Candidate gathering
    void gather_candidates();
    std::vector<IceCandidate> get_local_candidates() const;
    void add_remote_candidate(const IceCandidate& candidate);
    void add_remote_candidates(const std::vector<IceCandidate>& candidates);
    
    // Connectivity establishment
    void start_connectivity_checks();
    void restart_ice();
    
    // Data transmission
    bool send_data(const std::vector<uint8_t>& data);
    bool send_data_to(const std::vector<uint8_t>& data, const std::string& addr);
    
    // State
    IceConnectionState get_connection_state() const { return state_.load(); }
    bool is_connected() const { return state_.load() == IceConnectionState::CONNECTED || 
                                       state_.load() == IceConnectionState::COMPLETED; }
    
    // NAT traversal utilities
    NatType detect_nat_type();
    bool perform_hole_punching(const std::string& peer_ip, uint16_t peer_port);
    bool coordinate_connection(const nlohmann::json& signaling_data);
    
    // Callbacks
    void set_candidate_callback(IceCandidateCallback callback) { candidate_callback_ = callback; }
    void set_state_change_callback(IceStateChangeCallback callback) { state_change_callback_ = callback; }
    void set_connected_callback(IceConnectedCallback callback) { connected_callback_ = callback; }
    void set_data_callback(IceDataCallback callback) { data_callback_ = callback; }
    
    // Signaling support
    nlohmann::json get_local_description() const;
    bool set_remote_description(const nlohmann::json& remote_desc);
    nlohmann::json create_connection_offer() const;
    nlohmann::json create_connection_answer(const nlohmann::json& offer) const;
    
    // Statistics and debugging
    std::vector<IceCandidatePair> get_candidate_pairs() const;
    IceCandidatePair get_selected_pair() const;
    nlohmann::json get_statistics() const;

private:
    IceRole role_;
    IceConfig config_;
    std::atomic<bool> running_;
    std::atomic<IceConnectionState> state_;
    
    // Credentials
    std::string local_ufrag_;
    std::string local_pwd_;
    std::string remote_ufrag_;
    std::string remote_pwd_;
    
    // Candidates
    std::vector<IceCandidate> local_candidates_;
    std::vector<IceCandidate> remote_candidates_;
    mutable std::mutex candidates_mutex_;
    
    // Candidate pairs and connectivity checks
    std::vector<IceCandidatePair> candidate_pairs_;
    IceCandidatePair selected_pair_;
    mutable std::mutex pairs_mutex_;
    
    // Networking
    socket_t udp_socket_;
    socket_t tcp_socket_;
    std::unique_ptr<TurnClient> turn_client_;
    std::unique_ptr<NatTypeDetector> nat_detector_;
    
    // Threading
    std::thread gather_thread_;
    std::thread check_thread_;
    std::thread receive_thread_;
    
    // Conditional variables for immediate shutdown
    std::condition_variable shutdown_cv_;
    std::mutex shutdown_mutex_;
    
    // Callbacks
    IceCandidateCallback candidate_callback_;
    IceStateChangeCallback state_change_callback_;
    IceConnectedCallback connected_callback_;
    IceDataCallback data_callback_;
    
    // Internal methods
    void set_state(IceConnectionState new_state);
    void gather_host_candidates();
    void gather_server_reflexive_candidates();
    void gather_relay_candidates();
    void gather_tcp_candidates();
    
    void connectivity_check_loop();
    bool perform_connectivity_check(IceCandidatePair& pair);
    void form_candidate_pairs();
    void prioritize_candidate_pairs();
    void nominate_pair(IceCandidatePair& pair);
    
    void receive_loop();
    void handle_incoming_data(const std::vector<uint8_t>& data, const std::string& from_addr);
    bool is_stun_message(const std::vector<uint8_t>& data);
    void handle_stun_message(const std::vector<uint8_t>& data, const std::string& from_addr);
    
    uint32_t calculate_candidate_priority(IceCandidateType type, uint16_t local_pref, uint16_t component_id);
    std::string generate_foundation(const IceCandidate& candidate);
    std::string generate_ufrag();
    std::string generate_password();
    
    // Hole punching helpers
    bool udp_hole_punch(const std::string& peer_ip, uint16_t peer_port);
    bool tcp_hole_punch(const std::string& peer_ip, uint16_t peer_port);
    bool coordinate_hole_punch(const nlohmann::json& punch_data);
};

// Utility functions
std::string ice_candidate_type_to_string(IceCandidateType type);
IceCandidateType string_to_ice_candidate_type(const std::string& type_str);
std::string ice_transport_to_string(IceTransport transport);
IceTransport string_to_ice_transport(const std::string& transport_str);
std::string ice_connection_state_to_string(IceConnectionState state);

} // namespace librats

#endif // LIBRATS_ICE_H 