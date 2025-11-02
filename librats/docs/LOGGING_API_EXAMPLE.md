# Logging Control API Example

The RatsClient now provides a comprehensive API for controlling logging behavior. Here's how to use it:

## Basic Usage

```cpp
#include "librats.h"

int main() {
    librats::RatsClient client(8080);
    
    // Enable file logging (will use "rats.log" by default)
    client.set_logging_enabled(true);
    
    // Start the client
    client.start();
    
    // Your application logic here...
    
    client.stop();
    return 0;
}
```

## Advanced Logging Configuration

```cpp
#include "librats.h"

int main() {
    librats::RatsClient client(8080);
    
    // Configure logging before starting
    client.set_log_file_path("my_app.log");           // Custom log file
    client.set_log_level("DEBUG");                    // Set to debug level
    client.set_log_colors_enabled(true);              // Enable colored output
    client.set_log_timestamps_enabled(true);          // Enable timestamps
    client.set_log_rotation_size(50 * 1024 * 1024);   // 50MB rotation size
    client.set_log_retention_count(10);               // Keep 10 old log files
    
    // Enable file logging
    client.set_logging_enabled(true);
    
    client.start();
    
    // Check current logging status
    if (client.is_logging_enabled()) {
        std::cout << "Logging to: " << client.get_log_file_path() << std::endl;
    }
    
    // Your application logic here...
    
    // Clear log file if needed
    client.clear_log_file();
    
    client.stop();
    return 0;
}
```

## Available Log Levels

- `DEBUG` (0) - Most verbose, includes all messages
- `INFO` (1) - General information messages
- `WARN` (2) - Warning messages
- `ERROR` (3) - Error messages only

## API Reference

### File Logging Control
- `void set_logging_enabled(bool enabled)` - Enable/disable file logging
- `bool is_logging_enabled() const` - Check if file logging is enabled
- `void set_log_file_path(const std::string& file_path)` - Set log file path
- `std::string get_log_file_path() const` - Get current log file path

### Log Level Control
- `void set_log_level(LogLevel level)` - Set log level using enum
- `void set_log_level(const std::string& level_str)` - Set log level using string
- `LogLevel get_log_level() const` - Get current log level

### Output Formatting
- `void set_log_colors_enabled(bool enabled)` - Enable/disable colored output
- `bool is_log_colors_enabled() const` - Check if colors are enabled
- `void set_log_timestamps_enabled(bool enabled)` - Enable/disable timestamps
- `bool is_log_timestamps_enabled() const` - Check if timestamps are enabled

### Log Rotation
- `void set_log_rotation_size(size_t max_size_bytes)` - Set rotation size (default: 10MB)
- `void set_log_retention_count(int count)` - Set number of files to keep (default: 5)

### Utility
- `void clear_log_file()` - Clear/reset the current log file

## Default Behavior

When `set_logging_enabled(true)` is called:
- Default log file: "rats.log"
- Default log level: INFO
- Colors: Enabled (if terminal supports it)
- Timestamps: Enabled
- Rotation size: 10MB
- Retention count: 5 files

## Notes

- The logging system uses a singleton Logger instance shared across the application
- Log rotation happens automatically when the file size exceeds the configured limit
- Old log files are named with incremental numbers (e.g., rats.log.1, rats.log.2, etc.)
- Console output is always enabled regardless of file logging settings
- File logging must be explicitly enabled - it's disabled by default


