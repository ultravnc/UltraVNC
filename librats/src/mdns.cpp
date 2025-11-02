#include "mdns.h"
#include "network_utils.h"
#include "os.h"
#include "socket.h"
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>
#include <cstring>

#ifdef _WIN32
#include <ws2tcpip.h>
#include <iphlpapi.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <unistd.h>
#endif

namespace librats {

MdnsClient::MdnsClient(const std::string& service_instance_name, uint16_t service_port)
    : service_instance_name_(service_instance_name),
      service_port_(service_port),
      multicast_socket_(INVALID_SOCKET_VALUE),
      running_(false),
      announcing_(false),
      discovering_(false),
      announcement_interval_(std::chrono::seconds(60)),
      query_interval_(std::chrono::seconds(30)) {
    
    // Get local network information
    local_hostname_ = get_local_hostname();
    local_ip_address_ = get_local_ip_address();
    
    LOG_MDNS_INFO("Created mDNS client with hostname: " << local_hostname_ << ", IP: " << local_ip_address_);
}

MdnsClient::~MdnsClient() {
    stop();
}

bool MdnsClient::start() {
    if (running_.load()) {
        LOG_MDNS_WARN("mDNS client is already running");
        return true;
    }
    
    LOG_MDNS_INFO("Starting mDNS client");
    
    // Initialize socket library (safe to call multiple times)
    if (!init_socket_library()) {
        LOG_MDNS_ERROR("Failed to initialize socket library");
        return false;
    }
    
    // Create and configure multicast socket
    if (!create_multicast_socket()) {
        LOG_MDNS_ERROR("Failed to create multicast socket");
        return false;
    }
    
    // Join multicast group
    if (!join_multicast_group()) {
        LOG_MDNS_ERROR("Failed to join multicast group");
        close_multicast_socket();
        return false;
    }
    
    running_.store(true);
    
    // Start receiver thread
    receiver_thread_ = std::thread(&MdnsClient::receiver_loop, this);
    
    LOG_MDNS_INFO("mDNS client started successfully");
    return true;
}

void MdnsClient::stop() {
    if (!running_.load()) {
        return;
    }
    
    LOG_MDNS_INFO("Stopping mDNS client");
    
    // Trigger immediate shutdown of all background threads
    shutdown_immediate();
    
    // Close socket to break receiver loop
    close_multicast_socket();
    
    // Wait for threads to finish
    if (receiver_thread_.joinable()) {
        receiver_thread_.join();
    }

    if (announcer_thread_.joinable()) {
        announcer_thread_.join();
    }

    if (querier_thread_.joinable()) {
        querier_thread_.join();
    }
    
    // Clear discovered services
    {
        std::lock_guard<std::mutex> lock(services_mutex_);
        discovered_services_.clear();
    }
    
    LOG_MDNS_INFO("mDNS client stopped");
}

void MdnsClient::shutdown_immediate() {
    LOG_MDNS_INFO("Triggering immediate shutdown of mDNS background threads");
    
    // Stop all operations
    announcing_.store(false);
    discovering_.store(false);
    running_.store(false);
    
    // Notify all waiting threads to wake up immediately
    shutdown_cv_.notify_all();
}

bool MdnsClient::is_running() const {
    return running_.load();
}

bool MdnsClient::announce_service(const std::string& instance_name, uint16_t port, 
                                 const std::map<std::string, std::string>& txt_records) {
    if (!running_.load()) {
        LOG_MDNS_ERROR("mDNS client is not running");
        return false;
    }
    
    // Update service information
    service_instance_name_ = instance_name;
    service_port_ = port;
    txt_records_ = txt_records;
    
    if (announcing_.load()) {
        LOG_MDNS_INFO("Already announcing service, updating information");
        return true;
    }
    
    LOG_MDNS_INFO("Starting service announcement for: " << instance_name << " on port " << port);
    
    announcing_.store(true);
    announcer_thread_ = std::thread(&MdnsClient::announcer_loop, this);
    
    return true;
}

void MdnsClient::stop_announcing() {
    if (!announcing_.load()) {
        return;
    }
    
    LOG_MDNS_INFO("Stopping service announcement");
    announcing_.store(false);
    
    if (announcer_thread_.joinable()) {
        announcer_thread_.join();
    }
}

bool MdnsClient::is_announcing() const {
    return announcing_.load();
}

void MdnsClient::set_service_callback(MdnsServiceCallback callback) {
    service_callback_ = callback;
}

bool MdnsClient::start_discovery() {
    if (!running_.load()) {
        LOG_MDNS_ERROR("mDNS client is not running");
        return false;
    }
    
    if (discovering_.load()) {
        LOG_MDNS_INFO("Already discovering services");
        return true;
    }
    
    LOG_MDNS_INFO("Starting service discovery");
    
    discovering_.store(true);
    querier_thread_ = std::thread(&MdnsClient::querier_loop, this);
    
    return true;
}

void MdnsClient::stop_discovery() {
    if (!discovering_.load()) {
        return;
    }
    
    LOG_MDNS_INFO("Stopping service discovery");
    discovering_.store(false);
    
    if (querier_thread_.joinable()) {
        querier_thread_.join();
    }
}

bool MdnsClient::is_discovering() const {
    return discovering_.load();
}

bool MdnsClient::query_services() {
    if (!running_.load()) {
        LOG_MDNS_ERROR("mDNS client is not running");
        return false;
    }
    
    LOG_MDNS_INFO("Sending mDNS query for librats services");
    
    DnsMessage query = create_query_message();
    std::vector<uint8_t> packet = serialize_dns_message(query);
    
    return send_multicast_packet(packet);
}

std::vector<MdnsService> MdnsClient::get_discovered_services() const {
    std::lock_guard<std::mutex> lock(services_mutex_);
    std::vector<MdnsService> services;
    
    for (const auto& pair : discovered_services_) {
        services.push_back(pair.second);
    }
    
    return services;
}

std::vector<MdnsService> MdnsClient::get_recent_services(std::chrono::seconds max_age) const {
    std::lock_guard<std::mutex> lock(services_mutex_);
    std::vector<MdnsService> recent_services;
    auto now = std::chrono::steady_clock::now();
    
    for (const auto& pair : discovered_services_) {
        const MdnsService& service = pair.second;
        auto age = std::chrono::duration_cast<std::chrono::seconds>(now - service.last_seen);
        
        if (age <= max_age) {
            recent_services.push_back(service);
        }
    }
    
    return recent_services;
}

void MdnsClient::clear_old_services(std::chrono::seconds max_age) {
    std::lock_guard<std::mutex> lock(services_mutex_);
    auto now = std::chrono::steady_clock::now();
    
    auto it = discovered_services_.begin();
    while (it != discovered_services_.end()) {
        auto age = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.last_seen);
        
        if (age > max_age) {
            LOG_MDNS_DEBUG("Removing old service: " << it->second.service_name);
            it = discovered_services_.erase(it);
        } else {
            ++it;
        }
    }
}

