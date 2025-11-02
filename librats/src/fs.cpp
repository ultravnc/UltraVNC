#include "fs.h"
#include "logger.h"
#include <cstring>
#include <cstdlib>
#include <sys/stat.h>

#ifdef _WIN32
    #include <windows.h>
    #include <direct.h>
    #include <io.h>
    #define stat _stat
    #define mkdir(path, mode) _mkdir(path)
    #define access _access
    #define F_OK 0
    #define getcwd _getcwd
    #define chdir _chdir
#else
    #include <unistd.h>
    #include <dirent.h>
    #include <errno.h>
    #include <libgen.h>
#endif

namespace librats {

bool file_or_directory_exists(const char* path) {
    if (!path) return false;
    return access(path, F_OK) == 0;
}

bool file_exists(const char* path) {
    if (!path) return false;

    struct stat st;
    if (stat(path, &st) == 0) {
        return (st.st_mode & S_IFREG) != 0;
    }
    return false;
}

bool directory_exists(const char* path) {
    if (!path) return false;
    
    struct stat st;
    if (stat(path, &st) == 0) {
        return (st.st_mode & S_IFDIR) != 0;
    }
    return false;
}

bool create_file(const char* path, const char* content) {
    if (!path) return false;
    
    FILE* file = fopen(path, "wb");
    if (!file) {
        LOG_ERROR("FS", "Failed to create file: " << path);
        return false;
    }
    
    if (content) {
        size_t len = strlen(content);
        size_t written = fwrite(content, 1, len, file);
        fclose(file);
        
        if (written != len) {
            LOG_ERROR("FS", "Failed to write complete content to file: " << path);
            return false;
        }
    } else {
        fclose(file);
    }
    
    return true;
}

bool create_file_binary(const char* path, const void* data, size_t size) {
    if (!path) return false;
    
    FILE* file = fopen(path, "wb");
    if (!file) {
        LOG_ERROR("FS", "Failed to create binary file: " << path);
        return false;
    }
    
    if (data && size > 0) {
        size_t written = fwrite(data, 1, size, file);
        fclose(file);
        
        if (written != size) {
            LOG_ERROR("FS", "Failed to write complete binary data to file: " << path);
            return false;
        }
    } else {
        fclose(file);
    }
    
    return true;
}

bool append_to_file(const char* path, const char* content) {
    if (!path || !content) return false;
    
    FILE* file = fopen(path, "ab");
    if (!file) {
        LOG_ERROR("FS", "Failed to open file for appending: " << path);
        return false;
    }
    
    size_t len = strlen(content);
    size_t written = fwrite(content, 1, len, file);
    fclose(file);
    
    if (written != len) {
        LOG_ERROR("FS", "Failed to append complete content to file: " << path);
        return false;
    }
    
    return true;
}

char* read_file_text(const char* path, size_t* size_out) {
    if (!path) return nullptr;

    struct stat st;
    if (stat(path, &st) != 0) {
        LOG_ERROR("FS", "Failed to stat file: " << path);
        return nullptr;
    }

    size_t file_size = st.st_size;
    
    FILE* file = fopen(path, "rb");
    if (!file) {
        LOG_ERROR("FS", "Failed to open file for reading: " << path);
        return nullptr;
    }
    
    // Allocate buffer (+1 for null terminator)
    char* buffer = static_cast<char*>(malloc(file_size + 1));
    if (!buffer) {
        LOG_ERROR("FS", "Failed to allocate memory for file: " << path);
        fclose(file);
        return nullptr;
    }
    
    // Read file
    size_t bytes_read = fread(buffer, 1, file_size, file);
    fclose(file);
    
    // Null terminate
    buffer[bytes_read] = '\0';
    
    if (size_out) {
        *size_out = bytes_read;
    }
    
    return buffer;
}

void* read_file_binary(const char* path, size_t* size_out) {
    if (!path || !size_out) return nullptr;
    
    FILE* file = fopen(path, "rb");
    if (!file) {
        LOG_ERROR("FS", "Failed to open binary file for reading: " << path);
        return nullptr;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size < 0) {
        LOG_ERROR("FS", "Failed to get binary file size: " << path);
        fclose(file);
        return nullptr;
    }
    
    // Allocate buffer
    void* buffer = malloc(file_size);
    if (!buffer) {
        LOG_ERROR("FS", "Failed to allocate memory for binary file: " << path);
        fclose(file);
        return nullptr;
    }
    
    // Read file
    size_t bytes_read = fread(buffer, 1, file_size, file);
    fclose(file);
    
    *size_out = bytes_read;
    return buffer;
}

bool create_directory(const char* path) {
    if (!path) return false;
    
    if (directory_exists(path)) {
        return true; // Already exists
    }
    
#ifdef _WIN32
    return _mkdir(path) == 0;
#else
    return mkdir(path, 0755) == 0;
#endif
}

bool create_directories(const char* path) {
    if (!path) return false;
    
    if (directory_exists(path)) {
        return true; // Already exists
    }
    
    // Create a copy of the path to modify
    size_t len = strlen(path);
    char* path_copy = static_cast<char*>(malloc(len + 1));
    if (!path_copy) {
        LOG_ERROR("FS", "Failed to allocate memory for path copy in create_directories");
        return false;
    }
    
    memcpy(path_copy, path, len + 1);
    
    // Create parent directories recursively
    for (size_t i = 1; i < len; i++) {
        if (path_copy[i] == '/' || path_copy[i] == '\\') {
            path_copy[i] = '\0';
            
            if (!directory_exists(path_copy)) {
                if (!create_directory(path_copy)) {
                    free(path_copy);
                    return false;
                }
            }
            
            path_copy[i] = '/'; // Normalize to forward slash
        }
    }
    
    // Create the final directory
    bool result = create_directory(path_copy);
    free(path_copy);
    return result;
}

int64_t get_file_size(const char* path) {
    if (!path) return -1;
    
    struct stat st;
    if (stat(path, &st) == 0) {
        return st.st_size;
    }
    return -1;
}

bool is_file(const char* path) {
    return file_exists(path);
}

bool is_directory(const char* path) {
    return directory_exists(path);
}

bool delete_file(const char* path) {
    if (!path) return false;
    
    return remove(path) == 0;
}

bool delete_directory(const char* path) {
    if (!path) return false;
    
#ifdef _WIN32
    return RemoveDirectoryA(path) != 0;
#else
    return rmdir(path) == 0;
#endif
}

bool copy_file(const char* src_path, const char* dest_path) {
    if (!src_path || !dest_path) return false;

    FILE* src_file = fopen(src_path, "rb");
    if (!src_file) {
        LOG_ERROR("FS", "Failed to open source file for copying: " << src_path);
        return false;
    }

    FILE* dest_file = fopen(dest_path, "wb");
    if (!dest_file) {
        LOG_ERROR("FS", "Failed to open destination file for copying: " << dest_path);
        fclose(src_file);
        return false;
    }

    char buffer[4096];
    size_t bytes_read;
    bool success = true;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), src_file)) > 0) {
        if (fwrite(buffer, 1, bytes_read, dest_file) != bytes_read) {
            LOG_ERROR("FS", "Failed to write to destination file: " << dest_path);
            success = false;
            break;
        }
    }

    if (ferror(src_file)) {
        LOG_ERROR("FS", "Error reading from source file: " << src_path);
        success = false;
    }

    fclose(src_file);
    fclose(dest_file);

    if (!success) {
        delete_file(dest_path);
    }

    return success;
}

