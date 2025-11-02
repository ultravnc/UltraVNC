#include "file_transfer.h"
#include "librats.h"
#include "fs.h"
#include "logger.h"
#include "sha1.h"

// Define logging module for this file
#define LOG_FILE_TRANSFER_INFO(message) LOG_INFO("filetransfer", message)
#define LOG_FILE_TRANSFER_ERROR(message) LOG_ERROR("filetransfer", message)
#define LOG_FILE_TRANSFER_WARN(message) LOG_WARN("filetransfer", message)
#define LOG_FILE_TRANSFER_DEBUG(message) LOG_DEBUG("filetransfer", message)
#include <algorithm>
#include <random>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <cstdlib>

// Optional compression support
#ifdef LIBRATS_ENABLE_ZLIB
#include <zlib.h>
#endif

#ifdef LIBRATS_ENABLE_LZ4
#include <lz4.h>
#endif

namespace librats {

//=============================================================================
// DirectoryMetadata Implementation
//=============================================================================

uint64_t DirectoryMetadata::get_total_size() const {
    uint64_t total = 0;
    
    // Add size of files in this directory
    for (const auto& file : files) {
        total += file.file_size;
    }
    
    // Add size of subdirectories recursively
    for (const auto& subdir : subdirectories) {
        total += subdir.get_total_size();
    }
    
    return total;
}

size_t DirectoryMetadata::get_total_file_count() const {
    size_t count = files.size();
    
    // Add files from subdirectories recursively
    for (const auto& subdir : subdirectories) {
        count += subdir.get_total_file_count();
    }
    
    return count;
}

//=============================================================================
// FileTransferProgress Implementation
//=============================================================================

void FileTransferProgress::update_transfer_rates(uint64_t new_bytes_transferred) {
    auto now = std::chrono::steady_clock::now();
    auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update);
    
    if (time_diff.count() > 0) {
        uint64_t bytes_diff = new_bytes_transferred - bytes_transferred;
        transfer_rate_bps = (static_cast<double>(bytes_diff) * 1000.0) / time_diff.count();
        
        // Calculate average rate since start
        auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time);
        if (total_time.count() > 0) {
            average_rate_bps = (static_cast<double>(new_bytes_transferred) * 1000.0) / total_time.count();
        }
        
        // Estimate time remaining
        if (transfer_rate_bps > 0 && total_bytes > new_bytes_transferred) {
            uint64_t remaining_bytes = total_bytes - new_bytes_transferred;
            estimated_time_remaining = std::chrono::milliseconds(
                static_cast<int64_t>((remaining_bytes * 1000.0) / transfer_rate_bps)
            );
        } else {
            estimated_time_remaining = std::chrono::milliseconds(0);
        }
    }
    
    bytes_transferred = new_bytes_transferred;
    last_update = now;
}

//=============================================================================
// FileTransferManager Implementation
//=============================================================================

FileTransferManager::FileTransferManager(RatsClient& client, const FileTransferConfig& config)
    : client_(client), config_(config), running_(true),
      total_bytes_sent_(0), total_bytes_received_(0),
      total_files_sent_(0), total_files_received_(0) {
    
    start_time_ = std::chrono::steady_clock::now();
    initialize();
}

FileTransferManager::~FileTransferManager() {
    shutdown();
}

void FileTransferManager::initialize() {
    // Ensure temp directory exists
    create_directories(config_.temp_directory.c_str());
    
    // Register message handlers with RatsClient
    client_.on("file_transfer_request", [this](const std::string& peer_id, const nlohmann::json& data) {
        handle_transfer_request(peer_id, data);
    });
    
    client_.on("file_transfer_response", [this](const std::string& peer_id, const nlohmann::json& data) {
        handle_transfer_response(peer_id, data);
    });
    
    client_.on("file_chunk_metadata", [this](const std::string& peer_id, const nlohmann::json& data) {
        handle_chunk_metadata_message(peer_id, data);
    });
    
    // Note: Binary chunk data will be handled through the global binary_data_callback_
    // The FileTransferManager will check for file chunk magic headers in the callback
    
    client_.on("file_chunk_ack", [this](const std::string& peer_id, const nlohmann::json& data) {
        handle_chunk_ack_message(peer_id, data);
    });
    
    client_.on("file_transfer_control", [this](const std::string& peer_id, const nlohmann::json& data) {
        handle_transfer_control(peer_id, data);
    });
    
    client_.on("file_request", [this](const std::string& peer_id, const nlohmann::json& data) {
        handle_file_request(peer_id, data);
    });
    
    client_.on("directory_request", [this](const std::string& peer_id, const nlohmann::json& data) {
        handle_directory_request(peer_id, data);
    });
    
    // Start worker threads
    for (uint32_t i = 0; i < config_.max_concurrent_chunks; ++i) {
        worker_threads_.emplace_back(&FileTransferManager::worker_thread_loop, this);
    }
    
    // Start cleanup thread for pending chunks timeout
    worker_threads_.emplace_back(&FileTransferManager::cleanup_thread_loop, this);
    
    LOG_FILE_TRANSFER_INFO("FileTransferManager initialized with " << worker_threads_.size() << " worker threads");
}

void FileTransferManager::shutdown() {
    LOG_FILE_TRANSFER_INFO("FileTransferManager stopping...");
    running_.store(false);
    
    // Notify all condition variables to wake up waiting threads immediately
    work_condition_.notify_all();
    cleanup_condition_.notify_all();
    throttle_condition_.notify_all();
    
    // Join all worker threads
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    worker_threads_.clear();
    LOG_FILE_TRANSFER_INFO("FileTransferManager stopped");
}

void FileTransferManager::worker_thread_loop() {
    while (running_.load()) {
        std::unique_lock<std::mutex> lock(work_mutex_);
        work_condition_.wait(lock, [this] { return !work_queue_.empty() || !running_.load(); });
        
        if (!running_.load()) {
            break;
        }
        
        if (!work_queue_.empty()) {
            std::string transfer_id = work_queue_.front();
            work_queue_.pop();
            lock.unlock();
            
            process_transfer(transfer_id);
        }
    }
}

void FileTransferManager::cleanup_thread_loop() {
    const auto cleanup_interval = std::chrono::seconds(5);
    const auto pending_timeout = std::chrono::seconds(30);
    
    while (running_.load()) {
        // Use condition variable with timeout instead of sleep
        std::unique_lock<std::mutex> lock(cleanup_mutex_);
        cleanup_condition_.wait_for(lock, cleanup_interval, [this] { return !running_.load(); });
        
        if (!running_.load()) {
            break;
        }
        
        // Clean up expired pending chunks
        auto now = std::chrono::steady_clock::now();
        std::vector<std::string> expired_peers;
        
        {
            std::lock_guard<std::mutex> lock(chunks_mutex_);
            for (auto it = pending_chunks_.begin(); it != pending_chunks_.end();) {
                if (now - it->second.created_at > pending_timeout) {
                    LOG_FILE_TRANSFER_WARN("Pending chunk from peer " << it->first << 
                                          " timed out (transfer: " << it->second.transfer_id << 
                                          ", chunk: " << it->second.chunk_index << ")");
                    expired_peers.push_back(it->first);
                    it = pending_chunks_.erase(it);
                } else {
                    ++it;
                }
            }
        }
        
        // Send negative acknowledgments for expired chunks
        for (const auto& peer_id : expired_peers) {
            // We don't have direct access to the transfer info here, but the timeout logged above
            // provides sufficient information for debugging
            LOG_FILE_TRANSFER_DEBUG("Cleaned up " << expired_peers.size() << " expired pending chunks");
        }
    }
}

void FileTransferManager::set_config(const FileTransferConfig& config) {
    std::lock_guard<std::mutex> lock(transfers_mutex_);
    config_ = config;
    
    // Ensure temp directory exists
    create_directories(config_.temp_directory.c_str());
}

const FileTransferConfig& FileTransferManager::get_config() const {
    return config_;
}