void MdnsClient::set_announcement_interval(std::chrono::seconds interval) {
    announcement_interval_ = interval;
}

void MdnsClient::set_query_interval(std::chrono::seconds interval) {
    query_interval_ = interval;
}

bool MdnsClient::create_multicast_socket() {
    multicast_socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (!librats::is_valid_socket(multicast_socket_)) {
#ifdef _WIN32
        LOG_MDNS_ERROR("Failed to create UDP socket (error: " << WSAGetLastError() << ")");
#else
        LOG_MDNS_ERROR("Failed to create UDP socket (error: " << strerror(errno) << ")");
#endif
        return false;
    }
    
    // Set socket options for multicast
    int reuse = 1;
    if (setsockopt(multicast_socket_, SOL_SOCKET, SO_REUSEADDR, 
                   reinterpret_cast<const char*>(&reuse), sizeof(reuse)) < 0) {
        LOG_MDNS_WARN("Failed to set SO_REUSEADDR on multicast socket");
    }
    
#ifdef SO_REUSEPORT
    if (setsockopt(multicast_socket_, SOL_SOCKET, SO_REUSEPORT, 
                   reinterpret_cast<const char*>(&reuse), sizeof(reuse)) < 0) {
        LOG_MDNS_WARN("Failed to set SO_REUSEPORT on multicast socket");
    }
#endif
    
    // Bind to mDNS port
    sockaddr_in bind_addr{};
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = INADDR_ANY;
    bind_addr.sin_port = htons(MDNS_PORT);
    
    if (bind(multicast_socket_, reinterpret_cast<sockaddr*>(&bind_addr), sizeof(bind_addr)) < 0) {
#ifdef _WIN32
        LOG_MDNS_ERROR("Failed to bind to mDNS port " << MDNS_PORT << " (error: " << WSAGetLastError() << ")");
#else
        LOG_MDNS_ERROR("Failed to bind to mDNS port " << MDNS_PORT << " (error: " << strerror(errno) << ")");
#endif
        close_socket(multicast_socket_);
        multicast_socket_ = INVALID_SOCKET_VALUE;
        return false;
    }
    
    LOG_MDNS_DEBUG("Created and bound multicast socket to port " << MDNS_PORT);
    return true;
}

