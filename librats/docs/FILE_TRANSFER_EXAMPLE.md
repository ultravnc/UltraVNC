# LibRATS File Transfer API Example

This document demonstrates how to use the librats file transfer API for efficient, resumable file and directory transfers between peers.

## Overview

The file transfer system provides:
- **Chunked transfers** with configurable chunk sizes for efficient large file handling
- **Resume capability** for interrupted transfers
- **Progress tracking** with real-time transfer statistics
- **Directory transfer** with recursive subdirectory support
- **Compression support** (optional, for reduced bandwidth usage)
- **Checksum verification** to ensure data integrity
- **Concurrent chunk processing** for improved performance

## Basic Usage

### Setting Up File Transfer

```cpp
#include "librats.h"
using namespace librats;

// Create RatsClient
RatsClient client(8080);

// Configure file transfer settings (optional)
FileTransferConfig config;
config.chunk_size = 65536;           // 64KB chunks
config.max_concurrent_chunks = 4;    // 4 parallel chunks
config.verify_checksums = true;      // Enable integrity checks
config.allow_resume = true;          // Enable resume capability
config.temp_directory = "./temp";    // Temporary files directory

client.set_file_transfer_config(config);

// Set up event handlers
client.on_file_transfer_request([&client](const std::string& peer_id, 
                                          const FileMetadata& metadata, 
                                          const std::string& transfer_id) -> bool {
    std::cout << "File transfer request from " << peer_id << std::endl;
    std::cout << "Filename: " << metadata.filename << std::endl;
    std::cout << "Size: " << metadata.file_size << " bytes" << std::endl;
    
    // Auto-accept files smaller than 100MB
    if (metadata.file_size < 100 * 1024 * 1024) {
        std::string local_path = "./downloads/" + metadata.filename;
        client.accept_file_transfer(transfer_id, local_path);
        return true;
    }
    
    // Ask user for larger files
    std::cout << "Accept transfer? (y/n): ";
    char response;
    std::cin >> response;
    
    if (response == 'y' || response == 'Y') {
        std::string local_path = "./downloads/" + metadata.filename;
        client.accept_file_transfer(transfer_id, local_path);
        return true;
    } else {
        client.reject_file_transfer(transfer_id, "User declined");
        return false;
    }
});

client.on_file_transfer_progress([](const FileTransferProgress& progress) {
    std::cout << "Transfer " << progress.transfer_id << ": " 
              << progress.get_completion_percentage() << "% complete" << std::endl;
    std::cout << "Speed: " << (progress.transfer_rate_bps / 1024) << " KB/s" << std::endl;
    std::cout << "ETA: " << progress.estimated_time_remaining.count() / 1000 << " seconds" << std::endl;
});

client.on_file_transfer_completed([](const std::string& transfer_id, 
                                    bool success, 
                                    const std::string& error_message) {
    if (success) {
        std::cout << "Transfer completed: " << transfer_id << std::endl;
    } else {
        std::cout << "Transfer failed: " << transfer_id << " - " << error_message << std::endl;
    }
});

client.start();
```

### Sending Files

```cpp
// Send a single file
std::string transfer_id = client.send_file("peer_123", "/path/to/document.pdf");
if (!transfer_id.empty()) {
    std::cout << "File transfer initiated: " << transfer_id << std::endl;
} else {
    std::cout << "Failed to initiate file transfer" << std::endl;
}

// Send with custom remote filename
std::string transfer_id2 = client.send_file("peer_123", "/path/to/document.pdf", "renamed_document.pdf");

// Send an entire directory
std::string dir_transfer_id = client.send_directory("peer_123", "/path/to/folder", "remote_folder", true);
```

### Managing Active Transfers

```cpp
// Get progress for a specific transfer
auto progress = client.get_file_transfer_progress(transfer_id);
if (progress) {
    std::cout << "Progress: " << progress->get_completion_percentage() << "%" << std::endl;
    std::cout << "Status: " << static_cast<int>(progress->status) << std::endl;
    std::cout << "Transfer rate: " << progress->transfer_rate_bps << " bytes/sec" << std::endl;
}

// List all active transfers
auto active_transfers = client.get_active_file_transfers();
for (const auto& transfer : active_transfers) {
    std::cout << "Transfer " << transfer->transfer_id 
              << " (" << transfer->filename << "): " 
              << transfer->get_completion_percentage() << "%" << std::endl;
}

// Pause a transfer
client.pause_file_transfer(transfer_id);

// Resume a paused transfer
client.resume_file_transfer(transfer_id);

// Cancel a transfer
client.cancel_file_transfer(transfer_id);
```

### Getting Transfer Statistics

