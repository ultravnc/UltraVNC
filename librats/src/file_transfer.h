#pragma once

#include "socket.h"
#include "json.hpp"
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <atomic>
#include <chrono>
#include <thread>
#include <queue>
#include <condition_variable>

namespace librats {

// Forward declaration
class RatsClient;

/**
 * File transfer status codes
 */
enum class FileTransferStatus {
    PENDING,        // Transfer queued but not started
    STARTING,       // Transfer initialization in progress
    IN_PROGRESS,    // Transfer actively sending/receiving chunks
    PAUSED,         // Transfer temporarily paused
    COMPLETED,      // Transfer completed successfully
    FAILED,         // Transfer failed due to error
    CANCELLED,      // Transfer cancelled by user
    RESUMING        // Transfer resuming from interruption
};

/**
 * File transfer direction
 */
enum class FileTransferDirection {
    SENDING,        // We are sending the file
    RECEIVING       // We are receiving the file
};

// Note: Compression removed as requested - only binary chunks needed

/**
 * File transfer chunk information
 */
struct FileChunk {
    std::string transfer_id;        // Unique transfer identifier
    uint64_t chunk_index;           // Sequential chunk number (0-based)
    uint64_t total_chunks;          // Total number of chunks in transfer
    uint64_t chunk_size;            // Size of this specific chunk
    uint64_t file_offset;           // Offset in the original file
    std::vector<uint8_t> data;      // Chunk data payload
    std::string checksum;           // SHA256 checksum of chunk data
    
    FileChunk() : chunk_index(0), total_chunks(0), chunk_size(0), 
                  file_offset(0) {}
};

/**
 * File metadata for transfers
 */
struct FileMetadata {
    std::string filename;           // Original filename
    std::string relative_path;      // Relative path within directory structure
    uint64_t file_size;             // Total file size in bytes
    uint64_t last_modified;         // Last modification timestamp
    std::string mime_type;          // MIME type of the file
    std::string checksum;           // Full file checksum
    
    FileMetadata() : file_size(0), last_modified(0) {}
};

/**
 * Directory transfer metadata
 */
struct DirectoryMetadata {
    std::string directory_name;     // Directory name
    std::string relative_path;      // Relative path
    std::vector<FileMetadata> files; // Files in this directory level
    std::vector<DirectoryMetadata> subdirectories; // Nested directories
    
    // Calculate total transfer size
    uint64_t get_total_size() const;
    
    // Get total file count
    size_t get_total_file_count() const;
};

/**
 * File transfer progress information
 */
struct FileTransferProgress {
    std::string transfer_id;        // Transfer identifier
    std::string peer_id;            // Peer we're transferring with
    FileTransferDirection direction; // Send or receive
    FileTransferStatus status;      // Current status
    
    // File information
    std::string filename;           // File being transferred
    std::string local_path;         // Local file path
    uint64_t file_size;             // Total file size
    
    // Progress tracking
    uint64_t bytes_transferred;     // Bytes completed
    uint64_t total_bytes;           // Total bytes to transfer
    uint32_t chunks_completed;      // Chunks successfully transferred
    uint32_t total_chunks;          // Total chunks in transfer
    
    // Performance metrics
    std::chrono::steady_clock::time_point start_time;    // Transfer start time
    std::chrono::steady_clock::time_point last_update;   // Last progress update
    double transfer_rate_bps;       // Current transfer rate (bytes/second)
    double average_rate_bps;        // Average transfer rate since start
    std::chrono::milliseconds estimated_time_remaining; // ETA
    
    // Error information
    std::string error_message;      // Error details if failed
    uint32_t retry_count;           // Number of retries attempted
    
    FileTransferProgress() : direction(FileTransferDirection::SENDING), 
                           status(FileTransferStatus::PENDING),
                           file_size(0), bytes_transferred(0), total_bytes(0),
                           chunks_completed(0), total_chunks(0),
                           transfer_rate_bps(0.0), average_rate_bps(0.0),
                           retry_count(0) {
        start_time = std::chrono::steady_clock::now();
        last_update = start_time;
    }
    
