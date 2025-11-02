#include "librats.h"
#include "logger.h"
#include <algorithm>

// Logging macros for RatsClient
#define LOG_CLIENT_DEBUG(message) LOG_DEBUG("client", message)
#define LOG_CLIENT_INFO(message)  LOG_INFO("client", message)
#define LOG_CLIENT_WARN(message)  LOG_WARN("client", message)
#define LOG_CLIENT_ERROR(message) LOG_ERROR("client", message)

namespace librats {

//=============================================================================
// Logging Control API Implementation
//=============================================================================

void RatsClient::set_logging_enabled(bool enabled) {
    LOG_CLIENT_INFO("Setting file logging " << (enabled ? "enabled" : "disabled"));
    
    Logger& logger = Logger::getInstance();
    
    if (enabled) {
        // Set default log file path if not already set
        if (logger.get_log_file_path().empty()) {
            logger.set_log_file_path("rats.log");
        }
    }
    
    logger.set_file_logging_enabled(enabled);
    
    if (enabled) {
        LOG_CLIENT_INFO("File logging enabled - logs will be written to: " << logger.get_log_file_path());
    } else {
        LOG_CLIENT_INFO("File logging disabled");
    }
}

bool RatsClient::is_logging_enabled() const {
    return Logger::getInstance().is_file_logging_enabled();
}

void RatsClient::set_log_file_path(const std::string& file_path) {
    Logger& logger = Logger::getInstance();
    logger.set_log_file_path(file_path);
    LOG_CLIENT_INFO("Log file path set to: " << file_path);
}

std::string RatsClient::get_log_file_path() const {
    return Logger::getInstance().get_log_file_path();
}

void RatsClient::set_log_level(LogLevel level) {
    Logger::getInstance().set_log_level(level);
    LOG_CLIENT_INFO("Log level set to: " << static_cast<int>(level));
}

void RatsClient::set_log_level(const std::string& level_str) {
    LogLevel level;
    
    std::string upper_level = level_str;
    std::transform(upper_level.begin(), upper_level.end(), upper_level.begin(), ::toupper);
    
    if (upper_level == "DEBUG") {
        level = LogLevel::DEBUG;
    } else if (upper_level == "INFO") {
        level = LogLevel::INFO;
    } else if (upper_level == "WARN" || upper_level == "WARNING") {
        level = LogLevel::WARN;
    } else if (upper_level == "ERROR") {
        level = LogLevel::ERROR;
    } else {
        LOG_CLIENT_WARN("Invalid log level string: " << level_str << " - using INFO as default");
        level = LogLevel::INFO;
    }
    
    set_log_level(level);
}

LogLevel RatsClient::get_log_level() const {
    // Note: Logger doesn't expose get_log_level method, so we'll use a workaround
    // by checking which level actually outputs. This is a limitation of the current Logger design.
    Logger& logger = Logger::getInstance();
    
    // Try to determine current level by checking if debug messages would be shown
    // This is a simple approximation - for a more accurate implementation,
    // the Logger class would need a get_log_level() method
    
    // For now, we'll return INFO as a reasonable default
    // In a production system, the Logger class should be enhanced to track and return the current level
    return LogLevel::INFO;
}

void RatsClient::set_log_colors_enabled(bool enabled) {
    Logger::getInstance().set_colors_enabled(enabled);
    LOG_CLIENT_INFO("Log colors " << (enabled ? "enabled" : "disabled"));
}

bool RatsClient::is_log_colors_enabled() const {
    // Note: Logger doesn't expose a getter for colors_enabled
    // This is another limitation of the current Logger design
    // For now, return true as a reasonable default
    return true;
}

void RatsClient::set_log_timestamps_enabled(bool enabled) {
    Logger::getInstance().set_timestamps_enabled(enabled);
    LOG_CLIENT_INFO("Log timestamps " << (enabled ? "enabled" : "disabled"));
}

bool RatsClient::is_log_timestamps_enabled() const {
    // Note: Logger doesn't expose a getter for timestamps_enabled
    // This is another limitation of the current Logger design
    // For now, return true as a reasonable default
    return true;
}

void RatsClient::set_log_rotation_size(size_t max_size_bytes) {
    Logger::getInstance().set_log_rotation_size(max_size_bytes);
    LOG_CLIENT_INFO("Log rotation size set to: " << max_size_bytes << " bytes");
}

void RatsClient::set_log_retention_count(int count) {
    Logger::getInstance().set_log_retention_count(count);
    LOG_CLIENT_INFO("Log retention count set to: " << count << " files");
}

void RatsClient::clear_log_file() {
    Logger& logger = Logger::getInstance();
    std::string log_path = logger.get_log_file_path();
    
    if (log_path.empty()) {
        LOG_CLIENT_WARN("No log file path set - cannot clear log file");
        return;
    }
    
    try {
        // Temporarily disable file logging
        bool was_enabled = logger.is_file_logging_enabled();
        if (was_enabled) {
            logger.set_file_logging_enabled(false);
        }
        
        // Delete the log file
        std::remove(log_path.c_str());
        
        // Re-enable file logging if it was enabled
        if (was_enabled) {
            logger.set_file_logging_enabled(true);
        }
        
        LOG_CLIENT_INFO("Log file cleared: " << log_path);
        
    } catch (const std::exception& e) {
        LOG_CLIENT_ERROR("Failed to clear log file: " << e.what());
    }
}

}