bool MdnsClient::join_multicast_group() {
    if (!is_valid_socket(multicast_socket_)) {
        return false;
    }
    
    // Join IPv4 multicast group
    ip_mreq mreq{};
    inet_pton(AF_INET, MDNS_MULTICAST_IPv4.c_str(), &mreq.imr_multiaddr);
    mreq.imr_interface.s_addr = INADDR_ANY;
    
    if (setsockopt(multicast_socket_, IPPROTO_IP, IP_ADD_MEMBERSHIP, 
                   reinterpret_cast<const char*>(&mreq), sizeof(mreq)) < 0) {
#ifdef _WIN32
        LOG_MDNS_ERROR("Failed to join IPv4 multicast group (error: " << WSAGetLastError() << ")");
#else
        LOG_MDNS_ERROR("Failed to join IPv4 multicast group (error: " << strerror(errno) << ")");
#endif
        return false;
    }
    
    // Set multicast TTL
    int ttl = 255;
    if (setsockopt(multicast_socket_, IPPROTO_IP, IP_MULTICAST_TTL, 
                   reinterpret_cast<const char*>(&ttl), sizeof(ttl)) < 0) {
        LOG_MDNS_WARN("Failed to set multicast TTL");
    }
    
    // Disable loopback
    int loopback = 0;
    if (setsockopt(multicast_socket_, IPPROTO_IP, IP_MULTICAST_LOOP, 
                   reinterpret_cast<const char*>(&loopback), sizeof(loopback)) < 0) {
        LOG_MDNS_WARN("Failed to disable multicast loopback");
    }
    
    LOG_MDNS_DEBUG("Joined IPv4 multicast group: " << MDNS_MULTICAST_IPv4);
    return true;
}

bool MdnsClient::leave_multicast_group() {
    if (!is_valid_socket(multicast_socket_)) {
        return false;
    }
    
    ip_mreq mreq{};
    inet_pton(AF_INET, MDNS_MULTICAST_IPv4.c_str(), &mreq.imr_multiaddr);
    mreq.imr_interface.s_addr = INADDR_ANY;
    
    if (setsockopt(multicast_socket_, IPPROTO_IP, IP_DROP_MEMBERSHIP, 
                   reinterpret_cast<const char*>(&mreq), sizeof(mreq)) < 0) {
        LOG_MDNS_WARN("Failed to leave IPv4 multicast group");
        return false;
    }
    
    LOG_MDNS_DEBUG("Left IPv4 multicast group");
    return true;
}

void MdnsClient::close_multicast_socket() {
    if (librats::is_valid_socket(multicast_socket_)) {
        leave_multicast_group();
        librats::close_socket(multicast_socket_, true);
        multicast_socket_ = INVALID_SOCKET_VALUE;
    }
}

void MdnsClient::receiver_loop() {
    LOG_MDNS_DEBUG("mDNS receiver loop started");
    
    std::vector<uint8_t> buffer(4096);
    sockaddr_in sender_addr{};
    socklen_t addr_len = sizeof(sender_addr);
    
    while (running_.load()) {
        if (!librats::is_valid_socket(multicast_socket_)) {
            break;
        }
        
        int received = recvfrom(multicast_socket_, reinterpret_cast<char*>(buffer.data()), 
                                static_cast<int>(buffer.size()), 0, reinterpret_cast<sockaddr*>(&sender_addr), &addr_len);
        
        if (received <= 0) {
            if (running_.load()) {
                LOG_MDNS_ERROR("Failed to receive multicast packet");
            }
            break;
        }
        
        // Get sender IP address
        char sender_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &sender_addr.sin_addr, sender_ip, INET_ADDRSTRLEN);
        
        // Ignore packets from ourselves
        if (std::string(sender_ip) == local_ip_address_) {
            continue;
        }
        
        // Process the received packet
        std::vector<uint8_t> packet(buffer.begin(), buffer.begin() + received);
        handle_received_packet(packet, std::string(sender_ip));
    }
    
    LOG_MDNS_DEBUG("mDNS receiver loop ended");
}

void MdnsClient::announcer_loop() {
    LOG_MDNS_DEBUG("mDNS announcer loop started");
    
    // Send initial announcement immediately
    DnsMessage announcement = create_announcement_message();
    std::vector<uint8_t> packet = serialize_dns_message(announcement);
    send_multicast_packet(packet);
    
    auto last_announcement = std::chrono::steady_clock::now();
    
    while (announcing_.load() && running_.load()) {
        auto now = std::chrono::steady_clock::now();
        
        if (now - last_announcement >= announcement_interval_) {
            // Send periodic announcement
            announcement = create_announcement_message();
            packet = serialize_dns_message(announcement);
            
            if (send_multicast_packet(packet)) {
                LOG_MDNS_DEBUG("Sent service announcement");
            } else {
                LOG_MDNS_WARN("Failed to send service announcement");
            }
            
            last_announcement = now;
        }
        
        // Use conditional variable for responsive shutdown
        {
            std::unique_lock<std::mutex> lock(shutdown_mutex_);
            if (shutdown_cv_.wait_for(lock, std::chrono::milliseconds(500), [this] { return !announcing_.load() || !running_.load(); })) {
                break;
            }
        }
    }
    
    LOG_MDNS_DEBUG("mDNS announcer loop ended");
}

