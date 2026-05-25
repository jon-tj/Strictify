#pragma once

#include "models.h"

#include <string>
#include <vector>

std::vector<std::string> validateAndFixCards(
    const std::vector<MarkdownFile>& markdownFiles,
    const fs::path& schemaDir,
    const fs::path& reportBase);
