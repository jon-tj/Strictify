#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

std::string trim(const std::string& value);
std::string toLower(std::string value);
std::string normalizeKey(const std::string& key);
std::string stripQuotes(const std::string& input);
std::vector<std::string> parseBracketList(const std::string& value);
bool isIsoDate(const std::string& value);
std::string relativeOrOriginal(const fs::path& path, const fs::path& base);
std::string ensureMdExtension(std::string value);
std::string urlDecode(const std::string& value);
