#include "../include/block_cache.h"
#include "../include/search_name.h"
#include <chrono>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>

extern int search_file_old(const std::filesystem::path &dir, const std::string &file_name, int repeat_count); 

BlockCache cache(4096, 16);

void benchmark_search(const std::string &file_name, int repeat_count) {
  auto start_time = std::chrono::high_resolution_clock::now();
  int result = 0;
  
  for (int i = 0; i < repeat_count; ++i) {
    result = search_file("/", file_name); 
    if (result != 0) {
      break;
    }
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end_time - start_time;
  std::cout << "Total execution time: " << elapsed.count() << " seconds" << std::endl;

  if (result != 0) {
    std::cerr << "File search error" << std::endl;
  }
}

void test_block_cache() {
  const char *test_file = "../test_dir/file1.txt";
  const size_t block_size = 4096;
  const size_t max_cache_size = 16;

  BlockCache cache(block_size, max_cache_size);

  int fd = cache.open(test_file);
  if (fd == -1) {
    std::cerr << "File open error\n";
    return;
  }
  std::cout << "File successfully opened: " << fd << std::endl;

  const char *write_data = "Hello!";
  ssize_t written = cache.write(fd, write_data, strlen(write_data));
  if (written == -1) {
    std::cerr << "Error writing data\n";
    cache.close(fd);
    return;
  }
  std::cout << "Data successfully written: " << written << " bytes\n";

  if (cache.lseek(fd, 0, SEEK_SET) == -1) {
    std::cerr << "Error moving file pointer\n";
    cache.close(fd);
    return;
  }
  std::cout << "File pointer successfully moved to the beginning\n";

  char read_buffer[128] = {0};
  ssize_t read_bytes = cache.read(fd, read_buffer, sizeof(read_buffer) - 1);
  if (read_bytes == -1) {
    std::cerr << "Error reading data\n";
    cache.close(fd);
    return;
  }
  std::cout << "Data successfully read: " << read_buffer << std::endl;

  if (cache.fsync(fd) == -1) {
    std::cerr << "Error syncing data\n";
    cache.close(fd);
    return;
  }
  std::cout << "Data successfully synchronized to disk\n";

  if (cache.close(fd) == -1) {
    std::cerr << "Error closing file\n";
    return;
  }
  std::cout << "File successfully closed\n";
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0]
              << " <mode> [additional arguments]\n";
    std::cerr << "Modes: \n";
    std::cerr << "  search <file_name> <repeat_count>\n";
    std::cerr << "  old_search <file_name> <repeat_count>\n";
    std::cerr << "  test_cache\n";
    return 1;
  }

  std::string mode = argv[1];
  if (mode == "search") {
    if (argc < 4) {
      std::cerr << "Not enough arguments for search mode\n";
      return 1;
    }
    std::string file_name = argv[2];
    int repeat_count = std::stoi(argv[3]);
    benchmark_search(file_name, repeat_count);
  } else if (mode == "old_search") {
    if (argc < 4) {
      std::cerr << "Not enough arguments for old_search mode\n";
      return 1;
    }
    std::string file_name = argv[2];
    int repeat_count = std::stoi(argv[3]);
    int result = search_file_old("/", file_name, repeat_count);
    if (result != 0) {
      std::cerr << "Error searching file in old mode" << std::endl;
    }
  } else if (mode == "test_cache") {
    test_block_cache();
  }

  return 0;
}
