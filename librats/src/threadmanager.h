#ifndef THREADMANAGER_H
#define THREADMANAGER_H

#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include "logger.h"

namespace librats {

/**
 * ThreadManager - Manages background threads with graceful shutdown
 */
class ThreadManager {
public:
    ThreadManager();
    virtual ~ThreadManager();

    // Add a new managed thread with optional name for debugging
    void add_managed_thread(std::thread&& t, const std::string& name = "unnamed");

    // Clean up finished threads (non-blocking)
    void cleanup_finished_threads();

    // Signal shutdown to all threads
    void shutdown_all_threads();

    // Wait for all active threads to finish (blocking)
    void join_all_active_threads();

    // Get count of active threads
    size_t get_active_thread_count() const;

protected:
    // Notify waiting threads of shutdown
    void notify_shutdown();

    // Condition variable for shutdown coordination
    std::condition_variable shutdown_cv_;
    std::mutex shutdown_mutex_;

private:
    mutable std::mutex active_threads_mutex_;
    std::vector<std::thread> active_threads_;
    std::atomic<bool> shutdown_requested_{false};
};

} // namespace librats

#endif // THREADMANAGER_H

