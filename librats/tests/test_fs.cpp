#include <gtest/gtest.h>
#include "fs.h"
#include <iostream>
#include <string>

using namespace librats;

class FSTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Cleanup any leftover test files
        delete_file("test_file.txt");
        delete_file("test_binary.bin");
        delete_file("test_cpp_wrapper.txt");
        delete_file("test_move_src.txt");
        delete_file("test_move_dest.txt");
        delete_file("test_old_name.txt");
        delete_file("test_new_name.txt");
        delete_file("test_metadata.txt");
        delete_file("test_chunks.bin");
        delete_file("test_sized.bin");
        delete_file("validation_test.txt");
        delete_file("test_copy.txt");
        delete_file("test_file_exists.txt");
        delete_file("test_null_content.txt");
        delete_file("test_null_binary.bin");
        delete_file("test_chunk_edge_cases.bin");
        delete_file("validate_path_test.txt");
        delete_file("buffer_test.txt");
        delete_file("large_file_test.bin");
        delete_file("zero_size.bin");
        delete_file("test_listing/file1.txt");
        delete_file("test_listing/file2.bin");
        delete_directory("test_listing/subdir");
        delete_directory("test_listing");
        delete_directory("test_change_directory");
        delete_directory("test_dir_exists");
        delete_directory("validate_dir");
        delete_directory("empty_test_dir");
        delete_directory("test_directory/nested/deep");
        delete_directory("test_directory/nested");
        delete_directory("test_directory");
    }
    
    void TearDown() override {
        // Cleanup test files after each test
        delete_file("test_file.txt");
        delete_file("test_binary.bin");
        delete_file("test_cpp_wrapper.txt");
        delete_file("test_move_src.txt");
        delete_file("test_move_dest.txt");
        delete_file("test_old_name.txt");
        delete_file("test_new_name.txt");
        delete_file("test_metadata.txt");
        delete_file("test_chunks.bin");
        delete_file("test_sized.bin");
        delete_file("validation_test.txt");
        delete_file("test_copy.txt");
        delete_file("test_file_exists.txt");
        delete_file("test_null_content.txt");
        delete_file("test_null_binary.bin");
        delete_file("test_chunk_edge_cases.bin");
        delete_file("validate_path_test.txt");
        delete_file("buffer_test.txt");
        delete_file("large_file_test.bin");
        delete_file("zero_size.bin");
        delete_file("test_listing/file1.txt");
        delete_file("test_listing/file2.bin");
        delete_directory("test_listing/subdir");
        delete_directory("test_listing");
        delete_directory("test_change_directory");
        delete_directory("test_dir_exists");
        delete_directory("validate_dir");
        delete_directory("empty_test_dir");
        delete_directory("test_directory/nested/deep");
        delete_directory("test_directory/nested");
        delete_directory("test_directory");
    }
};

TEST_F(FSTest, BasicFileOperations) {
    const char* test_file = "test_file.txt";
    const char* test_content = "Hello, World!\nThis is a test file.";
    
    // Test file creation
    bool created = create_file(test_file, test_content);
    EXPECT_TRUE(created) << "Failed to create test file";
    std::cout << "✓ File created successfully" << std::endl;
    
    // Test file existence
    bool exists = file_or_directory_exists(test_file);
    EXPECT_TRUE(exists) << "File should exist after creation";
    std::cout << "✓ File existence check passed" << std::endl;
    
    // Test file reading
    size_t size;
    char* read_content = read_file_text(test_file, &size);
    EXPECT_NE(read_content, nullptr) << "Failed to read file";
    EXPECT_STREQ(read_content, test_content) << "File content mismatch";
    std::cout << "✓ File reading passed" << std::endl;
    std::cout << "  Content: " << read_content << std::endl;
    free_file_buffer(read_content);
    
    // Test file size
    int64_t file_size = get_file_size(test_file);
    EXPECT_EQ(file_size, (int64_t)strlen(test_content)) << "File size mismatch";
    std::cout << "✓ File size check passed (" << file_size << " bytes)" << std::endl;
    
    // Test file type check
    EXPECT_TRUE(is_file(test_file)) << "Should be identified as file";
    EXPECT_FALSE(is_directory(test_file)) << "Should not be identified as directory";
    std::cout << "✓ File type check passed" << std::endl;
    
    // Clean up
    bool deleted = delete_file(test_file);
    EXPECT_TRUE(deleted) << "Failed to delete test file";
    EXPECT_FALSE(file_or_directory_exists(test_file)) << "File should not exist after deletion";
    std::cout << "✓ File deletion passed" << std::endl;
}

