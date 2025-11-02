#pragma once

#include "socket.h"
#include "logger.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <map>
#include <cstdint>
#include <condition_variable>

namespace librats {

#define LOG_MDNS_DEBUG(message) LOG_DEBUG("mdns", message)
#define LOG_MDNS_INFO(message)  LOG_INFO("mdns", message)
#define LOG_MDNS_WARN(message)  LOG_WARN("mdns", message)
#define LOG_MDNS_ERROR(message) LOG_ERROR("mdns", message)

// mDNS protocol constants
const uint16_t MDNS_PORT = 5353;
const std::string MDNS_MULTICAST_IPv4 = "224.0.0.251";
const std::string MDNS_MULTICAST_IPv6 = "ff02::fb";
const std::string LIBRATS_SERVICE_TYPE = "_librats._tcp.local.";
const std::string LIBRATS_SERVICE_INSTANCE_SUFFIX = ".local.";

// DNS record types
enum class DnsRecordType : uint16_t {
    A = 1,
    PTR = 12,
    TXT = 16,
    AAAA = 28,
    SRV = 33
};

// DNS record classes
enum class DnsRecordClass : uint16_t {
    CLASS_IN = 1,
    CLASS_IN_FLUSH = 0x8001  // Cache flush bit set
};

// mDNS message flags
enum class MdnsFlags : uint16_t {
    QUERY = 0x0000,
    RESPONSE = 0x8000,
    AUTHORITATIVE = 0x8400
};

// Discovered service structure
struct MdnsService {
    std::string service_name;      // e.g., "rats-node-abc123._librats._tcp.local."
    std::string host_name;         // e.g., "MyComputer.local."
    std::string ip_address;        // IPv4 or IPv6 address
    uint16_t port;                 // Service port
    std::map<std::string, std::string> txt_records;  // TXT record key-value pairs
    std::chrono::steady_clock::time_point last_seen;
    
    MdnsService() : port(0) {}
    
    MdnsService(const std::string& name, const std::string& host, 
                const std::string& ip, uint16_t p)
        : service_name(name), host_name(host), ip_address(ip), port(p), 
          last_seen(std::chrono::steady_clock::now()) {}
};

// DNS message header
struct DnsHeader {
    uint16_t transaction_id;
    uint16_t flags;
    uint16_t question_count;
    uint16_t answer_count;
    uint16_t authority_count;
    uint16_t additional_count;
    
    DnsHeader() : transaction_id(0), flags(0), question_count(0), 
                  answer_count(0), authority_count(0), additional_count(0) {}
};

// DNS question structure
struct DnsQuestion {
    std::string name;
    DnsRecordType type;
    DnsRecordClass record_class;
    
    DnsQuestion() : type(DnsRecordType::PTR), record_class(DnsRecordClass::CLASS_IN) {}
    DnsQuestion(const std::string& n, DnsRecordType t, DnsRecordClass c) 
        : name(n), type(t), record_class(c) {}
};

// DNS resource record structure
struct DnsResourceRecord {
    std::string name;
    DnsRecordType type;
    DnsRecordClass record_class;
    uint32_t ttl;
    std::vector<uint8_t> data;
    
    DnsResourceRecord() : type(DnsRecordType::PTR), record_class(DnsRecordClass::CLASS_IN), ttl(120) {}
    DnsResourceRecord(const std::string& n, DnsRecordType t, DnsRecordClass c, uint32_t ttl_val) 
        : name(n), type(t), record_class(c), ttl(ttl_val) {}
};

// Complete DNS message structure
struct DnsMessage {
    DnsHeader header;
    std::vector<DnsQuestion> questions;
    std::vector<DnsResourceRecord> answers;
    std::vector<DnsResourceRecord> authorities;
    std::vector<DnsResourceRecord> additionals;
    
    DnsMessage() = default;
};

// mDNS service discovery callback
using MdnsServiceCallback = std::function<void(const MdnsService& service, bool is_new)>;

class MdnsClient {
public:
    explicit MdnsClient(const std::string& service_instance_name = "", uint16_t service_port = 0);
    ~MdnsClient();
    
    // Core functionality
    bool start();
    void stop();
    void shutdown_immediate();
    bool is_running() const;
    
    // Service announcement
    bool announce_service(const std::string& instance_name, uint16_t port, 
                         const std::map<std::string, std::string>& txt_records = {});
    void stop_announcing();
    bool is_announcing() const;
    
    // Service discovery
    void set_service_callback(MdnsServiceCallback callback);
    bool start_discovery();
    void stop_discovery();
    bool is_discovering() const;
    
    // Query for specific services
    bool query_services();
    
