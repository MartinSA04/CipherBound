#pragma once
#include "../state/Movement.h"
#include <cctype>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace StringUtils {

inline void trimRightInPlace(std::string &s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
        s.pop_back();
}

inline std::string capitalize(std::string s) {
    if (!s.empty())
        s[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(s[0])));
    return s;
}

inline std::vector<std::string> splitPipe(const std::string &s) {
    std::vector<std::string> parts;
    std::istringstream iss(s);
    std::string token;
    while (std::getline(iss, token, '|'))
        parts.push_back(token);
    return parts;
}

inline std::vector<std::string> splitSemicolon(const std::string &s) {
    std::vector<std::string> parts;
    if (s.empty() || s == "-")
        return parts;
    std::istringstream iss(s);
    std::string token;
    while (std::getline(iss, token, ';'))
        parts.push_back(token);
    return parts;
}

inline std::vector<std::string> splitDoubleAt(const std::string &s) {
    std::vector<std::string> parts;
    if (s.empty())
        return parts;
    size_t start = 0;
    while (true) {
        size_t pos = s.find("@@", start);
        if (pos == std::string::npos) {
            parts.push_back(s.substr(start));
            break;
        }
        parts.push_back(s.substr(start, pos - start));
        start = pos + 2;
    }
    return parts;
}

inline std::string replaceAll(std::string s, std::string_view needle,
                              std::string_view replacement) {
    if (needle.empty())
        return s;

    std::size_t pos = 0;
    while ((pos = s.find(needle, pos)) != std::string::npos) {
        s.replace(pos, needle.size(), replacement);
        pos += replacement.size();
    }
    return s;
}

inline std::string substitutePlayerName(std::string s, std::string_view playerName) {
    s = replaceAll(std::move(s), "{player}", playerName);
    s = replaceAll(std::move(s), "{player_name}", playerName);
    return s;
}

inline std::vector<std::string> substitutePlayerName(const std::vector<std::string> &lines,
                                                     std::string_view playerName) {
    std::vector<std::string> resolved;
    resolved.reserve(lines.size());
    for (const auto &line : lines)
        resolved.push_back(substitutePlayerName(line, playerName));
    return resolved;
}

inline Direction parseDirection(const std::string &s) {
    if (s == "up")
        return Direction::up;
    if (s == "down")
        return Direction::down;
    if (s == "left")
        return Direction::left;
    if (s == "right")
        return Direction::right;
    return Direction::down;
}

} // namespace StringUtils
