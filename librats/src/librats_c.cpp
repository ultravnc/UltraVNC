#include "librats_c.h"
#include "librats.h"
#include "logger.h"
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <cctype>
#include <unordered_map>

using namespace librats;

struct rats_client_wrapper {
    std::unique_ptr<RatsClient> client;

    // Stored C callbacks and user data
    rats_connection_cb connection_cb = nullptr;
    void* connection_ud = nullptr;

    rats_string_cb string_cb = nullptr;
    void* string_ud = nullptr;

    rats_binary_cb binary_cb = nullptr;
    void* binary_ud = nullptr;

    rats_json_cb json_cb = nullptr;
    void* json_ud = nullptr;

    rats_disconnect_cb disconnect_cb = nullptr;
    void* disconnect_ud = nullptr;

    rats_peer_discovered_cb peer_discovered_cb = nullptr;
    void* peer_discovered_ud = nullptr;

    rats_file_progress_cb file_progress_cb = nullptr;
    void* file_progress_ud = nullptr;

    // File transfer callbacks
    rats_file_request_cb file_request_cb = nullptr;
    void* file_request_ud = nullptr;
    
    rats_directory_request_cb directory_request_cb = nullptr;
    void* directory_request_ud = nullptr;
    
    rats_directory_progress_cb directory_progress_cb = nullptr;
    void* directory_progress_ud = nullptr;

    // Message handlers - using a simple map for demonstration
    std::unordered_map<std::string, std::pair<rats_message_cb, void*>> message_handlers;
    
    // GossipSub topic callbacks
    std::unordered_map<std::string, std::pair<rats_topic_message_cb, void*>> topic_message_handlers;
    std::unordered_map<std::string, std::pair<rats_topic_json_message_cb, void*>> topic_json_message_handlers;
    std::unordered_map<std::string, std::pair<rats_topic_peer_joined_cb, void*>> topic_peer_joined_handlers;
    std::unordered_map<std::string, std::pair<rats_topic_peer_left_cb, void*>> topic_peer_left_handlers;
};

static char* rats_strdup_owned(const std::string& s) {
    size_t n = s.size();
    char* out = static_cast<char*>(malloc(n + 1));
    if (!out) return nullptr;
    memcpy(out, s.c_str(), n);
    out[n] = '\0';
    return out;
}