TEST_F(FSTest, DirectoryOperations) {
    const char* test_dir = "test_directory";
    const char* nested_dir = "test_directory/nested/deep";
    
    // Test directory creation
    bool created = create_directory(test_dir);
    EXPECT_TRUE(created) << "Failed to create directory";
    std::cout << "✓ Directory created successfully" << std::endl;
    
    // Test directory existence
    bool exists = directory_exists(test_dir);
    EXPECT_TRUE(exists) << "Directory should exist after creation";
    std::cout << "✓ Directory existence check passed" << std::endl;
    
    // Test nested directory creation
    bool nested_created = create_directories(nested_dir);
    EXPECT_TRUE(nested_created) << "Failed to create nested directories";
    EXPECT_TRUE(directory_exists(nested_dir)) << "Nested directory should exist";
    std::cout << "✓ Nested directory creation passed" << std::endl;
    
    // Test directory type check
    EXPECT_TRUE(is_directory(test_dir)) << "Should be identified as directory";
    EXPECT_FALSE(is_file(test_dir)) << "Should not be identified as file";
    std::cout << "✓ Directory type check passed" << std::endl;
    
    // Clean up (Note: delete_directory only works for empty directories)
    delete_directory(nested_dir);
    delete_directory("test_directory/nested");
    delete_directory(test_dir);
    std::cout << "✓ Directory cleanup completed" << std::endl;
}

TEST_F(FSTest, BinaryFileOperations) {
    const char* binary_file = "test_binary.bin";
    const unsigned char binary_data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0xFF, 0xFE, 0xFD};
    const size_t data_size = sizeof(binary_data);
    
    // Test binary file creation
    bool created = create_file_binary(binary_file, binary_data, data_size);
    EXPECT_TRUE(created) << "Failed to create binary file";
    std::cout << "✓ Binary file created successfully" << std::endl;
    
    // Test binary file reading
    size_t read_size;
    unsigned char* read_data = (unsigned char*)read_file_binary(binary_file, &read_size);
    EXPECT_NE(read_data, nullptr) << "Failed to read binary file";
    EXPECT_EQ(read_size, data_size) << "Binary data size mismatch";
    
    // Compare binary data
    for (size_t i = 0; i < data_size; i++) {
        EXPECT_EQ(read_data[i], binary_data[i]) << "Binary data mismatch at index " << i;
    }
    std::cout << "✓ Binary file reading passed" << std::endl;
    
    free_file_buffer(read_data);
    
    // Clean up
    delete_file(binary_file);
    std::cout << "✓ Binary file cleanup completed" << std::endl;
}

TEST_F(FSTest, CppWrapperFunctions) {
    std::string test_file = "test_cpp_wrapper.txt";
    std::string test_content = "Testing C++ wrapper functions";
    
    // Test C++ string overloads
    bool created = create_file(test_file, test_content);
    EXPECT_TRUE(created) << "Failed to create file using C++ wrapper";
    std::cout << "✓ C++ wrapper file creation passed" << std::endl;
    
    bool exists = file_or_directory_exists(test_file);
    EXPECT_TRUE(exists) << "File existence check failed with C++ wrapper";
    std::cout << "✓ C++ wrapper existence check passed" << std::endl;
    
    std::string read_content = read_file_text_cpp(test_file);
    EXPECT_EQ(read_content, test_content) << "C++ wrapper content mismatch";
    std::cout << "✓ C++ wrapper file reading passed" << std::endl;
    
    // Clean up
    delete_file(test_file.c_str());
    std::cout << "✓ C++ wrapper cleanup completed" << std::endl;
}

