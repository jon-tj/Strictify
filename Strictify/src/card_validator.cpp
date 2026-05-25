#include "card_validator.h"

#include "utils.h"

#include <fstream>

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
            }

            if (inCard) {
                if (line.find("<front>") != std::string::npos || line.find("<fixed>") != std::string::npos) ++frontCount;
                if (line.find("<back>") != std::string::npos)  ++backCount;

                if (line.find("</card>") != std::string::npos) {
                    inCard = false;
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