    // Calculate completion percentage (0.0 to 100.0)
    double get_completion_percentage() const {
        if (total_bytes == 0) return 0.0;
        return (static_cast<double>(bytes_transferred) / total_bytes) * 100.0;
    }
    
    // Calculate elapsed time
    std::chrono::milliseconds get_elapsed_time() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time);
    }
    
    // Update transfer rate calculations
    void update_transfer_rates(uint64_t new_bytes_transferred);
};

/**
 * File transfer configuration
 */
struct FileTransferConfig {
    uint32_t chunk_size;            // Size of each chunk (default: 64KB)
    uint32_t max_concurrent_chunks; // Max chunks in flight (default: 4)
    uint32_t max_retries;           // Max retry attempts per chunk (default: 3)
    uint32_t timeout_seconds;       // Timeout per chunk (default: 30)
    bool verify_checksums;          // Verify chunk checksums (default: true)
    bool allow_resume;              // Allow resuming interrupted transfers (default: true)
    std::string temp_directory;     // Temporary directory for incomplete files
    
    FileTransferConfig() 
        : chunk_size(65536),        // 64KB chunks
          max_concurrent_chunks(4), 
          max_retries(3),
          timeout_seconds(30),
          verify_checksums(true),
          allow_resume(true),
          temp_directory("./temp_transfers") {}
};

/**
 * Callback function types for file transfer events
 */
using FileTransferProgressCallback = std::function<void(const FileTransferProgress&)>;
using FileTransferCompletedCallback = std::function<void(const std::string& transfer_id, bool success, const std::string& error_message)>;
using FileTransferRequestCallback = std::function<bool(const std::string& peer_id, const FileMetadata& metadata, const std::string& transfer_id)>;
using DirectoryTransferProgressCallback = std::function<void(const std::string& transfer_id, const std::string& current_file, uint64_t files_completed, uint64_t total_files, uint64_t bytes_completed, uint64_t total_bytes)>;
using FileRequestCallback = std::function<bool(const std::string& peer_id, const std::string& file_path, const std::string& transfer_id)>;
using DirectoryRequestCallback = std::function<bool(const std::string& peer_id, const std::string& directory_path, bool recursive, const std::string& transfer_id)>;

/**
 * File transfer manager class
 * Handles efficient chunked file transfers with resume capability
 */
class FileTransferManager {
public:
    /**
     * Constructor
     * @param client Reference to RatsClient for communication
     * @param config Transfer configuration settings
     */
    FileTransferManager(RatsClient& client, const FileTransferConfig& config = FileTransferConfig());
    
    /**
     * Destructor
     */
    ~FileTransferManager();
    
    // Configuration
    /**
     * Update transfer configuration
     * @param config New configuration settings
     */
    void set_config(const FileTransferConfig& config);
    
    /**
     * Get current configuration
     * @return Current configuration settings
     */
    const FileTransferConfig& get_config() const;
    
    /**
     * Handle binary data that might be file transfer chunks
     * @param peer_id Source peer ID  
     * @param binary_data Binary data received
     * @return true if this was a file transfer chunk, false otherwise
     */
    bool handle_binary_data(const std::string& peer_id, const std::vector<uint8_t>& binary_data);
    
    // Callback registration
    /**
     * Set progress callback for transfer updates
     * @param callback Function to call with progress updates
     */
    void set_progress_callback(FileTransferProgressCallback callback);
    
    /**
     * Set completion callback for transfer completion
     * @param callback Function to call when transfers complete
     */
    void set_completion_callback(FileTransferCompletedCallback callback);
    
    /**
     * Set incoming transfer request callback
     * @param callback Function to call when receiving transfer requests
     */
    void set_request_callback(FileTransferRequestCallback callback);
    
    /**
     * Set directory transfer progress callback
     * @param callback Function to call with directory transfer progress
     */
    void set_directory_progress_callback(DirectoryTransferProgressCallback callback);
    