TEST_F(FSTest, FileAppendOperation) {
    const char* test_file = "test_file.txt";
    const char* initial_content = "Initial content";
    const char* append_content = "\nAppended content";
    
    // Create initial file
    EXPECT_TRUE(create_file(test_file, initial_content));
    
    // Append to file
    EXPECT_TRUE(append_to_file(test_file, append_content));
    
    // Read and verify
    std::string expected = std::string(initial_content) + append_content;
    std::string actual = read_file_text_cpp(test_file);
    EXPECT_EQ(actual, expected) << "Appended content mismatch";
    
    std::cout << "✓ File append operation passed" << std::endl;
}

TEST_F(FSTest, FileCopyOperation) {
    const char* src_file = "test_file.txt";
    const char* dest_file = "test_copy.txt";
    const char* test_content = "Content to copy";
    
    // Create source file
    EXPECT_TRUE(create_file(src_file, test_content));
    
    // Copy file
    EXPECT_TRUE(copy_file(src_file, dest_file));
    
    // Verify both files exist and have same content
    EXPECT_TRUE(file_or_directory_exists(src_file));
    EXPECT_TRUE(file_or_directory_exists(dest_file));
    
    std::string src_content = read_file_text_cpp(src_file);
    std::string dest_content = read_file_text_cpp(dest_file);
    EXPECT_EQ(src_content, dest_content) << "Copied file content mismatch";
    
    // Clean up
    delete_file(dest_file);
    
    std::cout << "✓ File copy operation passed" << std::endl;
}

TEST_F(FSTest, NonExistentFileOperations) {
    const char* non_existent = "non_existent_file.txt";
    
    // Test operations on non-existent file
    EXPECT_FALSE(file_or_directory_exists(non_existent));
    EXPECT_FALSE(is_file(non_existent));
    EXPECT_EQ(get_file_size(non_existent), -1);
    EXPECT_EQ(read_file_text(non_existent), nullptr);
    
    size_t size;
    EXPECT_EQ(read_file_binary(non_existent, &size), nullptr);
    
    std::cout << "✓ Non-existent file operations handled correctly" << std::endl;
}

TEST_F(FSTest, MoveFileOperation) {
    const char* src_file = "test_move_src.txt";
    const char* dest_file = "test_move_dest.txt";
    const char* test_content = "Content to move";
    
    // Create source file
    EXPECT_TRUE(create_file(src_file, test_content));
    
    // Move file
    EXPECT_TRUE(move_file(src_file, dest_file));
    
    // Verify source doesn't exist and destination exists with correct content
    EXPECT_FALSE(file_or_directory_exists(src_file));
    EXPECT_TRUE(file_or_directory_exists(dest_file));
    
    std::string dest_content = read_file_text_cpp(dest_file);
    EXPECT_EQ(dest_content, test_content) << "Moved file content mismatch";
    
    // Clean up
    delete_file(dest_file);
    
    std::cout << "✓ File move operation passed" << std::endl;
}

TEST_F(FSTest, RenameFileOperation) {
    const char* old_name = "test_old_name.txt";
    const char* new_name = "test_new_name.txt";
    const char* test_content = "Content to rename";
    
    // Create file with old name
    EXPECT_TRUE(create_file(old_name, test_content));
    
    // Rename file
    EXPECT_TRUE(rename_file(old_name, new_name));
    
    // Verify old name doesn't exist and new name exists with correct content
    EXPECT_FALSE(file_or_directory_exists(old_name));
    EXPECT_TRUE(file_or_directory_exists(new_name));
    
    std::string content = read_file_text_cpp(new_name);
    EXPECT_EQ(content, test_content) << "Renamed file content mismatch";
    
    // Clean up
    delete_file(new_name);
    
    std::cout << "✓ File rename operation passed" << std::endl;
}