```cpp
auto stats = client.get_file_transfer_statistics();
std::cout << "Total bytes sent: " << stats["total_bytes_sent"] << std::endl;
std::cout << "Total bytes received: " << stats["total_bytes_received"] << std::endl;
std::cout << "Total files sent: " << stats["total_files_sent"] << std::endl;
std::cout << "Total files received: " << stats["total_files_received"] << std::endl;
std::cout << "Active transfers: " << stats["active_transfers"] << std::endl;
std::cout << "Success rate: " << stats["success_rate"] << std::endl;
std::cout << "Average send rate: " << stats["average_send_rate_bps"] << " bytes/sec" << std::endl;
```

## Advanced Usage

### Custom Transfer Configuration

```cpp
// High-performance configuration for fast networks
FileTransferConfig fast_config;
fast_config.chunk_size = 1024 * 1024;      // 1MB chunks
fast_config.max_concurrent_chunks = 8;      // 8 parallel chunks
fast_config.verify_checksums = false;       // Disable for speed
fast_config.compression = CompressionType::LZ4; // Fast compression

// Conservative configuration for slow/unreliable networks
FileTransferConfig safe_config;
safe_config.chunk_size = 32 * 1024;         // 32KB chunks
safe_config.max_concurrent_chunks = 2;      // 2 parallel chunks
safe_config.max_retries = 5;                // More retry attempts
safe_config.timeout_seconds = 60;           // Longer timeout
safe_config.verify_checksums = true;        // Enable integrity checks

client.set_file_transfer_config(fast_config);
```

### Directory Transfer with Progress

```cpp
client.on_directory_transfer_progress([](const std::string& transfer_id,
                                        const std::string& current_file,
                                        uint64_t files_completed,
                                        uint64_t total_files,
                                        uint64_t bytes_completed,
                                        uint64_t total_bytes) {
    double file_progress = (double)files_completed / total_files * 100.0;
    double byte_progress = (double)bytes_completed / total_bytes * 100.0;
    
    std::cout << "Directory transfer " << transfer_id << ":" << std::endl;
    std::cout << "  Current file: " << current_file << std::endl;
    std::cout << "  Files: " << files_completed << "/" << total_files 
              << " (" << file_progress << "%)" << std::endl;
    std::cout << "  Bytes: " << bytes_completed << "/" << total_bytes 
              << " (" << byte_progress << "%)" << std::endl;
});
```

### Working with File Metadata

```cpp
// Get detailed file information before sending
FileMetadata metadata = FileTransferManager::get_file_metadata("/path/to/file.txt");
std::cout << "Filename: " << metadata.filename << std::endl;
std::cout << "Size: " << metadata.file_size << " bytes" << std::endl;
std::cout << "MIME type: " << metadata.mime_type << std::endl;
std::cout << "Last modified: " << metadata.last_modified << std::endl;
std::cout << "Checksum: " << metadata.checksum << std::endl;

// Get directory information
DirectoryMetadata dir_metadata = FileTransferManager::get_directory_metadata("/path/to/directory", true);
std::cout << "Directory: " << dir_metadata.directory_name << std::endl;
std::cout << "Total size: " << dir_metadata.get_total_size() << " bytes" << std::endl;
std::cout << "Total files: " << dir_metadata.get_total_file_count() << std::endl;
```

### Error Handling and Recovery

```cpp
client.on_file_transfer_completed([&client](const std::string& transfer_id, 
                                           bool success, 
                                           const std::string& error_message) {
    if (!success) {
        std::cout << "Transfer failed: " << error_message << std::endl;
        
        // Get transfer details
        auto progress = client.get_file_transfer_progress(transfer_id);
        if (progress && progress->retry_count < 3) {
            std::cout << "Retrying transfer..." << std::endl;
            // Wait a bit before retrying
            std::this_thread::sleep_for(std::chrono::seconds(5));
            client.retry_transfer(transfer_id);
        } else {
            std::cout << "Max retries exceeded, transfer abandoned" << std::endl;
        }
    }
});
```

## Complete Example Program