void MdnsClient::querier_loop() {
    LOG_MDNS_DEBUG("mDNS querier loop started");
    
    // Send initial query immediately
    query_services();
    
    auto last_query = std::chrono::steady_clock::now();
    
    while (discovering_.load() && running_.load()) {
        auto now = std::chrono::steady_clock::now();
        
        if (now - last_query >= query_interval_) {
            // Send periodic query
            if (query_services()) {
                LOG_MDNS_DEBUG("Sent service query");
            } else {
                LOG_MDNS_WARN("Failed to send service query");
            }
            
            last_query = now;
        }
        
        // Clean up old services
        clear_old_services(std::chrono::seconds(600));
        
        // Use conditional variable for responsive shutdown
        {
            std::unique_lock<std::mutex> lock(shutdown_mutex_);
            if (shutdown_cv_.wait_for(lock, std::chrono::milliseconds(1000), [this] { return !discovering_.load() || !running_.load(); })) {
                break;
            }
        }
    }
    
    LOG_MDNS_DEBUG("mDNS querier loop ended");
}

void MdnsClient::handle_received_packet(const std::vector<uint8_t>& packet, const std::string& sender_ip) {
    LOG_MDNS_DEBUG("Received mDNS packet from " << sender_ip << " (" << packet.size() << " bytes)");
    
    DnsMessage message;
    if (!deserialize_dns_message(packet, message)) {
        LOG_MDNS_WARN("Failed to deserialize mDNS packet from " << sender_ip);
        return;
    }
    
    process_mdns_message(message, sender_ip);
}

void MdnsClient::process_mdns_message(const DnsMessage& message, const std::string& sender_ip) {
    // Check if this is a query or response
    bool is_response = (message.header.flags & static_cast<uint16_t>(MdnsFlags::RESPONSE)) != 0;
    
    if (is_response) {
        process_response(message, sender_ip);
    } else {
        process_query(message, sender_ip);
    }
}

void MdnsClient::process_query(const DnsMessage& query, const std::string& sender_ip) {
    LOG_MDNS_DEBUG("Processing mDNS query from " << sender_ip);
    
    // Check if any questions match our announced service
    if (!announcing_.load()) {
        return; // Not announcing anything
    }
    
    for (const auto& question : query.questions) {
        bool should_respond = false;
        
        // Check if the question is asking for our service type
        if (question.name == LIBRATS_SERVICE_TYPE && question.type == DnsRecordType::PTR) {
            should_respond = true;
        }
        
        // Check if the question is asking for our specific service instance
        std::string our_service_name = create_service_instance_name(service_instance_name_);
        if (question.name == our_service_name) {
            should_respond = true;
        }
        
        if (should_respond) {
            LOG_MDNS_DEBUG("Responding to mDNS query for: " << question.name);
            
            DnsMessage response = create_response_message(question);
            std::vector<uint8_t> packet = serialize_dns_message(response);
            
            // Add a small random delay to avoid response collisions
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(20, 120);
            std::this_thread::sleep_for(std::chrono::milliseconds(dis(gen)));
            
            send_multicast_packet(packet);
        }
    }
}

void MdnsClient::process_response(const DnsMessage& response, const std::string& sender_ip) {
    LOG_MDNS_DEBUG("Processing mDNS response from " << sender_ip);
    
    extract_service_from_response(response, sender_ip);
}

void MdnsClient::extract_service_from_response(const DnsMessage& response, const std::string& sender_ip) {
    MdnsService service;
    service.ip_address = sender_ip;
    service.last_seen = std::chrono::steady_clock::now();
    
    bool has_ptr = false;
    bool has_srv = false;
    bool has_txt = false;
    
    // Process all answer records
    for (const auto& record : response.answers) {
        if (record.type == DnsRecordType::PTR && is_librats_service(record.name)) {
            // Extract service instance name from PTR record data
            size_t offset = 0;
            service.service_name = read_dns_name(record.data, offset);
            has_ptr = true;
            LOG_MDNS_DEBUG("Found PTR record: " << service.service_name);
        }
        else if (record.type == DnsRecordType::SRV) {
            // Extract SRV record data
            uint16_t priority, weight;
            if (decode_srv_record(record.data, priority, weight, service.port, service.host_name)) {
                has_srv = true;
                LOG_MDNS_DEBUG("Found SRV record: " << service.host_name << ":" << service.port);
            }
        }
        else if (record.type == DnsRecordType::TXT) {
            // Extract TXT record data
            service.txt_records = decode_txt_record(record.data);
            has_txt = true;
            LOG_MDNS_DEBUG("Found TXT record with " << service.txt_records.size() << " entries");
        }
    }
    
    // Process additional records for A records
    for (const auto& record : response.additionals) {
        if (record.type == DnsRecordType::A && record.name == service.host_name) {
            // Override IP address from A record if available
            if (record.data.size() == 4) {
                char ip_str[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, record.data.data(), ip_str, INET_ADDRSTRLEN);
                service.ip_address = std::string(ip_str);
                LOG_MDNS_DEBUG("Found A record: " << service.ip_address);
            }
        }
    }
    
    // Only add service if we have the essential information
    if (has_ptr && has_srv && !service.service_name.empty() && service.port > 0) {
        add_or_update_service(service);
    }
}

