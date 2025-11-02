#pragma once

#include "rats_export.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle to a RatsClient instance
typedef void* rats_client_t;

// Memory helpers for returned strings
RATS_API void rats_string_free(const char* str);

// Version / ABI
RATS_API const char* rats_get_version_string(void);
RATS_API void rats_get_version(int* major, int* minor, int* patch, int* build);
RATS_API const char* rats_get_git_describe(void);
RATS_API uint32_t rats_get_abi(void);

// Client lifecycle
RATS_API rats_client_t rats_create(int listen_port);
RATS_API void rats_destroy(rats_client_t client);
RATS_API int rats_start(rats_client_t client);
RATS_API void rats_stop(rats_client_t client);

// Basic operations
RATS_API int rats_connect(rats_client_t client, const char* host, int port);
RATS_API int rats_get_listen_port(rats_client_t client);
RATS_API int rats_broadcast_string(rats_client_t client, const char* message);
RATS_API int rats_send_string(rats_client_t client, const char* peer_id, const char* message);

// Info
RATS_API int rats_get_peer_count(rats_client_t client);
RATS_API char* rats_get_our_peer_id(rats_client_t client); // caller must free with rats_string_free
RATS_API char* rats_get_connection_statistics_json(rats_client_t client); // caller must free with rats_string_free
RATS_API char** rats_get_validated_peer_ids(rats_client_t client, int* count); // caller must free array and strings
RATS_API char** rats_get_peer_ids(rats_client_t client, int* count); // caller must free array and strings
RATS_API char* rats_get_peer_info_json(rats_client_t client, const char* peer_id); // caller must free

// Logging controls (optional helpers)
RATS_API void rats_set_logging_enabled(int enabled);
RATS_API void rats_set_log_level(const char* level_str); // "DEBUG", "INFO", "WARN", "ERROR"
RATS_API void rats_set_log_file_path(rats_client_t client, const char* file_path);
RATS_API char* rats_get_log_file_path(rats_client_t client); // caller must free
RATS_API void rats_set_log_colors_enabled(rats_client_t client, int enabled);
RATS_API int rats_is_log_colors_enabled(rats_client_t client);
RATS_API void rats_set_log_timestamps_enabled(rats_client_t client, int enabled);
RATS_API int rats_is_log_timestamps_enabled(rats_client_t client);
RATS_API void rats_set_log_rotation_size(rats_client_t client, size_t max_size_bytes);
RATS_API void rats_set_log_retention_count(rats_client_t client, int count);
RATS_API void rats_clear_log_file(rats_client_t client);

// Error codes
typedef enum {
    RATS_SUCCESS = 0,
    RATS_ERROR_INVALID_HANDLE = -1,
    RATS_ERROR_INVALID_PARAMETER = -2,
    RATS_ERROR_NOT_RUNNING = -3,
    RATS_ERROR_OPERATION_FAILED = -4,
    RATS_ERROR_PEER_NOT_FOUND = -5,
    RATS_ERROR_MEMORY_ALLOCATION = -6,
    RATS_ERROR_JSON_PARSE = -7
} rats_error_t;

// Connection strategy enum
typedef enum {
    RATS_STRATEGY_DIRECT_ONLY = 0,
    RATS_STRATEGY_STUN_ASSISTED = 1,
    RATS_STRATEGY_ICE_FULL = 2,
    RATS_STRATEGY_TURN_RELAY = 3,
    RATS_STRATEGY_AUTO_ADAPTIVE = 4
} rats_connection_strategy_t;

// C callbacks
typedef void (*rats_connection_cb)(void* user_data, const char* peer_id);
typedef void (*rats_string_cb)(void* user_data, const char* peer_id, const char* message);
typedef void (*rats_binary_cb)(void* user_data, const char* peer_id, const void* data, size_t size);
typedef void (*rats_json_cb)(void* user_data, const char* peer_id, const char* json_str);
typedef void (*rats_disconnect_cb)(void* user_data, const char* peer_id);
typedef void (*rats_peer_discovered_cb)(void* user_data, const char* host, int port, const char* service_name);
typedef void (*rats_message_cb)(void* user_data, const char* peer_id, const char* message_data);

// Peer configuration
RATS_API rats_error_t rats_set_max_peers(rats_client_t client, int max_peers);
RATS_API int rats_get_max_peers(rats_client_t client);
RATS_API int rats_is_peer_limit_reached(rats_client_t client);

// Advanced connection methods
RATS_API rats_error_t rats_connect_with_strategy(rats_client_t client, const char* host, int port, 
                                                    rats_connection_strategy_t strategy);
RATS_API rats_error_t rats_disconnect_peer_by_id(rats_client_t client, const char* peer_id);

// Binary data operations
RATS_API rats_error_t rats_send_binary(rats_client_t client, const char* peer_id, 
                                          const void* data, size_t size);
RATS_API int rats_broadcast_binary(rats_client_t client, const void* data, size_t size);

// JSON operations
RATS_API rats_error_t rats_send_json(rats_client_t client, const char* peer_id, const char* json_str);
RATS_API int rats_broadcast_json(rats_client_t client, const char* json_str);

