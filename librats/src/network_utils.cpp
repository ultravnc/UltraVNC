#ifdef _WIN32
    // Include winsock2.h first to avoid conflicts with windows.h
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <iphlpapi.h>
#else
    #include <netdb.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>

    #ifdef RATS_ANDROID_OLD_API
        #include <ifaddrs-android.h>
    #else
        #include <ifaddrs.h>
    #endif
#endif



#include "network_utils.h"
#include "logger.h"
#include <cstring>
#include <iostream>
#include <vector>

// Network utilities module logging macros
#define LOG_NETUTILS_DEBUG(message) LOG_DEBUG("network_utils", message)
#define LOG_NETUTILS_INFO(message)  LOG_INFO("network_utils", message)
#define LOG_NETUTILS_WARN(message)  LOG_WARN("network_utils", message)
#define LOG_NETUTILS_ERROR(message) LOG_ERROR("network_utils", message)

namespace librats {
namespace network_utils {

std::string resolve_hostname(const std::string& hostname) {
    LOG_NETUTILS_DEBUG("Resolving hostname: " << hostname);
    
    // Handle empty string
    if (hostname.empty()) {
        LOG_NETUTILS_DEBUG("Empty hostname provided");
        return "";
    }
    
    // Check if it's already an IP address
    if (is_valid_ipv4(hostname)) {
        LOG_NETUTILS_DEBUG("Already an IP address: " << hostname);
        return hostname;
    }
    
    // Resolve hostname using getaddrinfo
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;  // IPv4
    hints.ai_socktype = SOCK_DGRAM;  // UDP
    
    int status = getaddrinfo(hostname.c_str(), nullptr, &hints, &result);
    if (status != 0) {
#ifdef _WIN32
        LOG_NETUTILS_ERROR("Failed to resolve hostname " << hostname << ": " << WSAGetLastError());
#else
        LOG_NETUTILS_ERROR("Failed to resolve hostname " << hostname << ": " << gai_strerror(status));
#endif
        return "";
    }
    
    char ip_str[INET_ADDRSTRLEN];
    struct sockaddr_in* addr_in = (struct sockaddr_in*)result->ai_addr;
    inet_ntop(AF_INET, &addr_in->sin_addr, ip_str, INET_ADDRSTRLEN);
    
    freeaddrinfo(result);
    
    LOG_NETUTILS_INFO("Resolved " << hostname << " to " << ip_str);
    return std::string(ip_str);
}

std::string resolve_hostname_v6(const std::string& hostname) {
    LOG_NETUTILS_DEBUG("Resolving hostname to IPv6: " << hostname);
    
    // Handle empty string
    if (hostname.empty()) {
        LOG_NETUTILS_DEBUG("Empty hostname provided");
        return "";
    }
    
    // Check if it's already an IPv6 address
    if (is_valid_ipv6(hostname)) {
        LOG_NETUTILS_DEBUG("Already an IPv6 address: " << hostname);
        return hostname;
    }
    
    // Resolve hostname using getaddrinfo
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6;  // IPv6
    hints.ai_socktype = SOCK_DGRAM;  // UDP
    
    int status = getaddrinfo(hostname.c_str(), nullptr, &hints, &result);
    if (status != 0) {
#ifdef _WIN32
        LOG_NETUTILS_ERROR("Failed to resolve hostname " << hostname << " to IPv6: " << WSAGetLastError());
#else
        LOG_NETUTILS_ERROR("Failed to resolve hostname " << hostname << " to IPv6: " << gai_strerror(status));
#endif
        return "";
    }
    
    char ip_str[INET6_ADDRSTRLEN];
    struct sockaddr_in6* addr_in6 = (struct sockaddr_in6*)result->ai_addr;
    inet_ntop(AF_INET6, &addr_in6->sin6_addr, ip_str, INET6_ADDRSTRLEN);
    
    freeaddrinfo(result);
    
    LOG_NETUTILS_INFO("Resolved " << hostname << " to IPv6 " << ip_str);
    return std::string(ip_str);
}

bool is_valid_ipv4(const std::string& ip_str) {
#ifdef __APPLE__
    // macOS-specific validation to handle inet_pton() accepting leading zeros
    // inet_pton() behaves differently on macOS vs Linux/Windows for leading zeros
    
    // Split the IP string into octets
    std::vector<std::string> octets;
    std::string current_octet;
    
    for (char c : ip_str) {
        if (c == '.') {
            octets.push_back(current_octet);
            current_octet.clear();
        } else {
            current_octet += c;
        }
    }
    
    // Add the last octet
    if (!current_octet.empty()) {
        octets.push_back(current_octet);
    }
    
    // Must have exactly 4 octets
    if (octets.size() != 4) {
        return false;
    }
    
    // Check each octet for validity
    for (const std::string& octet : octets) {
        // Empty octet is invalid
        if (octet.empty()) {
            return false;
        }
        
        // Check for leading zeros (invalid except for "0")
        if (octet.length() > 1 && octet[0] == '0') {
            return false;
        }
        
        // Check that all characters are digits
        for (char c : octet) {
            if (c < '0' || c > '9') {
                return false;
            }
        }
        
        // Check range (0-255)
        int value = std::stoi(octet);
        if (value < 0 || value > 255) {
            return false;
        }
    }
#endif
    // Linux/Windows: inet_pton() already rejects leading zeros correctly
    struct sockaddr_in sa;
    return inet_pton(AF_INET, ip_str.c_str(), &sa.sin_addr) == 1;
}

bool is_valid_ipv6(const std::string& ip_str) {
    struct sockaddr_in6 sa;
    return inet_pton(AF_INET6, ip_str.c_str(), &sa.sin6_addr) == 1;
}

bool is_hostname(const std::string& str) {
    // First check if it's an IP address - if so, it's not a hostname
    if (is_valid_ipv4(str) || is_valid_ipv6(str)) {
        return false;
    }
    
    // Check basic validation rules
    if (str.empty() || str.length() > 253) {
        return false;
    }
    
    // Check for invalid characters at start/end
    if (str.front() == '.' || str.back() == '.') {
        return false;
    }
    
    if (str.front() == '-' || str.back() == '-') {
        return false;
    }
    
    // Check for invalid patterns
    if (str.find("..") != std::string::npos) {
        return false;
    }
    
    if (str == ".") {
        return false;
    }
    
    // Check for invalid characters
    for (char c : str) {
        if (c == ' ' || c == '@' || c == '#' || c == '$' || c == '%' || 
            c == '^' || c == '&' || c == '*' || c == '(' || c == ')' || 
            c == '+' || c == '=' || c == '[' || c == ']' || c == '{' || 
            c == '}' || c == '|' || c == '\\' || c == '/' || c == '?' || 
            c == '<' || c == '>' || c == ',' || c == ';' || c == ':' || 
            c == '"' || c == '\'' || c == '`' || c == '~' || c == '!') {
            return false;
        }
    }
    
    return true;
}

std::string to_ip_address(const std::string& host) {
    return resolve_hostname(host);
}

std::vector<std::string> resolve_all_addresses(const std::string& hostname) {
    LOG_NETUTILS_DEBUG("Resolving all addresses for hostname: " << hostname);
    
    std::vector<std::string> addresses;
    
    // Check if it's already an IP address
    if (is_valid_ipv4(hostname)) {
        LOG_NETUTILS_DEBUG("Already an IP address: " << hostname);
        addresses.push_back(hostname);
        return addresses;
    }
    
    // Resolve hostname using getaddrinfo
    struct addrinfo hints, *result, *rp;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;  // IPv4
    hints.ai_socktype = SOCK_DGRAM;  // UDP
    
    int status = getaddrinfo(hostname.c_str(), nullptr, &hints, &result);
    if (status != 0) {
#ifdef _WIN32
        LOG_NETUTILS_ERROR("Failed to resolve hostname " << hostname << ": " << WSAGetLastError());
#else
        LOG_NETUTILS_ERROR("Failed to resolve hostname " << hostname << ": " << gai_strerror(status));
#endif
        return addresses;
    }
    
    // Iterate through all addresses
    for (rp = result; rp != nullptr; rp = rp->ai_next) {
        char ip_str[INET_ADDRSTRLEN];
        struct sockaddr_in* addr_in = (struct sockaddr_in*)rp->ai_addr;
        inet_ntop(AF_INET, &addr_in->sin_addr, ip_str, INET_ADDRSTRLEN);
        
        std::string ip_address(ip_str);
        addresses.push_back(ip_address);
        LOG_NETUTILS_DEBUG("Found address: " << ip_address);
    }
    
    freeaddrinfo(result);
    
    LOG_NETUTILS_INFO("Resolved " << hostname << " to " << addresses.size() << " addresses");
    return addresses;
}

std::vector<std::string> resolve_all_addresses_v6(const std::string& hostname) {
    LOG_NETUTILS_DEBUG("Resolving all IPv6 addresses for hostname: " << hostname);
    
    std::vector<std::string> addresses;
    
    // Check if it's already an IPv6 address
    if (is_valid_ipv6(hostname)) {
        LOG_NETUTILS_DEBUG("Already an IPv6 address: " << hostname);
        addresses.push_back(hostname);
        return addresses;
    }
    
    // Resolve hostname using getaddrinfo
    struct addrinfo hints, *result, *rp;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6;  // IPv6
    hints.ai_socktype = SOCK_DGRAM;  // UDP
    
    int status = getaddrinfo(hostname.c_str(), nullptr, &hints, &result);
    if (status != 0) {
#ifdef _WIN32
        LOG_NETUTILS_ERROR("Failed to resolve hostname " << hostname << " to IPv6: " << WSAGetLastError());
#else
        LOG_NETUTILS_ERROR("Failed to resolve hostname " << hostname << " to IPv6: " << gai_strerror(status));
#endif
        return addresses;
    }
    
    // Iterate through all addresses
    for (rp = result; rp != nullptr; rp = rp->ai_next) {
        char ip_str[INET6_ADDRSTRLEN];
        struct sockaddr_in6* addr_in6 = (struct sockaddr_in6*)rp->ai_addr;
        inet_ntop(AF_INET6, &addr_in6->sin6_addr, ip_str, INET6_ADDRSTRLEN);
        
        std::string ip_address(ip_str);
        addresses.push_back(ip_address);
        LOG_NETUTILS_DEBUG("Found IPv6 address: " << ip_address);
    }
    
    freeaddrinfo(result);
    
    LOG_NETUTILS_INFO("Resolved " << hostname << " to " << addresses.size() << " IPv6 addresses");
    return addresses;
}

std::vector<std::string> resolve_all_addresses_dual(const std::string& hostname) {
    LOG_NETUTILS_DEBUG("Resolving all addresses (dual stack) for hostname: " << hostname);
    
    std::vector<std::string> addresses;
    
    // Check if it's already an IP address
    if (is_valid_ipv4(hostname)) {
        LOG_NETUTILS_DEBUG("Already an IPv4 address: " << hostname);
        addresses.push_back(hostname);
        return addresses;
    }
    
    if (is_valid_ipv6(hostname)) {
        LOG_NETUTILS_DEBUG("Already an IPv6 address: " << hostname);
        addresses.push_back(hostname);
        return addresses;
    }
    
    // Resolve hostname using getaddrinfo with dual stack
    struct addrinfo hints, *result, *rp;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;  // Both IPv4 and IPv6
    hints.ai_socktype = SOCK_DGRAM;  // UDP
    
    int status = getaddrinfo(hostname.c_str(), nullptr, &hints, &result);
    if (status != 0) {
#ifdef _WIN32
        LOG_NETUTILS_ERROR("Failed to resolve hostname " << hostname << " (dual stack): " << WSAGetLastError());
#else
        LOG_NETUTILS_ERROR("Failed to resolve hostname " << hostname << " (dual stack): " << gai_strerror(status));
#endif
        return addresses;
    }
    
    // Iterate through all addresses
    for (rp = result; rp != nullptr; rp = rp->ai_next) {
        if (rp->ai_family == AF_INET) {
            char ip_str[INET_ADDRSTRLEN];
            struct sockaddr_in* addr_in = (struct sockaddr_in*)rp->ai_addr;
            inet_ntop(AF_INET, &addr_in->sin_addr, ip_str, INET_ADDRSTRLEN);
            
            std::string ip_address(ip_str);
            addresses.push_back(ip_address);
            LOG_NETUTILS_DEBUG("Found IPv4 address: " << ip_address);
        } else if (rp->ai_family == AF_INET6) {
            char ip_str[INET6_ADDRSTRLEN];
            struct sockaddr_in6* addr_in6 = (struct sockaddr_in6*)rp->ai_addr;
            inet_ntop(AF_INET6, &addr_in6->sin6_addr, ip_str, INET6_ADDRSTRLEN);
            
            std::string ip_address(ip_str);
            addresses.push_back(ip_address);
            LOG_NETUTILS_DEBUG("Found IPv6 address: " << ip_address);
        }
    }
    
    freeaddrinfo(result);
    
    LOG_NETUTILS_INFO("Resolved " << hostname << " to " << addresses.size() << " addresses (dual stack)");
    return addresses;
}

std::vector<std::string> get_local_interface_addresses_v4() {
    LOG_NETUTILS_DEBUG("Getting local IPv4 interface addresses");
    
    std::vector<std::string> addresses;
    
#ifdef _WIN32
    // Windows implementation using GetAdaptersAddresses
    DWORD dwRetVal = 0;
    ULONG outBufLen = 15000;
    ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
    
    PIP_ADAPTER_ADDRESSES pAddresses = nullptr;
    PIP_ADAPTER_ADDRESSES pCurrAddresses = nullptr;
    PIP_ADAPTER_UNICAST_ADDRESS pUnicast = nullptr;
    
    do {
        pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
        if (pAddresses == nullptr) {
            LOG_NETUTILS_ERROR("Memory allocation failed for GetAdaptersAddresses");
            return addresses;
        }

        dwRetVal = GetAdaptersAddresses(AF_INET, flags, nullptr, pAddresses, &outBufLen);

        if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
            free(pAddresses);
            pAddresses = nullptr;
        } else {
            break;
        }
    } while ((dwRetVal == ERROR_BUFFER_OVERFLOW) && (outBufLen < 65535));