bool move_file(const char* src_path, const char* dest_path) {
    if (!src_path || !dest_path) return false;
    
    if (rename(src_path, dest_path) == 0) {
        return true;
    }
    
    // If rename fails, try copy and delete
    if (copy_file(src_path, dest_path)) {
        return delete_file(src_path);
    }
    
    return false;
}

void free_file_buffer(void* buffer) {
    if (buffer) {
        free(buffer);
    }
}

bool get_current_directory(char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) return false;
    
    return getcwd(buffer, buffer_size) != nullptr;
}

bool set_current_directory(const char* path) {
    if (!path) return false;
    
    return chdir(path) == 0;
}

// File metadata operations
uint64_t get_file_modified_time(const char* path) {
    if (!path) return 0;
    
    struct stat st;
    if (stat(path, &st) == 0) {
        return static_cast<uint64_t>(st.st_mtime);
    }
    return 0;
}

std::string get_file_extension(const char* path) {
    if (!path) return "";
    
    const char* dot = strrchr(path, '.');
    if (dot && dot != path) {
        return std::string(dot);
    }
    return "";
}

std::string get_filename_from_path(const char* path) {
    if (!path) return "";
    
    const char* filename = strrchr(path, '/');
    if (!filename) {
        filename = strrchr(path, '\\');
    }
    
    if (filename) {
        return std::string(filename + 1);
    } else {
        return std::string(path);
    }
}

std::string get_parent_directory(const char* path) {
    if (!path) return "";
    
    std::string str_path(path);
    size_t pos = str_path.find_last_of("/\\");
    if (pos != std::string::npos) {
        return str_path.substr(0, pos);
    }
    return "";
}

// File chunk operations
bool write_file_chunk(const char* path, uint64_t offset, const void* data, size_t size) {
    if (!path || !data) return false;
    
    FILE* file = fopen(path, "r+b");
    if (!file) {
        // Try to create the file if it doesn't exist
        file = fopen(path, "w+b");
        if (!file) {
            LOG_ERROR("FS", "Failed to open file for chunk writing: " << path);
            return false;
        }
    }
    
    if (fseek(file, offset, SEEK_SET) != 0) {
        LOG_ERROR("FS", "Failed to seek to offset " << offset << " in file: " << path);
        fclose(file);
        return false;
    }
    
    size_t written = fwrite(data, 1, size, file);
    fclose(file);
    
    if (written != size) {
        LOG_ERROR("FS", "Failed to write complete chunk to file: " << path);
        return false;
    }
    
    return true;
}

