#ifndef BLOCK_CACHE_H
#define BLOCK_CACHE_H

#include <unordered_map>
#include <list>
#include <vector>
#include <cstddef>
#include <sys/types.h>

class BlockCache {
 public:
  BlockCache(size_t block_size, size_t max_cache_size);

  int open(const char* path);
  int close(int fd);
  ssize_t read(int fd, void* buf, size_t count);
  ssize_t write(int fd, const void* buf, size_t count);
  off_t lseek(int fd, off_t offset, int whence);
  int fsync(int fd);

 private:


  void evict_lru_page();

  struct CachePage {
    off_t offset;
    std::vector<char> data;
    bool referenced = false;
    bool modified = false;
    std::list<std::unordered_map<off_t, CachePage>::iterator>::iterator iterator;
  };

  size_t block_size_;
  size_t max_cache_size_;
  std::unordered_map<int, int> file_descriptors_;
  std::unordered_map<int, off_t> fd_offsets_;
  std::unordered_map<off_t, CachePage> cache_;
  std::list<std::unordered_map<off_t, CachePage>::iterator> cache_order_;
};

#endif // BLOCK_CACHE_H