bool MdnsClient::is_librats_service(const std::string& service_name) const {
    return service_name == LIBRATS_SERVICE_TYPE;
}

void MdnsClient::add_or_update_service(const MdnsService& service) {
    bool is_new = false;
    
    {
        std::lock_guard<std::mutex> lock(services_mutex_);
        auto it = discovered_services_.find(service.service_name);
        
        if (it == discovered_services_.end()) {
            // New service
            discovered_services_[service.service_name] = service;
            is_new = true;
            LOG_MDNS_INFO("Discovered new librats service: " << service.service_name 
                         << " at " << service.ip_address << ":" << service.port);
        } else {
            // Update existing service
            it->second = service;
            LOG_MDNS_DEBUG("Updated existing librats service: " << service.service_name);
        }
    }
    
    // Call callback if registered
    if (service_callback_) {
        try {
            service_callback_(service, is_new);
        } catch (const std::exception& e) {
            LOG_MDNS_ERROR("Exception in service callback: " << e.what());
        }
    }
}

DnsMessage MdnsClient::create_query_message() {
    DnsMessage query;
    
    // Set header
    query.header.transaction_id = 0;
    query.header.flags = static_cast<uint16_t>(MdnsFlags::QUERY);
    query.header.question_count = 1;
    
    // Add question for librats services
    DnsQuestion question(LIBRATS_SERVICE_TYPE, DnsRecordType::PTR, DnsRecordClass::CLASS_IN);
    query.questions.push_back(question);
    
    return query;
}

DnsMessage MdnsClient::create_announcement_message() {
    DnsMessage announcement;
    
    // Set header
    announcement.header.transaction_id = 0;
    announcement.header.flags = static_cast<uint16_t>(MdnsFlags::AUTHORITATIVE);
    
    std::string our_service_name = create_service_instance_name(service_instance_name_);
    std::string our_hostname = local_hostname_;
    if (our_hostname.find(".local.") == std::string::npos) {
        our_hostname += ".local.";
    }
    
    // Create PTR record
    DnsResourceRecord ptr_record = create_ptr_record(LIBRATS_SERVICE_TYPE, our_service_name);
    announcement.answers.push_back(ptr_record);
    
    // Create SRV record
    DnsResourceRecord srv_record = create_srv_record(our_service_name, our_hostname, service_port_);
    announcement.answers.push_back(srv_record);
    
    // Create TXT record
    DnsResourceRecord txt_record = create_txt_record(our_service_name, txt_records_);
    announcement.answers.push_back(txt_record);
    
    // Create A record
    DnsResourceRecord a_record = create_a_record(our_hostname, local_ip_address_);
    announcement.additionals.push_back(a_record);
    
    // Update header counts
    announcement.header.answer_count = static_cast<uint16_t>(announcement.answers.size());
    announcement.header.additional_count = static_cast<uint16_t>(announcement.additionals.size());
    
    return announcement;
}

DnsMessage MdnsClient::create_response_message(const DnsQuestion& question) {
    return create_announcement_message(); // Same as announcement for simplicity
}

DnsResourceRecord MdnsClient::create_ptr_record(const std::string& service_type, const std::string& instance_name, uint32_t ttl) {
    DnsResourceRecord record(service_type, DnsRecordType::PTR, DnsRecordClass::CLASS_IN_FLUSH, ttl);
    
    // PTR record data is the instance name
    std::vector<uint8_t> data;
    write_dns_name(data, instance_name);
    record.data = data;
    
    return record;
}

DnsResourceRecord MdnsClient::create_srv_record(const std::string& instance_name, const std::string& hostname, uint16_t port, uint32_t ttl) {
    DnsResourceRecord record(instance_name, DnsRecordType::SRV, DnsRecordClass::CLASS_IN_FLUSH, ttl);
    
    // SRV record format: priority (2), weight (2), port (2), target (variable)
    record.data = encode_srv_record(0, 0, port, hostname);
    
    return record;
}

DnsResourceRecord MdnsClient::create_txt_record(const std::string& instance_name, const std::map<std::string, std::string>& txt_data, uint32_t ttl) {
    DnsResourceRecord record(instance_name, DnsRecordType::TXT, DnsRecordClass::CLASS_IN_FLUSH, ttl);
    
    record.data = encode_txt_record(txt_data);
    
    return record;
}

