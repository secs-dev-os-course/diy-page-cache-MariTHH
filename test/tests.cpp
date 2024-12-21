#include <gtest/gtest.h>
#include "../include/block_cache.h"
#include <fstream>
#include <filesystem>

class BlockCacheTestFixture : public ::testing::Test {
protected:
    const size_t block_size = 4096;
    const size_t cache_size_limit = 16;
    const std::string file_path = "sample_data.bin";

    void SetUp() override {
        std::ofstream file(file_path, std::ios::binary);
        for (size_t i = 0; i < block_size * 5; ++i) {
            file.put(static_cast<char>(i % 256));  
        }
        file.close();
    }

    void TearDown() override {
        std::filesystem::remove(file_path);  
    }
};

TEST_F(BlockCacheTestFixture, CanOpenAndCloseFile) {
    BlockCache cache(block_size, cache_size_limit);

    int fd = cache.open(file_path.c_str());
    ASSERT_GT(fd, 0) << "Failed to open the file.";

    int close_status = cache.close(fd);
    ASSERT_EQ(close_status, 0) << "Failed to close the file.";
}

TEST_F(BlockCacheTestFixture, EnsureCorrectDataRead) {
    BlockCache cache(block_size, cache_size_limit);
    int fd = cache.open(file_path.c_str());
    ASSERT_GT(fd, 0);

    std::vector<char> data_buffer(block_size, 0);
    ssize_t bytes_read = cache.read(fd, data_buffer.data(), block_size);
    ASSERT_EQ(bytes_read, block_size) << "Incorrect number of bytes read.";

    for (size_t i = 0; i < block_size; ++i) {
        ASSERT_EQ(data_buffer[i], static_cast<char>(i % 256)) << "Data mismatch in the file.";
    }

    cache.close(fd);
}

TEST_F(BlockCacheTestFixture, CanWriteAndReadData) {
    BlockCache cache(block_size, cache_size_limit);
    int fd = cache.open(file_path.c_str());
    ASSERT_GT(fd, 0);

    std::vector<char> write_data(block_size, 'Y');
    ssize_t bytes_written = cache.write(fd, write_data.data(), block_size);
    ASSERT_EQ(bytes_written, block_size) << "Write operation failed.";

    std::vector<char> read_data(block_size, 0);
    cache.lseek(fd, 0, SEEK_SET); 
    ssize_t bytes_read = cache.read(fd, read_data.data(), block_size);
    ASSERT_EQ(bytes_read, block_size) << "Read operation failed.";

    for (size_t i = 0; i < block_size; ++i) {
        ASSERT_EQ(read_data[i], 'Y') << "Data corruption occurred during reading.";
    }

    cache.close(fd);
}

TEST_F(BlockCacheTestFixture, CacheEvictionUponOverflow) {
    BlockCache cache(block_size, cache_size_limit);
    int fd = cache.open(file_path.c_str());
    ASSERT_GT(fd, 0);

    std::vector<char> temp_buffer(block_size, 0);
    for (size_t i = 0; i < cache_size_limit + 1; ++i) {
        ssize_t bytes_read = cache.read(fd, temp_buffer.data(), block_size);
        ASSERT_EQ(bytes_read, block_size) << "Failed to read data from file.";
    }

    cache.close(fd);
}

TEST_F(BlockCacheTestFixture, VerifyCacheEvictionAndDataRecovery) {
    BlockCache cache(block_size, cache_size_limit);
    int fd = cache.open(file_path.c_str());
    ASSERT_GT(fd, 0);

    std::vector<char> buffer(block_size, 0);
    for (size_t i = 0; i < cache_size_limit; ++i) {
        ssize_t bytes_read = cache.read(fd, buffer.data(), block_size);
        ASSERT_EQ(bytes_read, block_size) << "Failed to read data from file.";
    }

    cache.close(fd);
 
    fd = cache.open(file_path.c_str());
    ASSERT_GT(fd, 0);
    ssize_t bytes_read = cache.read(fd, buffer.data(), block_size);
    ASSERT_EQ(bytes_read, block_size) << "Data recovery from cache failed.";

    cache.close(fd);
}

TEST_F(BlockCacheTestFixture, DataFlushOnFsync) {
    BlockCache cache(block_size, cache_size_limit);
    int fd = cache.open(file_path.c_str());
    ASSERT_GT(fd, 0);

    std::vector<char> buffer(block_size, 'M');
    ssize_t bytes_written = cache.write(fd, buffer.data(), block_size);
    ASSERT_EQ(bytes_written, block_size) << "Failed to write data.";

    int fsync_status = cache.fsync(fd);
    ASSERT_EQ(fsync_status, 0) << "fsync operation failed.";

    cache.close(fd);
}