TEST_F(FSTest, FileMetadataOperations) {
    const char* test_file = "test_metadata.txt";
    const char* test_content = "Metadata test content";
    
    // Create test file
    EXPECT_TRUE(create_file(test_file, test_content));
    
    // Test file modified time
    uint64_t mod_time = get_file_modified_time(test_file);
    EXPECT_GT(mod_time, 0) << "File modified time should be greater than 0";
    std::cout << "✓ File modified time: " << mod_time << std::endl;
    
    // Test file extension
    std::string ext = get_file_extension(test_file);
    EXPECT_EQ(ext, ".txt") << "File extension should be .txt";
    std::cout << "✓ File extension: " << ext << std::endl;
    
    // Test filename extraction
    std::string filename = get_filename_from_path(test_file);
    EXPECT_EQ(filename, "test_metadata.txt") << "Filename should match";
    std::cout << "✓ Filename from path: " << filename << std::endl;
    
    // Test parent directory
    std::string parent = get_parent_directory("dir/subdir/file.txt");
    EXPECT_EQ(parent, "dir/subdir") << "Parent directory should be correct";
    std::cout << "✓ Parent directory: " << parent << std::endl;
    
    // Clean up
    delete_file(test_file);
    
    std::cout << "✓ File metadata operations passed" << std::endl;
}

TEST_F(FSTest, FileChunkOperations) {
    const char* chunk_file = "test_chunks.bin";
    const char data1[] = "First chunk data";
    const char data2[] = "Second chunk";
    const size_t chunk1_size = strlen(data1);
    const size_t chunk2_size = strlen(data2);
    
    // Write first chunk at offset 0
    EXPECT_TRUE(write_file_chunk(chunk_file, 0, data1, chunk1_size));
    
    // Write second chunk at offset 20
    EXPECT_TRUE(write_file_chunk(chunk_file, 20, data2, chunk2_size));
    
    // Read first chunk
    char buffer1[32] = {0};
    EXPECT_TRUE(read_file_chunk(chunk_file, 0, buffer1, chunk1_size));
    EXPECT_STREQ(buffer1, data1) << "First chunk data mismatch";
    
    // Read second chunk
    char buffer2[32] = {0};
    EXPECT_TRUE(read_file_chunk(chunk_file, 20, buffer2, chunk2_size));
    EXPECT_STREQ(buffer2, data2) << "Second chunk data mismatch";
    
    // Clean up
    delete_file(chunk_file);
    
    std::cout << "✓ File chunk operations passed" << std::endl;
}

TEST_F(FSTest, CreateFileWithSize) {
    const char* sized_file = "test_sized.bin";
    const uint64_t file_size = 1024;
    
    // Create file with specific size
    EXPECT_TRUE(create_file_with_size(sized_file, file_size));
    
    // Verify file exists and has correct size
    EXPECT_TRUE(file_or_directory_exists(sized_file));
    EXPECT_EQ(get_file_size(sized_file), (int64_t)file_size) << "File size should match";
    
    // Clean up
    delete_file(sized_file);
    
    std::cout << "✓ Create file with size operation passed" << std::endl;
}

