#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "network_utils.h"
#include "socket.h"
#include <string>

using namespace librats;

class NetworkUtilsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize socket library for Windows
        init_socket_library();
    }
    
    void TearDown() override {
        // Cleanup socket library
        cleanup_socket_library();
    }
};

// Test IPv4 address validation
TEST_F(NetworkUtilsTest, IPv4ValidationTest) {
    // Valid IPv4 addresses
    EXPECT_TRUE(network_utils::is_valid_ipv4("127.0.0.1"));
    EXPECT_TRUE(network_utils::is_valid_ipv4("192.168.1.1"));
    EXPECT_TRUE(network_utils::is_valid_ipv4("0.0.0.0"));
    EXPECT_TRUE(network_utils::is_valid_ipv4("255.255.255.255"));
    EXPECT_TRUE(network_utils::is_valid_ipv4("10.0.0.1"));
    EXPECT_TRUE(network_utils::is_valid_ipv4("172.16.0.1"));
    EXPECT_TRUE(network_utils::is_valid_ipv4("8.8.8.8"));
    EXPECT_TRUE(network_utils::is_valid_ipv4("1.1.1.1"));
    
    // Invalid IPv4 addresses
    EXPECT_FALSE(network_utils::is_valid_ipv4("256.0.0.1"));       // Out of range
    EXPECT_FALSE(network_utils::is_valid_ipv4("192.168.1.256"));   // Out of range
    EXPECT_FALSE(network_utils::is_valid_ipv4("192.168.1"));       // Missing octet
    EXPECT_FALSE(network_utils::is_valid_ipv4("192.168.1.1.1"));   // Extra octet
    EXPECT_FALSE(network_utils::is_valid_ipv4("192.168.01.1"));    // Leading zero
    EXPECT_FALSE(network_utils::is_valid_ipv4("192.168.1.-1"));    // Negative number
    EXPECT_FALSE(network_utils::is_valid_ipv4("192.168.1.a"));     // Non-numeric
    EXPECT_FALSE(network_utils::is_valid_ipv4(""));                // Empty string
    EXPECT_FALSE(network_utils::is_valid_ipv4("192.168.1."));      // Trailing dot
    EXPECT_FALSE(network_utils::is_valid_ipv4(".192.168.1.1"));    // Leading dot
    EXPECT_FALSE(network_utils::is_valid_ipv4("192..168.1.1"));    // Double dot
    EXPECT_FALSE(network_utils::is_valid_ipv4("192.168. 1.1"));    // Space
    EXPECT_FALSE(network_utils::is_valid_ipv4("localhost"));       // Hostname
    EXPECT_FALSE(network_utils::is_valid_ipv4("google.com"));      // Domain
}

// Test IPv6 address validation
TEST_F(NetworkUtilsTest, IPv6ValidationTest) {
    // Valid IPv6 addresses
    EXPECT_TRUE(network_utils::is_valid_ipv6("::1"));
    EXPECT_TRUE(network_utils::is_valid_ipv6("::"));
    EXPECT_TRUE(network_utils::is_valid_ipv6("2001:db8::1"));
    EXPECT_TRUE(network_utils::is_valid_ipv6("2001:0db8:0000:0000:0000:ff00:0042:8329"));
    EXPECT_TRUE(network_utils::is_valid_ipv6("2001:db8:0:0:1:0:0:1"));
    EXPECT_TRUE(network_utils::is_valid_ipv6("::ffff:192.0.2.1"));  // IPv4-mapped
    EXPECT_TRUE(network_utils::is_valid_ipv6("fe80::1"));
    EXPECT_TRUE(network_utils::is_valid_ipv6("ff00::"));
    
    // Invalid IPv6 addresses
    EXPECT_FALSE(network_utils::is_valid_ipv6(""));                    // Empty string
    EXPECT_FALSE(network_utils::is_valid_ipv6("::1::"));               // Multiple double colons
    EXPECT_FALSE(network_utils::is_valid_ipv6("2001:db8::1::"));       // Multiple double colons
    EXPECT_FALSE(network_utils::is_valid_ipv6("2001:db8:0:0:1:0:0:1:1")); // Too many groups
    EXPECT_FALSE(network_utils::is_valid_ipv6("2001:db8:0:0:1:0:0:g"));   // Invalid hex
    EXPECT_FALSE(network_utils::is_valid_ipv6("2001:db8:0:0:1:0:0:12345")); // Group too long
    EXPECT_FALSE(network_utils::is_valid_ipv6("192.168.1.1"));        // IPv4 address
    EXPECT_FALSE(network_utils::is_valid_ipv6("localhost"));          // Hostname
    EXPECT_FALSE(network_utils::is_valid_ipv6("google.com"));         // Domain
}

