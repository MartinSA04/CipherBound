#pragma once

#include "../game_data/Cutscene.h"
#include <iosfwd>
#include <string>
#include <vector>

namespace CutsceneFormat {

struct ParseResult {
    Cutscene cutscene;
    std::vector<std::string> warnings;

    bool valid() const { return !cutscene.id.empty(); }
};

ParseResult parse(std::istream &input);

} // namespace CutsceneFormat
