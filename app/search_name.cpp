#include "../include/search_name.h"
#include "../include/block_cache.h"
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

extern BlockCache cache;

bool read_file_from_cache(const fs::path &file_path) {
    int fd = cache.open(file_path.c_str());
    if (fd == -1) {
        std::cerr << "Error opening file from cache: " << file_path << std::endl;
        return false;
    }

    char buf[256] = {0};
    ssize_t bytes_read = cache.read(fd, buf, sizeof(buf));
    if (bytes_read > 0) {
        std::cout << "Read from file: " << std::string(buf, bytes_read) << std::endl;
    } else if (bytes_read == -1) {
        std::cerr << "Error reading file from cache: " << file_path << std::endl;
        cache.close(fd); 
        return false;
    }

    if (cache.close(fd) == -1) {
        std::cerr << "Error closing file from cache: " << file_path << std::endl;
        return false;
    }

    return true;
}

int search_file(const fs::path &dir, const std::string &file_name) {
    try {
        for (const auto &entry : fs::recursive_directory_iterator(dir, fs::directory_options::skip_permission_denied)) {
            if (entry.path().filename() == file_name) {
                std::cout << "File found: " << entry.path() << std::endl;

                if (!read_file_from_cache(entry.path())) {
                    std::cerr << "Failed to read file: " << entry.path() << std::endl;
                    return -1; 
                }

                return 0;
            }
        }

        std::cout << "File not found\n";
        return 1;
    } catch (const fs::filesystem_error &e) {
        std::cerr << "Directory access error: " << e.what() << std::endl;
        return -1; 
    } catch (const std::exception &e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        return -1; 
    }
}