bool FileTransferManager::handle_binary_data(const std::string& peer_id, const std::vector<uint8_t>& binary_data) {
    // Check if this is a file chunk binary message
    const std::string magic = "FTCHUNK";
    
    if (binary_data.size() >= magic.length() && 
        std::memcmp(binary_data.data(), magic.c_str(), magic.length()) == 0) {
        
        // This is a file transfer chunk - handle it
        handle_chunk_binary_message(peer_id, binary_data);
        return true;
    }
    
    // Not a file transfer chunk
    return false;
}

void FileTransferManager::set_progress_callback(FileTransferProgressCallback callback) {
    progress_callback_ = callback;
}

void FileTransferManager::set_completion_callback(FileTransferCompletedCallback callback) {
    completion_callback_ = callback;
}

void FileTransferManager::set_request_callback(FileTransferRequestCallback callback) {
    request_callback_ = callback;
}

void FileTransferManager::set_directory_progress_callback(DirectoryTransferProgressCallback callback) {
    directory_progress_callback_ = callback;
}

void FileTransferManager::set_file_request_callback(FileRequestCallback callback) {
    file_request_callback_ = callback;
}

void FileTransferManager::set_directory_request_callback(DirectoryRequestCallback callback) {
    directory_request_callback_ = callback;
}

std::string FileTransferManager::send_file(const std::string& peer_id, const std::string& file_path, 
                                          const std::string& remote_filename) {
    // Validate file
    if (!validate_file_path(file_path, false)) {
        LOG_FILE_TRANSFER_ERROR("Invalid file path: " << file_path);
        return "";
    }
    
    // Get file metadata
    FileMetadata metadata = get_file_metadata(file_path);
    if (metadata.file_size == 0) {
        LOG_FILE_TRANSFER_ERROR("Failed to get metadata for file: " << file_path);
        return "";
    }
    
    // Use custom filename if provided
    if (!remote_filename.empty()) {
        metadata.filename = remote_filename;
    }
    
    return send_file_with_metadata(peer_id, file_path, metadata);
}

std::string FileTransferManager::send_file_with_metadata(const std::string& peer_id, const std::string& file_path,
                                                        const FileMetadata& metadata) {
    std::string transfer_id = generate_transfer_id();
    
    // Create transfer progress tracking
    auto progress = std::make_shared<FileTransferProgress>();
    progress->transfer_id = transfer_id;
    progress->peer_id = peer_id;
    progress->direction = FileTransferDirection::SENDING;
    progress->status = FileTransferStatus::STARTING;
    progress->filename = metadata.filename;
    progress->local_path = file_path;
    progress->file_size = metadata.file_size;
    progress->total_bytes = metadata.file_size;
    progress->total_chunks = (metadata.file_size + config_.chunk_size - 1) / config_.chunk_size;
    
    {
        std::lock_guard<std::mutex> lock(transfers_mutex_);
        active_transfers_[transfer_id] = progress;
    }
    
    // Send transfer request to peer
    nlohmann::json request_msg = create_transfer_request_message(metadata, transfer_id);
    client_.send(peer_id, "file_transfer_request", request_msg);
    
    LOG_FILE_TRANSFER_INFO("Initiated file transfer request: " << transfer_id << " (" << metadata.filename << " -> " << peer_id << ")");
    return transfer_id;
}

std::string FileTransferManager::send_directory(const std::string& peer_id, const std::string& directory_path,
                                               const std::string& remote_directory_name, bool recursive) {
    // Validate directory
    if (!directory_exists(directory_path)) {
        LOG_FILE_TRANSFER_ERROR("Invalid directory path: " << directory_path);
        return "";
    }
    
    // Get directory metadata
    DirectoryMetadata dir_metadata = get_directory_metadata(directory_path, recursive);
    if (dir_metadata.files.empty() && dir_metadata.subdirectories.empty()) {
        LOG_FILE_TRANSFER_ERROR("Directory is empty: " << directory_path);
        return "";
    }
    
    // Use custom directory name if provided
    if (!remote_directory_name.empty()) {
        dir_metadata.directory_name = remote_directory_name;
    }
    
    std::string transfer_id = generate_transfer_id();
    
    // Create transfer progress tracking for directory
    auto progress = std::make_shared<FileTransferProgress>();
    progress->transfer_id = transfer_id;
    progress->peer_id = peer_id;
    progress->direction = FileTransferDirection::SENDING;
    progress->status = FileTransferStatus::STARTING;
    progress->filename = dir_metadata.directory_name;
    progress->local_path = directory_path;
    progress->total_bytes = dir_metadata.get_total_size();
    progress->file_size = progress->total_bytes;
    
    {
        std::lock_guard<std::mutex> lock(transfers_mutex_);
        active_transfers_[transfer_id] = progress;
    }
    
    // Store directory metadata for processing
    {
        std::lock_guard<std::mutex> dir_lock(directory_transfers_mutex_);
        active_directory_transfers_[transfer_id] = dir_metadata;
    }
    
    // Create directory transfer request message
    nlohmann::json request_msg;
    request_msg["transfer_id"] = transfer_id;
    request_msg["type"] = "directory";
    
    // Serialize directory metadata with full file information
    nlohmann::json dir_metadata_json;
    dir_metadata_json["directory_name"] = dir_metadata.directory_name;
    dir_metadata_json["relative_path"] = dir_metadata.relative_path;
    dir_metadata_json["total_size"] = dir_metadata.get_total_size();
    dir_metadata_json["total_files"] = dir_metadata.get_total_file_count();
    
    // Serialize file metadata
    nlohmann::json files_json = nlohmann::json::array();
    for (const auto& file_meta : dir_metadata.files) {
        nlohmann::json file_json;
        file_json["filename"] = file_meta.filename;
        file_json["relative_path"] = file_meta.relative_path;
        file_json["file_size"] = file_meta.file_size;
        file_json["last_modified"] = file_meta.last_modified;
        file_json["mime_type"] = file_meta.mime_type;
        file_json["checksum"] = file_meta.checksum;
        files_json.push_back(file_json);
    }
    dir_metadata_json["files"] = files_json;
    
    // For now, we'll handle single-level directories (can be extended for nested later)
    dir_metadata_json["subdirectories"] = nlohmann::json::array();
    
    request_msg["directory_metadata"] = dir_metadata_json;
    
    client_.send(peer_id, "file_transfer_request", request_msg);
    
    LOG_FILE_TRANSFER_INFO("Initiated directory transfer request: " << transfer_id << " (" << dir_metadata.directory_name << " -> " << peer_id << ")");
    return transfer_id;
}

std::string FileTransferManager::request_file(const std::string& peer_id, const std::string& remote_file_path,
                                             const std::string& local_path) {
    std::string transfer_id = generate_transfer_id();
    
    // Create file request message
    nlohmann::json request_msg;
    request_msg["transfer_id"] = transfer_id;
    request_msg["type"] = "file_request";
    request_msg["remote_path"] = remote_file_path;
    request_msg["local_path"] = local_path;
    
    client_.send(peer_id, "file_request", request_msg);
    
    LOG_FILE_TRANSFER_INFO("Sent file request: " << transfer_id << " (" << remote_file_path << " from " << peer_id << ")");
    return transfer_id;
}

std::string FileTransferManager::request_directory(const std::string& peer_id, const std::string& remote_directory_path,
                                                   const std::string& local_directory_path, bool recursive) {
    std::string transfer_id = generate_transfer_id();
    
    // Create directory request message
    nlohmann::json request_msg;
    request_msg["transfer_id"] = transfer_id;
    request_msg["type"] = "directory_request";
    request_msg["remote_path"] = remote_directory_path;
    request_msg["local_path"] = local_directory_path;
    request_msg["recursive"] = recursive;
    
    client_.send(peer_id, "directory_request", request_msg);
    
    LOG_FILE_TRANSFER_INFO("Sent directory request: " << transfer_id << " (" << remote_directory_path << " from " << peer_id << ")");
    return transfer_id;
}