DnsResourceRecord MdnsClient::create_a_record(const std::string& hostname, const std::string& ip_address, uint32_t ttl) {
    DnsResourceRecord record(hostname, DnsRecordType::A, DnsRecordClass::CLASS_IN_FLUSH, ttl);
    
    // A record data is 4 bytes of IPv4 address
    sockaddr_in addr{};
    inet_pton(AF_INET, ip_address.c_str(), &addr.sin_addr);
    
    record.data.resize(4);
    std::memcpy(record.data.data(), &addr.sin_addr, 4);
    
    return record;
}

std::vector<uint8_t> MdnsClient::serialize_dns_message(const DnsMessage& message) {
    std::vector<uint8_t> buffer;
    
    // Write header
    write_uint16(buffer, message.header.transaction_id);
    write_uint16(buffer, message.header.flags);
    write_uint16(buffer, message.header.question_count);
    write_uint16(buffer, message.header.answer_count);
    write_uint16(buffer, message.header.authority_count);
    write_uint16(buffer, message.header.additional_count);
    
    // Write questions
    for (const auto& question : message.questions) {
        write_dns_name(buffer, question.name);
        write_uint16(buffer, static_cast<uint16_t>(question.type));
        write_uint16(buffer, static_cast<uint16_t>(question.record_class));
    }
    
    // Write answers
    for (const auto& record : message.answers) {
        write_dns_name(buffer, record.name);
        write_uint16(buffer, static_cast<uint16_t>(record.type));
        write_uint16(buffer, static_cast<uint16_t>(record.record_class));
        write_uint32(buffer, record.ttl);
        write_uint16(buffer, static_cast<uint16_t>(record.data.size()));
        buffer.insert(buffer.end(), record.data.begin(), record.data.end());
    }
    
    // Write authorities (same format as answers)
    for (const auto& record : message.authorities) {
        write_dns_name(buffer, record.name);
        write_uint16(buffer, static_cast<uint16_t>(record.type));
        write_uint16(buffer, static_cast<uint16_t>(record.record_class));
        write_uint32(buffer, record.ttl);
        write_uint16(buffer, static_cast<uint16_t>(record.data.size()));
        buffer.insert(buffer.end(), record.data.begin(), record.data.end());
    }
    
    // Write additionals (same format as answers)
    for (const auto& record : message.additionals) {
        write_dns_name(buffer, record.name);
        write_uint16(buffer, static_cast<uint16_t>(record.type));
        write_uint16(buffer, static_cast<uint16_t>(record.record_class));
        write_uint32(buffer, record.ttl);
        write_uint16(buffer, static_cast<uint16_t>(record.data.size()));
        buffer.insert(buffer.end(), record.data.begin(), record.data.end());
    }
    
    return buffer;
}

bool MdnsClient::deserialize_dns_message(const std::vector<uint8_t>& data, DnsMessage& message) {
    if (data.size() < 12) { // Minimum DNS header size
        return false;
    }
    
    size_t offset = 0;
    
    try {
        // Read header
        message.header.transaction_id = read_uint16(data, offset);
        message.header.flags = read_uint16(data, offset);
        message.header.question_count = read_uint16(data, offset);
        message.header.answer_count = read_uint16(data, offset);
        message.header.authority_count = read_uint16(data, offset);
        message.header.additional_count = read_uint16(data, offset);
        
        // Read questions
        message.questions.clear();
        for (uint16_t i = 0; i < message.header.question_count; ++i) {
            DnsQuestion question;
            question.name = read_dns_name(data, offset);
            question.type = static_cast<DnsRecordType>(read_uint16(data, offset));
            question.record_class = static_cast<DnsRecordClass>(read_uint16(data, offset));
            message.questions.push_back(question);
        }
        
        // Read answers
        message.answers.clear();
        for (uint16_t i = 0; i < message.header.answer_count; ++i) {
            DnsResourceRecord record;
            record.name = read_dns_name(data, offset);
            record.type = static_cast<DnsRecordType>(read_uint16(data, offset));
            record.record_class = static_cast<DnsRecordClass>(read_uint16(data, offset));
            record.ttl = read_uint32(data, offset);
            uint16_t data_length = read_uint16(data, offset);
            
            if (offset + data_length > data.size()) {
                return false;
            }
            
            record.data.assign(data.begin() + offset, data.begin() + offset + data_length);
            offset += data_length;
            
            message.answers.push_back(record);
        }
        
        // Read authorities (skip for simplicity)
        for (uint16_t i = 0; i < message.header.authority_count; ++i) {
            read_dns_name(data, offset); // name
            read_uint16(data, offset); // type
            read_uint16(data, offset); // class
            read_uint32(data, offset); // ttl
            uint16_t data_length = read_uint16(data, offset);
            offset += data_length;
        }
        
        // Read additionals
        message.additionals.clear();
        for (uint16_t i = 0; i < message.header.additional_count; ++i) {
            DnsResourceRecord record;
            record.name = read_dns_name(data, offset);
            record.type = static_cast<DnsRecordType>(read_uint16(data, offset));
            record.record_class = static_cast<DnsRecordClass>(read_uint16(data, offset));
            record.ttl = read_uint32(data, offset);
            uint16_t data_length = read_uint16(data, offset);
            
            if (offset + data_length > data.size()) {
                return false;
            }
            
            record.data.assign(data.begin() + offset, data.begin() + offset + data_length);
            offset += data_length;
            
            message.additionals.push_back(record);
        }
        
        return true;
        
    } catch (const std::exception& e) {
        LOG_MDNS_ERROR("Exception while deserializing DNS message: " << e.what());
        return false;
    }
}

