#include "librats.h"
#include "mdns.h"
#include <iostream>
#include <algorithm>
#include <chrono>
#include <memory>
#include <stdexcept>

#ifdef TESTING
#define LOG_CLIENT_DEBUG(message) LOG_DEBUG("client", "[pointer: " << this << "] " << message)
#define LOG_CLIENT_INFO(message)  LOG_INFO("client", "[pointer: " << this << "] " << message)
#define LOG_CLIENT_WARN(message)  LOG_WARN("client", "[pointer: " << this << "] " << message)
#define LOG_CLIENT_ERROR(message) LOG_ERROR("client", "[pointer: " << this << "] " << message)
#else
#define LOG_CLIENT_DEBUG(message) LOG_DEBUG("client", message)
#define LOG_CLIENT_INFO(message)  LOG_INFO("client", message)
#define LOG_CLIENT_WARN(message)  LOG_WARN("client", message)
#define LOG_CLIENT_ERROR(message) LOG_ERROR("client", message)
#endif

namespace librats {

// ===== mDNS DISCOVERY METHODS IMPLEMENTATION =====

bool RatsClient::start_mdns_discovery(const std::string& service_instance_name, 
                                     const std::map<std::string, std::string>& txt_records) {
    if (!running_.load()) {
        LOG_CLIENT_ERROR("RatsClient is not running, cannot start mDNS discovery");
        return false;
    }
    
    if (mdns_client_ && mdns_client_->is_running()) {
        LOG_CLIENT_WARN("mDNS discovery is already running");
        return true;
    }
    
    LOG_CLIENT_INFO("Starting mDNS service discovery and announcement");
    
    // Create service instance name from our peer ID if not provided
    std::string instance_name = service_instance_name;
    if (instance_name.empty()) {
        instance_name = "librats-" + get_our_peer_id().substr(0, 8);
    }
    
    // Create mDNS client
    mdns_client_ = std::make_unique<MdnsClient>(instance_name, listen_port_);
    
    // Set service discovery callback
    mdns_client_->set_service_callback([this](const MdnsService& service, bool is_new) {
        handle_mdns_service_discovery(service, is_new);
    });
    
    // Start mDNS client
    if (!mdns_client_->start()) {
        LOG_CLIENT_ERROR("Failed to start mDNS client");
        mdns_client_.reset();
        return false;
    }
    
    // Start service announcement
    if (!mdns_client_->announce_service(instance_name, listen_port_, txt_records)) {
        LOG_CLIENT_WARN("Failed to start mDNS service announcement");
    }
    
    // Start service discovery
    if (!mdns_client_->start_discovery()) {
        LOG_CLIENT_WARN("Failed to start mDNS service discovery");
    }
    
    LOG_CLIENT_INFO("mDNS discovery started successfully for service: " << instance_name);
    return true;
}

void RatsClient::stop_mdns_discovery() {
    if (!mdns_client_) {
        return;
    }
    
    LOG_CLIENT_INFO("Stopping mDNS discovery");
    
    mdns_client_->stop();
    mdns_client_.reset();
    
    LOG_CLIENT_INFO("mDNS discovery stopped");
}

bool RatsClient::is_mdns_running() const {
    return mdns_client_ && mdns_client_->is_running();
}

void RatsClient::set_mdns_callback(std::function<void(const std::string&, int, const std::string&)> callback) {
    mdns_callback_ = callback;
}

std::vector<MdnsService> RatsClient::get_mdns_services() const {
    if (!mdns_client_) {
        return {};
    }
    
    return mdns_client_->get_recent_services(std::chrono::seconds(300)); // Services seen in last 5 minutes
}

bool RatsClient::query_mdns_services() {
    if (!mdns_client_) {
        LOG_CLIENT_ERROR("mDNS client not initialized");
        return false;
    }
    
    return mdns_client_->query_services();
}

void RatsClient::handle_mdns_service_discovery(const MdnsService& service, bool is_new) {
    LOG_CLIENT_INFO("mDNS discovered " << (is_new ? "new" : "updated") << " librats service: " 
                   << service.service_name << " at " << service.ip_address << ":" << service.port);
    
    // Extract instance name from service name for logging
    std::string instance_name = service.service_name;
    size_t pos = instance_name.find("._librats._tcp.local.");
    if (pos != std::string::npos) {
        instance_name = instance_name.substr(0, pos);
    }
    
    // Call user callback if registered
    if (mdns_callback_) {
        try {
            mdns_callback_(service.ip_address, service.port, instance_name);
        } catch (const std::exception& e) {
            LOG_CLIENT_ERROR("Exception in mDNS callback: " << e.what());
        }
    }
    
    // Auto-connect to discovered services if they're new
    if (is_new) {
        // Check if this peer should be ignored (local interface)
        if (should_ignore_peer(service.ip_address, service.port)) {
            LOG_CLIENT_DEBUG("Ignoring mDNS discovered peer " << service.ip_address << ":" 
                            << service.port << " - local interface address");
            return;
        }
        
        // Check if we're already connected to this peer
        std::string normalized_peer_address = normalize_peer_address(service.ip_address, service.port);
        if (is_already_connected_to_address(normalized_peer_address)) {
            LOG_CLIENT_DEBUG("Already connected to mDNS discovered peer: " << normalized_peer_address);
            return;
        }
        
        // Check if peer limit is reached
        if (is_peer_limit_reached()) {
            LOG_CLIENT_DEBUG("Peer limit reached, not connecting to mDNS discovered peer " 
                            << service.ip_address << ":" << service.port);
            return;
        }
        
        LOG_CLIENT_INFO("Attempting to connect to mDNS discovered peer: " 
                       << service.ip_address << ":" << service.port);
        
        // Try to connect to the discovered peer (non-blocking)
        std::thread([this, service]() {
            if (connect_to_peer(service.ip_address, service.port)) {
                LOG_CLIENT_INFO("Successfully connected to mDNS discovered peer: " 
                               << service.ip_address << ":" << service.port);
            } else {
                LOG_CLIENT_DEBUG("Failed to connect to mDNS discovered peer: " 
                                << service.ip_address << ":" << service.port);
            }
        }).detach();
    }
}

} // namespace librats
