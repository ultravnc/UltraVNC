#include "librats.h"
// Logging macros for RatsClient
#define LOG_CLIENT_DEBUG(message) LOG_DEBUG("client", message)
#define LOG_CLIENT_INFO(message)  LOG_INFO("client", message)
#define LOG_CLIENT_WARN(message)  LOG_WARN("client", message)
#define LOG_CLIENT_ERROR(message) LOG_ERROR("client", message)

namespace librats {

//=============================================================================
// File Transfer API Implementation
//=============================================================================

FileTransferManager& RatsClient::get_file_transfer_manager() {
    if (!file_transfer_manager_) {
        throw std::runtime_error("File transfer manager not initialized");
    }
    return *file_transfer_manager_;
}

bool RatsClient::is_file_transfer_available() const {
    return file_transfer_manager_ != nullptr;
}

std::string RatsClient::send_file(const std::string& peer_id, const std::string& file_path, 
                                 const std::string& remote_filename) {
    if (!is_file_transfer_available()) {
        LOG_CLIENT_ERROR("File transfer manager not available");
        return "";
    }
    
    return file_transfer_manager_->send_file(peer_id, file_path, remote_filename);
}

std::string RatsClient::send_directory(const std::string& peer_id, const std::string& directory_path,
                                      const std::string& remote_directory_name, bool recursive) {
    if (!is_file_transfer_available()) {
        LOG_CLIENT_ERROR("File transfer manager not available");
        return "";
    }
    
    return file_transfer_manager_->send_directory(peer_id, directory_path, remote_directory_name, recursive);
}

bool RatsClient::accept_file_transfer(const std::string& transfer_id, const std::string& local_path) {
    if (!is_file_transfer_available()) {
        LOG_CLIENT_ERROR("File transfer manager not available");
        return false;
    }
    
    return file_transfer_manager_->accept_file_transfer(transfer_id, local_path);
}

bool RatsClient::reject_file_transfer(const std::string& transfer_id, const std::string& reason) {
    if (!is_file_transfer_available()) {
        LOG_CLIENT_ERROR("File transfer manager not available");
        return false;
    }
    
    return file_transfer_manager_->reject_file_transfer(transfer_id, reason);
}

bool RatsClient::accept_directory_transfer(const std::string& transfer_id, const std::string& local_path) {
    if (!is_file_transfer_available()) {
        LOG_CLIENT_ERROR("File transfer manager not available");
        return false;
    }
    
    return file_transfer_manager_->accept_directory_transfer(transfer_id, local_path);
}

bool RatsClient::reject_directory_transfer(const std::string& transfer_id, const std::string& reason) {
    if (!is_file_transfer_available()) {
        LOG_CLIENT_ERROR("File transfer manager not available");
        return false;
    }
    
    return file_transfer_manager_->reject_directory_transfer(transfer_id, reason);
}

bool RatsClient::pause_file_transfer(const std::string& transfer_id) {
    if (!is_file_transfer_available()) {
        LOG_CLIENT_ERROR("File transfer manager not available");
        return false;
    }
    
    return file_transfer_manager_->pause_transfer(transfer_id);
}

bool RatsClient::resume_file_transfer(const std::string& transfer_id) {
    if (!is_file_transfer_available()) {
        LOG_CLIENT_ERROR("File transfer manager not available");
        return false;
    }
    
    return file_transfer_manager_->resume_transfer(transfer_id);
}

bool RatsClient::cancel_file_transfer(const std::string& transfer_id) {
    if (!is_file_transfer_available()) {
        LOG_CLIENT_ERROR("File transfer manager not available");
        return false;
    }
    
    return file_transfer_manager_->cancel_transfer(transfer_id);
}

std::shared_ptr<FileTransferProgress> RatsClient::get_file_transfer_progress(const std::string& transfer_id) const {
    if (!is_file_transfer_available()) {
        LOG_CLIENT_ERROR("File transfer manager not available");
        return nullptr;
    }
    
    return file_transfer_manager_->get_transfer_progress(transfer_id);
}

std::vector<std::shared_ptr<FileTransferProgress>> RatsClient::get_active_file_transfers() const {
    if (!is_file_transfer_available()) {
        LOG_CLIENT_ERROR("File transfer manager not available");
        return {};
    }
    
    return file_transfer_manager_->get_active_transfers();
}

nlohmann::json RatsClient::get_file_transfer_statistics() const {
    if (!is_file_transfer_available()) {
        LOG_CLIENT_ERROR("File transfer manager not available");
        return nlohmann::json::object();
    }
    
    return file_transfer_manager_->get_transfer_statistics();
}

void RatsClient::set_file_transfer_config(const FileTransferConfig& config) {
    if (!is_file_transfer_available()) {
        LOG_CLIENT_ERROR("File transfer manager not available");
        return;
    }
    
    file_transfer_manager_->set_config(config);
}

const FileTransferConfig& RatsClient::get_file_transfer_config() const {
    if (!is_file_transfer_available()) {
        throw std::runtime_error("File transfer manager not available");
    }
    
    return file_transfer_manager_->get_config();
}

void RatsClient::on_file_transfer_progress(FileTransferProgressCallback callback) {
    if (!is_file_transfer_available()) {
        LOG_CLIENT_ERROR("File transfer manager not available");
        return;
    }
    
    file_transfer_manager_->set_progress_callback(callback);
}

void RatsClient::on_file_transfer_completed(FileTransferCompletedCallback callback) {
    if (!is_file_transfer_available()) {
        LOG_CLIENT_ERROR("File transfer manager not available");
        return;
    }
    
    file_transfer_manager_->set_completion_callback(callback);
}

void RatsClient::on_file_transfer_request(FileTransferRequestCallback callback) {
    if (!is_file_transfer_available()) {
        LOG_CLIENT_ERROR("File transfer manager not available");
        return;
    }
    
    file_transfer_manager_->set_request_callback(callback);
}

void RatsClient::on_directory_transfer_progress(DirectoryTransferProgressCallback callback) {
    if (!is_file_transfer_available()) {
        LOG_CLIENT_ERROR("File transfer manager not available");
        return;
    }
    
    file_transfer_manager_->set_directory_progress_callback(callback);
}

std::string RatsClient::request_file(const std::string& peer_id, const std::string& remote_file_path,
                                    const std::string& local_path) {
    if (!is_file_transfer_available()) {
        LOG_CLIENT_ERROR("File transfer manager not available");
        return "";
    }
    
    return file_transfer_manager_->request_file(peer_id, remote_file_path, local_path);
}

std::string RatsClient::request_directory(const std::string& peer_id, const std::string& remote_directory_path,
                                         const std::string& local_directory_path, bool recursive) {
    if (!is_file_transfer_available()) {
        LOG_CLIENT_ERROR("File transfer manager not available");
        return "";
    }
    
    return file_transfer_manager_->request_directory(peer_id, remote_directory_path, local_directory_path, recursive);
}

void RatsClient::on_file_request(FileRequestCallback callback) {
    if (!is_file_transfer_available()) {
        LOG_CLIENT_ERROR("File transfer manager not available");
        return;
    }
    
    file_transfer_manager_->set_file_request_callback(callback);
}

void RatsClient::on_directory_request(DirectoryRequestCallback callback) {
    if (!is_file_transfer_available()) {
        LOG_CLIENT_ERROR("File transfer manager not available");
        return;
    }
    
    file_transfer_manager_->set_directory_request_callback(callback);
}

}