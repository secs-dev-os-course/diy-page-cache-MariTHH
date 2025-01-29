#include "../include/block_cache.h"
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <list>
#include <unordered_map>
#include <vector>

BlockCache cache(4096, 16); 

BlockCache::BlockCache(size_t block_size, size_t max_cache_size)
    : block_size_(block_size), max_cache_size_(max_cache_size) {}

int BlockCache::open(const char* path) {
    int fd = ::open(path, O_RDWR | O_DIRECT);
    if (fd == -1) {
        perror("Error opening file");
        return -1;
    }
    file_descriptors_[fd] = fd;
    fd_offsets_[fd] = 0;
    return fd;
}

int BlockCache::close(int fd) {
    if (file_descriptors_.find(fd) == file_descriptors_.end()) {
        std::cerr << "Error: invalid file descriptor\n";
        return -1;
    }

    fsync(fd); 
    ::close(fd);
    file_descriptors_.erase(fd);
    fd_offsets_.erase(fd);
    return 0;
}
ssize_t BlockCache::read(int fd, void* buf, size_t count) {
    if (file_descriptors_.find(fd) == file_descriptors_.end()) {
        std::cerr << "Error: invalid file descriptor\n";
        return -1;
    }

    std::memset(buf, 0, count);
    off_t offset = fd_offsets_[fd];
    size_t bytes_read = 0;

    while (bytes_read < count) {
        off_t block_offset = (offset / block_size_) * block_size_;
        size_t page_offset = offset % block_size_;
        size_t bytes_to_read = std::min(count - bytes_read, block_size_ - page_offset);

        auto it = cache_.find(block_offset);
        if (it != cache_.end()) {
            cache_order_.splice(cache_order_.begin(), cache_order_, it->second.iterator);

            if (page_offset + bytes_to_read > block_size_) {
                std::cerr << "Error: invalid page offset or size\n";
                return -1;
            }

            std::memcpy((char*)buf + bytes_read, it->second.data.data() + page_offset, bytes_to_read);
        } else {
            CachePage page;
            page.offset = block_offset;
            page.data.resize(block_size_);
            std::memset(page.data.data(), 0, block_size_);

            ssize_t ret = ::pread(fd, page.data.data(), block_size_, block_offset);
            if (ret == -1) {
                perror("Error reading block from disk");
                return -1;
            }

            if (cache_.size() >= max_cache_size_ && !cache_order_.empty()) {
                evict_lru_page();
            }

            page.referenced = true;
            page.modified = false;
            auto cache_it = cache_.insert({block_offset, std::move(page)}).first;
            cache_it->second.iterator = cache_order_.insert(cache_order_.begin(), cache_it);

            std::memcpy((char*)buf + bytes_read, cache_[block_offset].data.data() + page_offset, bytes_to_read);
        }

        bytes_read += bytes_to_read;
        offset += bytes_to_read;
    }

    fd_offsets_[fd] = offset;
    return bytes_read;
}

ssize_t BlockCache::write(int fd, const void* buf, size_t count) {
    if (file_descriptors_.find(fd) == file_descriptors_.end()) {
        std::cerr << "Error: invalid file descriptor\n";
        return -1;
    }

    off_t offset = fd_offsets_[fd];
    size_t bytes_written = 0;

    while (bytes_written < count) {
        off_t block_offset = (offset / block_size_) * block_size_;
        size_t page_offset = offset % block_size_;
        size_t bytes_to_write = std::min(count - bytes_written, block_size_ - page_offset);

        auto it = cache_.find(block_offset);
        if (it == cache_.end()) {
            CachePage page;
            page.offset = block_offset;
            page.data.resize(block_size_);

            ssize_t ret = ::pread(fd, page.data.data(), block_size_, block_offset);
            if (ret == -1) {
                perror("Read error during write");
                return -1;
            }

            if (cache_.size() >= max_cache_size_) {
                evict_lru_page();
            }

            auto cache_it = cache_.insert({block_offset, std::move(page)}).first;
            cache_it->second.iterator = cache_order_.insert(cache_order_.begin(), cache_it);
        }

        std::memcpy(cache_[block_offset].data.data() + page_offset, (char*)buf + bytes_written, bytes_to_write);
        cache_[block_offset].modified = true;
        cache_order_.splice(cache_order_.begin(), cache_order_, cache_[block_offset].iterator);

        bytes_written += bytes_to_write;
        offset += bytes_to_write;
    }

    fd_offsets_[fd] = offset;
    return bytes_written;
}

off_t BlockCache::lseek(int fd, off_t offset, int whence) {
    if (file_descriptors_.find(fd) == file_descriptors_.end()) {
        std::cerr << "Error: invalid file descriptor\n";
        return -1;
    }

    off_t new_offset = ::lseek(fd, offset, whence);
    if (new_offset == -1) {
        perror("Lseek error");
        return -1;
    }

    fd_offsets_[fd] = new_offset;
    return new_offset;
}

int BlockCache::fsync(int fd) {
    if (file_descriptors_.find(fd) == file_descriptors_.end()) {
        std::cerr << "Error: invalid file descriptor\n";
        return -1;
    }

    for (auto it = cache_.begin(); it != cache_.end(); ++it) {
        if (it->second.modified) {
            ssize_t ret = ::pwrite(fd, it->second.data.data(), block_size_, it->second.offset);
            if (ret == -1) {
                perror("Write error during fsync");
                return -1;
            }
            it->second.modified = false;
        }
    }

    return 0;
}

void BlockCache::evict_lru_page() {
if (cache_order_.empty()) return; 
auto lru_it = cache_order_.back();
CachePage& page = lru_it->second;

if (page.modified) {
    if (file_descriptors_.empty()) {
        std::cerr << "Error: no open file descriptors\n";
        return;
    }
    int fd = file_descriptors_.begin()->first; 
    ::pwrite(fd, page.data.data(), block_size_, page.offset);
}

cache_.erase(lru_it);
cache_order_.pop_back();

}
