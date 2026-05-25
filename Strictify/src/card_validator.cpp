#include "card_validator.h"

#include "utils.h"

#include <fstream>

namespace {

std::string joinLines(const std::vector<std::string>& lines) {
    std::string joined;
    for (size_t i = 0; i < lines.size(); ++i) {
        joined += lines[i];
        if (i + 1 < lines.size()) {
            joined += '\n';
        }
    }
    return joined;
}

std::vector<std::string> extractTagBodies(const std::string& text, const std::string& tag) {
    std::vector<std::string> bodies;
    const std::string openTag = "<" + tag + ">";
    const std::string closeTag = "</" + tag + ">";

    size_t searchPos = 0;
    while (true) {
        const size_t openPos = text.find(openTag, searchPos);
        if (openPos == std::string::npos) {
            break;
        }

        const size_t bodyStart = openPos + openTag.size();
        const size_t closePos = text.find(closeTag, bodyStart);
        if (closePos == std::string::npos) {
            break;
        }

        bodies.push_back(text.substr(bodyStart, closePos - bodyStart));
        searchPos = closePos + closeTag.size();
    }

    return bodies;
}

bool hasAnyNonEmptyBody(const std::vector<std::string>& bodies) {
    for (const std::string& body : bodies) {
        if (!trim(body).empty()) {
            return true;
        }
    }
    return false;
}

} // namespace

static const char* CARD_JS_CONTENT =
R"(const cards = document.querySelectorAll("card")
for (const card of cards) {
    card.onclick = () => card.classList.toggle("flipped")
}

const stylesheet = document.createElement("style")
stylesheet.textContent = `
    card { 
        cursor: pointer;
        padding:0.2rem;
        background-color:rgba(100,100,100,0.2);
        border-radius:0.5rem;
        transition: background-color 0.1s;
    }
    card.flipped{background-color:rgba(100,100,100,0.5);}
    card back{display:none}
    card.flipped front{display:none}
    card.flipped back{display:inline-block}
`
document.body.appendChild(stylesheet)
)";

std::vector<std::string> validateAndFixCards(
    const std::vector<MarkdownFile>& markdownFiles,
    const fs::path& schemaDir,
    const fs::path& reportBase) {
    std::vector<std::string> errors;

    // Ensure card.js exists in the schema/src directory
    const fs::path cardJsPath = schemaDir / "card.js";
    if (!fs::exists(cardJsPath)) {
        std::ofstream jsOut(cardJsPath);
        jsOut << CARD_JS_CONTENT;
    }

    for (const MarkdownFile& file : markdownFiles) {
        const std::string filePath = relativeOrOriginal(file.path, reportBase);

        bool fileHasCard = false;
        bool fileHasScriptTag = false;
        bool inCard = false;
        int frontCount = 0; // counts <front> and <fixed>
        int backCount = 0;
        size_t cardStartLine = 0;
        std::vector<std::string> cardLines;

        for (size_t i = 0; i < file.lines.size(); ++i) {
            const std::string& line = file.lines[i];

            // Detect existing card.js script inclusion (any attribute quoting style)
            if (line.find("<script") != std::string::npos && line.find("src/card.js") != std::string::npos) {
                fileHasScriptTag = true;
            }

            // Opening <card> tag (does not match </card> since that starts with </)
            if (!inCard && line.find("<card>") != std::string::npos) {
                inCard = true;
                fileHasCard = true;
                cardStartLine = i + 1;
                frontCount = 0;
                backCount = 0;
                cardLines.clear();
            }

            if (inCard) {
                cardLines.push_back(line);
                if (line.find("<front>") != std::string::npos || line.find("<fixed>") != std::string::npos) ++frontCount;
                if (line.find("<back>") != std::string::npos)  ++backCount;

                if (line.find("</card>") != std::string::npos) {
                    inCard = false;

                    const std::string cardBody = joinLines(cardLines);
                    const std::vector<std::string> frontBodies = extractTagBodies(cardBody, "front");
                    const std::vector<std::string> fixedBodies = extractTagBodies(cardBody, "fixed");
                    const std::vector<std::string> backBodies = extractTagBodies(cardBody, "back");

                    std::vector<std::string> allFrontBodies = frontBodies;
                    allFrontBodies.insert(allFrontBodies.end(), fixedBodies.begin(), fixedBodies.end());

                    if (frontCount == 0) {
                        errors.push_back(filePath + ":" + std::to_string(cardStartLine) + ": <card> is missing a <front> or <fixed> element");
                    } else if (frontCount > 1) {
                        errors.push_back(filePath + ":" + std::to_string(cardStartLine) + ": <card> has more than one <front>/<fixed> element");
                    } else if (!hasAnyNonEmptyBody(allFrontBodies)) {
                        errors.push_back(filePath + ":" + std::to_string(cardStartLine) + ": <card> has an empty <front>/<fixed> field");
                    }
                    if (backCount == 0) {
                        errors.push_back(filePath + ":" + std::to_string(cardStartLine) + ": <card> is missing a <back> element");
                    } else if (backCount > 1) {
                        errors.push_back(filePath + ":" + std::to_string(cardStartLine) + ": <card> has more than one <back> element");
                    } else if (!hasAnyNonEmptyBody(backBodies)) {
                        errors.push_back(filePath + ":" + std::to_string(cardStartLine) + ": <card> has an empty <back> field");
                    }
                }
            }
        }

        // Report unclosed card
        if (inCard) {
            if (frontCount == 0) {
                errors.push_back(filePath + ":" + std::to_string(cardStartLine) + ": <card> is missing a <front> or <fixed> element");
            } else if (frontCount > 1) {
                errors.push_back(filePath + ":" + std::to_string(cardStartLine) + ": <card> has more than one <front>/<fixed> element");
            }
            if (backCount == 0) {
                errors.push_back(filePath + ":" + std::to_string(cardStartLine) + ": <card> is missing a <back> element");
            } else if (backCount > 1) {
                errors.push_back(filePath + ":" + std::to_string(cardStartLine) + ": <card> has more than one <back> element");
            }
            errors.push_back(filePath + ":" + std::to_string(cardStartLine) + ": unclosed <card> tag");
        }

        // Inject script tag if the file uses cards but doesn't load card.js yet
        if (fileHasCard && !fileHasScriptTag) {
            std::ofstream out(file.path, std::ios::app);
            out << "\n<script src=src/card.js></script>\n";
        }
    }

    return errors;
}
