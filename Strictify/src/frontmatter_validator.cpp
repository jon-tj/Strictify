#include "frontmatter_validator.h"

#include "utils.h"

#include <set>

std::vector<std::string> validateFrontmatter(
    const std::vector<MarkdownFile>& markdownFiles,
    const std::unordered_map<std::string, SchemaDefinition>& schemasByTag,
    const std::unordered_map<std::string, std::vector<const MarkdownFile*>>& markdownByFileName,
    const fs::path& root) {
    std::vector<std::string> errors;

    for (const MarkdownFile& file : markdownFiles) {
        const std::string filePath = relativeOrOriginal(file.path, root);

        for (const std::string& tag : file.tags) {
            const auto schemaIt = schemasByTag.find(toLower(tag));
            if (schemaIt == schemasByTag.end()) {
                continue;
            }

            const SchemaDefinition& schema = schemaIt->second;
            for (const auto& propertyEntry : schema.properties) {
                const std::string& propertyName = propertyEntry.first;
                const PropertyRule& rule = propertyEntry.second;
                const auto valueIt = file.frontmatter.find(propertyName);

                if (rule.required && valueIt == file.frontmatter.end()) {
                    errors.push_back(filePath + ": missing required field '" + propertyName + "' for tag #" + tag);
                    continue;
                }

                if (valueIt == file.frontmatter.end()) {
                    continue;
                }

                const std::string value = trim(valueIt->second);
                if (rule.type.has_value()) {
                    const std::string type = toLower(rule.type.value());
                    if (type == "string" && value.empty()) {
                        errors.push_back(filePath + ": field '" + propertyName + "' must be a non-empty string");
                    }
                }

                if (rule.format.has_value() && toLower(rule.format.value()) == "date" && !isIsoDate(value)) {
                    errors.push_back(filePath + ": field '" + propertyName + "' must be a valid date (YYYY-MM-DD)");
                }

                if (!rule.enumValues.empty()) {
                    bool found = false;
                    for (const std::string& enumValue : rule.enumValues) {
                        if (value == enumValue) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        errors.push_back(filePath + ": field '" + propertyName + "' has invalid value '" + value + "'");
                    }
                }

                if (!rule.refs.empty()) {
                    const bool referencesAreOptional =
                        rule.allowsPlainString || (rule.type.has_value() && toLower(rule.type.value()) == "string");
                    const std::string filename = ensureMdExtension(value);
                    const std::string lookup = toLower(fs::path(filename).filename().string());
                    const auto referencedIt = markdownByFileName.find(lookup);
                    if (referencedIt == markdownByFileName.end() || referencedIt->second.empty()) {
                        if (!referencesAreOptional) {
                            errors.push_back(filePath + ": field '" + propertyName + "' references missing file '" + filename + "'");
                        }
                        continue;
                    }

                    bool validReferenceTag = false;
                    std::string expectedTags;
                    for (size_t i = 0; i < rule.refs.size(); ++i) {
                        expectedTags += "#" + rule.refs[i];
                        if (i + 1 < rule.refs.size()) {
                            expectedTags += " or ";
                        }
                    }

                    for (const MarkdownFile* candidate : referencedIt->second) {
                        for (const std::string& refName : rule.refs) {
                            if (candidate->tags.find(refName) != candidate->tags.end()) {
                                validReferenceTag = true;
                                break;
                            }
                        }

                        if (validReferenceTag) {
                            break;
                        }
                    }

                    if (!validReferenceTag && !referencesAreOptional) {
                        errors.push_back(filePath + ": field '" + propertyName + "' references '" + filename + "' but it does not contain " + expectedTags);
                    }
                }
            }
        }
    }

    return errors;
}