    if (dwRetVal == NO_ERROR) {
        pCurrAddresses = pAddresses;
        while (pCurrAddresses) {
            pUnicast = pCurrAddresses->FirstUnicastAddress;
            while (pUnicast != nullptr) {
                if (pUnicast->Address.lpSockaddr->sa_family == AF_INET) {
                    sockaddr_in* sockaddr_ipv4 = (sockaddr_in*)pUnicast->Address.lpSockaddr;
                    char ip_str[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &sockaddr_ipv4->sin_addr, ip_str, INET_ADDRSTRLEN);
                    std::string ip_address(ip_str);
                    addresses.push_back(ip_address);
                    LOG_NETUTILS_DEBUG("Found local IPv4 address: " << ip_address);
                }
                pUnicast = pUnicast->Next;
            }
            pCurrAddresses = pCurrAddresses->Next;
        }
    } else {
        LOG_NETUTILS_ERROR("GetAdaptersAddresses failed with error: " << dwRetVal);
    }

    if (pAddresses) {
        free(pAddresses);
    }

#else
    // Linux/Unix implementation using getifaddrs
    struct ifaddrs *ifaddr, *ifa;
    
    if (getifaddrs(&ifaddr) == -1) {
        LOG_NETUTILS_ERROR("getifaddrs failed");
        return addresses;
    }

    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) continue;
        
