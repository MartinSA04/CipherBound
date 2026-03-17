#pragma once

#include <array>
#include <charconv>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace TextParse {

inline std::vector<std::string_view> splitView(std::string_view s, char delim) {
    std::vector<std::string_view> parts;
    std::size_t start = 0;
    while (true) {
        std::size_t pos = s.find(delim, start);
        if (pos == std::string_view::npos) {
            parts.push_back(s.substr(start));
            break;
        }
        parts.push_back(s.substr(start, pos - start));
        start = pos + 1;
    }
    return parts;
}

inline std::optional<std::pair<std::string_view, std::string_view>> splitOnce(std::string_view s,
                                                                              char delim) {
    std::size_t pos = s.find(delim);
    if (pos == std::string_view::npos)
        return std::nullopt;
    return std::make_pair(s.substr(0, pos), s.substr(pos + 1));
}

inline std::optional<int> parseInt(std::string_view s) {
    int value = 0;
    const char *begin = s.data();
    const char *end = begin + s.size();
    const auto [ptr, ec] = std::from_chars(begin, end, value);
    if (ec != std::errc() || ptr != end)
        return std::nullopt;
    return value;
}

template <std::size_t N>
inline std::optional<std::array<int, N>>
parseFixedIntFields(const std::vector<std::string_view> &fields, std::size_t offset = 0) {
    if (fields.size() < offset + N)
        return std::nullopt;

    std::array<int, N> values{};
    for (std::size_t i = 0; i < N; ++i) {
        const auto parsed = parseInt(fields[offset + i]);
        if (!parsed.has_value())
            return std::nullopt;
        values[i] = *parsed;
    }
    return values;
}

template <std::size_t N>
inline std::optional<std::array<int, N>> parseFixedIntList(std::string_view s, char delim) {
    const auto fields = splitView(s, delim);
    if (fields.size() != N)
        return std::nullopt;
    return parseFixedIntFields<N>(fields);
}

} // namespace TextParse