void MdnsClient::write_dns_name(std::vector<uint8_t>& buffer, const std::string& name) {
    std::string normalized = normalize_dns_name(name);
    
    std::istringstream iss(normalized);
    std::string label;
    
    while (std::getline(iss, label, '.')) {
        if (label.empty()) continue;
        
        if (label.length() > 63) {
            LOG_MDNS_WARN("DNS label too long, truncating: " << label);
            label = label.substr(0, 63);
        }
        
        buffer.push_back(static_cast<uint8_t>(label.length()));
        buffer.insert(buffer.end(), label.begin(), label.end());
    }
    
    buffer.push_back(0); // Root label
}

std::string MdnsClient::read_dns_name(const std::vector<uint8_t>& buffer, size_t& offset) {
    std::string name;
    bool jumped = false;
    size_t original_offset = offset;
    int jumps = 0;
    
    while (offset < buffer.size() && jumps < 10) {
        uint8_t length = buffer[offset++];
        
        if (length == 0) {
            break; // End of name
        }
        
        if ((length & 0xC0) == 0xC0) {
            // Compression pointer
            if (!jumped) {
                original_offset = offset + 1;
                jumped = true;
            }
            
            uint16_t pointer = ((length & 0x3F) << 8) | buffer[offset++];
            offset = pointer;
            jumps++;
            continue;
        }
        
        if (offset + length > buffer.size()) {
            break; // Invalid length
        }
        
        if (!name.empty()) {
            name += ".";
        }
        
        name.append(reinterpret_cast<const char*>(&buffer[offset]), length);
        offset += length;
    }
    
    if (jumped) {
        offset = original_offset;
    }
    
    return name;
}

void MdnsClient::write_uint16(std::vector<uint8_t>& buffer, uint16_t value) {
    buffer.push_back(static_cast<uint8_t>(value >> 8));
    buffer.push_back(static_cast<uint8_t>(value & 0xFF));
}

void MdnsClient::write_uint32(std::vector<uint8_t>& buffer, uint32_t value) {
    buffer.push_back(static_cast<uint8_t>(value >> 24));
    buffer.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>(value & 0xFF));
}

uint16_t MdnsClient::read_uint16(const std::vector<uint8_t>& buffer, size_t& offset) {
    if (offset + 2 > buffer.size()) {
        throw std::out_of_range("Buffer too small for uint16");
    }
    
    uint16_t value = (static_cast<uint16_t>(buffer[offset]) << 8) | buffer[offset + 1];
    offset += 2;
    return value;
}

uint32_t MdnsClient::read_uint32(const std::vector<uint8_t>& buffer, size_t& offset) {
    if (offset + 4 > buffer.size()) {
        throw std::out_of_range("Buffer too small for uint32");
    }
    
    uint32_t value = (static_cast<uint32_t>(buffer[offset]) << 24) |
                     (static_cast<uint32_t>(buffer[offset + 1]) << 16) |
                     (static_cast<uint32_t>(buffer[offset + 2]) << 8) |
                     buffer[offset + 3];
    offset += 4;
    return value;
}

std::vector<uint8_t> MdnsClient::encode_txt_record(const std::map<std::string, std::string>& txt_data) {
    std::vector<uint8_t> data;
    
    if (txt_data.empty()) {
        // Empty TXT record
        data.push_back(0);
        return data;
    }
    
    for (const auto& pair : txt_data) {
        std::string entry = pair.first;
        if (!pair.second.empty()) {
            entry += "=" + pair.second;
        }
        
        if (entry.length() > 255) {
            LOG_MDNS_WARN("TXT record entry too long, truncating: " << entry);
            entry = entry.substr(0, 255);
        }
        
        data.push_back(static_cast<uint8_t>(entry.length()));
        data.insert(data.end(), entry.begin(), entry.end());
    }
    
    return data;
}

