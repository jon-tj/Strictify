#pragma once

#include "models.h"

#include <string>
#include <unordered_map>
#include <vector>

std::vector<std::string> validateContentLinks(
    const std::vector<MarkdownFile>& markdownFiles,
    const std::unordered_map<std::string, SchemaDefinition>& schemasByTag,
    const fs::path& root);