bool FileTransferManager::accept_file_transfer(const std::string& transfer_id, const std::string& local_path) {
    std::lock_guard<std::mutex> lock(pending_mutex_);
    
    auto it = pending_transfers_.find(transfer_id);
    if (it == pending_transfers_.end()) {
        LOG_FILE_TRANSFER_ERROR("Transfer not found in pending transfers: " << transfer_id);
        return false;
    }
    
    PendingFileTransfer pending_transfer = it->second;
    pending_transfers_.erase(it);
    
    // Create transfer progress tracking
    auto progress = std::make_shared<FileTransferProgress>();
    progress->transfer_id = transfer_id;
    progress->peer_id = pending_transfer.peer_id;
    progress->direction = FileTransferDirection::RECEIVING;
    progress->status = FileTransferStatus::STARTING;
    progress->filename = pending_transfer.metadata.filename;
    progress->local_path = local_path;
    progress->file_size = pending_transfer.metadata.file_size;
    progress->total_bytes = pending_transfer.metadata.file_size;
    progress->total_chunks = (pending_transfer.metadata.file_size + config_.chunk_size - 1) / config_.chunk_size;
    
    {
        std::lock_guard<std::mutex> transfers_lock(transfers_mutex_);
        active_transfers_[transfer_id] = progress;
    }
    
    // Send acceptance response
    nlohmann::json response_msg = create_transfer_response_message(transfer_id, true);
    client_.send(pending_transfer.peer_id, "file_transfer_response", response_msg);
    
    // Add to work queue for processing
    {
        std::lock_guard<std::mutex> work_lock(work_mutex_);
        work_queue_.push(transfer_id);
    }
    work_condition_.notify_one();
    
    LOG_FILE_TRANSFER_INFO("Accepted file transfer: " << transfer_id << " (" << pending_transfer.metadata.filename << " from " << pending_transfer.peer_id << " -> " << local_path << ")");
    return true;
}

bool FileTransferManager::reject_file_transfer(const std::string& transfer_id, const std::string& reason) {
    std::lock_guard<std::mutex> lock(pending_mutex_);
    
    auto it = pending_transfers_.find(transfer_id);
    if (it == pending_transfers_.end()) {
        LOG_FILE_TRANSFER_ERROR("Transfer not found in pending transfers: " << transfer_id);
        return false;
    }
    
    PendingFileTransfer pending_transfer = it->second;
    pending_transfers_.erase(it);
    
    // Send rejection response
    nlohmann::json response_msg = create_transfer_response_message(transfer_id, false, reason);
    client_.send(pending_transfer.peer_id, "file_transfer_response", response_msg);
    
    LOG_FILE_TRANSFER_INFO("Rejected file transfer: " << transfer_id << " (" << pending_transfer.metadata.filename << " from " << pending_transfer.peer_id << ", reason: " << reason << ")");
    return true;
}

bool FileTransferManager::accept_directory_transfer(const std::string& transfer_id, const std::string& local_path) {
    std::lock_guard<std::mutex> lock(pending_mutex_);
    
    auto it = pending_directory_transfers_.find(transfer_id);
    if (it == pending_directory_transfers_.end()) {
        LOG_FILE_TRANSFER_ERROR("Directory transfer not found in pending transfers: " << transfer_id);
        return false;
    }
    
    PendingDirectoryTransfer pending_transfer = it->second;
    pending_directory_transfers_.erase(it);
    
    // Create transfer progress tracking
    auto progress = std::make_shared<FileTransferProgress>();
    progress->transfer_id = transfer_id;
    progress->peer_id = pending_transfer.peer_id;
    progress->direction = FileTransferDirection::RECEIVING;
    progress->status = FileTransferStatus::STARTING;
    progress->filename = pending_transfer.metadata.directory_name;
    progress->local_path = local_path;
    progress->total_bytes = pending_transfer.metadata.get_total_size();
    progress->file_size = progress->total_bytes;
    progress->total_chunks = 0; // Will be calculated per file
    
    {
        std::lock_guard<std::mutex> transfers_lock(transfers_mutex_);
        active_transfers_[transfer_id] = progress;
    }
    
    // Send acceptance response
    nlohmann::json response_msg = create_transfer_response_message(transfer_id, true);
    client_.send(pending_transfer.peer_id, "file_transfer_response", response_msg);
    
    // Store directory metadata for processing
    {
        std::lock_guard<std::mutex> dir_lock(directory_transfers_mutex_);
        active_directory_transfers_[transfer_id] = pending_transfer.metadata;
    }
    
    // Add to work queue for processing
    {
        std::lock_guard<std::mutex> work_lock(work_mutex_);
        work_queue_.push(transfer_id);
    }
    work_condition_.notify_one();
    
    LOG_FILE_TRANSFER_INFO("Accepted directory transfer: " << transfer_id << " (" << pending_transfer.metadata.directory_name << " from " << pending_transfer.peer_id << " -> " << local_path << ")");
    return true;
}

bool FileTransferManager::reject_directory_transfer(const std::string& transfer_id, const std::string& reason) {
    std::lock_guard<std::mutex> lock(pending_mutex_);
    
    auto it = pending_directory_transfers_.find(transfer_id);
    if (it == pending_directory_transfers_.end()) {
        LOG_FILE_TRANSFER_ERROR("Directory transfer not found in pending transfers: " << transfer_id);
        return false;
    }
    
    PendingDirectoryTransfer pending_transfer = it->second;
    pending_directory_transfers_.erase(it);
    
    // Send rejection response
    nlohmann::json response_msg = create_transfer_response_message(transfer_id, false, reason);
    client_.send(pending_transfer.peer_id, "file_transfer_response", response_msg);
    
    LOG_FILE_TRANSFER_INFO("Rejected directory transfer: " << transfer_id << " (" << pending_transfer.metadata.directory_name << " from " << pending_transfer.peer_id << ", reason: " << reason << ")");
    return true;
}

bool FileTransferManager::pause_transfer(const std::string& transfer_id) {
    std::lock_guard<std::mutex> lock(transfers_mutex_);
    
    auto it = active_transfers_.find(transfer_id);
    if (it == active_transfers_.end()) {
        return false;
    }
    
    auto& progress = it->second;
    if (progress->status == FileTransferStatus::IN_PROGRESS) {
        progress->status = FileTransferStatus::PAUSED;
        
        // Send pause control message
        nlohmann::json control_msg = create_control_message(transfer_id, "pause");
        client_.send(progress->peer_id, "file_transfer_control", control_msg);
        
        LOG_FILE_TRANSFER_INFO("Paused transfer: " << transfer_id);
        return true;
    }
    
    return false;
}

bool FileTransferManager::resume_transfer(const std::string& transfer_id) {
    std::lock_guard<std::mutex> lock(transfers_mutex_);
    
    auto it = active_transfers_.find(transfer_id);
    if (it == active_transfers_.end()) {
        return false;
    }
    
    auto& progress = it->second;
    if (progress->status == FileTransferStatus::PAUSED) {
        progress->status = FileTransferStatus::RESUMING;
        
        // Add to work queue for processing
        {
            std::lock_guard<std::mutex> work_lock(work_mutex_);
            work_queue_.push(transfer_id);
        }
        work_condition_.notify_one();
        
        // Send resume control message
        nlohmann::json control_msg = create_control_message(transfer_id, "resume");
        client_.send(progress->peer_id, "file_transfer_control", control_msg);
        
        LOG_FILE_TRANSFER_INFO("Resumed transfer: " << transfer_id);
        return true;
    }
    
    return false;
}

bool FileTransferManager::cancel_transfer(const std::string& transfer_id) {
    std::lock_guard<std::mutex> lock(transfers_mutex_);
    
    auto it = active_transfers_.find(transfer_id);
    if (it == active_transfers_.end()) {
        return false;
    }
    
    auto& progress = it->second;
    progress->status = FileTransferStatus::CANCELLED;
    
    // Send cancel control message
    nlohmann::json control_msg = create_control_message(transfer_id, "cancel");
    client_.send(progress->peer_id, "file_transfer_control", control_msg);
    
    // Move to completed transfers
    move_to_completed(transfer_id);
    
    LOG_FILE_TRANSFER_INFO("Cancelled transfer: " << transfer_id);
    return true;
}