        if (ifa->ifa_addr->sa_family == AF_INET) {
            char ip_str[INET_ADDRSTRLEN];
            struct sockaddr_in* addr_in = (struct sockaddr_in*)ifa->ifa_addr;
            inet_ntop(AF_INET, &addr_in->sin_addr, ip_str, INET_ADDRSTRLEN);
            std::string ip_address(ip_str);
            addresses.push_back(ip_address);
            LOG_NETUTILS_DEBUG("Found local IPv4 address: " << ip_address << " on interface " << ifa->ifa_name);
        }
    }

    freeifaddrs(ifaddr);
#endif
    
    LOG_NETUTILS_INFO("Found " << addresses.size() << " local IPv4 addresses");
    return addresses;
}

std::vector<std::string> get_local_interface_addresses_v6() {
    LOG_NETUTILS_DEBUG("Getting local IPv6 interface addresses");
    
    std::vector<std::string> addresses;
    
#ifdef _WIN32
    // Windows implementation using GetAdaptersAddresses
    DWORD dwRetVal = 0;
    ULONG outBufLen = 15000;
    ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
    
    PIP_ADAPTER_ADDRESSES pAddresses = nullptr;
    PIP_ADAPTER_ADDRESSES pCurrAddresses = nullptr;
    PIP_ADAPTER_UNICAST_ADDRESS pUnicast = nullptr;
    
    do {
        pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
        if (pAddresses == nullptr) {
            LOG_NETUTILS_ERROR("Memory allocation failed for GetAdaptersAddresses");
            return addresses;
        }

        dwRetVal = GetAdaptersAddresses(AF_INET6, flags, nullptr, pAddresses, &outBufLen);

        if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
            free(pAddresses);
            pAddresses = nullptr;
        } else {
            break;
        }
    } while ((dwRetVal == ERROR_BUFFER_OVERFLOW) && (outBufLen < 65535));

    if (dwRetVal == NO_ERROR) {
        pCurrAddresses = pAddresses;
        while (pCurrAddresses) {
            pUnicast = pCurrAddresses->FirstUnicastAddress;
            while (pUnicast != nullptr) {
                if (pUnicast->Address.lpSockaddr->sa_family == AF_INET6) {
                    sockaddr_in6* sockaddr_ipv6 = (sockaddr_in6*)pUnicast->Address.lpSockaddr;
                    char ip_str[INET6_ADDRSTRLEN];
                    inet_ntop(AF_INET6, &sockaddr_ipv6->sin6_addr, ip_str, INET6_ADDRSTRLEN);
                    std::string ip_address(ip_str);
                    addresses.push_back(ip_address);
                    LOG_NETUTILS_DEBUG("Found local IPv6 address: " << ip_address);
                }
                pUnicast = pUnicast->Next;
            }
            pCurrAddresses = pCurrAddresses->Next;
        }
    } else {
        LOG_NETUTILS_ERROR("GetAdaptersAddresses failed with error: " << dwRetVal);
    }

    if (pAddresses) {
        free(pAddresses);
    }

