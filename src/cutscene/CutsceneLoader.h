/**
 * @file
 * @brief File-loading wrapper around the cutscene parser.
 * @ingroup cutscene_system
 */

#pragma once

#include "../game_data/Cutscene.h"
#include <filesystem>
#include <string>
#include <vector>

/**
 * @brief Resolves and loads cutscene files from disk.
 * @ingroup cutscene_system
 */
namespace CutsceneLoader {

/// Result of resolving and loading a cutscene file.
struct LoadResult {
    std::filesystem::path resolvedPath; ///< Path after runtime asset resolution.
    Cutscene cutscene;                  ///< Parsed cutscene payload.
    std::vector<std::string> warnings;  ///< Non-fatal parse warnings.
    bool opened{false};                 ///< Whether the file could be opened.

    /// Returns whether the file was opened and the parsed cutscene looks valid.
    bool valid() const { return opened && !cutscene.id.empty(); }
};

/// Resolves a path, opens the file, and parses it as a cutscene.
LoadResult load(const std::filesystem::path &path);

} // namespace CutsceneLoader
