#pragma once

#include <filesystem>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

struct PropertyRule {
    bool required = false;
    std::optional<std::string> type;
    std::optional<std::string> format;
    std::vector<std::string> enumValues;
    std::vector<std::string> refs;
};

struct SchemaDefinition {
    std::string name;
    std::unordered_map<std::string, PropertyRule> properties;
};

struct MarkdownFile {
    fs::path path;
    std::unordered_map<std::string, std::string> frontmatter;
    std::set<std::string> tags;
    std::vector<std::string> lines;
};
