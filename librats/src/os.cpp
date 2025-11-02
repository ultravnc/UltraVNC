#include "os.h"
#include "logger.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <thread>
#include <cstdlib>

#ifdef _WIN32
    #include <windows.h>
    #include <sysinfoapi.h>
    #include <versionhelpers.h>
    #include <intrin.h>
    #pragma comment(lib, "version.lib")
#elif __APPLE__
    #include <sys/utsname.h>
    #include <sys/sysctl.h>
    #include <unistd.h>
    #include <mach/mach.h>
    #include <mach/vm_statistics.h>
    #include <mach/mach_types.h>
    #include <mach/mach_init.h>
    #include <mach/mach_host.h>
#else
    #include <sys/utsname.h>
    #include <sys/sysinfo.h>
    #include <unistd.h>
    #include <fstream>
    #include <regex>
#endif

namespace librats {

#ifdef _WIN32

std::string get_os_name() {
    return "Windows";
}

std::string get_os_version() {
    std::string version = "Unknown";
    
    OSVERSIONINFOEX osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    
    if (GetVersionEx((OSVERSIONINFO*)&osvi)) {
        std::ostringstream oss;
        oss << osvi.dwMajorVersion << "." << osvi.dwMinorVersion;
        if (osvi.dwBuildNumber > 0) {
            oss << "." << osvi.dwBuildNumber;
        }
        version = oss.str();
    }
    
    return version;
}

std::string get_architecture() {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    
    switch (si.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_AMD64:
            return "x64";
        case PROCESSOR_ARCHITECTURE_INTEL:
            return "x86";
        case PROCESSOR_ARCHITECTURE_ARM:
            return "ARM";
        case PROCESSOR_ARCHITECTURE_ARM64:
            return "ARM64";
        default:
            return "Unknown";
    }
}

std::string get_hostname() {
    char hostname[256];
    DWORD hostname_len = sizeof(hostname);
    if (GetComputerNameA(hostname, &hostname_len)) {
        return std::string(hostname);
    }
    return "Unknown";
}

std::string get_cpu_model() {
    int CPUInfo[4] = {-1};
    unsigned nExIds, i = 0;
    char CPUBrandString[0x40];
    
    __cpuid(CPUInfo, 0x80000000);
    nExIds = CPUInfo[0];
    
    for (i = 0x80000000; i <= nExIds; ++i) {
        __cpuid(CPUInfo, i);
        
        if (i == 0x80000002)
            memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));
        else if (i == 0x80000003)
            memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));
        else if (i == 0x80000004)
            memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));
    }
    
    return std::string(CPUBrandString);
}

int get_cpu_cores() {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return static_cast<int>(si.dwNumberOfProcessors);
}

int get_cpu_logical_cores() {
    return get_cpu_cores(); // Same as cores on Windows with this method
}

uint64_t get_total_memory_mb() {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    return memInfo.ullTotalPhys / (1024 * 1024);
}

uint64_t get_available_memory_mb() {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    return memInfo.ullAvailPhys / (1024 * 1024);
}

#elif __APPLE__

std::string get_os_name() {
    return "macOS";
}

std::string get_os_version() {
    struct utsname unameData;
    uname(&unameData);
    return std::string(unameData.release);
}

std::string get_architecture() {
    struct utsname unameData;
    uname(&unameData);
    return std::string(unameData.machine);
}

std::string get_hostname() {
    struct utsname unameData;
    uname(&unameData);
    return std::string(unameData.nodename);
}

std::string get_cpu_model() {
    char buffer[256];
    size_t size = sizeof(buffer);
    if (sysctlbyname("machdep.cpu.brand_string", buffer, &size, NULL, 0) == 0) {
        return std::string(buffer);
    }
    return "Unknown";
}

int get_cpu_cores() {
    int cores = 0;
    size_t size = sizeof(cores);
    if (sysctlbyname("hw.physicalcpu", &cores, &size, NULL, 0) == 0) {
        return cores;
    }
    return std::thread::hardware_concurrency();
}

int get_cpu_logical_cores() {
    int cores = 0;
    size_t size = sizeof(cores);
    if (sysctlbyname("hw.logicalcpu", &cores, &size, NULL, 0) == 0) {
        return cores;
    }
    return std::thread::hardware_concurrency();
}

uint64_t get_total_memory_mb() {
    uint64_t memsize = 0;
    size_t size = sizeof(memsize);
    if (sysctlbyname("hw.memsize", &memsize, &size, NULL, 0) == 0) {
        return memsize / (1024 * 1024);
    }
    return 0;
}

uint64_t get_available_memory_mb() {
    vm_size_t page_size;
    mach_port_t mach_port = mach_host_self();
    vm_statistics64_data_t vm_stat;
    mach_msg_type_number_t host_size = sizeof(vm_statistics64_data_t) / sizeof(natural_t);
    
    host_page_size(mach_port, &page_size);
    host_statistics64(mach_port, HOST_VM_INFO, (host_info64_t)&vm_stat, &host_size);
    
    uint64_t free_memory = (uint64_t)vm_stat.free_count * (uint64_t)page_size;
    return free_memory / (1024 * 1024);
}

