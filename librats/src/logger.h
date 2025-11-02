#pragma once

#include <string>
#include <iostream>
#include <mutex>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <cstdint>
#include <fstream>
#include "fs.h"

#ifdef _WIN32
    #include <windows.h>
    #include <io.h>
    #define isatty _isatty
    #define fileno _fileno
    // Undefine Windows ERROR macro to avoid conflicts with our enum
    #ifdef ERROR
        #undef ERROR
    #endif
#else
    #include <unistd.h>
#endif

namespace librats {

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3
};

class Logger {
public:
    // Singleton pattern
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }
    
    // Delete copy constructor and assignment operator
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    // Set the minimum log level
    void set_log_level(LogLevel level) {
        std::lock_guard<std::mutex> lock(mutex_);
        min_level_ = level;
    }
    
    // Enable/disable colors
    void set_colors_enabled(bool enabled) {
        std::lock_guard<std::mutex> lock(mutex_);
        colors_enabled_ = enabled;
    }
    
    // Enable/disable timestamps
    void set_timestamps_enabled(bool enabled) {
        std::lock_guard<std::mutex> lock(mutex_);
        timestamps_enabled_ = enabled;
    }
    
    // File logging configuration
    void set_file_logging_enabled(bool enabled) {
        std::lock_guard<std::mutex> lock(mutex_);
        file_logging_enabled_ = enabled;
        if (enabled && !log_file_path_.empty()) {
            open_log_file();
        } else if (!enabled) {
            close_log_file();
        }
    }
    
    void set_log_file_path(const std::string& path) {
        std::lock_guard<std::mutex> lock(mutex_);
        log_file_path_ = path;
        if (file_logging_enabled_) {
            close_log_file();
            open_log_file();
        }
    }
    
    void set_log_rotation_size(size_t max_size_bytes) {
        std::lock_guard<std::mutex> lock(mutex_);
        max_log_file_size_ = max_size_bytes;
    }
    
    void set_log_retention_count(int count) {
        std::lock_guard<std::mutex> lock(mutex_);
        max_log_files_ = count;
    }
    
    // Get current file logging status
    bool is_file_logging_enabled() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return file_logging_enabled_;
    }
    
    std::string get_log_file_path() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return log_file_path_;
    }
    
    // Main logging function
    void log(LogLevel level, const std::string& module, const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (level < min_level_) {
            return;
        }
        
        // Prepare console output with colors
        std::ostringstream console_oss;
        
        // Add timestamp if enabled
        if (timestamps_enabled_) {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) % 1000;
            
            struct tm local_tm;
#ifdef _WIN32
            localtime_s(&local_tm, &time_t);
#else
            local_tm = *std::localtime(&time_t);
#endif
            console_oss << "[" << std::put_time(&local_tm, "%H:%M:%S");
            console_oss << "." << std::setfill('0') << std::setw(3) << ms.count() << "] ";
        }
        
        // Add colored log level
        if (colors_enabled_ && is_terminal_) {
            console_oss << get_color_code(level) << "[" << get_level_string(level) << "]" << get_reset_code();
        } else {
            console_oss << "[" << get_level_string(level) << "]";
        }
        
        // Add colored module tag
        if (!module.empty()) {
            if (colors_enabled_ && is_terminal_) {
                console_oss << " " << get_module_color(module) << "[" << module << "]" << get_reset_code();
            } else {
                console_oss << " [" << module << "]";
            }
        }
        
        // Add message
        console_oss << " " << message << std::endl;
        
        // Output to appropriate console stream
        if (level >= LogLevel::ERROR) {
            std::cerr << console_oss.str();
            std::cerr.flush();
        } else {
            std::cout << console_oss.str();
            std::cout.flush();
        }
        
        // Also write to file if file logging is enabled
        if (file_logging_enabled_ && log_file_.is_open()) {
            write_to_file(level, module, message);
        }
    }

