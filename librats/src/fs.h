#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <cstdio>
#include "rats_export.h"

namespace librats {

// File/Directory existence check
bool file_or_directory_exists(const char* path);
RATS_API bool directory_exists(const char* path);
bool file_exists(const char* path);

// File creation and writing
bool create_file(const char* path, const char* content);
bool create_file_binary(const char* path, const void* data, size_t size);
bool append_to_file(const char* path, const char* content);

// File reading
char* read_file_text(const char* path, size_t* size_out = nullptr);
void* read_file_binary(const char* path, size_t* size_out);

// Directory operations
bool create_directory(const char* path);
RATS_API bool create_directories(const char* path); // Create parent directories if needed

// File information
int64_t get_file_size(const char* path);
bool is_file(const char* path);
bool is_directory(const char* path);

// File operations
bool delete_file(const char* path);
bool delete_directory(const char* path);
bool copy_file(const char* src_path, const char* dest_path);
bool move_file(const char* src_path, const char* dest_path);

// File metadata operations
uint64_t get_file_modified_time(const char* path);
std::string get_file_extension(const char* path);
std::string get_filename_from_path(const char* path);
std::string get_parent_directory(const char* path);

// File chunk operations
bool write_file_chunk(const char* path, uint64_t offset, const void* data, size_t size);
bool read_file_chunk(const char* path, uint64_t offset, void* buffer, size_t size);

// Advanced file operations
bool create_file_with_size(const char* path, uint64_t size); // Pre-allocate file space
bool rename_file(const char* old_path, const char* new_path);

// Directory listing
struct DirectoryEntry {
    std::string name;
    std::string path;
    bool is_directory;
    uint64_t size;
    uint64_t modified_time;
};
bool list_directory(const char* path, std::vector<DirectoryEntry>& entries);

// Path utilities
std::string combine_paths(const std::string& base, const std::string& relative);
bool validate_path(const char* path, bool check_write_access = false);

// Utility functions
void free_file_buffer(void* buffer); // Free memory allocated by read functions
bool get_current_directory(char* buffer, size_t buffer_size);
bool set_current_directory(const char* path);

// C++ convenience wrappers
inline bool file_or_directory_exists(const std::string& path) { return file_or_directory_exists(path.c_str()); }
inline bool file_exists(const std::string& path) { return file_exists(path.c_str()); }
inline bool directory_exists(const std::string& path) { return directory_exists(path.c_str()); }
inline bool create_file(const std::string& path, const std::string& content) { 
    return create_file(path.c_str(), content.c_str()); 
}
inline std::string read_file_text_cpp(const std::string& path) {
    size_t size;
    char* content = read_file_text(path.c_str(), &size);
    if (!content) return "";
    std::string result(content, size);
    free_file_buffer(content);
    return result;
}

// Additional C++ wrappers for new functions
inline uint64_t get_file_modified_time(const std::string& path) { return get_file_modified_time(path.c_str()); }
inline std::string get_file_extension(const std::string& path) { return get_file_extension(path.c_str()); }
inline std::string get_filename_from_path(const std::string& path) { return get_filename_from_path(path.c_str()); }
inline std::string get_parent_directory(const std::string& path) { return get_parent_directory(path.c_str()); }
inline bool write_file_chunk(const std::string& path, uint64_t offset, const void* data, size_t size) { 
    return write_file_chunk(path.c_str(), offset, data, size); 
}
inline bool read_file_chunk(const std::string& path, uint64_t offset, void* buffer, size_t size) { 
    return read_file_chunk(path.c_str(), offset, buffer, size); 
}
inline bool create_file_with_size(const std::string& path, uint64_t size) { return create_file_with_size(path.c_str(), size); }
inline bool rename_file(const std::string& old_path, const std::string& new_path) { 
    return rename_file(old_path.c_str(), new_path.c_str()); 
}
inline bool validate_path(const std::string& path, bool check_write_access = false) { 
    return validate_path(path.c_str(), check_write_access); 
}

} // namespace librats 