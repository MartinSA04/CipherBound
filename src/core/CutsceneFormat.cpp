#include "CutsceneFormat.h"
#include "StringUtils.h"
#include "TextParse.h"

using StringUtils::parseDirection;
using StringUtils::splitSemicolon;
using StringUtils::trimRightInPlace;

namespace CutsceneFormat {
namespace {

enum class CutsceneSection { none, header, steps };

std::optional<CutsceneStep> parseStep(std::string_view line) {
    const auto parts = TextParse::splitView(line, '|');
    if (parts.empty())
        return std::nullopt;

    const std::string_view command = parts[0];
    CutsceneStep step{};

    if (command == "move" && parts.size() >= 4) {
        const auto coords = TextParse::parseFixedIntFields<2>(parts, 2);
        if (!coords.has_value())
            return std::nullopt;
        step.type = CutsceneStep::Type::move;
        step.target = std::string(parts[1]);
        step.x = (*coords)[0];
        step.y = (*coords)[1];
        return step;
    }

    if (command == "walk" && parts.size() >= 3) {
        step.type = CutsceneStep::Type::walk;
        step.target = std::string(parts[1]);
        step.direction = parseDirection(std::string(parts[2]));
        return step;
    }

    if (command == "face" && parts.size() >= 3) {
        step.type = CutsceneStep::Type::face;
        step.target = std::string(parts[1]);
        step.direction = parseDirection(std::string(parts[2]));
        return step;
    }

    if (command == "say" && parts.size() >= 3) {
        step.type = CutsceneStep::Type::say;
        step.speaker = std::string(parts[1]);
        step.lines = splitSemicolon(std::string(parts[2]));
        return step;
    }

    if (command == "wait" && parts.size() >= 2) {
        const auto frames = TextParse::parseInt(parts[1]);
        if (!frames.has_value())
            return std::nullopt;
        step.type = CutsceneStep::Type::wait;
        step.frames = *frames;
        return step;
    }

    if (command == "sync") {
        step.type = CutsceneStep::Type::sync;
        return step;
    }

    if (command == "flag" && parts.size() >= 2) {
        step.type = CutsceneStep::Type::flag;
        step.flagName = std::string(parts[1]);
        return step;
    }

    if (command == "hide" && parts.size() >= 2) {
        step.type = CutsceneStep::Type::hide;
        step.target = std::string(parts[1]);
        return step;
    }

    if (command == "show" && parts.size() >= 2) {
        step.type = CutsceneStep::Type::show;
        step.target = std::string(parts[1]);
        return step;
    }

    return std::nullopt;
}

} // namespace

ParseResult parse(std::istream &input) {
    ParseResult result;
    CutsceneSection section = CutsceneSection::none;

    std::string line;
    while (std::getline(input, line)) {
        trimRightInPlace(line);

        if (line.empty() || line[0] == '#')
            continue;

        if (line == "[header]") {
            section = CutsceneSection::header;
            continue;
        }
        if (line == "[steps]") {
            section = CutsceneSection::steps;
            continue;
        }

        if (section == CutsceneSection::header) {
            const auto keyVal = TextParse::splitOnce(line, '|');
            if (keyVal.has_value() && keyVal->first == "id")
                result.cutscene.id = std::string(keyVal->second);
            continue;
        }

        if (section == CutsceneSection::steps) {
            const auto step = parseStep(line);
            if (!step.has_value()) {
                result.warnings.push_back("Invalid step: " + line);
                continue;
            }
            result.cutscene.steps.push_back(*step);
        }
    }

    return result;
}

} // namespace CutsceneFormat
