#include "content_validator.h"

#include "utils.h"

#include <regex>

namespace {

std::string normalizeLinkTarget(std::string target) {
    target = trim(target);
    if (target.empty()) {
        return target;
    }

    if (target.front() == '<' && target.back() == '>' && target.size() >= 2) {
        target = target.substr(1, target.size() - 2);
    }

    const size_t hashPos = target.find('#');
    if (hashPos != std::string::npos) {
        target = target.substr(0, hashPos);
    }

    const size_t queryPos = target.find('?');
    if (queryPos != std::string::npos) {
        target = target.substr(0, queryPos);
    }

    const size_t spacePos = target.find(' ');
    if (spacePos != std::string::npos) {
        target = target.substr(0, spacePos);
    }

    return trim(target);
}

bool isExternalTarget(const std::string& target) {
    if (target.empty()) {
        return true;
    }
    if (target[0] == '#') {
        return true;
    }
    if (target.rfind("//", 0) == 0) {
        return true;
    }

    static const std::regex schemeRegex(R"(^[A-Za-z][A-Za-z0-9+.-]*:)");
    return std::regex_search(target, schemeRegex);
}

} // namespace

std::vector<std::string> validateContentLinks(
    const std::vector<MarkdownFile>& markdownFiles,
    const std::unordered_map<std::string, SchemaDefinition>& schemasByTag,
    const fs::path& root) {
    std::vector<std::string> errors;
    static const std::regex linkRegex(R"(\[[^\]]*\]\(([^)]+)\))");

    for (const MarkdownFile& file : markdownFiles) {
        const std::string filePath = relativeOrOriginal(file.path, root);

        for (const std::string& tag : file.tags) {
            if (schemasByTag.find(toLower(tag)) == schemasByTag.end()) {
                errors.push_back(filePath + ": unknown entity tag #" + tag + " (no matching schema in src)");
            }
        }

        for (const std::string& line : file.lines) {
            std::sregex_iterator iter(line.begin(), line.end(), linkRegex);
            std::sregex_iterator end;
            for (; iter != end; ++iter) {
                const std::string rawTarget = iter->str(1);
                std::string target = normalizeLinkTarget(rawTarget);
                if (isExternalTarget(target)) {
                    continue;
                }

                target = urlDecode(target);
                if (target.empty()) {
                    continue;
                }

                fs::path resolved = (file.path.parent_path() / fs::path(target)).lexically_normal();
                if (!fs::exists(resolved)) {
                    errors.push_back(filePath + ": invalid link target '" + rawTarget + "' (resolved: '" + target + "')");
                }
            }
        }
    }

    return errors;
}
