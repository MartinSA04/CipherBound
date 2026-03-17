#pragma once

#include "../data/Cutscene.h"
#include <filesystem>
#include <string>
#include <vector>

namespace CutsceneLoader {

struct LoadResult {
    std::filesystem::path resolvedPath;
    Cutscene cutscene;
    std::vector<std::string> warnings;
    bool opened{false};

    bool valid() const { return opened && !cutscene.id.empty(); }
};

LoadResult load(const std::filesystem::path &path);

} // namespace CutsceneLoader