// DHT Discovery
RATS_API rats_error_t rats_start_dht_discovery(rats_client_t client, int dht_port);
RATS_API void rats_stop_dht_discovery(rats_client_t client);
RATS_API int rats_is_dht_running(rats_client_t client);
RATS_API rats_error_t rats_announce_for_hash(rats_client_t client, const char* content_hash, int port);
RATS_API size_t rats_get_dht_routing_table_size(rats_client_t client);

// Automatic discovery
RATS_API void rats_start_automatic_peer_discovery(rats_client_t client);
RATS_API void rats_stop_automatic_peer_discovery(rats_client_t client);
RATS_API int rats_is_automatic_discovery_running(rats_client_t client);
RATS_API char* rats_get_discovery_hash(rats_client_t client); // caller must free
RATS_API char* rats_get_rats_peer_discovery_hash(void); // caller must free - static method

// mDNS Discovery  
RATS_API rats_error_t rats_start_mdns_discovery(rats_client_t client, const char* service_name);
RATS_API void rats_stop_mdns_discovery(rats_client_t client);
RATS_API int rats_is_mdns_running(rats_client_t client);
RATS_API rats_error_t rats_query_mdns_services(rats_client_t client);

// Encryption
RATS_API rats_error_t rats_set_encryption_enabled(rats_client_t client, int enabled);
RATS_API int rats_is_encryption_enabled(rats_client_t client);
RATS_API char* rats_get_encryption_key(rats_client_t client); // caller must free
RATS_API rats_error_t rats_set_encryption_key(rats_client_t client, const char* key_hex);
RATS_API char* rats_generate_encryption_key(rats_client_t client); // caller must free

// Protocol configuration
RATS_API rats_error_t rats_set_protocol_name(rats_client_t client, const char* protocol_name);
RATS_API rats_error_t rats_set_protocol_version(rats_client_t client, const char* protocol_version);
RATS_API char* rats_get_protocol_name(rats_client_t client); // caller must free
RATS_API char* rats_get_protocol_version(rats_client_t client); // caller must free

// Message Exchange API
RATS_API rats_error_t rats_on_message(rats_client_t client, const char* message_type, 
                                         rats_message_cb callback, void* user_data);
RATS_API rats_error_t rats_send_message(rats_client_t client, const char* peer_id, 
                                           const char* message_type, const char* data);
RATS_API rats_error_t rats_broadcast_message(rats_client_t client, const char* message_type, 
                                                const char* data);

// File Transfer
RATS_API char* rats_send_file(rats_client_t client, const char* peer_id, 
                                 const char* file_path, const char* remote_filename); // returns transfer_id, caller must free
RATS_API char* rats_send_directory(rats_client_t client, const char* peer_id, const char* directory_path, const char* remote_directory_name, int recursive); // returns transfer_id, caller must free
RATS_API char* rats_request_file(rats_client_t client, const char* peer_id, const char* remote_file_path, const char* local_path); // returns transfer_id, caller must free
RATS_API char* rats_request_directory(rats_client_t client, const char* peer_id, const char* remote_directory_path, const char* local_directory_path, int recursive); // returns transfer_id, caller must free
RATS_API rats_error_t rats_accept_file_transfer(rats_client_t client, const char* transfer_id, 
                                                   const char* local_path);
RATS_API rats_error_t rats_reject_file_transfer(rats_client_t client, const char* transfer_id, 
                                                   const char* reason);
RATS_API rats_error_t rats_cancel_file_transfer(rats_client_t client, const char* transfer_id);
RATS_API rats_error_t rats_accept_directory_transfer(rats_client_t client, const char* transfer_id, const char* local_path);
RATS_API rats_error_t rats_reject_directory_transfer(rats_client_t client, const char* transfer_id, const char* reason);
RATS_API rats_error_t rats_pause_file_transfer(rats_client_t client, const char* transfer_id);
RATS_API rats_error_t rats_resume_file_transfer(rats_client_t client, const char* transfer_id);
RATS_API char* rats_get_file_transfer_progress_json(rats_client_t client, const char* transfer_id); // caller must free
RATS_API char* rats_get_file_transfer_statistics_json(rats_client_t client); // caller must free

// File transfer callbacks
typedef void (*rats_file_request_cb)(void* user_data, const char* peer_id, const char* transfer_id, const char* remote_path, const char* filename);
typedef void (*rats_directory_request_cb)(void* user_data, const char* peer_id, const char* transfer_id, const char* remote_path, const char* directory_name);
typedef void (*rats_directory_progress_cb)(void* user_data, const char* transfer_id, int files_completed, int total_files, const char* current_file);
typedef void (*rats_file_progress_cb)(void* user_data, const char* transfer_id, int progress_percent, const char* status);

