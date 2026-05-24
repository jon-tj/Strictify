#include "content_validator.h"
#include "frontmatter_validator.h"
#include "parsers.h"
#include "utils.h"

#include <algorithm>
#include <filesystem>
#include <future>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage: strictify <notes_root> [schema_dir]" << '\n';
        return 1;
    }

    fs::path root = fs::path(argv[1]);
    fs::path schemaDir = argc > 2 ? fs::path(argv[2]) : root / "src";

    std::error_code ec;
    root = fs::weakly_canonical(root, ec);
    if (ec) {
        std::cerr << "Unable to resolve root path: " << root.string() << '\n';
        return 1;
    }

    schemaDir = fs::weakly_canonical(schemaDir, ec);
    if (ec) {
        std::cerr << "Unable to resolve schema directory: " << schemaDir.string() << '\n';
        return 1;
    }

    std::unordered_map<std::string, SchemaDefinition> schemasByTag;
    for (const fs::directory_entry& entry : fs::directory_iterator(schemaDir)) {
        if (!entry.is_regular_file() || toLower(entry.path().extension().string()) != ".yaml") {
            continue;
        }

        SchemaDefinition schema = parseSchemaFile(entry.path());
        schemasByTag[toLower(schema.name)] = std::move(schema);
    }

    std::vector<fs::path> markdownPaths;
    for (const fs::directory_entry& entry : fs::recursive_directory_iterator(root)) {
        if (entry.is_regular_file() && toLower(entry.path().extension().string()) == ".md") {
            markdownPaths.push_back(entry.path());
        }
    }

    std::vector<std::future<std::optional<MarkdownFile>>> futures;
    futures.reserve(markdownPaths.size());
    for (const fs::path& markdownPath : markdownPaths) {
        futures.push_back(std::async(std::launch::async, [markdownPath]() {
            return parseMarkdownFile(markdownPath);
        }));
    }

    std::vector<MarkdownFile> markdownFiles;
    markdownFiles.reserve(markdownPaths.size());
    std::vector<std::string> errors;

    for (size_t i = 0; i < futures.size(); ++i) {
        std::optional<MarkdownFile> parsed = futures[i].get();
        if (!parsed.has_value()) {
            errors.push_back(relativeOrOriginal(markdownPaths[i], root) + ": invalid or unterminated frontmatter");
            continue;
        }
        markdownFiles.push_back(std::move(parsed.value()));
    }

    std::unordered_map<std::string, std::vector<const MarkdownFile*>> markdownByFileName;
    for (const MarkdownFile& markdownFile : markdownFiles) {
        markdownByFileName[toLower(markdownFile.path.filename().string())].push_back(&markdownFile);
    }

    std::vector<std::string> frontmatterErrors =
        validateFrontmatter(markdownFiles, schemasByTag, markdownByFileName, root);
    std::vector<std::string> contentErrors = validateContentLinks(markdownFiles, root);

    errors.insert(errors.end(), frontmatterErrors.begin(), frontmatterErrors.end());
    errors.insert(errors.end(), contentErrors.begin(), contentErrors.end());

    std::sort(errors.begin(), errors.end());
    if (!errors.empty()) {
        for (const std::string& error : errors) {
            std::cout << error << '\n';
        }
        return 1;
    }

    std::cout << "Validation passed: all markdown frontmatter and content checks succeeded." << '\n';
    return 0;
}
