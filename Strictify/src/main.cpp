#include "card_validator.h"
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

    const fs::path inputPath = fs::path(argv[1]);
    fs::path root = inputPath;

    std::error_code ec;
    root = fs::weakly_canonical(root, ec);
    if (ec) {
        std::cerr << "Unable to resolve input path: " << inputPath.string() << '\n';
        return 1;
    }

    const bool inputIsDirectory = fs::is_directory(root);
    const bool inputIsFile = fs::is_regular_file(root);
    if (!inputIsDirectory && !inputIsFile) {
        std::cerr << "Input path is neither a directory nor a file: " << root.string() << '\n';
        return 1;
    }

    fs::path schemaDir;
    if (argc > 2) {
        schemaDir = fs::path(argv[2]);
    } else {
        schemaDir = inputIsDirectory ? (root / "src") : (root.parent_path() / "src");
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
    if (inputIsDirectory) {
        for (const fs::directory_entry& entry : fs::recursive_directory_iterator(root)) {
            if (entry.is_regular_file() && toLower(entry.path().extension().string()) == ".md") {
                markdownPaths.push_back(entry.path());
            }
        }
    } else if (toLower(root.extension().string()) == ".md") {
        markdownPaths.push_back(root);
    } else {
        std::cerr << "Input file must be a markdown file: " << root.string() << '\n';
        return 1;
    }

    const fs::path reportBase = inputIsDirectory ? root : root.parent_path();

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
            errors.push_back(relativeOrOriginal(markdownPaths[i], reportBase) + ":1: invalid or unterminated frontmatter");
            continue;
        }
        markdownFiles.push_back(std::move(parsed.value()));
    }

    std::unordered_map<std::string, std::vector<const MarkdownFile*>> markdownByFileName;
    for (const MarkdownFile& markdownFile : markdownFiles) {
        markdownByFileName[toLower(markdownFile.path.filename().string())].push_back(&markdownFile);
    }

    std::vector<std::string> frontmatterErrors =
        validateFrontmatter(markdownFiles, schemasByTag, markdownByFileName, reportBase);
    std::vector<std::string> contentErrors = validateContentLinks(markdownFiles, schemasByTag, reportBase);
    std::vector<std::string> cardErrors = validateAndFixCards(markdownFiles, schemaDir, reportBase);

    errors.insert(errors.end(), frontmatterErrors.begin(), frontmatterErrors.end());
    errors.insert(errors.end(), contentErrors.begin(), contentErrors.end());
    errors.insert(errors.end(), cardErrors.begin(), cardErrors.end());

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
