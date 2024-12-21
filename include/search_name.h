#ifndef SEARCH_NAME_H
#define SEARCH_NAME_H

#include <filesystem>
#include <string>

// Функция поиска файла в директории с возможностью чтения через кэш
#pragma once
#include <filesystem>
bool read_file_from_cache(const std::filesystem::path& file_path);
int search_file(const std::filesystem::path& dir, const std::string& file_name);


#endif // SEARCH_NAME_H
