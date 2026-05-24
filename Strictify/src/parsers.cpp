#include "parsers.h"

#include "utils.h"

#include <fstream>
#include <regex>

std::optional<std::string> captureRefName(const std::string& line) {
    static const std::regex refRegex(R"REF(^\s*\$ref:\s*"?#/\$defs/([A-Za-z0-9_\-]+)"?\s*$)REF");
    std::smatch match;
    if (std::regex_match(line, match, refRegex)) {
        return match[1].str();
    }
    return std::nullopt;
}

SchemaDefinition parseSchemaFile(const fs::path& schemaPath) {
    SchemaDefinition schema;
    schema.name = schemaPath.stem().string();

    std::ifstream input(schemaPath);
    if (!input.is_open()) {
        return schema;
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(input, line)) {
        lines.push_back(line);
    }

    std::set<std::string> required;
    for (const std::string& rawLine : lines) {
        std::string trimmed = trim(rawLine);
        if (trimmed.rfind("required:", 0) == 0) {
            const std::string listText = trim(trimmed.substr(std::string("required:").size()));
            for (const std::string& item : parseBracketList(listText)) {
                required.insert(normalizeKey(item));
            }
        }
    }

    bool inProperties = false;
    size_t index = 0;
    while (index < lines.size()) {
        const std::string& raw = lines[index];
        const std::string trimmed = trim(raw);

        if (trimmed == "properties:") {
            inProperties = true;
            ++index;
            continue;
        }

        if (!inProperties) {
            ++index;
            continue;
        }

        static const std::regex propertyRegex(R"(^\s{2}([A-Za-z0-9_\-]+):\s*$)");
        std::smatch propertyMatch;
        if (!std::regex_match(raw, propertyMatch, propertyRegex)) {
            ++index;
            continue;
        }

        const std::string propertyName = propertyMatch[1].str();
        const std::string normalizedProperty = normalizeKey(propertyName);
        PropertyRule rule;
        rule.required = required.find(normalizedProperty) != required.end();

        ++index;
        while (index < lines.size()) {
            const std::string& innerRaw = lines[index];
            if (innerRaw.size() >= 2 && innerRaw.substr(0, 2) == "  " &&
                (innerRaw.size() < 4 || innerRaw.substr(0, 4) != "    ")) {
                break;
            }

            const std::string innerTrimmed = trim(innerRaw);

            if (innerTrimmed.rfind("type:", 0) == 0) {
                rule.type = stripQuotes(innerTrimmed.substr(std::string("type:").size()));
            } else if (innerTrimmed.rfind("format:", 0) == 0) {
                rule.format = stripQuotes(innerTrimmed.substr(std::string("format:").size()));
            } else if (innerTrimmed.rfind("enum:", 0) == 0) {
                rule.enumValues = parseBracketList(innerTrimmed.substr(std::string("enum:").size()));
            } else if (innerTrimmed.rfind("$ref:", 0) == 0) {
                const std::optional<std::string> ref = captureRefName(innerTrimmed);
                if (ref.has_value()) {
                    rule.refs.push_back(ref.value());
                }
            } else if (innerTrimmed == "anyOf:") {
                ++index;
                while (index < lines.size()) {
                    const std::string anyOfLine = trim(lines[index]);
                    if (anyOfLine.rfind("- ", 0) != 0) {
                        --index;
                        break;
                    }

                    const std::string refCandidate = trim(anyOfLine.substr(2));
                    const std::optional<std::string> ref = captureRefName(refCandidate);
                    if (ref.has_value()) {
                        rule.refs.push_back(ref.value());
                    }

                    ++index;
                }
            }

            ++index;
        }

        schema.properties[normalizedProperty] = std::move(rule);
    }

    return schema;
}

std::optional<MarkdownFile> parseMarkdownFile(const fs::path& markdownPath) {
    std::ifstream input(markdownPath);
    if (!input.is_open()) {
        return std::nullopt;
    }

    MarkdownFile file;
    file.path = markdownPath;

    std::string line;
    while (std::getline(input, line)) {
        file.lines.push_back(line);
    }

    bool inFrontmatter = false;
    bool frontmatterComplete = false;
    if (!file.lines.empty() && trim(file.lines.front()) == "---") {
        inFrontmatter = true;
        for (size_t i = 1; i < file.lines.size(); ++i) {
            const std::string current = trim(file.lines[i]);
            if (current == "---") {
                frontmatterComplete = true;
                break;
            }

            const size_t separator = file.lines[i].find(':');
            if (separator == std::string::npos) {
                continue;
            }

            const std::string rawKey = file.lines[i].substr(0, separator);
            const std::string rawValue = file.lines[i].substr(separator + 1);
            const std::string key = normalizeKey(rawKey);
            const std::string value = stripQuotes(rawValue);
            if (!key.empty()) {
                file.frontmatter[key] = value;
            }
        }
    }

    if (inFrontmatter && !frontmatterComplete) {
        return std::nullopt;
    }

    static const std::regex tagRegex(R"((^|\s)#([A-Za-z][A-Za-z0-9_\-]*))");
    for (const std::string& contentLine : file.lines) {
        std::sregex_iterator iter(contentLine.begin(), contentLine.end(), tagRegex);
        std::sregex_iterator end;
        for (; iter != end; ++iter) {
            file.tags.insert(iter->str(2));
        }
    }

    return file;
}
