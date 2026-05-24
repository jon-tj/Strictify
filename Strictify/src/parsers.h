#pragma once

#include "models.h"

#include <optional>

SchemaDefinition parseSchemaFile(const fs::path& schemaPath);
std::optional<MarkdownFile> parseMarkdownFile(const fs::path& markdownPath);