bool read_file_chunk(const char* path, uint64_t offset, void* buffer, size_t size) {
    if (!path || !buffer) return false;
    
    FILE* file = fopen(path, "rb");
    if (!file) {
        LOG_ERROR("FS", "Failed to open file for chunk reading: " << path);
        return false;
    }
    
    if (fseek(file, offset, SEEK_SET) != 0) {
        LOG_ERROR("FS", "Failed to seek to offset " << offset << " in file: " << path);
        fclose(file);
        return false;
    }
    
    size_t bytes_read = fread(buffer, 1, size, file);
    fclose(file);
    
    if (bytes_read != size) {
        LOG_ERROR("FS", "Failed to read complete chunk from file: " << path);
        return false;
    }
    
    return true;
}

// Advanced file operations
bool create_file_with_size(const char* path, uint64_t size) {
    if (!path) return false;
    
    FILE* file = fopen(path, "wb");
    if (!file) {
        LOG_ERROR("FS", "Failed to create file with size: " << path);
        return false;
    }
    
    if (size > 0) {
        // Pre-allocate file space by seeking to size-1 and writing a byte
        if (fseek(file, size - 1, SEEK_SET) == 0) {
            fputc(0, file);
        }
    }
    
    fclose(file);
    return true;
}

bool rename_file(const char* old_path, const char* new_path) {
    if (!old_path || !new_path) return false;
    
    return rename(old_path, new_path) == 0;
}

// Path utilities
std::string combine_paths(const std::string& base, const std::string& relative) {
    if (base.empty()) return relative;
    if (relative.empty()) return base;
    
    std::string result = base;
    
    // Normalize all backslashes to forward slashes in result
    for (char& c : result) {
        if (c == '\\') c = '/';
    }
    
    // Ensure base path ends with separator
    if (result.back() != '/') {
        result += '/';
    }
    
    // Remove leading separator from relative path and normalize
    std::string rel = relative;
    if (!rel.empty() && (rel.front() == '/' || rel.front() == '\\')) {
        rel = rel.substr(1);
    }
    
    // Normalize backslashes to forward slashes in relative path
    for (char& c : rel) {
        if (c == '\\') c = '/';
    }
    
    return result + rel;
}

bool validate_path(const char* path, bool check_write_access) {
    if (!path) return false;
    
    if (check_write_access) {
        // Check if we can write to the parent directory
        std::string parent = get_parent_directory(path);
        if (!parent.empty() && !directory_exists(parent.c_str())) {
            return false;
        }
        // Additional write permission checks could be added here
    } else {
        // Check if path exists and is a regular file (not a directory)
        if (!is_file(path)) {
            return false;
        }
    }
    
    return true;
}

// Directory listing
bool list_directory(const char* path, std::vector<DirectoryEntry>& entries) {
    if (!path || !directory_exists(path)) return false;
    
    entries.clear();
    
#ifdef _WIN32
    WIN32_FIND_DATAA find_data;
    std::string search_path = std::string(path) + "\\*";
    HANDLE find_handle = FindFirstFileA(search_path.c_str(), &find_data);
    
    if (find_handle == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    do {
        // Skip "." and ".." entries
        if (strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0) {
            continue;
        }
        
        DirectoryEntry entry;
        entry.name = find_data.cFileName;
        entry.path = combine_paths(path, entry.name);
        entry.is_directory = (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        
        if (!entry.is_directory) {
            entry.size = (static_cast<uint64_t>(find_data.nFileSizeHigh) << 32) | find_data.nFileSizeLow;
        } else {
            entry.size = 0;
        }
        
        // Convert Windows FILETIME to Unix timestamp
        FILETIME ft = find_data.ftLastWriteTime;
        LARGE_INTEGER li;
        li.LowPart = ft.dwLowDateTime;
        li.HighPart = ft.dwHighDateTime;
        // Convert from 100ns intervals since 1601 to seconds since 1970
        entry.modified_time = (li.QuadPart - 116444736000000000LL) / 10000000LL;
        
        entries.push_back(entry);
        
    } while (FindNextFileA(find_handle, &find_data));
    
    FindClose(find_handle);
    
#else
    DIR* dir = opendir(path);
    if (!dir) {
        return false;
    }
    
    struct dirent* entry_ptr;
    while ((entry_ptr = readdir(dir)) != nullptr) {
        // Skip "." and ".." entries
        if (strcmp(entry_ptr->d_name, ".") == 0 || strcmp(entry_ptr->d_name, "..") == 0) {
            continue;
        }
        
        DirectoryEntry entry;
        entry.name = entry_ptr->d_name;
        entry.path = combine_paths(path, entry.name);
        
        struct stat st;
        if (stat(entry.path.c_str(), &st) == 0) {
            entry.is_directory = S_ISDIR(st.st_mode);
            entry.size = entry.is_directory ? 0 : static_cast<uint64_t>(st.st_size);
            entry.modified_time = static_cast<uint64_t>(st.st_mtime);
        } else {
            entry.is_directory = false;
            entry.size = 0;
            entry.modified_time = 0;
        }
        
        entries.push_back(entry);
    }
    
    closedir(dir);
#endif
    
    return true;
}

} // namespace librats 