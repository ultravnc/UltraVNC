#include "threadmanager.h"

// ThreadManager module logging macros
#define LOG_THREAD_DEBUG(message) LOG_DEBUG("thread", message)
#define LOG_THREAD_INFO(message)  LOG_INFO("thread", message)
#define LOG_THREAD_WARN(message)  LOG_WARN("thread", message)
#define LOG_THREAD_ERROR(message) LOG_ERROR("thread", message)

namespace librats {

ThreadManager::ThreadManager() {
    LOG_THREAD_DEBUG("ThreadManager initialized");
}

ThreadManager::~ThreadManager() {
    // Ensure all threads are properly cleaned up
    join_all_active_threads();
    LOG_THREAD_DEBUG("ThreadManager destroyed");
}

void ThreadManager::add_managed_thread(std::thread&& t, const std::string& name) {
    // Check if shutdown has been requested - don't add new threads during shutdown
    if (shutdown_requested_.load()) {
        LOG_THREAD_WARN("Ignoring thread detach during shutdown: " << name);
        // Join the thread so it can finish on its own during shutdown
        if (t.joinable()) {
            t.join();
        }
        return;
    }
    
    std::lock_guard<std::mutex> lock(active_threads_mutex_);
    
    // Double-check after acquiring lock
    if (shutdown_requested_.load()) {
        LOG_THREAD_WARN("Ignoring thread detach during shutdown (double-check): " << name);
        // Join the thread so it can finish on its own during shutdown
        if (t.joinable()) {
            t.join();
        }
        return;
    }
    
    active_threads_.emplace_back(std::move(t));
    LOG_THREAD_DEBUG("Added managed thread: " << name << " (total: " << active_threads_.size() << ")");
}

void ThreadManager::cleanup_finished_threads() {
    // we wait until all threads are finished thus mean cleanup is done
    join_all_active_threads();
}

void ThreadManager::shutdown_all_threads() {
    LOG_THREAD_INFO("Initiating shutdown of all background threads");
    
    // Set shutdown flag first
    shutdown_requested_.store(true);
    
    // Notify all waiting threads to wake up immediately
    notify_shutdown();
}

void ThreadManager::join_all_active_threads() {
    std::vector<std::thread> threads_to_join;
    
    // Move threads out of the container while holding the lock
    {
        std::lock_guard<std::mutex> lock(active_threads_mutex_);
        LOG_THREAD_INFO("Waiting for " << active_threads_.size() << " managed threads to finish");
        
        if (active_threads_.empty()) {
            LOG_THREAD_INFO("No active threads to join");
            return;
        }
        
        // Move all threads to local vector to avoid holding lock during join
        threads_to_join = std::move(active_threads_);
        active_threads_.clear();
    }
    
    // Join threads without holding the mutex
    for (auto& t : threads_to_join) {
        if (t.joinable()) {
            try {
                t.join();
            } catch (const std::exception& e) {
                LOG_THREAD_ERROR("Exception while joining thread: " << e.what());
            }
        }
    }
    
    LOG_THREAD_INFO("All managed threads have been cleaned up");
}

size_t ThreadManager::get_active_thread_count() const {
    std::lock_guard<std::mutex> lock(active_threads_mutex_);
    return active_threads_.size();
}

void ThreadManager::notify_shutdown() {
    shutdown_cv_.notify_all();
}

} // namespace librats

