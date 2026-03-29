#include "NPCDialogueFormat.h"
#include "../../common/StringUtils.h"
#include <cctype>
#include <optional>

using StringUtils::trimRightInPlace;

namespace NPCDialogueFormat {
namespace {

std::string trimCopy(std::string value) {
    std::size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])))
        ++start;

    std::size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])))
        --end;

    return value.substr(start, end - start);
}

bool isValidFlagName(std::string_view value) {
    if (value.empty())
        return false;

    for (const char ch : value) {
        const bool isIdentifierChar = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                                      (ch >= '0' && ch <= '9') || ch == '_';
        if (!isIdentifierChar)
            return false;
    }

    return true;
}

std::optional<std::string> parseStageHeader(const std::string &line) {
    if (line.size() < 2 || line.front() != '[' || line.back() != ']')
        return std::nullopt;

    const std::string header = trimCopy(line.substr(1, line.size() - 2));
    if (header == "default")
        return std::string{};

    constexpr std::string_view flagPrefix = "flag ";
    if (header.rfind(flagPrefix, 0) == 0) {
        const std::string flagName = trimCopy(header.substr(flagPrefix.size()));
        if (isValidFlagName(flagName))
            return flagName;
    }

    return std::nullopt;
}

} // namespace

ParseResult parse(std::istream &input) {
    ParseResult result;
    int currentStage = -1;

    std::string line;
    while (std::getline(input, line)) {
        trimRightInPlace(line);
        const std::string trimmed = trimCopy(line);

        if (trimmed.empty())
            continue;
        if (trimmed.front() == '#')
            continue;

        if (const auto header = parseStageHeader(trimmed); header.has_value()) {
            result.stages.push_back(DialogueStage{*header, {}});
            currentStage = static_cast<int>(result.stages.size()) - 1;
            continue;
        }

        if (!trimmed.empty() && trimmed.front() == '[' && trimmed.back() == ']') {
            result.warnings.push_back("Invalid dialogue stage header: " + trimmed);
            continue;
        }

        if (currentStage < 0) {
            result.stages.push_back(DialogueStage{"", {}});
            currentStage = 0;
        }

        result.stages[static_cast<std::size_t>(currentStage)].lines.push_back(line);
    }

    return result;
}

} // namespace NPCDialogueFormat