    /**
     * Set file request callback (called when receiving file requests)
     * @param callback Function to call when receiving file requests
     */
    void set_file_request_callback(FileRequestCallback callback);
    
    /**
     * Set directory request callback (called when receiving directory requests)
     * @param callback Function to call when receiving directory requests
     */
    void set_directory_request_callback(DirectoryRequestCallback callback);
    
    // File transfer operations
    /**
     * Send a file to a peer
     * @param peer_id Target peer ID
     * @param file_path Local file path to send
     * @param remote_filename Optional remote filename (default: use local name)
     * @return Transfer ID if successful, empty string if failed
     */
    std::string send_file(const std::string& peer_id, const std::string& file_path, 
                         const std::string& remote_filename = "");
    
    /**
     * Send a file with custom metadata
     * @param peer_id Target peer ID
     * @param file_path Local file path to send
     * @param metadata Custom file metadata
     * @return Transfer ID if successful, empty string if failed
     */
    std::string send_file_with_metadata(const std::string& peer_id, const std::string& file_path,
                                       const FileMetadata& metadata);
    
    /**
     * Send an entire directory to a peer
     * @param peer_id Target peer ID
     * @param directory_path Local directory path to send
     * @param remote_directory_name Optional remote directory name
     * @param recursive Whether to include subdirectories (default: true)
     * @return Transfer ID if successful, empty string if failed
     */
    std::string send_directory(const std::string& peer_id, const std::string& directory_path,
                              const std::string& remote_directory_name = "", bool recursive = true);
    
    /**
     * Request a file from a remote peer
     * @param peer_id Target peer ID
     * @param remote_file_path Path to file on remote peer
     * @param local_path Local path where file should be saved
     * @return Transfer ID if successful, empty string if failed
     */
    std::string request_file(const std::string& peer_id, const std::string& remote_file_path,
                            const std::string& local_path);
    
    /**
     * Request a directory from a remote peer
     * @param peer_id Target peer ID
     * @param remote_directory_path Path to directory on remote peer
     * @param local_directory_path Local path where directory should be saved
     * @param recursive Whether to include subdirectories (default: true)
     * @return Transfer ID if successful, empty string if failed
     */
    std::string request_directory(const std::string& peer_id, const std::string& remote_directory_path,
                                 const std::string& local_directory_path, bool recursive = true);
    
    /**
     * Accept an incoming file transfer
     * @param transfer_id Transfer identifier from request
     * @param local_path Local path where file should be saved
     * @return true if accepted successfully
     */
    bool accept_file_transfer(const std::string& transfer_id, const std::string& local_path);
    
    /**
     * Reject an incoming file transfer
     * @param transfer_id Transfer identifier from request
     * @param reason Optional reason for rejection
     * @return true if rejected successfully
     */
    bool reject_file_transfer(const std::string& transfer_id, const std::string& reason = "");
    
    /**
     * Accept an incoming directory transfer
     * @param transfer_id Transfer identifier from request
     * @param local_path Local path where directory should be saved
     * @return true if accepted successfully
     */
    bool accept_directory_transfer(const std::string& transfer_id, const std::string& local_path);
    
    /**
     * Reject an incoming directory transfer
     * @param transfer_id Transfer identifier from request
     * @param reason Optional reason for rejection
     * @return true if rejected successfully
     */
    bool reject_directory_transfer(const std::string& transfer_id, const std::string& reason = "");
    
    // Transfer control
    /**
     * Pause an active transfer
     * @param transfer_id Transfer to pause
     * @return true if paused successfully
     */
    bool pause_transfer(const std::string& transfer_id);
    
    /**
     * Resume a paused transfer
     * @param transfer_id Transfer to resume
     * @return true if resumed successfully
     */
    bool resume_transfer(const std::string& transfer_id);
    
    /**
     * Cancel an active or paused transfer
     * @param transfer_id Transfer to cancel
     * @return true if cancelled successfully
     */
    bool cancel_transfer(const std::string& transfer_id);
    