extern "C" {

void rats_string_free(const char* str) {
    if (str) free((void*)str);
}

const char* rats_get_version_string(void) {
    return rats_get_library_version_string();
}

void rats_get_version(int* major, int* minor, int* patch, int* build) {
    rats_get_library_version(major, minor, patch, build);
}

const char* rats_get_git_describe(void) {
    return rats_get_library_git_describe();
}

uint32_t rats_get_abi(void) {
    return rats_get_library_abi();
}

rats_client_t rats_create(int listen_port) {
    try {
        rats_client_wrapper* wrap = new rats_client_wrapper();
        wrap->client = std::make_unique<RatsClient>(listen_port);
        return static_cast<rats_client_t>(wrap);
    } catch (...) {
        return nullptr;
    }
}

void rats_destroy(rats_client_t handle) {
    if (!handle) return;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    // Ensure stopped
    wrap->client->stop();
    delete wrap;
}

int rats_start(rats_client_t handle) {
    if (!handle) return RATS_ERROR_INVALID_HANDLE;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    return wrap->client->start() ? RATS_SUCCESS : RATS_ERROR_OPERATION_FAILED;
}

void rats_stop(rats_client_t handle) {
    if (!handle) return;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    wrap->client->stop();
}

int rats_connect(rats_client_t handle, const char* host, int port) {
    if (!handle || !host) return 0;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    return wrap->client->connect_to_peer(std::string(host), port) ? 1 : 0;
}

int rats_get_listen_port(rats_client_t handle) {
    if (!handle) return 0;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    return wrap->client->get_listen_port();
}

int rats_broadcast_string(rats_client_t handle, const char* message) {
    if (!handle || !message) return 0;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    return wrap->client->broadcast_string_to_peers(std::string(message));
}

int rats_send_string(rats_client_t handle, const char* peer_id, const char* message) {
    if (!handle) return RATS_ERROR_INVALID_HANDLE;
    if (!peer_id || !message) return RATS_ERROR_INVALID_PARAMETER;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    return wrap->client->send_string_to_peer_id(std::string(peer_id), std::string(message)) ? RATS_SUCCESS : RATS_ERROR_OPERATION_FAILED;
}

int rats_get_peer_count(rats_client_t handle) {
    if (!handle) return 0;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    return wrap->client->get_peer_count();
}

char* rats_get_our_peer_id(rats_client_t handle) {
    if (!handle) return nullptr;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    return rats_strdup_owned(wrap->client->get_our_peer_id());
}

char* rats_get_connection_statistics_json(rats_client_t handle) {
    if (!handle) return nullptr;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    auto json = wrap->client->get_connection_statistics();
    return rats_strdup_owned(json.dump());
}

void rats_set_logging_enabled(int enabled) {
    // Global logger control through any client instance is awkward; use singleton
    Logger::getInstance().set_file_logging_enabled(enabled != 0);
}

void rats_set_log_level(const char* level_str) {
    if (!level_str) return;
    std::string lvl(level_str);
    // Map string to LogLevel
    std::string upper = lvl;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    LogLevel level = LogLevel::INFO;
    if (upper == "DEBUG") level = LogLevel::DEBUG;
    else if (upper == "INFO") level = LogLevel::INFO;
    else if (upper == "WARN" || upper == "WARNING") level = LogLevel::WARN;
    else if (upper == "ERROR") level = LogLevel::ERROR;
    Logger::getInstance().set_log_level(level);
}


void rats_set_log_file_path(rats_client_t handle, const char* file_path) {
    if (!handle || !file_path) return;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    wrap->client->set_log_file_path(std::string(file_path));
}

char* rats_get_log_file_path(rats_client_t handle) {
    if (!handle) return nullptr;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    return rats_strdup_owned(wrap->client->get_log_file_path());
}

void rats_set_log_colors_enabled(rats_client_t handle, int enabled) {
    if (!handle) return;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    wrap->client->set_log_colors_enabled(enabled != 0);
}

int rats_is_log_colors_enabled(rats_client_t handle) {
    if (!handle) return 0;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    return wrap->client->is_log_colors_enabled() ? 1 : 0;
}

void rats_set_log_timestamps_enabled(rats_client_t handle, int enabled) {
    if (!handle) return;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    wrap->client->set_log_timestamps_enabled(enabled != 0);
}

int rats_is_log_timestamps_enabled(rats_client_t handle) {
    if (!handle) return 0;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    return wrap->client->is_log_timestamps_enabled() ? 1 : 0;
}

void rats_set_log_rotation_size(rats_client_t handle, size_t max_size_bytes) {
    if (!handle) return;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    wrap->client->set_log_rotation_size(max_size_bytes);
}

void rats_set_log_retention_count(rats_client_t handle, int count) {
    if (!handle) return;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    wrap->client->set_log_retention_count(count);
}

void rats_clear_log_file(rats_client_t handle) {
    if (!handle) return;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    wrap->client->clear_log_file();
}

void rats_set_connection_callback(rats_client_t handle, rats_connection_cb cb, void* user_data) {
    if (!handle) return;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    wrap->connection_cb = cb;
    wrap->connection_ud = user_data;
    wrap->client->set_connection_callback([wrap](socket_t, const std::string& peer_id) {
        if (wrap->connection_cb) {
            wrap->connection_cb(wrap->connection_ud, peer_id.c_str());
        }
    });
}

void rats_set_string_callback(rats_client_t handle, rats_string_cb cb, void* user_data) {
    if (!handle) return;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    wrap->string_cb = cb;
    wrap->string_ud = user_data;
    wrap->client->set_string_data_callback([wrap](socket_t, const std::string& peer_id, const std::string& data) {
        if (wrap->string_cb) {
            wrap->string_cb(wrap->string_ud, peer_id.c_str(), data.c_str());
        }
    });
}

void rats_set_disconnect_callback(rats_client_t handle, rats_disconnect_cb cb, void* user_data) {
    if (!handle) return;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    wrap->disconnect_cb = cb;
    wrap->disconnect_ud = user_data;
    wrap->client->set_disconnect_callback([wrap](socket_t, const std::string& peer_id) {
        if (wrap->disconnect_cb) {
            wrap->disconnect_cb(wrap->disconnect_ud, peer_id.c_str());
        }
    });
}

// ===================== NEW C API IMPLEMENTATIONS =====================

// Peer configuration
rats_error_t rats_set_max_peers(rats_client_t handle, int max_peers) {
    if (!handle) return RATS_ERROR_INVALID_HANDLE;
    if (max_peers <= 0) return RATS_ERROR_INVALID_PARAMETER;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    wrap->client->set_max_peers(max_peers);
    return RATS_SUCCESS;
}

int rats_get_max_peers(rats_client_t handle) {
    if (!handle) return 0;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    return wrap->client->get_max_peers();
}

int rats_is_peer_limit_reached(rats_client_t handle) {
    if (!handle) return 0;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    return wrap->client->is_peer_limit_reached() ? 1 : 0;
}

// Advanced connection methods
rats_error_t rats_connect_with_strategy(rats_client_t handle, const char* host, int port, 
                                        rats_connection_strategy_t strategy) {
    if (!handle || !host) return RATS_ERROR_INVALID_PARAMETER;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    
    ConnectionStrategy cpp_strategy;
    switch (strategy) {
        case RATS_STRATEGY_DIRECT_ONLY: cpp_strategy = ConnectionStrategy::DIRECT_ONLY; break;
        case RATS_STRATEGY_STUN_ASSISTED: cpp_strategy = ConnectionStrategy::STUN_ASSISTED; break;
        case RATS_STRATEGY_ICE_FULL: cpp_strategy = ConnectionStrategy::ICE_FULL; break;
        case RATS_STRATEGY_TURN_RELAY: cpp_strategy = ConnectionStrategy::TURN_RELAY; break;
        case RATS_STRATEGY_AUTO_ADAPTIVE: cpp_strategy = ConnectionStrategy::AUTO_ADAPTIVE; break;
        default: return RATS_ERROR_INVALID_PARAMETER;
    }
    
    return wrap->client->connect_to_peer(std::string(host), port, cpp_strategy) ? 
           RATS_SUCCESS : RATS_ERROR_OPERATION_FAILED;
}

rats_error_t rats_disconnect_peer_by_id(rats_client_t handle, const char* peer_id) {
    if (!handle || !peer_id) return RATS_ERROR_INVALID_PARAMETER;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    wrap->client->disconnect_peer_by_id(std::string(peer_id));
    return RATS_SUCCESS;
}

// Binary data operations  
rats_error_t rats_send_binary(rats_client_t handle, const char* peer_id, const void* data, size_t size) {
    if (!handle || !peer_id || !data || size == 0) return RATS_ERROR_INVALID_PARAMETER;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    
    std::vector<uint8_t> binary_data(static_cast<const uint8_t*>(data), 
                                     static_cast<const uint8_t*>(data) + size);
    
    return wrap->client->send_binary_to_peer_id(std::string(peer_id), binary_data) ? 
           RATS_SUCCESS : RATS_ERROR_OPERATION_FAILED;
}

int rats_broadcast_binary(rats_client_t handle, const void* data, size_t size) {
    if (!handle || !data || size == 0) return 0;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    
    std::vector<uint8_t> binary_data(static_cast<const uint8_t*>(data), 
                                     static_cast<const uint8_t*>(data) + size);
    
    return wrap->client->broadcast_binary_to_peers(binary_data);
}

// JSON operations
rats_error_t rats_send_json(rats_client_t handle, const char* peer_id, const char* json_str) {
    if (!handle || !peer_id || !json_str) return RATS_ERROR_INVALID_PARAMETER;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    
    try {
        nlohmann::json json_data = nlohmann::json::parse(json_str);
        return wrap->client->send_json_to_peer_id(std::string(peer_id), json_data) ? 
               RATS_SUCCESS : RATS_ERROR_OPERATION_FAILED;
    } catch (const nlohmann::json::exception&) {
        return RATS_ERROR_JSON_PARSE;
    }
}

int rats_broadcast_json(rats_client_t handle, const char* json_str) {
    if (!handle || !json_str) return 0;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    
    try {
        nlohmann::json json_data = nlohmann::json::parse(json_str);
        return wrap->client->broadcast_json_to_peers(json_data);
    } catch (const nlohmann::json::exception&) {
        return 0;
    }
}

// DHT Discovery
rats_error_t rats_start_dht_discovery(rats_client_t handle, int dht_port) {
    if (!handle) return RATS_ERROR_INVALID_HANDLE;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    return wrap->client->start_dht_discovery(dht_port) ? RATS_SUCCESS : RATS_ERROR_OPERATION_FAILED;
}

void rats_stop_dht_discovery(rats_client_t handle) {
    if (!handle) return;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    wrap->client->stop_dht_discovery();
}

int rats_is_dht_running(rats_client_t handle) {
    if (!handle) return 0;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    return wrap->client->is_dht_running() ? 1 : 0;
}

rats_error_t rats_announce_for_hash(rats_client_t handle, const char* content_hash, int port) {
    if (!handle || !content_hash) return RATS_ERROR_INVALID_PARAMETER;
    if (strlen(content_hash) != 40) return RATS_ERROR_INVALID_PARAMETER; // SHA1 hash must be 40 chars
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    
    uint16_t announce_port = (port <= 0) ? 0 : static_cast<uint16_t>(port);
    return wrap->client->announce_for_hash(std::string(content_hash), announce_port) ? 
           RATS_SUCCESS : RATS_ERROR_OPERATION_FAILED;
}

size_t rats_get_dht_routing_table_size(rats_client_t handle) {
    if (!handle) return 0;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    return wrap->client->get_dht_routing_table_size();
}

// Automatic discovery

void rats_start_automatic_peer_discovery(rats_client_t handle) {
    if (!handle) return;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    wrap->client->start_automatic_peer_discovery();
}

void rats_stop_automatic_peer_discovery(rats_client_t handle) {
    if (!handle) return;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    wrap->client->stop_automatic_peer_discovery();
}

int rats_is_automatic_discovery_running(rats_client_t handle) {
    if (!handle) return 0;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    return wrap->client->is_automatic_discovery_running() ? 1 : 0;
}

char* rats_get_discovery_hash(rats_client_t handle) {
    if (!handle) return nullptr;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    return rats_strdup_owned(wrap->client->get_discovery_hash());
}

char* rats_get_rats_peer_discovery_hash(void) {
    return rats_strdup_owned(RatsClient::get_rats_peer_discovery_hash());
}

// mDNS Discovery
rats_error_t rats_start_mdns_discovery(rats_client_t handle, const char* service_name) {
    if (!handle) return RATS_ERROR_INVALID_HANDLE;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    
    std::string service = service_name ? std::string(service_name) : std::string("");
    return wrap->client->start_mdns_discovery(service) ? RATS_SUCCESS : RATS_ERROR_OPERATION_FAILED;
}

void rats_stop_mdns_discovery(rats_client_t handle) {
    if (!handle) return;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    wrap->client->stop_mdns_discovery();
}

int rats_is_mdns_running(rats_client_t handle) {
    if (!handle) return 0;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    return wrap->client->is_mdns_running() ? 1 : 0;
}

rats_error_t rats_query_mdns_services(rats_client_t handle) {
    if (!handle) return RATS_ERROR_INVALID_HANDLE;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    return wrap->client->query_mdns_services() ? RATS_SUCCESS : RATS_ERROR_OPERATION_FAILED;
}

// Encryption
rats_error_t rats_set_encryption_enabled(rats_client_t handle, int enabled) {
    if (!handle) return RATS_ERROR_INVALID_HANDLE;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    wrap->client->set_encryption_enabled(enabled != 0);
    return RATS_SUCCESS;
}

int rats_is_encryption_enabled(rats_client_t handle) {
    if (!handle) return 0;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    return wrap->client->is_encryption_enabled() ? 1 : 0;
}

char* rats_get_encryption_key(rats_client_t handle) {
    if (!handle) return nullptr;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    return rats_strdup_owned(wrap->client->get_encryption_key());
}

rats_error_t rats_set_encryption_key(rats_client_t handle, const char* key_hex) {
    if (!handle || !key_hex) return RATS_ERROR_INVALID_PARAMETER;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    return wrap->client->set_encryption_key(std::string(key_hex)) ? 
           RATS_SUCCESS : RATS_ERROR_INVALID_PARAMETER;
}

char* rats_generate_encryption_key(rats_client_t handle) {
    if (!handle) return nullptr;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    return rats_strdup_owned(wrap->client->generate_new_encryption_key());
}

// Protocol configuration
rats_error_t rats_set_protocol_name(rats_client_t handle, const char* protocol_name) {
    if (!handle || !protocol_name) return RATS_ERROR_INVALID_PARAMETER;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    wrap->client->set_protocol_name(std::string(protocol_name));
    return RATS_SUCCESS;
}

rats_error_t rats_set_protocol_version(rats_client_t handle, const char* protocol_version) {
    if (!handle || !protocol_version) return RATS_ERROR_INVALID_PARAMETER;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    wrap->client->set_protocol_version(std::string(protocol_version));
    return RATS_SUCCESS;
}

char* rats_get_protocol_name(rats_client_t handle) {
    if (!handle) return nullptr;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    return rats_strdup_owned(wrap->client->get_protocol_name());
}

char* rats_get_protocol_version(rats_client_t handle) {
    if (!handle) return nullptr;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    return rats_strdup_owned(wrap->client->get_protocol_version());
}

// Message Exchange API
rats_error_t rats_on_message(rats_client_t handle, const char* message_type, 
                             rats_message_cb callback, void* user_data) {
    if (!handle || !message_type || !callback) return RATS_ERROR_INVALID_PARAMETER;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    
    // Store the C callback
    wrap->message_handlers[std::string(message_type)] = std::make_pair(callback, user_data);
    
    // Set up C++ callback that calls the C callback
    wrap->client->on(std::string(message_type), [wrap, callback, user_data](const std::string& peer_id, const nlohmann::json& data) {
        if (callback) {
            std::string json_str = data.dump();
            callback(user_data, peer_id.c_str(), json_str.c_str());
        }
    });
    
    return RATS_SUCCESS;
}

rats_error_t rats_send_message(rats_client_t handle, const char* peer_id, 
                               const char* message_type, const char* data) {
    if (!handle || !peer_id || !message_type || !data) return RATS_ERROR_INVALID_PARAMETER;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    
    try {
        nlohmann::json json_data = nlohmann::json::parse(data);
        wrap->client->send(std::string(peer_id), std::string(message_type), json_data);
        return RATS_SUCCESS;
    } catch (const nlohmann::json::exception&) {
        // If not valid JSON, send as string value
        nlohmann::json string_data = std::string(data);
        wrap->client->send(std::string(peer_id), std::string(message_type), string_data);
        return RATS_SUCCESS;
    }
}

rats_error_t rats_broadcast_message(rats_client_t handle, const char* message_type, const char* data) {
    if (!handle || !message_type || !data) return RATS_ERROR_INVALID_PARAMETER;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    
    try {
        nlohmann::json json_data = nlohmann::json::parse(data);
        wrap->client->send(std::string(message_type), json_data);
        return RATS_SUCCESS;
    } catch (const nlohmann::json::exception&) {
        // If not valid JSON, send as string value
        nlohmann::json string_data = std::string(data);
        wrap->client->send(std::string(message_type), string_data);
        return RATS_SUCCESS;
    }
}

// File Transfer
char* rats_send_file(rats_client_t handle, const char* peer_id, 
                     const char* file_path, const char* remote_filename) {
    if (!handle || !peer_id || !file_path) return nullptr;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    
    std::string remote_name = remote_filename ? std::string(remote_filename) : std::string("");
    std::string transfer_id = wrap->client->send_file(std::string(peer_id), 
                                                      std::string(file_path), remote_name);
    
    return transfer_id.empty() ? nullptr : rats_strdup_owned(transfer_id);
}

rats_error_t rats_accept_file_transfer(rats_client_t handle, const char* transfer_id, 
                                       const char* local_path) {
    if (!handle || !transfer_id || !local_path) return RATS_ERROR_INVALID_PARAMETER;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    
    return wrap->client->accept_file_transfer(std::string(transfer_id), std::string(local_path)) ? 
           RATS_SUCCESS : RATS_ERROR_OPERATION_FAILED;
}

rats_error_t rats_reject_file_transfer(rats_client_t handle, const char* transfer_id, 
                                       const char* reason) {
    if (!handle || !transfer_id) return RATS_ERROR_INVALID_PARAMETER;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    
    std::string reject_reason = reason ? std::string(reason) : std::string("");
    return wrap->client->reject_file_transfer(std::string(transfer_id), reject_reason) ? 
           RATS_SUCCESS : RATS_ERROR_OPERATION_FAILED;
}

rats_error_t rats_cancel_file_transfer(rats_client_t handle, const char* transfer_id) {
    if (!handle || !transfer_id) return RATS_ERROR_INVALID_PARAMETER;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    
    return wrap->client->cancel_file_transfer(std::string(transfer_id)) ? 
           RATS_SUCCESS : RATS_ERROR_OPERATION_FAILED;
}

// Enhanced callbacks
void rats_set_binary_callback(rats_client_t handle, rats_binary_cb cb, void* user_data) {
    if (!handle) return;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    wrap->binary_cb = cb;
    wrap->binary_ud = user_data;
    wrap->client->set_binary_data_callback([wrap](socket_t, const std::string& peer_id, const std::vector<uint8_t>& data) {
        if (wrap->binary_cb) {
            wrap->binary_cb(wrap->binary_ud, peer_id.c_str(), data.data(), data.size());
        }
    });
}

void rats_set_json_callback(rats_client_t handle, rats_json_cb cb, void* user_data) {
    if (!handle) return;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    wrap->json_cb = cb;
    wrap->json_ud = user_data;
    wrap->client->set_json_data_callback([wrap](socket_t, const std::string& peer_id, const nlohmann::json& data) {
        if (wrap->json_cb) {
            std::string json_str = data.dump();
            wrap->json_cb(wrap->json_ud, peer_id.c_str(), json_str.c_str());
        }
    });
}

void rats_set_peer_discovered_callback(rats_client_t handle, rats_peer_discovered_cb cb, void* user_data) {
    if (!handle) return;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    wrap->peer_discovered_cb = cb;
    wrap->peer_discovered_ud = user_data;
    wrap->client->set_mdns_callback([wrap](const std::string& host, int port, const std::string& service_name) {
        if (wrap->peer_discovered_cb) {
            wrap->peer_discovered_cb(wrap->peer_discovered_ud, host.c_str(), port, service_name.c_str());
        }
    });
}

void rats_set_file_progress_callback(rats_client_t handle, rats_file_progress_cb cb, void* user_data) {
    if (!handle) return;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    wrap->file_progress_cb = cb;
    wrap->file_progress_ud = user_data;
    
    wrap->client->on_file_transfer_progress([wrap](const FileTransferProgress& progress) {
        if (wrap->file_progress_cb) {
            int progress_percent = progress.get_completion_percentage();
            std::string status_str = "IN_PROGRESS"; // Default status
            switch (progress.status) {
                case FileTransferStatus::PENDING: status_str = "PENDING"; break;
                case FileTransferStatus::STARTING: status_str = "STARTING"; break;
                case FileTransferStatus::IN_PROGRESS: status_str = "IN_PROGRESS"; break;
                case FileTransferStatus::PAUSED: status_str = "PAUSED"; break;
                case FileTransferStatus::COMPLETED: status_str = "COMPLETED"; break;
                case FileTransferStatus::FAILED: status_str = "FAILED"; break;
                case FileTransferStatus::CANCELLED: status_str = "CANCELLED"; break;
                case FileTransferStatus::RESUMING: status_str = "RESUMING"; break;
            }
            wrap->file_progress_cb(wrap->file_progress_ud, progress.transfer_id.c_str(), progress_percent, status_str.c_str());
        }
    });
}

char* rats_send_directory(rats_client_t handle, const char* peer_id, const char* directory_path, const char* remote_directory_name, int recursive) {
    if (!handle || !peer_id || !directory_path) return nullptr;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    
    std::string remote_name = remote_directory_name ? std::string(remote_directory_name) : std::string("");
    std::string transfer_id = wrap->client->send_directory(std::string(peer_id), std::string(directory_path), remote_name, recursive != 0);
    
    return transfer_id.empty() ? nullptr : rats_strdup_owned(transfer_id);
}

char* rats_request_file(rats_client_t handle, const char* peer_id, const char* remote_file_path, const char* local_path) {
    if (!handle || !peer_id || !remote_file_path || !local_path) return nullptr;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    
    std::string transfer_id = wrap->client->request_file(std::string(peer_id), std::string(remote_file_path), std::string(local_path));
    
    return transfer_id.empty() ? nullptr : rats_strdup_owned(transfer_id);
}

char* rats_request_directory(rats_client_t handle, const char* peer_id, const char* remote_directory_path, const char* local_directory_path, int recursive) {
    if (!handle || !peer_id || !remote_directory_path || !local_directory_path) return nullptr;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    
    std::string transfer_id = wrap->client->request_directory(std::string(peer_id), std::string(remote_directory_path), std::string(local_directory_path), recursive != 0);
    
    return transfer_id.empty() ? nullptr : rats_strdup_owned(transfer_id);
}

rats_error_t rats_accept_directory_transfer(rats_client_t handle, const char* transfer_id, const char* local_path) {
    if (!handle || !transfer_id || !local_path) return RATS_ERROR_INVALID_PARAMETER;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    
    return wrap->client->accept_directory_transfer(std::string(transfer_id), std::string(local_path)) ? RATS_SUCCESS : RATS_ERROR_OPERATION_FAILED;
}

rats_error_t rats_reject_directory_transfer(rats_client_t handle, const char* transfer_id, const char* reason) {
    if (!handle || !transfer_id) return RATS_ERROR_INVALID_PARAMETER;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    
    std::string reject_reason = reason ? std::string(reason) : std::string("");
    return wrap->client->reject_directory_transfer(std::string(transfer_id), reject_reason) ? RATS_SUCCESS : RATS_ERROR_OPERATION_FAILED;
}

rats_error_t rats_pause_file_transfer(rats_client_t handle, const char* transfer_id) {
    if (!handle || !transfer_id) return RATS_ERROR_INVALID_PARAMETER;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    
    return wrap->client->pause_file_transfer(std::string(transfer_id)) ? RATS_SUCCESS : RATS_ERROR_OPERATION_FAILED;
}

rats_error_t rats_resume_file_transfer(rats_client_t handle, const char* transfer_id) {
    if (!handle || !transfer_id) return RATS_ERROR_INVALID_PARAMETER;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    
    return wrap->client->resume_file_transfer(std::string(transfer_id)) ? RATS_SUCCESS : RATS_ERROR_OPERATION_FAILED;
}

char* rats_get_file_transfer_progress_json(rats_client_t handle, const char* transfer_id) {
    if (!handle || !transfer_id) return nullptr;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    
    auto progress = wrap->client->get_file_transfer_progress(std::string(transfer_id));
    if (!progress) return nullptr;
    
    // Convert progress to JSON
    nlohmann::json progress_json;
    progress_json["transfer_id"] = progress->transfer_id;
    progress_json["filename"] = progress->filename;
    progress_json["local_path"] = progress->local_path;
    progress_json["peer_id"] = progress->peer_id;
    progress_json["bytes_transferred"] = progress->bytes_transferred;
    progress_json["total_bytes"] = progress->total_bytes;
    progress_json["completion_percentage"] = progress->get_completion_percentage();
    progress_json["status"] = static_cast<int>(progress->status);
    progress_json["direction"] = static_cast<int>(progress->direction);
    
    return rats_strdup_owned(progress_json.dump());
}

char* rats_get_file_transfer_statistics_json(rats_client_t handle) {
    if (!handle) return nullptr;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    auto stats = wrap->client->get_file_transfer_statistics();
    return rats_strdup_owned(stats.dump());
}

// File transfer callback setters
void rats_set_file_request_callback(rats_client_t handle, rats_file_request_cb cb, void* user_data) {
    if (!handle) return;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    wrap->file_request_cb = cb;
    wrap->file_request_ud = user_data;
    
    if (cb) {
        wrap->client->on_file_request([wrap](const std::string& peer_id, const std::string& file_path, const std::string& transfer_id) {
            if (wrap->file_request_cb) {
                wrap->file_request_cb(wrap->file_request_ud, peer_id.c_str(), transfer_id.c_str(), file_path.c_str(), "");
            }
            return true; // Accept the file request
        });
    }
}

void rats_set_directory_request_callback(rats_client_t handle, rats_directory_request_cb cb, void* user_data) {
    if (!handle) return;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    wrap->directory_request_cb = cb;
    wrap->directory_request_ud = user_data;
    
    if (cb) {
        wrap->client->on_directory_request([wrap](const std::string& peer_id, const std::string& directory_path, bool recursive, const std::string& transfer_id) {
            if (wrap->directory_request_cb) {
                wrap->directory_request_cb(wrap->directory_request_ud, peer_id.c_str(), transfer_id.c_str(), directory_path.c_str(), "");
            }
            return true; // Accept the directory request
        });
    }
}

void rats_set_directory_progress_callback(rats_client_t handle, rats_directory_progress_cb cb, void* user_data) {
    if (!handle) return;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    wrap->directory_progress_cb = cb;
    wrap->directory_progress_ud = user_data;
    
    if (cb) {
        wrap->client->on_directory_transfer_progress([wrap](const std::string& transfer_id, const std::string& current_file, uint64_t files_completed, uint64_t total_files, uint64_t bytes_completed, uint64_t total_bytes) {
            if (wrap->directory_progress_cb) {
                wrap->directory_progress_cb(wrap->directory_progress_ud, transfer_id.c_str(), static_cast<int>(files_completed), static_cast<int>(total_files), current_file.c_str());
            }
        });
    }
}

// Peer information
char** rats_get_peer_ids(rats_client_t handle, int* count) {
    if (!handle || !count) {
        if (count) *count = 0;
        return nullptr;
    }
    
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    auto peers = wrap->client->get_validated_peers();
    
    *count = static_cast<int>(peers.size());
    if (*count == 0) return nullptr;
    
    char** peer_ids = static_cast<char**>(malloc(*count * sizeof(char*)));
    if (!peer_ids) {
        *count = 0;
        return nullptr;
    }
    
    for (int i = 0; i < *count; ++i) {
        peer_ids[i] = rats_strdup_owned(peers[i].peer_id);
        if (!peer_ids[i]) {
            // Clean up on allocation failure
            for (int j = 0; j < i; ++j) {
                free(peer_ids[j]);
            }
            free(peer_ids);
            *count = 0;
            return nullptr;
        }
    }
    
    return peer_ids;
}

char* rats_get_peer_info_json(rats_client_t handle, const char* peer_id) {
    if (!handle || !peer_id) return nullptr;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    
    const RatsPeer* peer = wrap->client->get_peer_by_id(std::string(peer_id));
    if (!peer) return nullptr;
    
    // Create JSON representation of peer info
    nlohmann::json peer_info;
    peer_info["peer_id"] = peer->peer_id;
    peer_info["ip"] = peer->ip;
    peer_info["port"] = peer->port;
    peer_info["is_outgoing"] = peer->is_outgoing;
    peer_info["handshake_completed"] = peer->is_handshake_completed();
    peer_info["version"] = peer->version;
    peer_info["encryption_enabled"] = peer->encryption_enabled;
    
    return rats_strdup_owned(peer_info.dump());
}

char** rats_get_validated_peer_ids(rats_client_t handle, int* count) {
    if (!handle || !count) {
        if (count) *count = 0;
        return nullptr;
    }
    
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    auto peers = wrap->client->get_validated_peers();
    
    *count = static_cast<int>(peers.size());
    if (*count == 0) return nullptr;
    
    char** peer_ids = static_cast<char**>(malloc(*count * sizeof(char*)));
    if (!peer_ids) {
        *count = 0;
        return nullptr;
    }
    
    for (int i = 0; i < *count; ++i) {
        peer_ids[i] = rats_strdup_owned(peers[i].peer_id);
        if (!peer_ids[i]) {
            // Clean up on allocation failure
            for (int j = 0; j < i; ++j) {
                free(peer_ids[j]);
            }
            free(peer_ids);
            *count = 0;
            return nullptr;
        }
    }
    
    return peer_ids;
}

// ===================== GOSSIPSUB FUNCTIONALITY =====================

int rats_is_gossipsub_available(rats_client_t handle) {
    if (!handle) return 0;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    return wrap->client->is_gossipsub_available() ? 1 : 0;
}

int rats_is_gossipsub_running(rats_client_t handle) {
    if (!handle) return 0;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    return wrap->client->is_gossipsub_running() ? 1 : 0;
}

rats_error_t rats_subscribe_to_topic(rats_client_t handle, const char* topic) {
    if (!handle || !topic) return RATS_ERROR_INVALID_PARAMETER;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    return wrap->client->subscribe_to_topic(std::string(topic)) ? RATS_SUCCESS : RATS_ERROR_OPERATION_FAILED;
}

rats_error_t rats_unsubscribe_from_topic(rats_client_t handle, const char* topic) {
    if (!handle || !topic) return RATS_ERROR_INVALID_PARAMETER;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    return wrap->client->unsubscribe_from_topic(std::string(topic)) ? RATS_SUCCESS : RATS_ERROR_OPERATION_FAILED;
}

int rats_is_subscribed_to_topic(rats_client_t handle, const char* topic) {
    if (!handle || !topic) return 0;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    return wrap->client->is_subscribed_to_topic(std::string(topic)) ? 1 : 0;
}

char** rats_get_subscribed_topics(rats_client_t handle, int* count) {
    if (!handle || !count) {
        if (count) *count = 0;
        return nullptr;
    }
    
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    auto topics = wrap->client->get_subscribed_topics();
    
    *count = static_cast<int>(topics.size());
    if (*count == 0) return nullptr;
    
    char** topic_array = static_cast<char**>(malloc(*count * sizeof(char*)));
    if (!topic_array) {
        *count = 0;
        return nullptr;
    }
    
    for (int i = 0; i < *count; ++i) {
        topic_array[i] = rats_strdup_owned(topics[i]);
        if (!topic_array[i]) {
            // Clean up on allocation failure
            for (int j = 0; j < i; ++j) {
                free(topic_array[j]);
            }
            free(topic_array);
            *count = 0;
            return nullptr;
        }
    }
    
    return topic_array;
}

rats_error_t rats_publish_to_topic(rats_client_t handle, const char* topic, const char* message) {
    if (!handle || !topic || !message) return RATS_ERROR_INVALID_PARAMETER;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    return wrap->client->publish_to_topic(std::string(topic), std::string(message)) ? RATS_SUCCESS : RATS_ERROR_OPERATION_FAILED;
}

rats_error_t rats_publish_json_to_topic(rats_client_t handle, const char* topic, const char* json_str) {
    if (!handle || !topic || !json_str) return RATS_ERROR_INVALID_PARAMETER;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    
    try {
        nlohmann::json json_data = nlohmann::json::parse(json_str);
        return wrap->client->publish_json_to_topic(std::string(topic), json_data) ? RATS_SUCCESS : RATS_ERROR_OPERATION_FAILED;
    } catch (const nlohmann::json::exception&) {
        return RATS_ERROR_JSON_PARSE;
    }
}

char** rats_get_topic_peers(rats_client_t handle, const char* topic, int* count) {
    if (!handle || !topic || !count) {
        if (count) *count = 0;
        return nullptr;
    }
    
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    auto peers = wrap->client->get_topic_peers(std::string(topic));
    
    *count = static_cast<int>(peers.size());
    if (*count == 0) return nullptr;
    
    char** peer_array = static_cast<char**>(malloc(*count * sizeof(char*)));
    if (!peer_array) {
        *count = 0;
        return nullptr;
    }
    
    for (int i = 0; i < *count; ++i) {
        peer_array[i] = rats_strdup_owned(peers[i]);
        if (!peer_array[i]) {
            // Clean up on allocation failure
            for (int j = 0; j < i; ++j) {
                free(peer_array[j]);
            }
            free(peer_array);
            *count = 0;
            return nullptr;
        }
    }
    
    return peer_array;
}

char** rats_get_topic_mesh_peers(rats_client_t handle, const char* topic, int* count) {
    if (!handle || !topic || !count) {
        if (count) *count = 0;
        return nullptr;
    }
    
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    auto peers = wrap->client->get_topic_mesh_peers(std::string(topic));
    
    *count = static_cast<int>(peers.size());
    if (*count == 0) return nullptr;
    
    char** peer_array = static_cast<char**>(malloc(*count * sizeof(char*)));
    if (!peer_array) {
        *count = 0;
        return nullptr;
    }
    
    for (int i = 0; i < *count; ++i) {
        peer_array[i] = rats_strdup_owned(peers[i]);
        if (!peer_array[i]) {
            // Clean up on allocation failure
            for (int j = 0; j < i; ++j) {
                free(peer_array[j]);
            }
            free(peer_array);
            *count = 0;
            return nullptr;
        }
    }
    
    return peer_array;
}

char* rats_get_gossipsub_statistics_json(rats_client_t handle) {
    if (!handle) return nullptr;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    auto stats = wrap->client->get_gossipsub_statistics();
    return rats_strdup_owned(stats.dump());
}

// GossipSub callback setters
void rats_set_topic_message_callback(rats_client_t handle, const char* topic, rats_topic_message_cb cb, void* user_data) {
    if (!handle || !topic) return;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    
    std::string topic_str(topic);
    if (cb) {
        wrap->topic_message_handlers[topic_str] = std::make_pair(cb, user_data);
        wrap->client->on_topic_message(topic_str, [wrap, cb, user_data](const std::string& peer_id, const std::string& topic, const std::string& message) {
            if (cb) {
                cb(user_data, peer_id.c_str(), topic.c_str(), message.c_str());
            }
        });
    } else {
        wrap->topic_message_handlers.erase(topic_str);
    }
}

void rats_set_topic_json_message_callback(rats_client_t handle, const char* topic, rats_topic_json_message_cb cb, void* user_data) {
    if (!handle || !topic) return;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    
    std::string topic_str(topic);
    if (cb) {
        wrap->topic_json_message_handlers[topic_str] = std::make_pair(cb, user_data);
        wrap->client->on_topic_json_message(topic_str, [wrap, cb, user_data](const std::string& peer_id, const std::string& topic, const nlohmann::json& message) {
            if (cb) {
                std::string json_str = message.dump();
                cb(user_data, peer_id.c_str(), topic.c_str(), json_str.c_str());
            }
        });
    } else {
        wrap->topic_json_message_handlers.erase(topic_str);
    }
}

void rats_set_topic_peer_joined_callback(rats_client_t handle, const char* topic, rats_topic_peer_joined_cb cb, void* user_data) {
    if (!handle || !topic) return;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    
    std::string topic_str(topic);
    if (cb) {
        wrap->topic_peer_joined_handlers[topic_str] = std::make_pair(cb, user_data);
        wrap->client->on_topic_peer_joined(topic_str, [wrap, cb, user_data](const std::string& peer_id, const std::string& topic) {
            if (cb) {
                cb(user_data, peer_id.c_str(), topic.c_str());
            }
        });
    } else {
        wrap->topic_peer_joined_handlers.erase(topic_str);
    }
}

void rats_set_topic_peer_left_callback(rats_client_t handle, const char* topic, rats_topic_peer_left_cb cb, void* user_data) {
    if (!handle || !topic) return;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    
    std::string topic_str(topic);
    if (cb) {
        wrap->topic_peer_left_handlers[topic_str] = std::make_pair(cb, user_data);
        wrap->client->on_topic_peer_left(topic_str, [wrap, cb, user_data](const std::string& peer_id, const std::string& topic) {
            if (cb) {
                cb(user_data, peer_id.c_str(), topic.c_str());
            }
        });
    } else {
        wrap->topic_peer_left_handlers.erase(topic_str);
    }
}

void rats_clear_topic_callbacks(rats_client_t handle, const char* topic) {
    if (!handle || !topic) return;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    
    std::string topic_str(topic);
    wrap->topic_message_handlers.erase(topic_str);
    wrap->topic_json_message_handlers.erase(topic_str);
    wrap->topic_peer_joined_handlers.erase(topic_str);
    wrap->topic_peer_left_handlers.erase(topic_str);
    
    wrap->client->off_topic(topic_str);
}

// ===================== NAT TRAVERSAL AND STUN =====================

rats_error_t rats_discover_and_ignore_public_ip(rats_client_t handle, const char* stun_server, int stun_port) {
    if (!handle) return RATS_ERROR_INVALID_HANDLE;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    
    std::string server = stun_server ? std::string(stun_server) : "stun.l.google.com";
    int port = (stun_port > 0) ? stun_port : 19302;
    
    return wrap->client->discover_and_ignore_public_ip(server, port) ? RATS_SUCCESS : RATS_ERROR_OPERATION_FAILED;
}

char* rats_get_public_ip(rats_client_t handle) {
    if (!handle) return nullptr;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    std::string public_ip = wrap->client->get_public_ip();
    return public_ip.empty() ? nullptr : rats_strdup_owned(public_ip);
}

int rats_detect_nat_type(rats_client_t handle) {
    if (!handle) return 0; // UNKNOWN
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    return static_cast<int>(wrap->client->detect_nat_type());
}

char* rats_get_nat_characteristics_json(rats_client_t handle) {
    if (!handle) return nullptr;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    auto characteristics = wrap->client->get_nat_characteristics();
    
    // Convert NatTypeInfo to JSON manually
    nlohmann::json nat_json;
    nat_json["has_nat"] = characteristics.has_nat;
    nat_json["filtering_behavior"] = static_cast<int>(characteristics.filtering_behavior);
    nat_json["mapping_behavior"] = static_cast<int>(characteristics.mapping_behavior);
    nat_json["preserves_port"] = characteristics.preserves_port;
    nat_json["hairpin_support"] = characteristics.hairpin_support;
    nat_json["description"] = characteristics.description;
    
    return rats_strdup_owned(nat_json.dump());
}

void rats_add_ignored_address(rats_client_t handle, const char* ip_address) {
    if (!handle || !ip_address) return;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    wrap->client->add_ignored_address(std::string(ip_address));
}

char* rats_get_nat_traversal_statistics_json(rats_client_t handle) {
    if (!handle) return nullptr;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    auto stats = wrap->client->get_nat_traversal_statistics();
    return rats_strdup_owned(stats.dump());
}

// ===================== ICE COORDINATION =====================

char* rats_create_ice_offer(rats_client_t handle, const char* peer_id) {
    if (!handle || !peer_id) return nullptr;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    auto offer = wrap->client->create_ice_offer(std::string(peer_id));
    return rats_strdup_owned(offer.dump());
}

rats_error_t rats_connect_with_ice(rats_client_t handle, const char* peer_id, const char* ice_offer_json) {
    if (!handle || !peer_id || !ice_offer_json) return RATS_ERROR_INVALID_PARAMETER;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    
    try {
        nlohmann::json offer = nlohmann::json::parse(ice_offer_json);
        return wrap->client->connect_with_ice(std::string(peer_id), offer) ? RATS_SUCCESS : RATS_ERROR_OPERATION_FAILED;
    } catch (const nlohmann::json::exception&) {
        return RATS_ERROR_JSON_PARSE;
    }
}

rats_error_t rats_handle_ice_answer(rats_client_t handle, const char* peer_id, const char* ice_answer_json) {
    if (!handle || !peer_id || !ice_answer_json) return RATS_ERROR_INVALID_PARAMETER;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    
    try {
        nlohmann::json answer = nlohmann::json::parse(ice_answer_json);
        return wrap->client->handle_ice_answer(std::string(peer_id), answer) ? RATS_SUCCESS : RATS_ERROR_OPERATION_FAILED;
    } catch (const nlohmann::json::exception&) {
        return RATS_ERROR_JSON_PARSE;
    }
}

// ===================== CONFIGURATION PERSISTENCE =====================

rats_error_t rats_load_configuration(rats_client_t handle) {
    if (!handle) return RATS_ERROR_INVALID_HANDLE;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    return wrap->client->load_configuration() ? RATS_SUCCESS : RATS_ERROR_OPERATION_FAILED;
}

rats_error_t rats_save_configuration(rats_client_t handle) {
    if (!handle) return RATS_ERROR_INVALID_HANDLE;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    return wrap->client->save_configuration() ? RATS_SUCCESS : RATS_ERROR_OPERATION_FAILED;
}

rats_error_t rats_set_data_directory(rats_client_t handle, const char* directory_path) {
    if (!handle || !directory_path) return RATS_ERROR_INVALID_PARAMETER;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    return wrap->client->set_data_directory(std::string(directory_path)) ? RATS_SUCCESS : RATS_ERROR_OPERATION_FAILED;
}

char* rats_get_data_directory(rats_client_t handle) {
    if (!handle) return nullptr;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    return rats_strdup_owned(wrap->client->get_data_directory());
}

int rats_load_and_reconnect_peers(rats_client_t handle) {
    if (!handle) return 0;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    return wrap->client->load_and_reconnect_peers();
}

int rats_load_historical_peers(rats_client_t handle) {
    if (!handle) return 0;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    return wrap->client->load_historical_peers() ? 1 : 0;
}

int rats_save_historical_peers(rats_client_t handle) {
    if (!handle) return 0;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    return wrap->client->save_historical_peers() ? 1 : 0;
}

void rats_clear_historical_peers(rats_client_t handle) {
    if (!handle) return;
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    wrap->client->clear_historical_peers();
}

char** rats_get_historical_peer_ids(rats_client_t handle, int* count) {
    if (!handle || !count) {
        if (count) *count = 0;
        return nullptr;
    }
    
    rats_client_wrapper* wrap = static_cast<rats_client_wrapper*>(handle);
    auto historical_peers = wrap->client->get_historical_peers();
    
    *count = static_cast<int>(historical_peers.size());
    if (*count == 0) return nullptr;
    
    char** peer_ids = static_cast<char**>(malloc(*count * sizeof(char*)));
    if (!peer_ids) {
        *count = 0;
        return nullptr;
    }
    
    for (int i = 0; i < *count; ++i) {
        peer_ids[i] = rats_strdup_owned(historical_peers[i].peer_id);
        if (!peer_ids[i]) {
            // Clean up on allocation failure
            for (int j = 0; j < i; ++j) {
                free(peer_ids[j]);
            }
            free(peer_ids);
            *count = 0;
            return nullptr;
        }
    }
    
    return peer_ids;
}

} // extern "C"


