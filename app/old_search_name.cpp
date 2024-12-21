#include <fstream>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

bool read_file(const fs::path &file_path) {
    std::ifstream file(file_path, std::ios::in | std::ios::binary);
    if (!file) {
        std::cerr << "Error opening file: " << file_path << std::endl;
        return false;
    }

    char buf[256] = {0};
    file.read(buf, sizeof(buf));
    std::streamsize bytes_read = file.gcount();
    if (bytes_read > 0) {
        std::cout << "Read from file: " << std::string(buf, bytes_read) << std::endl;
    } else {
        std::cerr << "Error reading file: " << file_path << std::endl;
        return false;
    }

    return true;
}

int search_file_old(const fs::path &dir, const std::string &file_name, int repeat_count) {
    try {
        for (int i = 0; i < repeat_count; ++i) {
            for (const auto &entry : fs::recursive_directory_iterator(dir, fs::directory_options::skip_permission_denied)) {
                if (entry.path().filename() == file_name) {
                    std::cout << "File found: " << entry.path() << std::endl;
                    if (!read_file(entry.path())) {
                        return -1; 
                    }
                    return 0; 
                }
            }
        }
        std::cout << "File not found\n";
        return 1; 
    } catch (const fs::filesystem_error &e) {
        std::cerr << "Access error: " << e.what() << std::endl;
        return -1; 
    }
}