TEST_F(FSTest, DirectoryListingOperations) {
    const char* test_dir = "test_listing";
    const char* sub_dir = "test_listing/subdir";
    const char* test_file1 = "test_listing/file1.txt";
    const char* test_file2 = "test_listing/file2.bin";
    
    // Create test directory structure
    EXPECT_TRUE(create_directory(test_dir));
    EXPECT_TRUE(create_directory(sub_dir));
    EXPECT_TRUE(create_file(test_file1, "File 1 content"));
    EXPECT_TRUE(create_file(test_file2, "File 2 content"));
    
    // List directory contents
    std::vector<DirectoryEntry> entries;
    EXPECT_TRUE(list_directory(test_dir, entries));
    
    // Should have 3 entries: subdir, file1.txt, file2.bin
    EXPECT_EQ(entries.size(), 3) << "Should have 3 directory entries";
    
    // Verify entries (order may vary)
    bool found_subdir = false, found_file1 = false, found_file2 = false;
    for (const auto& entry : entries) {
        if (entry.name == "subdir" && entry.is_directory) {
            found_subdir = true;
        } else if (entry.name == "file1.txt" && !entry.is_directory) {
            found_file1 = true;
            EXPECT_GT(entry.size, 0) << "File1 should have size > 0";
        } else if (entry.name == "file2.bin" && !entry.is_directory) {
            found_file2 = true;
            EXPECT_GT(entry.size, 0) << "File2 should have size > 0";
        }
    }
    
    EXPECT_TRUE(found_subdir) << "Should find subdirectory";
    EXPECT_TRUE(found_file1) << "Should find file1.txt";
    EXPECT_TRUE(found_file2) << "Should find file2.bin";
    
    // Clean up
    delete_file(test_file1);
    delete_file(test_file2);
    delete_directory(sub_dir);
    delete_directory(test_dir);
    
    std::cout << "✓ Directory listing operations passed" << std::endl;
}

TEST_F(FSTest, PathUtilities) {
    // Test path combination
    std::string combined1 = combine_paths("base/path", "relative/file.txt");
    EXPECT_EQ(combined1, "base/path/relative/file.txt") << "Path combination failed";
    
    std::string combined2 = combine_paths("base/path/", "/relative/file.txt");
    EXPECT_EQ(combined2, "base/path/relative/file.txt") << "Path combination with separators failed";
    
    std::string combined3 = combine_paths("", "relative/file.txt");
    EXPECT_EQ(combined3, "relative/file.txt") << "Empty base path combination failed";
    
    std::string combined4 = combine_paths("base/path", "");
    EXPECT_EQ(combined4, "base/path") << "Empty relative path combination failed";
    
    // Test path validation
    const char* test_file = "validation_test.txt";
    EXPECT_TRUE(create_file(test_file, "test"));
    
    EXPECT_TRUE(validate_path(test_file, false)) << "Valid existing file should pass validation";
    EXPECT_FALSE(validate_path("non_existent_file.txt", false)) << "Non-existent file should fail validation";
    
    delete_file(test_file);
    
    std::cout << "✓ Path utilities passed" << std::endl;
}

TEST_F(FSTest, DirectoryOperationsAdvanced) {
    char current_dir[1024];
    const char* test_change_dir = "test_change_directory";
    
    // Get current directory
    EXPECT_TRUE(get_current_directory(current_dir, sizeof(current_dir)));
    EXPECT_GT(strlen(current_dir), 0) << "Current directory should not be empty";
    std::cout << "✓ Current directory: " << current_dir << std::endl;
    
    // Create test directory for changing to
    EXPECT_TRUE(create_directory(test_change_dir));
    
    // Change to test directory
    EXPECT_TRUE(set_current_directory(test_change_dir));
    
    // Verify we're in the new directory
    char new_dir[1024];
    EXPECT_TRUE(get_current_directory(new_dir, sizeof(new_dir)));
    std::string new_dir_str(new_dir);
    EXPECT_NE(new_dir_str.find(test_change_dir), std::string::npos) 
        << "Should be in test directory";
    
    // Change back to original directory
    EXPECT_TRUE(set_current_directory(current_dir));
    
    // Clean up
    delete_directory(test_change_dir);
    
    std::cout << "✓ Directory operations advanced passed" << std::endl;
}