#else
    // Linux/Unix implementation using getifaddrs
    struct ifaddrs *ifaddr, *ifa;
    
    if (getifaddrs(&ifaddr) == -1) {
        LOG_NETUTILS_ERROR("getifaddrs failed");
        return addresses;
    }

    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) continue;
        
        if (ifa->ifa_addr->sa_family == AF_INET6) {
            char ip_str[INET6_ADDRSTRLEN];
            struct sockaddr_in6* addr_in6 = (struct sockaddr_in6*)ifa->ifa_addr;
            inet_ntop(AF_INET6, &addr_in6->sin6_addr, ip_str, INET6_ADDRSTRLEN);
            std::string ip_address(ip_str);
            addresses.push_back(ip_address);
            LOG_NETUTILS_DEBUG("Found local IPv6 address: " << ip_address << " on interface " << ifa->ifa_name);
        }
    }

    freeifaddrs(ifaddr);
#endif
    
    LOG_NETUTILS_INFO("Found " << addresses.size() << " local IPv6 addresses");
    return addresses;
}

std::vector<std::string> get_local_interface_addresses() {
    LOG_NETUTILS_DEBUG("Getting all local interface addresses (IPv4 and IPv6)");
    
    std::vector<std::string> addresses;
    
    // Get IPv4 addresses
    auto ipv4_addresses = get_local_interface_addresses_v4();
    addresses.insert(addresses.end(), ipv4_addresses.begin(), ipv4_addresses.end());
    
    // Get IPv6 addresses
    auto ipv6_addresses = get_local_interface_addresses_v6();
    addresses.insert(addresses.end(), ipv6_addresses.begin(), ipv6_addresses.end());
    
    LOG_NETUTILS_INFO("Found " << addresses.size() << " total local interface addresses (" 
                      << ipv4_addresses.size() << " IPv4, " << ipv6_addresses.size() << " IPv6)");
    
    return addresses;
}

bool is_local_interface_address(const std::string& ip_address) {
    LOG_NETUTILS_DEBUG("Checking if " << ip_address << " is a local interface address");
    
    // Get all local addresses and check if the given address is in the list
    auto local_addresses = get_local_interface_addresses();
    
    for (const auto& local_addr : local_addresses) {
        if (local_addr == ip_address) {
            LOG_NETUTILS_DEBUG(ip_address << " is a local interface address");
            return true;
        }
    }
    
    // Also check common localhost addresses
    if (ip_address == "127.0.0.1" || ip_address == "::1" || ip_address == "localhost") {
        LOG_NETUTILS_DEBUG(ip_address << " is a localhost address");
        return true;
    }
    
    LOG_NETUTILS_DEBUG(ip_address << " is not a local interface address");
    return false;
}

} // namespace network_utils
} // namespace librats 
