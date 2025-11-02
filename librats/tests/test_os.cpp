#include <gtest/gtest.h>
#include "os.h"
#include <iostream>

using namespace librats;

class OSTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Test setup if needed
    }
    
    void TearDown() override {
        // Test cleanup if needed
    }
};

TEST_F(OSTest, GetOSName) {
    std::string os_name = get_os_name();
    EXPECT_FALSE(os_name.empty());
    EXPECT_NE(os_name, "Unknown");
    
    // Print for debugging
    std::cout << "OS Name: " << os_name << std::endl;
}

TEST_F(OSTest, GetOSVersion) {
    std::string os_version = get_os_version();
    EXPECT_FALSE(os_version.empty());
    
    // Print for debugging
    std::cout << "OS Version: " << os_version << std::endl;
}

TEST_F(OSTest, GetArchitecture) {
    std::string architecture = get_architecture();
    EXPECT_FALSE(architecture.empty());
    EXPECT_NE(architecture, "Unknown");
    
    // Print for debugging
    std::cout << "Architecture: " << architecture << std::endl;
}

TEST_F(OSTest, GetHostname) {
    std::string hostname = get_hostname();
    EXPECT_FALSE(hostname.empty());
    EXPECT_NE(hostname, "Unknown");
    
    // Print for debugging
    std::cout << "Hostname: " << hostname << std::endl;
}

TEST_F(OSTest, GetCPUModel) {
    std::string cpu_model = get_cpu_model();
    EXPECT_FALSE(cpu_model.empty());
    
    // Print for debugging
    std::cout << "CPU Model: " << cpu_model << std::endl;
}

TEST_F(OSTest, GetCPUCores) {
    int cpu_cores = get_cpu_cores();
    EXPECT_GT(cpu_cores, 0);
    EXPECT_LE(cpu_cores, 256); // Reasonable upper bound
    
    // Print for debugging
    std::cout << "CPU Cores: " << cpu_cores << std::endl;
}

TEST_F(OSTest, GetCPULogicalCores) {
    int logical_cores = get_cpu_logical_cores();
    EXPECT_GT(logical_cores, 0);
    EXPECT_LE(logical_cores, 512); // Reasonable upper bound
    
    // Print for debugging
    std::cout << "CPU Logical Cores: " << logical_cores << std::endl;
}

TEST_F(OSTest, GetTotalMemory) {
    uint64_t total_memory = get_total_memory_mb();
    EXPECT_GT(total_memory, 0);
    EXPECT_LT(total_memory, 1024 * 1024); // Less than 1TB should be reasonable
    
    // Print for debugging
    std::cout << "Total Memory: " << total_memory << " MB" << std::endl;
}

TEST_F(OSTest, GetAvailableMemory) {
    uint64_t available_memory = get_available_memory_mb();
    EXPECT_GT(available_memory, 0);
    
    uint64_t total_memory = get_total_memory_mb();
    EXPECT_LE(available_memory, total_memory);
    
    // Print for debugging
    std::cout << "Available Memory: " << available_memory << " MB" << std::endl;
}

TEST_F(OSTest, GetSystemInfo) {
    SystemInfo info = get_system_info();
    
    // Verify all fields are populated
    EXPECT_FALSE(info.os_name.empty());
    EXPECT_FALSE(info.os_version.empty());
    EXPECT_FALSE(info.architecture.empty());
    EXPECT_FALSE(info.hostname.empty());
    EXPECT_FALSE(info.cpu_model.empty());
    EXPECT_GT(info.cpu_cores, 0);
    EXPECT_GT(info.cpu_logical_cores, 0);
    EXPECT_GT(info.total_memory_mb, 0);
    EXPECT_GT(info.available_memory_mb, 0);
    
    // Print full system info for debugging
    std::cout << "\n=== Full System Information ===" << std::endl;
    std::cout << "OS: " << info.os_name << " " << info.os_version << std::endl;
    std::cout << "Architecture: " << info.architecture << std::endl;
    std::cout << "Hostname: " << info.hostname << std::endl;
    std::cout << "CPU: " << info.cpu_model << std::endl;
    std::cout << "CPU Cores: " << info.cpu_cores << " physical, " << info.cpu_logical_cores << " logical" << std::endl;
    std::cout << "Memory: " << info.total_memory_mb << " MB total, " << info.available_memory_mb << " MB available" << std::endl;
    std::cout << "===============================" << std::endl;
}

TEST_F(OSTest, PrintSystemInfo) {
    // This test just ensures the print function doesn't crash
    EXPECT_NO_THROW(print_system_info());
}

TEST_F(OSTest, LogicalCoresGreaterThanOrEqualToPhysicalCores) {
    int physical_cores = get_cpu_cores();
    int logical_cores = get_cpu_logical_cores();
    
    // Logical cores should be >= physical cores (hyperthreading)
    EXPECT_GE(logical_cores, physical_cores);
} 