TEST_F(FSTest, FileExistsFunction) {
    const char* test_file = "test_file_exists.txt";
    const char* test_dir = "test_dir_exists";
    
    // Test file_exists function specifically (different from file_or_directory_exists)
    EXPECT_FALSE(file_exists(test_file)) << "Non-existent file should return false";
    EXPECT_FALSE(file_exists(nullptr)) << "Null path should return false";
    
    // Create a file and test
    EXPECT_TRUE(create_file(test_file, "test content"));
    EXPECT_TRUE(file_exists(test_file)) << "Existing file should return true";
    
    // Create a directory and verify file_exists returns false for directories
    EXPECT_TRUE(create_directory(test_dir));
    EXPECT_FALSE(file_exists(test_dir)) << "Directory should not be identified as file";
    EXPECT_TRUE(directory_exists(test_dir)) << "Directory should be identified as directory";
    
    // Clean up
    delete_file(test_file);
    delete_directory(test_dir);
    
    std::cout << "✓ file_exists function test passed" << std::endl;
}

TEST_F(FSTest, ErrorHandlingAndEdgeCases) {
    // Test null pointer handling
    EXPECT_FALSE(file_or_directory_exists(nullptr));
    EXPECT_FALSE(file_exists(nullptr));
    EXPECT_FALSE(directory_exists(nullptr));
    EXPECT_FALSE(create_file(nullptr, "content"));
    EXPECT_TRUE(create_file("test.txt", nullptr)); // null content should create empty file
    EXPECT_EQ(read_file_text(nullptr), nullptr);
    EXPECT_EQ(read_file_binary(nullptr, nullptr), nullptr);
    EXPECT_FALSE(create_directory(nullptr));
    EXPECT_FALSE(create_directories(nullptr));
    EXPECT_EQ(get_file_size(nullptr), -1);
    EXPECT_FALSE(delete_file(nullptr));
    EXPECT_FALSE(delete_directory(nullptr));
    EXPECT_FALSE(copy_file(nullptr, "dest"));
    EXPECT_FALSE(copy_file("src", nullptr));
    EXPECT_FALSE(move_file(nullptr, "dest"));
    EXPECT_FALSE(move_file("src", nullptr));
    EXPECT_FALSE(rename_file(nullptr, "new"));
    EXPECT_FALSE(rename_file("old", nullptr));
    EXPECT_EQ(get_file_modified_time(nullptr), 0);
    EXPECT_EQ(get_file_extension(nullptr), "");
    EXPECT_EQ(get_filename_from_path(nullptr), "");
    EXPECT_EQ(get_parent_directory(nullptr), "");
    EXPECT_FALSE(write_file_chunk(nullptr, 0, "data", 4));
    EXPECT_FALSE(read_file_chunk(nullptr, 0, nullptr, 4));
    EXPECT_FALSE(create_file_with_size(nullptr, 100));
    EXPECT_FALSE(validate_path(nullptr, false));
    EXPECT_FALSE(get_current_directory(nullptr, 1024));
    EXPECT_FALSE(set_current_directory(nullptr));
    
    // Test with empty paths
    EXPECT_FALSE(file_or_directory_exists(""));
    EXPECT_FALSE(create_file("", "content"));
    EXPECT_FALSE(create_directory(""));
    
    // Clean up test file created during null content test
    delete_file("test.txt");
    
    std::cout << "✓ Error handling and edge cases passed" << std::endl;
}

TEST_F(FSTest, CreateFileNullContent) {
    const char* test_file = "test_null_content.txt";
    
    // Test creating file with null content (should create empty file)
    EXPECT_TRUE(create_file(test_file, nullptr));
    EXPECT_TRUE(file_exists(test_file));
    EXPECT_EQ(get_file_size(test_file), 0) << "File with null content should be empty";
    
    // Clean up
    delete_file(test_file);
    
    std::cout << "✓ Create file with null content test passed" << std::endl;
}