bool FileTransferManager::retry_transfer(const std::string& transfer_id) {
    // Look in completed transfers for failed ones
    std::lock_guard<std::mutex> lock(transfers_mutex_);
    
    auto it = completed_transfers_.find(transfer_id);
    if (it == completed_transfers_.end()) {
        return false;
    }
    
    auto progress = it->second;
    if (progress->status != FileTransferStatus::FAILED) {
        return false;
    }
    
    // Reset progress and move back to active
    progress->status = FileTransferStatus::STARTING;
    progress->bytes_transferred = 0;
    progress->chunks_completed = 0;
    progress->retry_count++;
    progress->error_message.clear();
    progress->start_time = std::chrono::steady_clock::now();
    
    active_transfers_[transfer_id] = progress;
    completed_transfers_.erase(it);
    
    // Add to work queue
    {
        std::lock_guard<std::mutex> work_lock(work_mutex_);
        work_queue_.push(transfer_id);
    }
    work_condition_.notify_one();
    
    LOG_FILE_TRANSFER_INFO("Retrying transfer: " << transfer_id);
    return true;
}

std::shared_ptr<FileTransferProgress> FileTransferManager::get_transfer_progress(const std::string& transfer_id) const {
    std::lock_guard<std::mutex> lock(transfers_mutex_);
    
    auto it = active_transfers_.find(transfer_id);
    if (it != active_transfers_.end()) {
        return it->second;
    }
    
    auto completed_it = completed_transfers_.find(transfer_id);
    if (completed_it != completed_transfers_.end()) {
        return completed_it->second;
    }
    
    return nullptr;
}

std::vector<std::shared_ptr<FileTransferProgress>> FileTransferManager::get_active_transfers() const {
    std::lock_guard<std::mutex> lock(transfers_mutex_);
    
    std::vector<std::shared_ptr<FileTransferProgress>> transfers;
    transfers.reserve(active_transfers_.size());
    
    for (const auto& pair : active_transfers_) {
        transfers.push_back(pair.second);
    }
    
    return transfers;
}

std::vector<std::shared_ptr<FileTransferProgress>> FileTransferManager::get_transfer_history(size_t limit) const {
    std::lock_guard<std::mutex> lock(transfers_mutex_);
    
    std::vector<std::shared_ptr<FileTransferProgress>> transfers;
    
    for (const auto& pair : completed_transfers_) {
        transfers.push_back(pair.second);
    }
    
    // Sort by completion time (most recent first)
    std::sort(transfers.begin(), transfers.end(), 
              [](const auto& a, const auto& b) {
                  return a->last_update > b->last_update;
              });
    
    if (limit > 0 && transfers.size() > limit) {
        transfers.resize(limit);
    }
    
    return transfers;
}

void FileTransferManager::clear_transfer_history() {
    std::lock_guard<std::mutex> lock(transfers_mutex_);
    completed_transfers_.clear();
    LOG_FILE_TRANSFER_INFO("Cleared transfer history");
}

nlohmann::json FileTransferManager::get_transfer_statistics() const {
    std::lock_guard<std::mutex> stats_lock(stats_mutex_);
    std::lock_guard<std::mutex> transfers_lock(transfers_mutex_);
    
    auto now = std::chrono::steady_clock::now();
    auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);
    
    nlohmann::json stats;
    stats["uptime_seconds"] = uptime.count();
    stats["total_bytes_sent"] = total_bytes_sent_;
    stats["total_bytes_received"] = total_bytes_received_;
    stats["total_files_sent"] = total_files_sent_;
    stats["total_files_received"] = total_files_received_;
    stats["active_transfers"] = active_transfers_.size();
    stats["completed_transfers"] = completed_transfers_.size();
    
    // Calculate success rate
    size_t successful_transfers = 0;
    for (const auto& pair : completed_transfers_) {
        if (pair.second->status == FileTransferStatus::COMPLETED) {
            successful_transfers++;
        }
    }
    
    if (!completed_transfers_.empty()) {
        stats["success_rate"] = static_cast<double>(successful_transfers) / completed_transfers_.size();
    } else {
        stats["success_rate"] = 0.0;
    }
    
    // Average transfer rate
    if (uptime.count() > 0) {
        stats["average_send_rate_bps"] = static_cast<double>(total_bytes_sent_) / uptime.count();
        stats["average_receive_rate_bps"] = static_cast<double>(total_bytes_received_) / uptime.count();
    } else {
        stats["average_send_rate_bps"] = 0.0;
        stats["average_receive_rate_bps"] = 0.0;
    }
    
    return stats;
}

// Static utility functions
std::string FileTransferManager::calculate_file_checksum(const std::string& file_path, const std::string& algorithm) {
    if (algorithm == "sha256") {
        // Use a simple SHA1 implementation for now (we can extend this)
        size_t file_size;
        void* file_data = read_file_binary(file_path.c_str(), &file_size);
        if (!file_data) {
            return "";
        }
        
        SHA1 sha1;
        sha1.update(reinterpret_cast<const uint8_t*>(file_data), file_size);
        std::string result = sha1.finalize();
        
        free_file_buffer(file_data);
        return result;
    }
    
    // For MD5 or other algorithms, we would need additional implementations
    return "";
}

FileMetadata FileTransferManager::get_file_metadata(const std::string& file_path) {
    FileMetadata metadata;
    
    try {
        if (!file_or_directory_exists(file_path.c_str())) {
            return metadata;
        }
        
        metadata.filename = get_filename_from_path(file_path.c_str());
        int64_t file_size = get_file_size(file_path.c_str());
        if (file_size >= 0) {
            metadata.file_size = static_cast<uint64_t>(file_size);
        }
        
        // Get last modification time
        metadata.last_modified = get_file_modified_time(file_path.c_str());
        
        // Calculate checksum (optional, can be expensive for large files)
        if (metadata.file_size < 100 * 1024 * 1024) { // Only for files < 100MB
            metadata.checksum = calculate_file_checksum(file_path, "sha256");
        }
        
        // Determine MIME type based on extension
        metadata.mime_type = get_mime_type(file_path);
        
    } catch (const std::exception& e) {
        LOG_FILE_TRANSFER_ERROR("Failed to get file metadata for " << file_path << ": " << e.what());
    }
    
    return metadata;
}

DirectoryMetadata FileTransferManager::get_directory_metadata(const std::string& directory_path, bool recursive) {
    DirectoryMetadata metadata;
    
    try {
        metadata.directory_name = get_filename_from_path(directory_path.c_str());
        
        std::vector<DirectoryEntry> entries;
        if (list_directory(directory_path.c_str(), entries)) {
            for (const auto& entry : entries) {
                if (!entry.is_directory) {
                    FileMetadata file_meta = get_file_metadata(entry.path);
                    file_meta.relative_path = entry.name;
                    metadata.files.push_back(file_meta);
                }
                else if (recursive) {
                    DirectoryMetadata subdir_meta = get_directory_metadata(entry.path, true);
                    subdir_meta.relative_path = entry.name;
                    metadata.subdirectories.push_back(subdir_meta);
                }
            }
        }
        
    } catch (const std::exception& e) {
        LOG_FILE_TRANSFER_ERROR("Failed to get directory metadata for " << directory_path << ": " << e.what());
    }
    
    return metadata;
}

bool FileTransferManager::validate_file_path(const std::string& file_path, bool check_write) {
    return validate_path(file_path.c_str(), check_write);
}

// Private implementation methods

std::string FileTransferManager::generate_transfer_id() const {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    ss << std::hex;
    for (int i = 0; i < 32; ++i) {
        ss << dis(gen);
    }
    
    return ss.str();
}