// Additional file transfer callbacks
RATS_API void rats_set_file_request_callback(rats_client_t client, rats_file_request_cb cb, void* user_data);
RATS_API void rats_set_directory_request_callback(rats_client_t client, rats_directory_request_cb cb, void* user_data);
RATS_API void rats_set_file_progress_callback(rats_client_t client, rats_file_progress_cb cb, void* user_data);
RATS_API void rats_set_directory_progress_callback(rats_client_t client, rats_directory_progress_cb cb, void* user_data);

// Enhanced callbacks
RATS_API void rats_set_connection_callback(rats_client_t client, rats_connection_cb cb, void* user_data);
RATS_API void rats_set_string_callback(rats_client_t client, rats_string_cb cb, void* user_data);
RATS_API void rats_set_binary_callback(rats_client_t client, rats_binary_cb cb, void* user_data);
RATS_API void rats_set_json_callback(rats_client_t client, rats_json_cb cb, void* user_data);
RATS_API void rats_set_disconnect_callback(rats_client_t client, rats_disconnect_cb cb, void* user_data);
RATS_API void rats_set_peer_discovered_callback(rats_client_t client, rats_peer_discovered_cb cb, void* user_data);

// GossipSub functionality
RATS_API int rats_is_gossipsub_available(rats_client_t client);
RATS_API int rats_is_gossipsub_running(rats_client_t client);
RATS_API rats_error_t rats_subscribe_to_topic(rats_client_t client, const char* topic);
RATS_API rats_error_t rats_unsubscribe_from_topic(rats_client_t client, const char* topic);
RATS_API int rats_is_subscribed_to_topic(rats_client_t client, const char* topic);
RATS_API char** rats_get_subscribed_topics(rats_client_t client, int* count); // caller must free array and strings
RATS_API rats_error_t rats_publish_to_topic(rats_client_t client, const char* topic, const char* message);
RATS_API rats_error_t rats_publish_json_to_topic(rats_client_t client, const char* topic, const char* json_str);
RATS_API char** rats_get_topic_peers(rats_client_t client, const char* topic, int* count); // caller must free array and strings
RATS_API char** rats_get_topic_mesh_peers(rats_client_t client, const char* topic, int* count); // caller must free array and strings
RATS_API char* rats_get_gossipsub_statistics_json(rats_client_t client); // caller must free

// GossipSub callbacks
typedef void (*rats_topic_message_cb)(void* user_data, const char* peer_id, const char* topic, const char* message);
typedef void (*rats_topic_json_message_cb)(void* user_data, const char* peer_id, const char* topic, const char* json_str);
typedef void (*rats_topic_peer_joined_cb)(void* user_data, const char* peer_id, const char* topic);
typedef void (*rats_topic_peer_left_cb)(void* user_data, const char* peer_id, const char* topic);

RATS_API void rats_set_topic_message_callback(rats_client_t client, const char* topic, rats_topic_message_cb cb, void* user_data);
RATS_API void rats_set_topic_json_message_callback(rats_client_t client, const char* topic, rats_topic_json_message_cb cb, void* user_data);
RATS_API void rats_set_topic_peer_joined_callback(rats_client_t client, const char* topic, rats_topic_peer_joined_cb cb, void* user_data);
RATS_API void rats_set_topic_peer_left_callback(rats_client_t client, const char* topic, rats_topic_peer_left_cb cb, void* user_data);
RATS_API void rats_clear_topic_callbacks(rats_client_t client, const char* topic);

// NAT Traversal and STUN
RATS_API rats_error_t rats_discover_and_ignore_public_ip(rats_client_t client, const char* stun_server, int stun_port);
RATS_API char* rats_get_public_ip(rats_client_t client); // caller must free
RATS_API int rats_detect_nat_type(rats_client_t client); // Returns NAT type as integer
RATS_API char* rats_get_nat_characteristics_json(rats_client_t client); // caller must free
RATS_API void rats_add_ignored_address(rats_client_t client, const char* ip_address);
RATS_API char* rats_get_nat_traversal_statistics_json(rats_client_t client); // caller must free

// ICE coordination
RATS_API char* rats_create_ice_offer(rats_client_t client, const char* peer_id); // caller must free JSON string
RATS_API rats_error_t rats_connect_with_ice(rats_client_t client, const char* peer_id, const char* ice_offer_json);
RATS_API rats_error_t rats_handle_ice_answer(rats_client_t client, const char* peer_id, const char* ice_answer_json);

// Configuration persistence
RATS_API rats_error_t rats_load_configuration(rats_client_t client);
RATS_API rats_error_t rats_save_configuration(rats_client_t client);
RATS_API rats_error_t rats_set_data_directory(rats_client_t client, const char* directory_path);
RATS_API char* rats_get_data_directory(rats_client_t client); // caller must free
RATS_API int rats_load_and_reconnect_peers(rats_client_t client);
RATS_API int rats_load_historical_peers(rats_client_t client);
RATS_API int rats_save_historical_peers(rats_client_t client);
RATS_API void rats_clear_historical_peers(rats_client_t client);
RATS_API char** rats_get_historical_peer_ids(rats_client_t client, int* count); // caller must free array and strings

#ifdef __cplusplus
} // extern "C"
#endif


