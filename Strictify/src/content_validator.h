#pragma once

#include "models.h"

#include <string>
#include <vector>

std::vector<std::string> validateContentLinks(
    const std::vector<MarkdownFile>& markdownFiles,
    const fs::path& root);