    // Get discovered services
    std::vector<MdnsService> get_discovered_services() const;
    std::vector<MdnsService> get_recent_services(std::chrono::seconds max_age = std::chrono::seconds(300)) const;
    void clear_old_services(std::chrono::seconds max_age = std::chrono::seconds(600));
    
    // Configuration
    void set_announcement_interval(std::chrono::seconds interval);
    void set_query_interval(std::chrono::seconds interval);
    
private:
    // Core properties
    std::string service_instance_name_;
    uint16_t service_port_;
    std::map<std::string, std::string> txt_records_;
    
    // Network properties
    socket_t multicast_socket_;
    std::string local_hostname_;
    std::string local_ip_address_;
    
    // Threading and state
    std::atomic<bool> running_;
    std::atomic<bool> announcing_;
    std::atomic<bool> discovering_;
    std::thread receiver_thread_;
    std::thread announcer_thread_;
    std::thread querier_thread_;
    
    // Conditional variables for immediate shutdown
    std::condition_variable shutdown_cv_;
    std::mutex shutdown_mutex_;
    
    // Discovery state
    mutable std::mutex services_mutex_;
    std::map<std::string, MdnsService> discovered_services_;
    MdnsServiceCallback service_callback_;
    
    // Timing configuration
    std::chrono::seconds announcement_interval_;
    std::chrono::seconds query_interval_;
    
    // Socket operations
    bool create_multicast_socket();
    bool join_multicast_group();
    bool leave_multicast_group();
    void close_multicast_socket();
    
    // Message handling threads
    void receiver_loop();
    void announcer_loop();
    void querier_loop();
    
    // Packet processing
    void handle_received_packet(const std::vector<uint8_t>& packet, const std::string& sender_ip);
    void process_mdns_message(const DnsMessage& message, const std::string& sender_ip);
    void process_query(const DnsMessage& query, const std::string& sender_ip);
    void process_response(const DnsMessage& response, const std::string& sender_ip);
    
    // Service processing
    void extract_service_from_response(const DnsMessage& response, const std::string& sender_ip);
    bool is_librats_service(const std::string& service_name) const;
    void add_or_update_service(const MdnsService& service);
    
    // Message creation
    DnsMessage create_query_message();
    DnsMessage create_announcement_message();
    DnsMessage create_response_message(const DnsQuestion& question);
    
    // DNS record creation
    DnsResourceRecord create_ptr_record(const std::string& service_type, const std::string& instance_name, uint32_t ttl = 120);
    DnsResourceRecord create_srv_record(const std::string& instance_name, const std::string& hostname, uint16_t port, uint32_t ttl = 120);
    DnsResourceRecord create_txt_record(const std::string& instance_name, const std::map<std::string, std::string>& txt_data, uint32_t ttl = 120);
    DnsResourceRecord create_a_record(const std::string& hostname, const std::string& ip_address, uint32_t ttl = 120);
    
    // DNS serialization/deserialization
    std::vector<uint8_t> serialize_dns_message(const DnsMessage& message);
    bool deserialize_dns_message(const std::vector<uint8_t>& data, DnsMessage& message);
    
    // DNS name compression helpers
    void write_dns_name(std::vector<uint8_t>& buffer, const std::string& name);
    std::string read_dns_name(const std::vector<uint8_t>& buffer, size_t& offset);
    void write_uint16(std::vector<uint8_t>& buffer, uint16_t value);
    void write_uint32(std::vector<uint8_t>& buffer, uint32_t value);
    uint16_t read_uint16(const std::vector<uint8_t>& buffer, size_t& offset);
    uint32_t read_uint32(const std::vector<uint8_t>& buffer, size_t& offset);
    
    // TXT record helpers
    std::vector<uint8_t> encode_txt_record(const std::map<std::string, std::string>& txt_data);
    std::map<std::string, std::string> decode_txt_record(const std::vector<uint8_t>& txt_data);
    
    // SRV record helpers
    std::vector<uint8_t> encode_srv_record(uint16_t priority, uint16_t weight, uint16_t port, const std::string& target);
    bool decode_srv_record(const std::vector<uint8_t>& srv_data, uint16_t& priority, uint16_t& weight, uint16_t& port, std::string& target);
    
    // Utility functions
    std::string get_local_hostname();
    std::string get_local_ip_address();
    std::string create_service_instance_name(const std::string& instance_name);
    std::string extract_instance_name_from_service(const std::string& service_name);
    bool send_multicast_packet(const std::vector<uint8_t>& packet);
    
    // Name validation
    bool is_valid_dns_name(const std::string& name) const;
    std::string normalize_dns_name(const std::string& name) const;
};

} // namespace librats 