#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <cctype>
#include "../world/Map.h"

namespace StringUtils
{

inline std::string capitalize(std::string s)
{
    if (!s.empty())
        s[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(s[0])));
    return s;
}

inline std::vector<std::string> splitPipe(const std::string &s)
{
    std::vector<std::string> parts;
    std::istringstream iss(s);
    std::string token;
    while (std::getline(iss, token, '|'))
        parts.push_back(token);
    return parts;
}

inline std::vector<std::string> splitSemicolon(const std::string &s)
{
    std::vector<std::string> parts;
    if (s.empty() || s == "-")
        return parts;
    std::istringstream iss(s);
    std::string token;
    while (std::getline(iss, token, ';'))
        parts.push_back(token);
    return parts;
}

inline std::vector<std::string> splitDoubleAt(const std::string &s)
{
    std::vector<std::string> parts;
    if (s.empty())
        return parts;
    size_t start = 0;
    while (true)
    {
        size_t pos = s.find("@@", start);
        if (pos == std::string::npos)
        {
            parts.push_back(s.substr(start));
            break;
        }
        parts.push_back(s.substr(start, pos - start));
        start = pos + 2;
    }
    return parts;
}

inline Direction parseDirection(const std::string &s)
{
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