void FileTransferManager::process_transfer(const std::string& transfer_id) {
    // Exit immediately if shutting down
    if (!running_.load()) {
        return;
    }
    
    auto progress = get_transfer_progress(transfer_id);
    if (!progress) {
        return;
    }
    
    // Check again before processing (in case shutdown occurred during progress retrieval)
    if (!running_.load()) {
        return;
    }
    
    // Check if this is a directory transfer
    bool is_directory_transfer = false;
    {
        std::lock_guard<std::mutex> dir_lock(directory_transfers_mutex_);
        is_directory_transfer = active_directory_transfers_.find(transfer_id) != active_directory_transfers_.end();
    }
    
    if (is_directory_transfer) {
        if (progress->direction == FileTransferDirection::SENDING) {
            start_directory_send(transfer_id);
        } else {
            start_directory_receive(transfer_id);
        }
    } else {
        if (progress->direction == FileTransferDirection::SENDING) {
            start_file_send(transfer_id);
        } else {
            start_file_receive(transfer_id);
        }
    }
}

void FileTransferManager::start_file_send(const std::string& transfer_id) {
    auto progress = get_transfer_progress(transfer_id);
    if (!progress) {
        return;
    }
    
    progress->status = FileTransferStatus::IN_PROGRESS;
    update_transfer_progress(transfer_id);
    
    // Read file and create chunks
    uint64_t chunk_index = 0;
    uint64_t file_offset = 0;
    
    while (file_offset < progress->file_size && running_.load()) {
        // Check if transfer was cancelled or paused
        auto current_progress = get_transfer_progress(transfer_id);
        if (!current_progress || current_progress->status == FileTransferStatus::CANCELLED ||
            current_progress->status == FileTransferStatus::PAUSED) {
            return;
        }
        
        uint32_t chunk_size = (std::min)(static_cast<uint64_t>(config_.chunk_size), 
                                      progress->file_size - file_offset);
        
        FileChunk chunk;
        chunk.transfer_id = transfer_id;
        chunk.chunk_index = chunk_index;
        chunk.total_chunks = progress->total_chunks;
        chunk.chunk_size = chunk_size;
        chunk.file_offset = file_offset;
        chunk.data.resize(chunk_size);
        
        // Read chunk data
        if (!read_file_chunk(progress->local_path.c_str(), file_offset, chunk.data.data(), chunk_size)) {
            complete_transfer(transfer_id, false, "Failed to read complete chunk from file");
            return;
        }
        
        // Calculate checksum
        if (config_.verify_checksums) {
            chunk.checksum = calculate_chunk_checksum(chunk.data);
        }
        
        // Note: Compression removed as requested - only binary chunks needed
        
        // Send chunk metadata first
        nlohmann::json metadata_msg = create_chunk_metadata_message(chunk);
        client_.send(progress->peer_id, "file_chunk_metadata", metadata_msg);
        
        // Send chunk binary data
        std::vector<uint8_t> binary_msg = create_chunk_binary_message(chunk);
        client_.send_binary_to_peer_id(progress->peer_id, binary_msg, MessageDataType::BINARY);
        
        chunk_index++;
        file_offset += chunk_size;
        
        // Update progress
        update_transfer_progress(transfer_id, chunk_size);
        
        // Throttle sending to avoid overwhelming the peer - use condition variable for fast shutdown
        if (running_.load()) {
            std::unique_lock<std::mutex> lock(throttle_mutex_);
            throttle_condition_.wait_for(lock, std::chrono::milliseconds(10), [this] { return !running_.load(); });
        }
    }
    
    LOG_FILE_TRANSFER_INFO("Completed sending file chunks for transfer: " << transfer_id);
}

void FileTransferManager::start_file_receive(const std::string& transfer_id) {
    // Exit immediately if shutting down
    if (!running_.load()) {
        return;
    }
    
    auto progress = get_transfer_progress(transfer_id);
    if (!progress) {
        return;
    }
    
    progress->status = FileTransferStatus::IN_PROGRESS;
    
    // Create temporary file for receiving
    if (!create_temp_file(transfer_id, progress->file_size)) {
        complete_transfer(transfer_id, false, "Failed to create temporary file");
        return;
    }
    
    update_transfer_progress(transfer_id);
    LOG_FILE_TRANSFER_INFO("Started receiving file transfer: " << transfer_id);
}

void FileTransferManager::start_directory_send(const std::string& transfer_id) {
    auto progress = get_transfer_progress(transfer_id);
    if (!progress) {
        return;
    }
    
    DirectoryMetadata dir_metadata;
    {
        std::lock_guard<std::mutex> dir_lock(directory_transfers_mutex_);
        auto it = active_directory_transfers_.find(transfer_id);
        if (it == active_directory_transfers_.end()) {
            complete_transfer(transfer_id, false, "Directory metadata not found");
            return;
        }
        dir_metadata = it->second;
    }
    
    progress->status = FileTransferStatus::IN_PROGRESS;
    update_transfer_progress(transfer_id);
    
    // Send each file in the directory
    for (const auto& file_metadata : dir_metadata.files) {
        if (!running_.load()) {
            return;
        }
        
        // Check if transfer was cancelled or paused
        auto current_progress = get_transfer_progress(transfer_id);
        if (!current_progress || current_progress->status == FileTransferStatus::CANCELLED ||
            current_progress->status == FileTransferStatus::PAUSED) {
            return;
        }
        
        std::string full_file_path = combine_paths(progress->local_path, file_metadata.relative_path);
        
        // Send individual file using existing file transfer logic
        std::string file_transfer_id = send_file_with_metadata(progress->peer_id, full_file_path, file_metadata);
        
        if (file_transfer_id.empty()) {
            complete_transfer(transfer_id, false, "Failed to send file: " + file_metadata.filename);
            return;
        }
        
        // Wait for individual file transfer to complete
        // Note: In a real implementation, we might want to track multiple concurrent file transfers
        // For now, we'll send files sequentially
    }
    
    LOG_FILE_TRANSFER_INFO("Completed sending directory transfer: " << transfer_id);
    complete_transfer(transfer_id, true);
}

void FileTransferManager::start_directory_receive(const std::string& transfer_id) {
    auto progress = get_transfer_progress(transfer_id);
    if (!progress) {
        return;
    }
    
    DirectoryMetadata dir_metadata;
    {
        std::lock_guard<std::mutex> dir_lock(directory_transfers_mutex_);
        auto it = active_directory_transfers_.find(transfer_id);
        if (it == active_directory_transfers_.end()) {
            complete_transfer(transfer_id, false, "Directory metadata not found");
            return;
        }
        dir_metadata = it->second;
    }
    
    progress->status = FileTransferStatus::IN_PROGRESS;
    
    // Create directory structure
    std::string base_path = progress->local_path;
    if (!ensure_directory_exists(base_path)) {
        complete_transfer(transfer_id, false, "Failed to create local directory structure");
        return;
    }
    
    // Create subdirectories if needed
    for (const auto& file_metadata : dir_metadata.files) {
        std::string file_dir = combine_paths(base_path, get_parent_directory(file_metadata.relative_path.c_str()));
        if (!file_dir.empty() && !ensure_directory_exists(file_dir)) {
            complete_transfer(transfer_id, false, "Failed to create subdirectory: " + file_dir);
            return;
        }
    }
    
    update_transfer_progress(transfer_id);
    LOG_FILE_TRANSFER_INFO("Started receiving directory transfer: " << transfer_id);
    
    // Note: Individual files will be received as separate file transfer requests
    // The directory transfer completion will be handled when all files are received
}