// Test hostname validation
TEST_F(NetworkUtilsTest, HostnameValidationTest) {
    // Valid hostnames
    EXPECT_TRUE(network_utils::is_hostname("localhost"));
    EXPECT_TRUE(network_utils::is_hostname("google.com"));
    EXPECT_TRUE(network_utils::is_hostname("www.example.org"));
    EXPECT_TRUE(network_utils::is_hostname("sub.domain.example.com"));
    EXPECT_TRUE(network_utils::is_hostname("test-server"));
    EXPECT_TRUE(network_utils::is_hostname("server123"));
    EXPECT_TRUE(network_utils::is_hostname("a.b.c.d.e.f.g"));
    
    // Invalid hostnames (but valid IP addresses)
    EXPECT_FALSE(network_utils::is_hostname("192.168.1.1"));
    EXPECT_FALSE(network_utils::is_hostname("127.0.0.1"));
    EXPECT_FALSE(network_utils::is_hostname("::1"));
    EXPECT_FALSE(network_utils::is_hostname("2001:db8::1"));
    
    // Invalid hostnames
    EXPECT_FALSE(network_utils::is_hostname(""));           // Empty string
    EXPECT_FALSE(network_utils::is_hostname("."));          // Just a dot
    EXPECT_FALSE(network_utils::is_hostname(".."));         // Double dot
    EXPECT_FALSE(network_utils::is_hostname("host..name")); // Double dot in middle
    EXPECT_FALSE(network_utils::is_hostname(".hostname"));  // Leading dot
    EXPECT_FALSE(network_utils::is_hostname("hostname."));  // Trailing dot
    EXPECT_FALSE(network_utils::is_hostname("host name"));  // Space
    EXPECT_FALSE(network_utils::is_hostname("host@name"));  // Special character
}

// Test hostname resolution (IPv4)
TEST_F(NetworkUtilsTest, HostnameResolutionIPv4Test) {
    // Test localhost resolution
    std::string localhost_ip = network_utils::resolve_hostname("localhost");
    EXPECT_FALSE(localhost_ip.empty());
    EXPECT_TRUE(network_utils::is_valid_ipv4(localhost_ip));
    
    // Test that IP addresses are returned as-is
    std::string ip_result = network_utils::resolve_hostname("127.0.0.1");
    EXPECT_EQ(ip_result, "127.0.0.1");
    
    ip_result = network_utils::resolve_hostname("192.168.1.1");
    EXPECT_EQ(ip_result, "192.168.1.1");
    
    // Test invalid hostname resolution
    std::string invalid_result = network_utils::resolve_hostname("invalid.hostname.that.does.not.exist.12345");
    EXPECT_TRUE(invalid_result.empty());
    
    // Test empty string
    std::string empty_result = network_utils::resolve_hostname("");
    EXPECT_TRUE(empty_result.empty());
}

// Test hostname resolution (IPv6)
TEST_F(NetworkUtilsTest, HostnameResolutionIPv6Test) {
    // Test localhost resolution
    std::string localhost_ipv6 = network_utils::resolve_hostname_v6("localhost");
    if (!localhost_ipv6.empty()) {
        EXPECT_TRUE(network_utils::is_valid_ipv6(localhost_ipv6));
    }
    
    // Test that IPv6 addresses are returned as-is
    std::string ipv6_result = network_utils::resolve_hostname_v6("::1");
    EXPECT_EQ(ipv6_result, "::1");
    
    ipv6_result = network_utils::resolve_hostname_v6("2001:db8::1");
    EXPECT_EQ(ipv6_result, "2001:db8::1");
    
    // Test invalid hostname resolution
    std::string invalid_result = network_utils::resolve_hostname_v6("invalid.hostname.that.does.not.exist.12345");
    EXPECT_TRUE(invalid_result.empty());
    
    // Test empty string
    std::string empty_result = network_utils::resolve_hostname_v6("");
    EXPECT_TRUE(empty_result.empty());
}

