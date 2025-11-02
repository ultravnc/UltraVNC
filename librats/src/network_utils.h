#pragma once

#include <string>
#include <vector>

namespace librats {
namespace network_utils {

/**
 * Resolve hostname to IP address
 * @param hostname The hostname to resolve (can be hostname or IP address)
 * @return IP address string, or empty string on error
 * 
 * Example usage:
 *   std::string ip = network_utils::resolve_hostname("google.com");
 *   std::string ip2 = network_utils::resolve_hostname("192.168.1.1"); // returns same IP
 */
std::string resolve_hostname(const std::string& hostname);

/**
 * Resolve hostname to IPv6 address
 * @param hostname The hostname to resolve (can be hostname or IPv6 address)
 * @return IPv6 address string, or empty string on error
 * 
 * Example usage:
 *   std::string ipv6 = network_utils::resolve_hostname_v6("google.com");
 *   std::string ipv6_2 = network_utils::resolve_hostname_v6("::1"); // returns same IPv6
 */
std::string resolve_hostname_v6(const std::string& hostname);

/**
 * Check if a string is a valid IPv4 address
 * @param ip_str The string to validate
 * @return true if valid IPv4 address, false otherwise
 * 
 * Example usage:
 *   bool valid = network_utils::is_valid_ipv4("192.168.1.1"); // true
 *   bool invalid = network_utils::is_valid_ipv4("invalid.ip"); // false
 */
bool is_valid_ipv4(const std::string& ip_str);

/**
 * Check if a string is a valid IPv6 address
 * @param ip_str The string to validate
 * @return true if valid IPv6 address, false otherwise
 * 
 * Example usage:
 *   bool valid = network_utils::is_valid_ipv6("::1"); // true
 *   bool valid2 = network_utils::is_valid_ipv6("2001:db8::1"); // true
 *   bool invalid = network_utils::is_valid_ipv6("invalid.ip"); // false
 */
bool is_valid_ipv6(const std::string& ip_str);

/**
 * Check if a string is a hostname (not an IP address)
 * @param str The string to check
 * @return true if it's a hostname, false if it's an IP address
 * 
 * Example usage:
 *   bool is_host = network_utils::is_hostname("google.com"); // true
 *   bool is_ip = network_utils::is_hostname("192.168.1.1"); // false
 *   bool is_ipv6 = network_utils::is_hostname("::1"); // false
 */
bool is_hostname(const std::string& str);

/**
 * Convert hostname or IP to IP address (alias for resolve_hostname)
 * @param host The hostname or IP address to convert
 * @return IP address string, or empty string on error
 * 
 * Example usage:
 *   std::string ip = network_utils::to_ip_address("example.com");
 */
std::string to_ip_address(const std::string& host);

/**
 * Get all IP addresses for a hostname
 * @param hostname The hostname to resolve
 * @return Vector of IP addresses, empty if resolution fails
 * 
 * Example usage:
 *   auto ips = network_utils::resolve_all_addresses("google.com");
 *   for (const auto& ip : ips) {
 *       std::cout << "IP: " << ip << std::endl;
 *   }
 */
std::vector<std::string> resolve_all_addresses(const std::string& hostname);

/**
 * Get all IPv6 addresses for a hostname
 * @param hostname The hostname to resolve
 * @return Vector of IPv6 addresses, empty if resolution fails
 * 
 * Example usage:
 *   auto ipv6s = network_utils::resolve_all_addresses_v6("google.com");
 *   for (const auto& ipv6 : ipv6s) {
 *       std::cout << "IPv6: " << ipv6 << std::endl;
 *   }
 */
std::vector<std::string> resolve_all_addresses_v6(const std::string& hostname);

/**
 * Get all IP addresses (both IPv4 and IPv6) for a hostname
 * @param hostname The hostname to resolve
 * @return Vector of IP addresses, empty if resolution fails
 * 
 * Example usage:
 *   auto ips = network_utils::resolve_all_addresses_dual("google.com");
 *   for (const auto& ip : ips) {
 *       std::cout << "IP: " << ip << std::endl;
 *   }
 */
std::vector<std::string> resolve_all_addresses_dual(const std::string& hostname);

/**
 * Get all local network interface addresses (IPv4 and IPv6)
 * @return Vector of local IP addresses from all network interfaces
 * 
 * Example usage:
 *   auto local_ips = network_utils::get_local_interface_addresses();
 *   for (const auto& ip : local_ips) {
 *       std::cout << "Local IP: " << ip << std::endl;
 *   }
 */
std::vector<std::string> get_local_interface_addresses();

/**
 * Get all local IPv4 network interface addresses
 * @return Vector of local IPv4 addresses from all network interfaces
 * 
 * Example usage:
 *   auto local_ipv4s = network_utils::get_local_interface_addresses_v4();
 *   for (const auto& ip : local_ipv4s) {
 *       std::cout << "Local IPv4: " << ip << std::endl;
 *   }
 */
std::vector<std::string> get_local_interface_addresses_v4();

/**
 * Get all local IPv6 network interface addresses
 * @return Vector of local IPv6 addresses from all network interfaces
 * 
 * Example usage:
 *   auto local_ipv6s = network_utils::get_local_interface_addresses_v6();
 *   for (const auto& ip : local_ipv6s) {
 *       std::cout << "Local IPv6: " << ip << std::endl;
 *   }
 */
std::vector<std::string> get_local_interface_addresses_v6();

/**
 * Check if an IP address is a local interface address
 * @param ip_address The IP address to check
 * @return true if the IP address belongs to a local network interface
 * 
 * Example usage:
 *   bool is_local = network_utils::is_local_interface_address("192.168.1.100");
 */
bool is_local_interface_address(const std::string& ip_address);

} // namespace network_utils
} // namespace librats 