std::map<std::string, std::string> MdnsClient::decode_txt_record(const std::vector<uint8_t>& txt_data) {
    std::map<std::string, std::string> result;
    
    size_t offset = 0;
    while (offset < txt_data.size()) {
        uint8_t length = txt_data[offset++];
        
        if (length == 0 || offset + length > txt_data.size()) {
            break;
        }
        
        std::string entry(reinterpret_cast<const char*>(&txt_data[offset]), length);
        offset += length;
        
        // Parse key=value format
        size_t eq_pos = entry.find('=');
        if (eq_pos != std::string::npos) {
            std::string key = entry.substr(0, eq_pos);
            std::string value = entry.substr(eq_pos + 1);
            result[key] = value;
        } else {
            result[entry] = "";
        }
    }
    
    return result;
}

std::vector<uint8_t> MdnsClient::encode_srv_record(uint16_t priority, uint16_t weight, uint16_t port, const std::string& target) {
    std::vector<uint8_t> data;
    
    write_uint16(data, priority);
    write_uint16(data, weight);
    write_uint16(data, port);
    write_dns_name(data, target);
    
    return data;
}

bool MdnsClient::decode_srv_record(const std::vector<uint8_t>& srv_data, uint16_t& priority, uint16_t& weight, uint16_t& port, std::string& target) {
    if (srv_data.size() < 6) {
        return false;
    }
    
    size_t offset = 0;
    
    try {
        priority = read_uint16(srv_data, offset);
        weight = read_uint16(srv_data, offset);
        port = read_uint16(srv_data, offset);
        target = read_dns_name(srv_data, offset);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

std::string MdnsClient::get_local_hostname() {
    librats::SystemInfo sys_info = librats::get_system_info();
    std::string hostname = sys_info.hostname;
    
    if (hostname.empty()) {
        hostname = "unknown-host";
    }
    
    // Replace spaces and special characters
    std::replace_if(hostname.begin(), hostname.end(), 
                    [](char c) { return !std::isalnum(c) && c != '-'; }, '-');
    
    return hostname;
}

std::string MdnsClient::get_local_ip_address() {
    auto addresses = librats::network_utils::get_local_interface_addresses();
    
    // Prefer non-loopback IPv4 addresses
    for (const auto& addr : addresses) {
        if (addr != "127.0.0.1" && addr != "::1" && addr.find(':') == std::string::npos) {
            return addr;
        }
    }
    
    // Fall back to first available address
    if (!addresses.empty()) {
        return addresses[0];
    }
    
    return "127.0.0.1";
}

std::string MdnsClient::create_service_instance_name(const std::string& instance_name) {
    std::string clean_name = instance_name;
    
    // Replace spaces and special characters
    std::replace_if(clean_name.begin(), clean_name.end(), 
                    [](char c) { return !std::isalnum(c) && c != '-'; }, '-');
    
    if (clean_name.empty()) {
        clean_name = "librats-node";
    }
    
    return clean_name + "." + LIBRATS_SERVICE_TYPE;
}

std::string MdnsClient::extract_instance_name_from_service(const std::string& service_name) {
    size_t pos = service_name.find("." + LIBRATS_SERVICE_TYPE);
    if (pos != std::string::npos) {
        return service_name.substr(0, pos);
    }
    return service_name;
}

bool MdnsClient::send_multicast_packet(const std::vector<uint8_t>& packet) {
    if (!librats::is_valid_socket(multicast_socket_)) {
        return false;
    }
    
    sockaddr_in dest_addr{};
    dest_addr.sin_family = AF_INET;
    inet_pton(AF_INET, MDNS_MULTICAST_IPv4.c_str(), &dest_addr.sin_addr);
    dest_addr.sin_port = htons(MDNS_PORT);
    
    int sent = sendto(multicast_socket_, reinterpret_cast<const char*>(packet.data()), 
                      static_cast<int>(packet.size()), 0, reinterpret_cast<sockaddr*>(&dest_addr), sizeof(dest_addr));
    
    if (sent < 0 || static_cast<size_t>(sent) != packet.size()) {
#ifdef _WIN32
        LOG_MDNS_ERROR("Failed to send multicast packet (error: " << WSAGetLastError() << ")");
#else
        LOG_MDNS_ERROR("Failed to send multicast packet (error: " << strerror(errno) << ")");
#endif
        return false;
    }
    
    return true;
}

bool MdnsClient::is_valid_dns_name(const std::string& name) const {
    if (name.empty() || name.length() > 255) {
        return false;
    }
    
    return true; // Simplified validation
}

std::string MdnsClient::normalize_dns_name(const std::string& name) const {
    std::string normalized = name;
    
    // Ensure name ends with dot if not empty
    if (!normalized.empty() && normalized.back() != '.') {
        normalized += '.';
    }
    
    return normalized;
}

} // namespace librats 