#else // Linux

std::string get_os_name() {
    std::ifstream file("/etc/os-release");
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("PRETTY_NAME=") == 0) {
            std::string name = line.substr(12);
            // Remove quotes if present
            if (name.front() == '"' && name.back() == '"') {
                name = name.substr(1, name.length() - 2);
            }
            return name;
        }
    }
    
    struct utsname unameData;
    uname(&unameData);
    return std::string(unameData.sysname);
}

std::string get_os_version() {
    struct utsname unameData;
    uname(&unameData);
    return std::string(unameData.release);
}

std::string get_architecture() {
    struct utsname unameData;
    uname(&unameData);
    return std::string(unameData.machine);
}

std::string get_hostname() {
    struct utsname unameData;
    uname(&unameData);
    return std::string(unameData.nodename);
}

std::string get_cpu_model() {
    std::ifstream file("/proc/cpuinfo");
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("model name") != std::string::npos) {
            size_t pos = line.find(':');
            if (pos != std::string::npos) {
                std::string model = line.substr(pos + 1);
                // Trim whitespace
                model.erase(0, model.find_first_not_of(" \t"));
                model.erase(model.find_last_not_of(" \t") + 1);
                return model;
            }
        }
    }
    return "Unknown";
}

int get_cpu_cores() {
    std::ifstream file("/proc/cpuinfo");
    std::string line;
    int cores = 0;
    
    while (std::getline(file, line)) {
        if (line.find("cpu cores") != std::string::npos) {
            size_t pos = line.find(':');
            if (pos != std::string::npos) {
                cores = std::stoi(line.substr(pos + 1));
                break;
            }
        }
    }
    
    if (cores == 0) {
        cores = std::thread::hardware_concurrency();
    }
    
    return cores;
}

int get_cpu_logical_cores() {
    return std::thread::hardware_concurrency();
}

uint64_t get_total_memory_mb() {
    std::ifstream file("/proc/meminfo");
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("MemTotal:") == 0) {
            std::regex regex(R"(MemTotal:\s+(\d+)\s+kB)");
            std::smatch match;
            if (std::regex_search(line, match, regex)) {
                return std::stoull(match[1]) / 1024; // Convert kB to MB
            }
        }
    }
    return 0;
}

uint64_t get_available_memory_mb() {
    std::ifstream file("/proc/meminfo");
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("MemAvailable:") == 0) {
            std::regex regex(R"(MemAvailable:\s+(\d+)\s+kB)");
            std::smatch match;
            if (std::regex_search(line, match, regex)) {
                return std::stoull(match[1]) / 1024; // Convert kB to MB
            }
        }
    }
    return 0;
}

#endif

SystemInfo get_system_info() {
    SystemInfo info;
    info.os_name = get_os_name();
    info.os_version = get_os_version();
    info.architecture = get_architecture();
    info.hostname = get_hostname();
    info.cpu_model = get_cpu_model();
    info.cpu_cores = get_cpu_cores();
    info.cpu_logical_cores = get_cpu_logical_cores();
    info.total_memory_mb = get_total_memory_mb();
    info.available_memory_mb = get_available_memory_mb();
    return info;
}

void print_system_info() {
    SystemInfo info = get_system_info();
    
    std::cout << "=== System Information ===" << std::endl;
    std::cout << "OS: " << info.os_name << " " << info.os_version << std::endl;
    std::cout << "Architecture: " << info.architecture << std::endl;
    std::cout << "Hostname: " << info.hostname << std::endl;
    std::cout << "CPU: " << info.cpu_model << std::endl;
    std::cout << "CPU Cores: " << info.cpu_cores << " physical, " << info.cpu_logical_cores << " logical" << std::endl;
    std::cout << "Memory: " << info.total_memory_mb << " MB total, " << info.available_memory_mb << " MB available" << std::endl;
    std::cout << "=========================" << std::endl;
}

bool supports_unicode() {
    static bool checked = false;
    static bool unicode_supported = false;
    
    if (!checked) {
        std::string os = get_os_name();
        // Check if we're on Windows, which often has console encoding issues
        if (os.find("Windows") != std::string::npos) {
            // Try to detect if we're in a proper Unicode-enabled terminal
            // For simplicity, we'll assume Windows console doesn't support Unicode well
            // unless specific environment variables are set
            const char* term = std::getenv("TERM");
            const char* wt = std::getenv("WT_SESSION"); // Windows Terminal
            unicode_supported = (term && std::string(term) != "dumb") || wt;
        } else {
            // On Unix-like systems, assume Unicode is supported
            unicode_supported = true;
        }
        checked = true;
    }
    
    return unicode_supported;
}

} // namespace librats 