    /**
     * Retry a failed transfer
     * @param transfer_id Transfer to retry
     * @return true if retry initiated successfully
     */
    bool retry_transfer(const std::string& transfer_id);
    
    // Information and monitoring
    /**
     * Get progress information for a transfer
     * @param transfer_id Transfer to query
     * @return Progress information or nullptr if not found
     */
    std::shared_ptr<FileTransferProgress> get_transfer_progress(const std::string& transfer_id) const;
    
    /**
     * Get all active transfers
     * @return Vector of transfer progress objects
     */
    std::vector<std::shared_ptr<FileTransferProgress>> get_active_transfers() const;
    
    /**
     * Get transfer history
     * @param limit Maximum number of entries to return (0 for all)
     * @return Vector of completed transfer progress objects
     */
    std::vector<std::shared_ptr<FileTransferProgress>> get_transfer_history(size_t limit = 0) const;
    
    /**
     * Clear transfer history
     */
    void clear_transfer_history();
    
    /**
     * Get statistics about transfers
     * @return JSON object with transfer statistics
     */
    nlohmann::json get_transfer_statistics() const;
    
    // Utility functions
    /**
     * Calculate file checksum
     * @param file_path Path to file
     * @param algorithm Hash algorithm ("md5", "sha256")
     * @return Checksum string or empty if failed
     */
    static std::string calculate_file_checksum(const std::string& file_path, const std::string& algorithm = "sha256");
    
    /**
     * Get file metadata
     * @param file_path Path to file
     * @return File metadata structure
     */
    static FileMetadata get_file_metadata(const std::string& file_path);
    
    /**
     * Get directory metadata
     * @param directory_path Path to directory
     * @param recursive Whether to scan recursively
     * @return Directory metadata structure
     */
    static DirectoryMetadata get_directory_metadata(const std::string& directory_path, bool recursive = true);
    
    /**
     * Validate file path and permissions
     * @param file_path Path to validate
     * @param check_write Whether to check write permissions
     * @return true if valid and accessible
     */
    static bool validate_file_path(const std::string& file_path, bool check_write = false);

private:
    RatsClient& client_;
    FileTransferConfig config_;
    
    // Transfer tracking
    mutable std::mutex transfers_mutex_;
    std::unordered_map<std::string, std::shared_ptr<FileTransferProgress>> active_transfers_;
    std::unordered_map<std::string, std::shared_ptr<FileTransferProgress>> completed_transfers_;
    
    // Pending transfers (not yet accepted/rejected)
    mutable std::mutex pending_mutex_;
    struct PendingFileTransfer {
        FileMetadata metadata;
        std::string peer_id;
    };
    struct PendingDirectoryTransfer {
        DirectoryMetadata metadata;
        std::string peer_id;
    };
    std::unordered_map<std::string, PendingFileTransfer> pending_transfers_;
    std::unordered_map<std::string, PendingDirectoryTransfer> pending_directory_transfers_;
    
    // Active directory transfers
    mutable std::mutex directory_transfers_mutex_;
    std::unordered_map<std::string, DirectoryMetadata> active_directory_transfers_;
    
    // Chunk management
    mutable std::mutex chunks_mutex_;
    std::unordered_map<std::string, std::queue<FileChunk>> outgoing_chunks_;
    std::unordered_map<std::string, std::unordered_map<uint64_t, FileChunk>> received_chunks_;
    
    // Active chunk transfers waiting for binary data
    struct PendingChunk {
        std::string transfer_id;
        uint64_t chunk_index;
        uint64_t total_chunks;
        uint64_t chunk_size;
        uint64_t file_offset;
        std::string checksum;
        std::chrono::steady_clock::time_point created_at;
    };
    std::unordered_map<std::string, PendingChunk> pending_chunks_; // key: peer_id
    
    // Worker threads
    std::vector<std::thread> worker_threads_;
    std::atomic<bool> running_;
    std::condition_variable work_condition_;
    std::mutex work_mutex_;
    std::queue<std::string> work_queue_; // Transfer IDs that need processing
    