private:
    Logger() : min_level_(LogLevel::INFO), colors_enabled_(true), timestamps_enabled_(true),
               file_logging_enabled_(false), max_log_file_size_(10 * 1024 * 1024), 
               max_log_files_(5), current_file_size_(0) {
        // Check if we're outputting to a terminal
        is_terminal_ = isatty(fileno(stdout));
        
        // On Windows, enable ANSI color codes
#ifdef _WIN32
        if (is_terminal_) {
            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            DWORD dwMode = 0;
            GetConsoleMode(hOut, &dwMode);
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
#endif
    }
    
    ~Logger() {
        close_log_file();
    }
    
    std::string get_level_string(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO:  return "INFO ";
            case LogLevel::WARN:  return "WARN ";
            case LogLevel::ERROR: return "ERROR";
            default: return "UNKNOWN";
        }
    }
    
    std::string get_color_code(LogLevel level) {
        if (!colors_enabled_ || !is_terminal_) return "";
        
        switch (level) {
            case LogLevel::DEBUG: return "\033[36m";  // Cyan
            case LogLevel::INFO:  return "\033[32m";  // Green
            case LogLevel::WARN:  return "\033[33m";  // Yellow
            case LogLevel::ERROR: return "\033[31m";  // Red
            default: return "";
        }
    }
    
    std::string get_module_color(const std::string& module) {
        if (!colors_enabled_ || !is_terminal_) return "";
        
        // Generate hash for module name
        uint32_t hash = hash_string(module);
        
        // Map hash to a predefined set of nice, readable colors
        const char* colors[] = {
            "\033[35m",  // Magenta
            "\033[36m",  // Cyan
            "\033[94m",  // Bright Blue
            "\033[95m",  // Bright Magenta
            "\033[96m",  // Bright Cyan
            "\033[93m",  // Bright Yellow
            "\033[91m",  // Bright Red
            "\033[92m",  // Bright Green
            "\033[90m",  // Bright Black (Gray)
            "\033[37m",  // White
            "\033[34m",  // Blue
            "\033[33m",  // Yellow
            "\033[31m",  // Red
            "\033[32m",  // Green
            "\033[97m",  // Bright White
            "\033[38;5;208m", // Orange
            "\033[38;5;165m", // Pink
            "\033[38;5;141m", // Purple
            "\033[38;5;51m",  // Bright Turquoise
            "\033[38;5;226m", // Bright Yellow
            "\033[38;5;46m",  // Bright Green
            "\033[38;5;196m", // Bright Red
            "\033[38;5;21m",  // Bright Blue
            "\033[38;5;129m"  // Bright Purple
        };
        
        size_t color_count = sizeof(colors) / sizeof(colors[0]);
        return colors[hash % color_count];
    }
    
    // Simple hash function for strings
    uint32_t hash_string(const std::string& str) {
        uint32_t hash = 5381;
        for (char c : str) {
            hash = ((hash << 5) + hash) + c; // hash * 33 + c
        }
        return hash;
    }
    
    std::string get_reset_code() {
        if (!colors_enabled_ || !is_terminal_) return "";
        return "\033[0m";
    }
    
    // File logging methods
    void write_to_file(LogLevel level, const std::string& module, const std::string& message) {
        if (!log_file_.is_open()) return;
        
        // Check if rotation is needed
        if (max_log_file_size_ > 0 && current_file_size_ >= max_log_file_size_) {
            rotate_log_files();
        }
        
        std::ostringstream file_oss;
        
        // Add timestamp (always enabled for file logs)
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        struct tm local_tm;
#ifdef _WIN32
        localtime_s(&local_tm, &time_t);
#else
        local_tm = *std::localtime(&time_t);
#endif
        file_oss << "[" << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
        file_oss << "." << std::setfill('0') << std::setw(3) << ms.count() << "] ";
        
        // Add log level (no colors in file)
        file_oss << "[" << get_level_string(level) << "]";
        
        // Add module tag
        if (!module.empty()) {
            file_oss << " [" << module << "]";
        }
        
        // Add message
        file_oss << " " << message << std::endl;
        
        std::string log_line = file_oss.str();
        log_file_ << log_line;
        log_file_.flush();
        
        current_file_size_ += log_line.length();
    }
    
    void open_log_file() {
        if (log_file_path_.empty()) return;
        
        close_log_file();
        
        // Create directory if it doesn't exist
        size_t last_slash = log_file_path_.find_last_of("/\\");
        if (last_slash != std::string::npos) {
            std::string dir_path = log_file_path_.substr(0, last_slash);
            if (!directory_exists(dir_path.c_str())) {
                create_directories(dir_path.c_str());
            }
        }
        
        log_file_.open(log_file_path_, std::ios::app);
        if (log_file_.is_open()) {
            // Get current file size
            log_file_.seekp(0, std::ios::end);
            current_file_size_ = static_cast<size_t>(log_file_.tellp());
            log_file_.seekp(0, std::ios::end); // Ensure we're at the end for appending
        }
    }
    
    void close_log_file() {
        if (log_file_.is_open()) {
            log_file_.close();
        }
        current_file_size_ = 0;
    }
    
    void rotate_log_files() {
        if (log_file_path_.empty() || max_log_files_ <= 0) return;
        
        close_log_file();
        
        // Move existing log files
        for (int i = max_log_files_ - 1; i >= 1; i--) {
            std::string old_name = log_file_path_ + "." + std::to_string(i);
            std::string new_name = log_file_path_ + "." + std::to_string(i + 1);
            
            // Delete the oldest file if it exists
            if (i == max_log_files_ - 1) {
                std::remove(new_name.c_str());
            }
            
            // Rename old file to new name
            std::rename(old_name.c_str(), new_name.c_str());
        }
        
        // Move current log file to .1
        std::string backup_name = log_file_path_ + ".1";
        std::rename(log_file_path_.c_str(), backup_name.c_str());
        
        // Reopen the log file (new empty file)
        open_log_file();
    }
    
    mutable std::mutex mutex_;
    LogLevel min_level_;
    bool colors_enabled_;
    bool timestamps_enabled_;
    bool is_terminal_;
    
    // File logging members
    bool file_logging_enabled_;
    std::string log_file_path_;
    std::ofstream log_file_;
    size_t max_log_file_size_;
    int max_log_files_;
    size_t current_file_size_;
};

} // namespace librats

// Convenience macros for easy logging
#define LOG_DEBUG(module, message) \
    do { \
        std::ostringstream oss; \
        oss << message; \
        librats::Logger::getInstance().log(librats::LogLevel::DEBUG, module, oss.str()); \
    } while(0)

#define LOG_INFO(module, message) \
    do { \
        std::ostringstream oss; \
        oss << message; \
        librats::Logger::getInstance().log(librats::LogLevel::INFO, module, oss.str()); \
    } while(0)

#define LOG_WARN(module, message) \
    do { \
        std::ostringstream oss; \
        oss << message; \
        librats::Logger::getInstance().log(librats::LogLevel::WARN, module, oss.str()); \
    } while(0)

#define LOG_ERROR(module, message) \
    do { \
        std::ostringstream oss; \
        oss << message; \
        librats::Logger::getInstance().log(librats::LogLevel::ERROR, module, oss.str()); \
    } while(0)

 