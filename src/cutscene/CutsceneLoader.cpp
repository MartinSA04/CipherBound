#include "CutsceneLoader.h"
#include "../common/FilePaths.h"
#include "CutsceneFormat.h"
#include <fstream>

namespace CutsceneLoader {

LoadResult load(const std::filesystem::path &path) {
    LoadResult result;
    result.resolvedPath = FilePaths::resolveExistingPath(path);

    std::ifstream input(result.resolvedPath);
    if (!input.is_open())
        return result;

    result.opened = true;

    auto parsed = CutsceneFormat::parse(input);
    result.warnings = std::move(parsed.warnings);
    result.cutscene = std::move(parsed.cutscene);
    return result;
}

} // namespace CutsceneLoader
