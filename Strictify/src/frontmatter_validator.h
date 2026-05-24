#pragma once

#include "models.h"

#include <string>
#include <unordered_map>
#include <vector>

std::vector<std::string> validateFrontmatter(
    const std::vector<MarkdownFile>& markdownFiles,
    const std::unordered_map<std::string, SchemaDefinition>& schemasByTag,
    const std::unordered_map<std::string, std::vector<const MarkdownFile*>>& markdownByFileName,
    const fs::path& root);