    // Cleanup thread synchronization
    std::condition_variable cleanup_condition_;
    std::mutex cleanup_mutex_;
    
    // Throttling synchronization
    std::condition_variable throttle_condition_;
    std::mutex throttle_mutex_;
    
    // Callbacks
    FileTransferProgressCallback progress_callback_;
    FileTransferCompletedCallback completion_callback_;
    FileTransferRequestCallback request_callback_;
    DirectoryTransferProgressCallback directory_progress_callback_;
    FileRequestCallback file_request_callback_;
    DirectoryRequestCallback directory_request_callback_;
    
    // Statistics
    mutable std::mutex stats_mutex_;
    uint64_t total_bytes_sent_;
    uint64_t total_bytes_received_;
    uint64_t total_files_sent_;
    uint64_t total_files_received_;
    std::chrono::steady_clock::time_point start_time_;
    
    // Private methods
    void initialize();
    void shutdown();
    void worker_thread_loop();
    void cleanup_thread_loop();
    void process_transfer(const std::string& transfer_id);
    
    // Transfer management
    std::string generate_transfer_id() const;
    void start_file_send(const std::string& transfer_id);
    void start_file_receive(const std::string& transfer_id);
    void start_directory_send(const std::string& transfer_id);
    void start_directory_receive(const std::string& transfer_id);
    void handle_chunk_received(const FileChunk& chunk);
    void handle_chunk_ack(const std::string& transfer_id, uint64_t chunk_index, bool success);
    
    // File operations
    bool create_temp_file(const std::string& transfer_id, uint64_t file_size);
    bool finalize_received_file(const std::string& transfer_id, const std::string& final_path);
    
    // Checksum validation
    bool verify_chunk_checksum(const FileChunk& chunk);
    std::string calculate_chunk_checksum(const std::vector<uint8_t>& data);
    
    // Network message handling
    void handle_transfer_request(const std::string& peer_id, const nlohmann::json& message);
    void handle_transfer_response(const std::string& peer_id, const nlohmann::json& message);
    void handle_chunk_metadata_message(const std::string& peer_id, const nlohmann::json& message);
    void handle_chunk_binary_message(const std::string& peer_id, const std::vector<uint8_t>& binary_data);
    void handle_chunk_ack_message(const std::string& peer_id, const nlohmann::json& message);
    void handle_transfer_control(const std::string& peer_id, const nlohmann::json& message);
    void handle_file_request(const std::string& peer_id, const nlohmann::json& message);
    void handle_directory_request(const std::string& peer_id, const nlohmann::json& message);
    
    // Message creation
    nlohmann::json create_transfer_request_message(const FileMetadata& metadata, const std::string& transfer_id);
    nlohmann::json create_transfer_response_message(const std::string& transfer_id, bool accepted, const std::string& reason = "");
    std::vector<uint8_t> create_chunk_binary_message(const FileChunk& chunk);
    nlohmann::json create_chunk_metadata_message(const FileChunk& chunk);
    nlohmann::json create_chunk_ack_message(const std::string& transfer_id, uint64_t chunk_index, bool success, const std::string& error = "");
    nlohmann::json create_control_message(const std::string& transfer_id, const std::string& action, const nlohmann::json& data = nlohmann::json::object());
    
    // Progress tracking
    void update_transfer_progress(const std::string& transfer_id, uint64_t bytes_delta = 0);
    void complete_transfer(const std::string& transfer_id, bool success, const std::string& error_message = "");
    void move_to_completed(const std::string& transfer_id);
    
    // File system utilities
    static bool ensure_directory_exists(const std::string& directory_path);
    static std::string get_temp_file_path(const std::string& transfer_id, const std::string& temp_dir);
    static std::string extract_filename(const std::string& file_path);
    static std::string get_mime_type(const std::string& file_path);
    
    // Binary chunk transmission helper
    bool parse_chunk_binary_header(const std::vector<uint8_t>& binary_data, FileChunk& chunk);
};

} // namespace librats

