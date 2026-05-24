#include "utils.h"

#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>

std::string trim(const std::string& value) {
    size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])) != 0) {
        ++start;
    }

    size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
        --end;
    }

    return value.substr(start, end - start);
}

std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string normalizeKey(const std::string& key) {
    return toLower(trim(key));
}

std::string stripQuotes(const std::string& input) {
    std::string value = trim(input);
    if (value.size() >= 2) {
        const char first = value.front();
        const char last = value.back();
        if ((first == '"' && last == '"') || (first == '\'' && last == '\'')) {
            return value.substr(1, value.size() - 2);
        }
    }
    return value;
}

std::vector<std::string> parseBracketList(const std::string& value) {
    std::vector<std::string> result;
    std::string text = trim(value);
    if (text.empty() || text.front() != '[' || text.back() != ']') {
        return result;
    }

    std::string body = text.substr(1, text.size() - 2);
    std::string token;
    std::stringstream stream(body);
    while (std::getline(stream, token, ',')) {
        token = stripQuotes(token);
        if (!token.empty()) {
            result.push_back(token);
        }
    }

    return result;
}

bool isIsoDate(const std::string& value) {
    static const std::regex dateRegex(R"(^([0-9]{4})-([0-9]{2})-([0-9]{2})$)");
    std::smatch match;
    if (!std::regex_match(value, match, dateRegex)) {
        return false;
    }

    int year = std::stoi(match[1].str());
    int month = std::stoi(match[2].str());
    int day = std::stoi(match[3].str());

    if (month < 1 || month > 12 || day < 1) {
        return false;
    }

    const int daysInMonth[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int maxDay = daysInMonth[month];
    const bool leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    if (month == 2 && leap) {
        maxDay = 29;
    }

    return day <= maxDay;
}

std::string relativeOrOriginal(const fs::path& path, const fs::path& base) {
    std::error_code ec;
    fs::path rel = fs::relative(path, base, ec);
    if (ec) {
        return path.generic_string();
    }
    return rel.generic_string();
}

std::string ensureMdExtension(std::string value) {
    value = trim(value);
    if (value.size() >= 3) {
        const std::string tail = toLower(value.substr(value.size() - 3));
        if (tail == ".md") {
            return value;
        }
    }
    return value + ".md";
}

std::string urlDecode(const std::string& value) {
    std::string result;
    result.reserve(value.size());

    for (size_t i = 0; i < value.size(); ++i) {
        const char ch = value[i];
        if (ch == '%' && i + 2 < value.size() && std::isxdigit(static_cast<unsigned char>(value[i + 1])) != 0 &&
            std::isxdigit(static_cast<unsigned char>(value[i + 2])) != 0) {
            const std::string hex = value.substr(i + 1, 2);
            const char decoded = static_cast<char>(std::stoi(hex, nullptr, 16));
            result.push_back(decoded);
            i += 2;
            continue;
        }

        if (ch == '+') {
            result.push_back(' ');
            continue;
        }

        result.push_back(ch);
    }

    return result;
}