// Test edge cases for IPv4 validation
TEST_F(NetworkUtilsTest, IPv4EdgeCasesTest) {
    // Test boundary values
    EXPECT_TRUE(network_utils::is_valid_ipv4("0.0.0.0"));
    EXPECT_TRUE(network_utils::is_valid_ipv4("255.255.255.255"));
    
    // Test just over boundary
    EXPECT_FALSE(network_utils::is_valid_ipv4("256.0.0.0"));
    EXPECT_FALSE(network_utils::is_valid_ipv4("0.256.0.0"));
    EXPECT_FALSE(network_utils::is_valid_ipv4("0.0.256.0"));
    EXPECT_FALSE(network_utils::is_valid_ipv4("0.0.0.256"));
    
    // Test leading zeros (should be invalid)
    EXPECT_FALSE(network_utils::is_valid_ipv4("01.0.0.0"));
    EXPECT_FALSE(network_utils::is_valid_ipv4("0.01.0.0"));
    EXPECT_FALSE(network_utils::is_valid_ipv4("0.0.01.0"));
    EXPECT_FALSE(network_utils::is_valid_ipv4("0.0.0.01"));
    
    // Test various malformed addresses
    EXPECT_FALSE(network_utils::is_valid_ipv4("1.2.3"));       // Too few octets
    EXPECT_FALSE(network_utils::is_valid_ipv4("1.2.3.4.5"));   // Too many octets
    EXPECT_FALSE(network_utils::is_valid_ipv4("1.2.3."));      // Trailing dot
    EXPECT_FALSE(network_utils::is_valid_ipv4(".1.2.3.4"));    // Leading dot
    EXPECT_FALSE(network_utils::is_valid_ipv4("1..2.3.4"));    // Double dot
    EXPECT_FALSE(network_utils::is_valid_ipv4("1.2.3.4 "));    // Trailing space
    EXPECT_FALSE(network_utils::is_valid_ipv4(" 1.2.3.4"));    // Leading space
}

// Test edge cases for IPv6 validation
TEST_F(NetworkUtilsTest, IPv6EdgeCasesTest) {
    // Test various valid compressed forms
    EXPECT_TRUE(network_utils::is_valid_ipv6("::"));
    EXPECT_TRUE(network_utils::is_valid_ipv6("::1"));
    EXPECT_TRUE(network_utils::is_valid_ipv6("1::"));
    EXPECT_TRUE(network_utils::is_valid_ipv6("1::1"));
    EXPECT_TRUE(network_utils::is_valid_ipv6("1:2::3:4"));
    
    // Test invalid compressed forms
    EXPECT_FALSE(network_utils::is_valid_ipv6("::1::"));       // Multiple double colons
    EXPECT_FALSE(network_utils::is_valid_ipv6("1::2::3"));     // Multiple double colons
    EXPECT_FALSE(network_utils::is_valid_ipv6(":::"));         // Triple colon
    EXPECT_FALSE(network_utils::is_valid_ipv6("1:::2"));       // Triple colon
    
    // Test case sensitivity (should be case insensitive)
    EXPECT_TRUE(network_utils::is_valid_ipv6("2001:DB8::1"));
    EXPECT_TRUE(network_utils::is_valid_ipv6("2001:db8::1"));
    EXPECT_TRUE(network_utils::is_valid_ipv6("2001:Db8::1"));
    
    // Test various malformed addresses
    EXPECT_FALSE(network_utils::is_valid_ipv6("2001:db8::1::"));   // Trailing double colon
    EXPECT_FALSE(network_utils::is_valid_ipv6("::2001:db8::1"));   // Leading double colon with more
    EXPECT_FALSE(network_utils::is_valid_ipv6("2001:db8:0:0:1:0:0:1:1")); // Too many groups
    EXPECT_FALSE(network_utils::is_valid_ipv6("2001:db8:0:0:1:0:0"));      // Too few groups without compression
}

// Test hostname edge cases
TEST_F(NetworkUtilsTest, HostnameEdgeCasesTest) {
    // Test single character hostnames
    EXPECT_TRUE(network_utils::is_hostname("a"));
    EXPECT_TRUE(network_utils::is_hostname("x"));
    
    // Test numeric hostnames (should be valid hostnames, not IP addresses)
    EXPECT_TRUE(network_utils::is_hostname("123"));
    EXPECT_TRUE(network_utils::is_hostname("server123"));
    EXPECT_TRUE(network_utils::is_hostname("123server"));
    
    // Test hyphen handling
    EXPECT_TRUE(network_utils::is_hostname("test-server"));
    EXPECT_TRUE(network_utils::is_hostname("a-b-c"));
    EXPECT_FALSE(network_utils::is_hostname("-hostname"));  // Leading hyphen
    EXPECT_FALSE(network_utils::is_hostname("hostname-"));  // Trailing hyphen
    
    // Test long hostnames
    std::string long_hostname(253, 'a');  // Maximum hostname length
    EXPECT_TRUE(network_utils::is_hostname(long_hostname));
    
    std::string too_long_hostname(254, 'a');  // Too long
    EXPECT_FALSE(network_utils::is_hostname(too_long_hostname));
}