TEST_F(FSTest, BinaryFileWithNullData) {
    const char* binary_file = "test_null_binary.bin";
    
    // Test creating binary file with null data (should create empty file)
    EXPECT_TRUE(create_file_binary(binary_file, nullptr, 0));
    EXPECT_TRUE(file_exists(binary_file));
    EXPECT_EQ(get_file_size(binary_file), 0) << "Binary file with null data should be empty";
    
    // Test with null data but non-zero size (should create empty file)
    EXPECT_TRUE(create_file_binary(binary_file, nullptr, 100));
    EXPECT_EQ(get_file_size(binary_file), 0) << "Binary file with null data should be empty regardless of size parameter";
    
    // Clean up
    delete_file(binary_file);
    
    std::cout << "✓ Binary file with null data test passed" << std::endl;
}

TEST_F(FSTest, FileChunkEdgeCases) {
    const char* chunk_file = "test_chunk_edge_cases.bin";
    const char test_data[] = "Test chunk data";
    const size_t data_size = strlen(test_data);
    
    // Test writing chunk to non-existent file (should create file)
    EXPECT_TRUE(write_file_chunk(chunk_file, 0, test_data, data_size));
    EXPECT_TRUE(file_exists(chunk_file));
    
    // Test reading from chunk with null buffer
    EXPECT_FALSE(read_file_chunk(chunk_file, 0, nullptr, data_size));
    
    // Test writing chunk with null data
    EXPECT_FALSE(write_file_chunk(chunk_file, 0, nullptr, data_size));
    
    // Test reading/writing chunk beyond file size
    char buffer[32] = {0};
    uint64_t large_offset = 10000;
    EXPECT_FALSE(read_file_chunk(chunk_file, large_offset, buffer, sizeof(buffer))) 
        << "Reading beyond file size should fail";
    
    // Clean up
    delete_file(chunk_file);
    
    std::cout << "✓ File chunk edge cases test passed" << std::endl;
}

TEST_F(FSTest, PathUtilitiesAdvanced) {
    // Test get_file_extension edge cases
    EXPECT_EQ(get_file_extension("file.txt"), ".txt");
    EXPECT_EQ(get_file_extension("file.tar.gz"), ".gz");
    EXPECT_EQ(get_file_extension("file"), "");
    EXPECT_EQ(get_file_extension(".hidden"), "");
    EXPECT_EQ(get_file_extension("path/to/file.ext"), ".ext");
    EXPECT_EQ(get_file_extension(""), "");
    
    // Test get_filename_from_path edge cases
    EXPECT_EQ(get_filename_from_path("path/to/file.txt"), "file.txt");
    EXPECT_EQ(get_filename_from_path("path\\to\\file.txt"), "file.txt");
    EXPECT_EQ(get_filename_from_path("file.txt"), "file.txt");
    EXPECT_EQ(get_filename_from_path(""), "");
    EXPECT_EQ(get_filename_from_path("/"), "");
    EXPECT_EQ(get_filename_from_path("\\"), "");
    
    // Test get_parent_directory edge cases
    EXPECT_EQ(get_parent_directory("path/to/file.txt"), "path/to");
    EXPECT_EQ(get_parent_directory("path\\to\\file.txt"), "path\\to");
    EXPECT_EQ(get_parent_directory("file.txt"), "");
    EXPECT_EQ(get_parent_directory(""), "");
    
    // Test combine_paths edge cases
    EXPECT_EQ(combine_paths("", ""), "");
    EXPECT_EQ(combine_paths("base", ""), "base");
    EXPECT_EQ(combine_paths("", "relative"), "relative");
    EXPECT_EQ(combine_paths("base/", "relative"), "base/relative");
    EXPECT_EQ(combine_paths("base", "/relative"), "base/relative");
    EXPECT_EQ(combine_paths("base\\", "\\relative"), "base/relative");
    
    std::cout << "✓ Advanced path utilities test passed" << std::endl;
}