```cpp
#include "librats.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace librats;

int main() {
    // Create client
    RatsClient client(8080);
    
    // Configure file transfers
    FileTransferConfig config;
    config.chunk_size = 64 * 1024;      // 64KB chunks
    config.max_concurrent_chunks = 4;    // 4 parallel transfers
    config.verify_checksums = true;      // Verify integrity
    config.allow_resume = true;          // Allow resume
    client.set_file_transfer_config(config);
    
    // Set up transfer event handlers
    client.on_file_transfer_request([&client](const std::string& peer_id, 
                                              const FileMetadata& metadata, 
                                              const std::string& transfer_id) -> bool {
        std::cout << "📁 Incoming file transfer request:" << std::endl;
        std::cout << "   From: " << peer_id << std::endl;
        std::cout << "   File: " << metadata.filename << std::endl;
        std::cout << "   Size: " << metadata.file_size << " bytes" << std::endl;
        
        // Auto-accept and save to downloads folder
        std::string local_path = "./downloads/" + metadata.filename;
        client.accept_file_transfer(transfer_id, local_path);
        std::cout << "✅ Transfer accepted -> " << local_path << std::endl;
        return true;
    });
    
    client.on_file_transfer_progress([](const FileTransferProgress& progress) {
        std::cout << "📊 " << progress.filename << ": " 
                  << std::fixed << std::setprecision(1) 
                  << progress.get_completion_percentage() << "% "
                  << "(" << progress.transfer_rate_bps / 1024 << " KB/s)" << std::endl;
    });
    
    client.on_file_transfer_completed([](const std::string& transfer_id, 
                                        bool success, 
                                        const std::string& error_message) {
        if (success) {
            std::cout << "✅ Transfer completed: " << transfer_id << std::endl;
        } else {
            std::cout << "❌ Transfer failed: " << error_message << std::endl;
        }
    });
    
    // Start the client
    if (!client.start()) {
        std::cerr << "Failed to start client" << std::endl;
        return 1;
    }
    
    std::cout << "🚀 RatsClient started on port 8080" << std::endl;
    std::cout << "📁 File transfer system ready" << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  send <peer_id> <file_path>     - Send a file" << std::endl;
    std::cout << "  senddir <peer_id> <dir_path>   - Send a directory" << std::endl;
    std::cout << "  list                           - List active transfers" << std::endl;
    std::cout << "  stats                          - Show transfer statistics" << std::endl;
    std::cout << "  quit                           - Exit" << std::endl;
    
    // Command loop
    std::string command;
    while (std::getline(std::cin, command)) {
        if (command == "quit") {
            break;
        } else if (command == "list") {
            auto transfers = client.get_active_file_transfers();
            std::cout << "Active transfers (" << transfers.size() << "):" << std::endl;
            for (const auto& transfer : transfers) {
                std::cout << "  " << transfer->transfer_id << " - " 
                          << transfer->filename << " (" 
                          << transfer->get_completion_percentage() << "%)" << std::endl;
            }
        } else if (command == "stats") {
            auto stats = client.get_file_transfer_statistics();
            std::cout << "Transfer Statistics:" << std::endl;
            std::cout << "  Bytes sent: " << stats["total_bytes_sent"] << std::endl;
            std::cout << "  Bytes received: " << stats["total_bytes_received"] << std::endl;
            std::cout << "  Files sent: " << stats["total_files_sent"] << std::endl;
            std::cout << "  Files received: " << stats["total_files_received"] << std::endl;
            std::cout << "  Success rate: " << stats["success_rate"] << std::endl;
        } else if (command.substr(0, 4) == "send") {
            // Parse send command
            size_t first_space = command.find(' ', 5);
            size_t second_space = command.find(' ', first_space + 1);
            
            if (first_space != std::string::npos && second_space != std::string::npos) {
                std::string peer_id = command.substr(5, first_space - 5);
                std::string file_path = command.substr(first_space + 1);
                
                std::string transfer_id = client.send_file(peer_id, file_path);
                if (!transfer_id.empty()) {
                    std::cout << "📤 Transfer initiated: " << transfer_id << std::endl;
                } else {
                    std::cout << "❌ Failed to initiate transfer" << std::endl;
                }
            } else {
                std::cout << "Usage: send <peer_id> <file_path>" << std::endl;
            }
        } else if (command.substr(0, 7) == "senddir") {
            // Parse senddir command  
            size_t first_space = command.find(' ', 8);
            size_t second_space = command.find(' ', first_space + 1);
            
            if (first_space != std::string::npos && second_space != std::string::npos) {
                std::string peer_id = command.substr(8, first_space - 8);
                std::string dir_path = command.substr(first_space + 1);
                
                std::string transfer_id = client.send_directory(peer_id, dir_path);
                if (!transfer_id.empty()) {
                    std::cout << "📁 Directory transfer initiated: " << transfer_id << std::endl;
                } else {
                    std::cout << "❌ Failed to initiate directory transfer" << std::endl;
                }
            } else {
                std::cout << "Usage: senddir <peer_id> <dir_path>" << std::endl;
            }
        }
    }
    
    std::cout << "Shutting down..." << std::endl;
    client.stop();
    return 0;
}
```

## Performance Tips

1. **Chunk Size**: Larger chunks (1MB+) for fast networks, smaller chunks (32KB-64KB) for slow/unreliable networks
2. **Concurrent Chunks**: More parallel chunks improve throughput but use more memory
3. **Checksums**: Disable for trusted networks to improve speed
4. **Compression**: Use LZ4 for good compression with minimal CPU overhead
5. **Resume**: Enable for unreliable connections to avoid retransmitting large files

## Security Considerations

- All file transfers respect the encryption settings of the RatsClient
- Checksums provide integrity verification
- File paths are validated to prevent directory traversal attacks
- Transfer requests can be filtered and approved by the application

## Limitations

- Maximum file size is limited by available disk space and memory
- Binary data is base64-encoded for JSON transport (30% overhead)
- Directory transfers are currently sequential (files sent one by one)
- Compression support requires optional dependencies (zlib, lz4)