// Test resolver with real-world domains (these might fail in isolated test environments)
TEST_F(NetworkUtilsTest, RealWorldResolutionTest) {
    // Test resolution of well-known domains
    // Note: These tests might fail in environments without internet access
    std::string google_ip = network_utils::resolve_hostname("google.com");
    if (!google_ip.empty()) {
        EXPECT_TRUE(network_utils::is_valid_ipv4(google_ip));
    }
    
    std::string cloudflare_ip = network_utils::resolve_hostname("cloudflare.com");
    if (!cloudflare_ip.empty()) {
        EXPECT_TRUE(network_utils::is_valid_ipv4(cloudflare_ip));
    }
    
    // Test IPv6 resolution
    std::string google_ipv6 = network_utils::resolve_hostname_v6("google.com");
    if (!google_ipv6.empty()) {
        EXPECT_TRUE(network_utils::is_valid_ipv6(google_ipv6));
    }
}

// Test consistency between resolution functions
TEST_F(NetworkUtilsTest, ResolutionConsistencyTest) {
    // Test that IP addresses are consistently returned
    std::string ipv4_addr = "8.8.8.8";
    std::string resolved_ipv4 = network_utils::resolve_hostname(ipv4_addr);
    EXPECT_EQ(resolved_ipv4, ipv4_addr);
    
    std::string ipv6_addr = "2001:4860:4860::8888";
    std::string resolved_ipv6 = network_utils::resolve_hostname_v6(ipv6_addr);
    EXPECT_EQ(resolved_ipv6, ipv6_addr);
    
    // Test that invalid addresses return empty strings
    std::string invalid_result = network_utils::resolve_hostname("999.999.999.999");
    EXPECT_TRUE(invalid_result.empty());
    
    std::string invalid_ipv6_result = network_utils::resolve_hostname_v6("invalid::ipv6::address");
    EXPECT_TRUE(invalid_ipv6_result.empty());
}

// Test special IP addresses
TEST_F(NetworkUtilsTest, SpecialIPAddressesTest) {
    // Test loopback addresses
    EXPECT_TRUE(network_utils::is_valid_ipv4("127.0.0.1"));
    EXPECT_TRUE(network_utils::is_valid_ipv6("::1"));
    
    // Test any address
    EXPECT_TRUE(network_utils::is_valid_ipv4("0.0.0.0"));
    EXPECT_TRUE(network_utils::is_valid_ipv6("::"));
    
    // Test broadcast address
    EXPECT_TRUE(network_utils::is_valid_ipv4("255.255.255.255"));
    
    // Test private network addresses
    EXPECT_TRUE(network_utils::is_valid_ipv4("10.0.0.1"));      // Class A private
    EXPECT_TRUE(network_utils::is_valid_ipv4("172.16.0.1"));    // Class B private
    EXPECT_TRUE(network_utils::is_valid_ipv4("192.168.1.1"));   // Class C private
    
    // Test link-local addresses
    EXPECT_TRUE(network_utils::is_valid_ipv4("169.254.1.1"));   // IPv4 link-local
    EXPECT_TRUE(network_utils::is_valid_ipv6("fe80::1"));       // IPv6 link-local
}

// Test performance with many validations
TEST_F(NetworkUtilsTest, PerformanceTest) {
    // Test IPv4 validation performance
    for (int i = 0; i < 1000; ++i) {
        EXPECT_TRUE(network_utils::is_valid_ipv4("192.168.1.1"));
        EXPECT_FALSE(network_utils::is_valid_ipv4("256.256.256.256"));
    }
    
    // Test IPv6 validation performance
    for (int i = 0; i < 1000; ++i) {
        EXPECT_TRUE(network_utils::is_valid_ipv6("2001:db8::1"));
        EXPECT_FALSE(network_utils::is_valid_ipv6("invalid::ipv6::address"));
    }
    
    // Test hostname validation performance
    for (int i = 0; i < 1000; ++i) {
        EXPECT_TRUE(network_utils::is_hostname("localhost"));
        EXPECT_FALSE(network_utils::is_hostname("192.168.1.1"));
    }
} 