TEST_F(FSTest, ValidatePathAdvanced) {
    const char* test_file = "validate_path_test.txt";
    const char* test_dir = "validate_dir";
    
    // Create test file and directory
    EXPECT_TRUE(create_file(test_file, "test content"));
    EXPECT_TRUE(create_directory(test_dir));
    
    // Test validate_path with check_write_access = false (default)
    EXPECT_TRUE(validate_path(test_file, false)) << "Existing file should be valid";
    EXPECT_FALSE(validate_path(test_dir, false)) << "Directory should not be valid as file";
    EXPECT_FALSE(validate_path("non_existent.txt", false)) << "Non-existent file should not be valid";
    
    // Test validate_path with check_write_access = true
    EXPECT_TRUE(validate_path("new_file_in_current_dir.txt", true)) 
        << "New file in existing directory should be valid for writing";
    EXPECT_FALSE(validate_path("nonexistent_dir/new_file.txt", true)) 
        << "New file in non-existent directory should not be valid for writing";
    
    // Clean up
    delete_file(test_file);
    delete_directory(test_dir);
    
    std::cout << "✓ Advanced validate_path test passed" << std::endl;
}

TEST_F(FSTest, DirectoryListingEdgeCases) {
    const char* empty_dir = "empty_test_dir";
    const char* non_existent_dir = "non_existent_dir";
    
    // Test listing empty directory
    EXPECT_TRUE(create_directory(empty_dir));
    std::vector<DirectoryEntry> entries;
    EXPECT_TRUE(list_directory(empty_dir, entries));
    EXPECT_EQ(entries.size(), 0) << "Empty directory should have no entries";
    
    // Test listing non-existent directory
    EXPECT_FALSE(list_directory(non_existent_dir, entries)) 
        << "Listing non-existent directory should fail";
    
    // Test with null path
    EXPECT_FALSE(list_directory(nullptr, entries)) 
        << "Listing null path should fail";
    
    // Clean up
    delete_directory(empty_dir);
    
    std::cout << "✓ Directory listing edge cases test passed" << std::endl;
}

TEST_F(FSTest, FileBufferManagement) {
    const char* test_file = "buffer_test.txt";
    const char* test_content = "Buffer management test content";
    
    // Create test file
    EXPECT_TRUE(create_file(test_file, test_content));
    
    // Test reading and proper buffer cleanup
    size_t size;
    char* buffer1 = read_file_text(test_file, &size);
    EXPECT_NE(buffer1, nullptr);
    EXPECT_EQ(size, strlen(test_content));
    
    void* buffer2 = read_file_binary(test_file, &size);
    EXPECT_NE(buffer2, nullptr);
    EXPECT_EQ(size, strlen(test_content));
    
    // Test free_file_buffer with valid pointers
    free_file_buffer(buffer1);
    free_file_buffer(buffer2);
    
    // Test free_file_buffer with null pointer (should not crash)
    free_file_buffer(nullptr);
    
    // Clean up
    delete_file(test_file);
    
    std::cout << "✓ File buffer management test passed" << std::endl;
}

TEST_F(FSTest, LargeFileOperations) {
    const char* large_file = "large_file_test.bin";
    const uint64_t large_size = 10240; // 10KB
    
    // Test creating large file
    EXPECT_TRUE(create_file_with_size(large_file, large_size));
    EXPECT_TRUE(file_exists(large_file));
    EXPECT_EQ(get_file_size(large_file), (int64_t)large_size);
    
    // Test creating zero-size file
    const char* zero_file = "zero_size.bin";
    EXPECT_TRUE(create_file_with_size(zero_file, 0));
    EXPECT_TRUE(file_exists(zero_file));
    EXPECT_EQ(get_file_size(zero_file), 0);
    
    // Clean up
    delete_file(large_file);
    delete_file(zero_file);
    
    std::cout << "✓ Large file operations test passed" << std::endl;
} 