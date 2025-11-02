#pragma once

#include <string>
#include <cstdint>

namespace librats {

struct SystemInfo {
    std::string os_name;
    std::string os_version;
    std::string architecture;
    std::string hostname;
    std::string cpu_model;
    int cpu_cores;
    int cpu_logical_cores;
    uint64_t total_memory_mb;
    uint64_t available_memory_mb;
};

// Get comprehensive system information
SystemInfo get_system_info();

// Individual functions for specific info
std::string get_os_name();
std::string get_os_version();
std::string get_architecture();
std::string get_hostname();
std::string get_cpu_model();
int get_cpu_cores();
int get_cpu_logical_cores();
uint64_t get_total_memory_mb();
uint64_t get_available_memory_mb();

// Unicode support detection
bool supports_unicode();

// Utility function to print system info
void print_system_info();

} // namespace librats 