bool FileTransferManager::create_temp_file(const std::string& transfer_id, uint64_t file_size) {
    std::string temp_path = get_temp_file_path(transfer_id, config_.temp_directory);
    
    try {
        create_directories(config_.temp_directory.c_str());
        
        // Create file with pre-allocated size
        if (!create_file_with_size(temp_path.c_str(), file_size)) {
            LOG_FILE_TRANSFER_ERROR("Failed to create temp file " << temp_path);
            return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        LOG_FILE_TRANSFER_ERROR("Failed to create temp file " << temp_path << ": " << e.what());
        return false;
    }
}

std::string FileTransferManager::get_temp_file_path(const std::string& transfer_id, const std::string& temp_dir) {
    return combine_paths(temp_dir, transfer_id + ".tmp");
}

bool FileTransferManager::ensure_directory_exists(const std::string& directory_path) {
    return create_directories(directory_path.c_str());
}

std::string FileTransferManager::extract_filename(const std::string& file_path) {
    return get_filename_from_path(file_path);
}

std::string FileTransferManager::get_mime_type(const std::string& file_path) {
    std::string extension = get_file_extension(file_path);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    // Basic MIME type mapping
    static const std::unordered_map<std::string, std::string> mime_types = {
        {".txt", "text/plain"},
        {".json", "application/json"},
        {".xml", "application/xml"},
        {".html", "text/html"},
        {".css", "text/css"},
        {".js", "application/javascript"},
        {".pdf", "application/pdf"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".gif", "image/gif"},
        {".bmp", "image/bmp"},
        {".svg", "image/svg+xml"},
        {".mp3", "audio/mpeg"},
        {".wav", "audio/wav"},
        {".mp4", "video/mp4"},
        {".avi", "video/x-msvideo"},
        {".zip", "application/zip"},
        {".tar", "application/x-tar"},
        {".gz", "application/gzip"}
    };
    
    auto it = mime_types.find(extension);
    return (it != mime_types.end()) ? it->second : "application/octet-stream";
}

// Message handling methods

void FileTransferManager::handle_transfer_request(const std::string& peer_id, const nlohmann::json& message) {
    try {
        std::string transfer_id = message["transfer_id"];
        
        if (message.contains("directory_metadata")) {
            // Directory transfer request
            DirectoryMetadata dir_metadata;
            auto dir_info = message["directory_metadata"];
            dir_metadata.directory_name = dir_info["directory_name"];
            dir_metadata.relative_path = dir_info["relative_path"];
            
            // Parse files and subdirectories from the metadata
            if (dir_info.contains("files")) {
                for (const auto& file_json : dir_info["files"]) {
                    FileMetadata file_meta;
                    file_meta.filename = file_json["filename"];
                    file_meta.relative_path = file_json["relative_path"];
                    file_meta.file_size = file_json["file_size"];
                    file_meta.mime_type = file_json.value("mime_type", "application/octet-stream");
                    file_meta.checksum = file_json.value("checksum", "");
                    file_meta.last_modified = file_json.value("last_modified", 0);
                    dir_metadata.files.push_back(file_meta);
                }
            }
            
            // For now, handle as single directory level (can be expanded for nested later)
            
            {
                std::lock_guard<std::mutex> lock(pending_mutex_);
                PendingDirectoryTransfer pending_transfer;
                pending_transfer.metadata = dir_metadata;
                pending_transfer.peer_id = peer_id;
                pending_directory_transfers_[transfer_id] = pending_transfer;
            }
            
            // Call user callback to approve/reject
            if (request_callback_) {
                // Create a dummy file metadata for compatibility with existing callback
                FileMetadata dummy_metadata;
                dummy_metadata.filename = dir_metadata.directory_name + " (directory)";
                dummy_metadata.file_size = dir_info.value("total_size", 0);
                
                bool accepted = request_callback_(peer_id, dummy_metadata, transfer_id);
                if (accepted) {
                    std::string local_path = "./" + dir_metadata.directory_name;
                    accept_directory_transfer(transfer_id, local_path);
                } else {
                    reject_directory_transfer(transfer_id, "Rejected by user");
                }
            } else {
                // Auto-reject if no callback is set
                reject_directory_transfer(transfer_id, "No request handler configured");
            }
            
            LOG_FILE_TRANSFER_INFO("Received directory transfer request from " << peer_id << " for " << dir_metadata.directory_name);
            return;
        }
        
        // Single file transfer request
        FileMetadata metadata;
        auto file_info = message["file_metadata"];
        metadata.filename = file_info["filename"];
        metadata.file_size = file_info["file_size"];
        metadata.mime_type = file_info.value("mime_type", "application/octet-stream");
        metadata.checksum = file_info.value("checksum", "");
        
        {
            std::lock_guard<std::mutex> lock(pending_mutex_);
            PendingFileTransfer pending_transfer;
            pending_transfer.metadata = metadata;
            pending_transfer.peer_id = peer_id;
            pending_transfers_[transfer_id] = pending_transfer;
        }
        
        // Call user callback to approve/reject
        if (request_callback_) {
            bool accepted = request_callback_(peer_id, metadata, transfer_id);
            if (accepted) {
                // We need a way to specify the local path - this should be part of the callback
                // For now, we'll create a default path
                std::string local_path = "./" + metadata.filename;
                accept_file_transfer(transfer_id, local_path);
            } else {
                reject_file_transfer(transfer_id, "Rejected by user");
            }
        } else {
            // Auto-reject if no callback is set
            reject_file_transfer(transfer_id, "No request handler configured");
        }
        
    } catch (const std::exception& e) {
        LOG_FILE_TRANSFER_ERROR("Error handling transfer request: " << e.what());
    }
}

void FileTransferManager::handle_transfer_response(const std::string& peer_id, const nlohmann::json& message) {
    try {
        std::string transfer_id = message["transfer_id"];
        bool accepted = message["accepted"];
        
        auto progress = get_transfer_progress(transfer_id);
        if (!progress) {
            LOG_FILE_TRANSFER_ERROR("Received response for unknown transfer: " << transfer_id);
            return;
        }
        
        if (accepted) {
            // Start sending file
            {
                std::lock_guard<std::mutex> work_lock(work_mutex_);
                work_queue_.push(transfer_id);
            }
            work_condition_.notify_one();
            
            LOG_FILE_TRANSFER_INFO("Transfer accepted by peer: " << transfer_id);
        } else {
            std::string reason = message.value("reason", "No reason provided");
            complete_transfer(transfer_id, false, "Transfer rejected by peer: " + reason);
        }
        
    } catch (const std::exception& e) {
        LOG_FILE_TRANSFER_ERROR("Error handling transfer response: " << e.what());
    }
}

void FileTransferManager::handle_chunk_metadata_message(const std::string& peer_id, const nlohmann::json& message) {
    try {
        PendingChunk pending;
        pending.transfer_id = message["transfer_id"];
        pending.chunk_index = message["chunk_index"];
        pending.total_chunks = message["total_chunks"];
        pending.chunk_size = message["chunk_size"];
        pending.file_offset = message["file_offset"];
        pending.checksum = message.value("checksum", "");
        pending.created_at = std::chrono::steady_clock::now();
        
        // Store pending chunk metadata waiting for binary data
        {
            std::lock_guard<std::mutex> lock(chunks_mutex_);
            pending_chunks_[peer_id] = pending;
        }
        
        LOG_FILE_TRANSFER_DEBUG("Stored chunk metadata for transfer " << pending.transfer_id << 
                               ", chunk " << pending.chunk_index << " from peer " << peer_id);
        
    } catch (const std::exception& e) {
        LOG_FILE_TRANSFER_ERROR("Error handling chunk metadata message: " << e.what());
    }
}

void FileTransferManager::handle_chunk_binary_message(const std::string& peer_id, const std::vector<uint8_t>& binary_data) {
    try {
        // Parse binary chunk header and get the chunk data
        FileChunk chunk;
        if (!parse_chunk_binary_header(binary_data, chunk)) {
            LOG_FILE_TRANSFER_ERROR("Failed to parse chunk binary header from peer " << peer_id);
            return;
        }
        
        // Get pending chunk metadata
        PendingChunk pending;
        bool found_pending = false;
        {
            std::lock_guard<std::mutex> lock(chunks_mutex_);
            auto it = pending_chunks_.find(peer_id);
            if (it != pending_chunks_.end()) {
                pending = it->second;
                pending_chunks_.erase(it);
                found_pending = true;
            }
        }
        
        if (!found_pending) {
            LOG_FILE_TRANSFER_ERROR("No pending chunk metadata found for peer " << peer_id);
            return;
        }
        
        // Combine metadata with binary data
        chunk.transfer_id = pending.transfer_id;
        chunk.chunk_index = pending.chunk_index;
        chunk.total_chunks = pending.total_chunks;
        chunk.chunk_size = pending.chunk_size;
        chunk.file_offset = pending.file_offset;
        chunk.checksum = pending.checksum;
        
        // Verify chunk size matches
        if (chunk.data.size() != pending.chunk_size) {
            LOG_FILE_TRANSFER_ERROR("Chunk size mismatch: expected " << pending.chunk_size << 
                                   ", got " << chunk.data.size() << " for transfer " << pending.transfer_id);
            
            // Send negative acknowledgment
            nlohmann::json ack_msg = create_chunk_ack_message(chunk.transfer_id, chunk.chunk_index, false, "Size mismatch");
            client_.send(peer_id, "file_chunk_ack", ack_msg);
            return;
        }
        
        // Verify checksum if enabled
        if (config_.verify_checksums && !chunk.checksum.empty()) {
            if (!verify_chunk_checksum(chunk)) {
                LOG_FILE_TRANSFER_ERROR("Chunk checksum verification failed for transfer " << chunk.transfer_id << 
                                       ", chunk " << chunk.chunk_index);
                
                // Send negative acknowledgment
                nlohmann::json ack_msg = create_chunk_ack_message(chunk.transfer_id, chunk.chunk_index, false, "Checksum mismatch");
                client_.send(peer_id, "file_chunk_ack", ack_msg);
                return;
            }
        }
        
        // Process the received chunk
        handle_chunk_received(chunk);
        
        // Send positive acknowledgment
        nlohmann::json ack_msg = create_chunk_ack_message(chunk.transfer_id, chunk.chunk_index, true);
        client_.send(peer_id, "file_chunk_ack", ack_msg);
        
        LOG_FILE_TRANSFER_DEBUG("Successfully processed chunk " << chunk.chunk_index << 
                               " for transfer " << chunk.transfer_id << " from peer " << peer_id);
        
    } catch (const std::exception& e) {
        LOG_FILE_TRANSFER_ERROR("Error handling chunk binary message: " << e.what());
    }
}

void FileTransferManager::handle_chunk_ack_message(const std::string& peer_id, const nlohmann::json& message) {
    try {
        std::string transfer_id = message["transfer_id"];
        uint64_t chunk_index = message["chunk_index"];
        bool success = message["success"];
        
        handle_chunk_ack(transfer_id, chunk_index, success);
        
    } catch (const std::exception& e) {
        LOG_FILE_TRANSFER_ERROR("Error handling chunk ack message: " << e.what());
    }
}

void FileTransferManager::handle_transfer_control(const std::string& peer_id, const nlohmann::json& message) {
    try {
        std::string transfer_id = message["transfer_id"];
        std::string action = message["action"];
        
        if (action == "pause") {
            pause_transfer(transfer_id);
        } else if (action == "resume") {
            resume_transfer(transfer_id);
        } else if (action == "cancel") {
            cancel_transfer(transfer_id);
        }
        
    } catch (const std::exception& e) {
        LOG_FILE_TRANSFER_ERROR("Error handling transfer control message: " << e.what());
    }
}

// Message creation methods

nlohmann::json FileTransferManager::create_transfer_request_message(const FileMetadata& metadata, const std::string& transfer_id) {
    nlohmann::json message;
    message["transfer_id"] = transfer_id;
    message["type"] = "file";
    message["file_metadata"] = {
        {"filename", metadata.filename},
        {"file_size", metadata.file_size},
        {"mime_type", metadata.mime_type},
        {"checksum", metadata.checksum},
        {"last_modified", metadata.last_modified}
    };
    return message;
}

nlohmann::json FileTransferManager::create_transfer_response_message(const std::string& transfer_id, bool accepted, const std::string& reason) {
    nlohmann::json message;
    message["transfer_id"] = transfer_id;
    message["accepted"] = accepted;
    if (!reason.empty()) {
        message["reason"] = reason;
    }
    return message;
}

nlohmann::json FileTransferManager::create_chunk_metadata_message(const FileChunk& chunk) {
    nlohmann::json message;
    message["transfer_id"] = chunk.transfer_id;
    message["chunk_index"] = chunk.chunk_index;
    message["total_chunks"] = chunk.total_chunks;
    message["chunk_size"] = chunk.chunk_size;
    message["file_offset"] = chunk.file_offset;
    message["checksum"] = chunk.checksum;
    
    return message;
}

std::vector<uint8_t> FileTransferManager::create_chunk_binary_message(const FileChunk& chunk) {
    // Create binary message with header: "FTCHUNK" + chunk data
    const std::string magic = "FTCHUNK";
    std::vector<uint8_t> message;
    
    // Reserve space for the entire message
    message.reserve(magic.length() + chunk.data.size());
    
    // Add magic header
    message.insert(message.end(), magic.begin(), magic.end());
    
    // Add chunk data
    message.insert(message.end(), chunk.data.begin(), chunk.data.end());
    
    return message;
}

nlohmann::json FileTransferManager::create_chunk_ack_message(const std::string& transfer_id, uint64_t chunk_index, bool success, const std::string& error) {
    nlohmann::json message;
    message["transfer_id"] = transfer_id;
    message["chunk_index"] = chunk_index;
    message["success"] = success;
    if (!error.empty()) {
        message["error"] = error;
    }
    return message;
}

nlohmann::json FileTransferManager::create_control_message(const std::string& transfer_id, const std::string& action, const nlohmann::json& data) {
    nlohmann::json message;
    message["transfer_id"] = transfer_id;
    message["action"] = action;
    if (!data.empty()) {
        message["data"] = data;
    }
    return message;
}

// Progress tracking methods

void FileTransferManager::update_transfer_progress(const std::string& transfer_id, uint64_t bytes_delta) {
    auto progress = get_transfer_progress(transfer_id);
    if (!progress) {
        return;
    }
    
    if (bytes_delta > 0) {
        progress->update_transfer_rates(progress->bytes_transferred + bytes_delta);
        
        // Update statistics
        {
            std::lock_guard<std::mutex> stats_lock(stats_mutex_);
            if (progress->direction == FileTransferDirection::SENDING) {
                total_bytes_sent_ += bytes_delta;
            } else {
                total_bytes_received_ += bytes_delta;
            }
        }
    }
    
    // Call progress callback
    if (progress_callback_) {
        progress_callback_(*progress);
    }
}

void FileTransferManager::complete_transfer(const std::string& transfer_id, bool success, const std::string& error_message) {
    auto progress = get_transfer_progress(transfer_id);
    if (!progress) {
        return;
    }
    
    progress->status = success ? FileTransferStatus::COMPLETED : FileTransferStatus::FAILED;
    progress->error_message = error_message;
    
    // Update statistics
    {
        std::lock_guard<std::mutex> stats_lock(stats_mutex_);
        if (progress->direction == FileTransferDirection::SENDING) {
            total_files_sent_++;
        } else {
            total_files_received_++;
        }
    }
    
    // Move to completed transfers
    move_to_completed(transfer_id);
    
    // Call completion callback
    if (completion_callback_) {
        completion_callback_(transfer_id, success, error_message);
    }
    
    LOG_FILE_TRANSFER_INFO("Transfer " << (success ? "completed" : "failed") << ": " << transfer_id);
}

void FileTransferManager::move_to_completed(const std::string& transfer_id) {
    std::lock_guard<std::mutex> lock(transfers_mutex_);
    
    auto it = active_transfers_.find(transfer_id);
    if (it != active_transfers_.end()) {
        completed_transfers_[transfer_id] = it->second;
        active_transfers_.erase(it);
    }
}

// Binary chunk transmission implementation (replaces base64 encoding)
//
// PERFORMANCE OPTIMIZATIONS:
// - Replaced JSON + base64 encoding with direct binary transmission
// - Eliminates 33% encoding overhead from base64 
// - Reduces CPU usage for encoding/decoding
// - Minimizes memory allocations for string conversions
// - Uses two-phase protocol: metadata (JSON) + binary data (raw)
// - Added cleanup thread for timeout handling of split messages
//
// PROTOCOL CHANGES:
// 1. Send file_chunk_metadata (JSON) with transfer info
// 2. Send binary data with "FTCHUNK" magic header + raw file data
// 3. Receive acknowledgments as before
//
// This approach provides maximum performance while maintaining protocol robustness.

void FileTransferManager::handle_chunk_received(const FileChunk& chunk) {
    // Write chunk to temporary file
    std::string temp_path = get_temp_file_path(chunk.transfer_id, config_.temp_directory);
    
    if (!write_file_chunk(temp_path.c_str(), chunk.file_offset, chunk.data.data(), chunk.chunk_size)) {
        LOG_FILE_TRANSFER_ERROR("Failed to write chunk to temp file: " << temp_path);
        return;
    }
    
    // Update progress
    update_transfer_progress(chunk.transfer_id, chunk.chunk_size);
    
    // Check if transfer is complete
    auto progress = get_transfer_progress(chunk.transfer_id);
    if (progress) {
        progress->chunks_completed++;
        if (progress->chunks_completed == progress->total_chunks) {
            // Transfer complete - move temp file to final location
            if (finalize_received_file(chunk.transfer_id, progress->local_path)) {
                complete_transfer(chunk.transfer_id, true);
            } else {
                complete_transfer(chunk.transfer_id, false, "Failed to finalize received file");
            }
        }
    }
}

void FileTransferManager::handle_chunk_ack(const std::string& transfer_id, uint64_t chunk_index, bool success) {
    auto progress = get_transfer_progress(transfer_id);
    if (!progress) {
        return;
    }
    
    if (success) {
        progress->chunks_completed++;
        if (progress->chunks_completed == progress->total_chunks) {
            complete_transfer(transfer_id, true);
        }
    } else {
        // Handle chunk failure - could retry or fail the transfer
        LOG_FILE_TRANSFER_ERROR("Chunk " << chunk_index << " failed for transfer " << transfer_id);
    }
}

bool FileTransferManager::finalize_received_file(const std::string& transfer_id, const std::string& final_path) {
    std::string temp_path = get_temp_file_path(transfer_id, config_.temp_directory);
    
    try {
        // Ensure destination directory exists
        std::string dest_dir = get_parent_directory(final_path.c_str());
        if (!dest_dir.empty()) {
            ensure_directory_exists(dest_dir);
        }
        
        // Move temp file to final location
        if (!rename_file(temp_path.c_str(), final_path.c_str())) {
            LOG_FILE_TRANSFER_ERROR("Failed to rename temp file to final location: " << temp_path << " -> " << final_path);
            return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        LOG_FILE_TRANSFER_ERROR("Failed to finalize file " << final_path << ": " << e.what());
        return false;
    }
}

std::string FileTransferManager::calculate_chunk_checksum(const std::vector<uint8_t>& data) {
    SHA1 sha1;
    sha1.update(data.data(), data.size());
    return sha1.finalize();
}

bool FileTransferManager::parse_chunk_binary_header(const std::vector<uint8_t>& binary_data, FileChunk& chunk) {
    const std::string magic = "FTCHUNK";
    
    // Check minimum size (magic header + at least some data)
    if (binary_data.size() < magic.length()) {
        return false;
    }
    
    // Verify magic header
    if (std::memcmp(binary_data.data(), magic.c_str(), magic.length()) != 0) {
        return false;
    }
    
    // Extract chunk data (everything after the magic header)
    size_t data_start = magic.length();
    size_t data_size = binary_data.size() - data_start;
    
    chunk.data.resize(data_size);
    std::memcpy(chunk.data.data(), binary_data.data() + data_start, data_size);
    
    return true;
}

bool FileTransferManager::verify_chunk_checksum(const FileChunk& chunk) {
    if (!config_.verify_checksums || chunk.checksum.empty()) {
        return true;
    }
    
    std::string calculated = calculate_chunk_checksum(chunk.data);
    return calculated == chunk.checksum;
}

void FileTransferManager::handle_file_request(const std::string& peer_id, const nlohmann::json& message) {
    try {
        std::string transfer_id = message["transfer_id"];
        std::string remote_path = message["remote_path"];
        
        // Check if file exists and is accessible
        if (!file_or_directory_exists(remote_path)) {
            LOG_FILE_TRANSFER_WARN("File request denied - file not found: " << remote_path);
            nlohmann::json response;
            response["transfer_id"] = transfer_id;
            response["accepted"] = false;
            response["reason"] = "File not found or not accessible";
            client_.send(peer_id, "file_transfer_response", response);
            return;
        }
        
        // Call user callback to approve/reject
        if (file_request_callback_) {
            bool accepted = file_request_callback_(peer_id, remote_path, transfer_id);
            if (accepted) {
                // Start file transfer
                send_file(peer_id, remote_path);
                LOG_FILE_TRANSFER_INFO("Accepted file request: " << remote_path << " for " << peer_id);
            } else {
                nlohmann::json response;
                response["transfer_id"] = transfer_id;
                response["accepted"] = false;
                response["reason"] = "Request rejected by user";
                client_.send(peer_id, "file_transfer_response", response);
                LOG_FILE_TRANSFER_INFO("Rejected file request: " << remote_path << " for " << peer_id);
            }
        } else {
            // Auto-reject if no callback is set
            nlohmann::json response;
            response["transfer_id"] = transfer_id;
            response["accepted"] = false;
            response["reason"] = "No file request handler configured";
            client_.send(peer_id, "file_transfer_response", response);
            LOG_FILE_TRANSFER_WARN("Auto-rejected file request - no handler: " << remote_path);
        }
        
    } catch (const std::exception& e) {
        LOG_FILE_TRANSFER_ERROR("Error handling file request: " << e.what());
    }
}

void FileTransferManager::handle_directory_request(const std::string& peer_id, const nlohmann::json& message) {
    try {
        std::string transfer_id = message["transfer_id"];
        std::string remote_path = message["remote_path"];
        bool recursive = message.value("recursive", true);
        
        // Check if directory exists and is accessible
        if (!directory_exists(remote_path)) {
            LOG_FILE_TRANSFER_WARN("Directory request denied - directory not found: " << remote_path);
            nlohmann::json response;
            response["transfer_id"] = transfer_id;
            response["accepted"] = false;
            response["reason"] = "Directory not found or not accessible";
            client_.send(peer_id, "file_transfer_response", response);
            return;
        }
        
        // Call user callback to approve/reject
        if (directory_request_callback_) {
            bool accepted = directory_request_callback_(peer_id, remote_path, recursive, transfer_id);
            if (accepted) {
                // Start directory transfer
                send_directory(peer_id, remote_path, "", recursive);
                LOG_FILE_TRANSFER_INFO("Accepted directory request: " << remote_path << " for " << peer_id);
            } else {
                nlohmann::json response;
                response["transfer_id"] = transfer_id;
                response["accepted"] = false;
                response["reason"] = "Request rejected by user";
                client_.send(peer_id, "file_transfer_response", response);
                LOG_FILE_TRANSFER_INFO("Rejected directory request: " << remote_path << " for " << peer_id);
            }
        } else {
            // Auto-reject if no callback is set
            nlohmann::json response;
            response["transfer_id"] = transfer_id;
            response["accepted"] = false;
            response["reason"] = "No directory request handler configured";
            client_.send(peer_id, "file_transfer_response", response);
            LOG_FILE_TRANSFER_WARN("Auto-rejected directory request - no handler: " << remote_path);
        }
        
    } catch (const std::exception& e) {
        LOG_FILE_TRANSFER_ERROR("Error handling directory request: " << e.what());
    }
}

} // namespace librats
