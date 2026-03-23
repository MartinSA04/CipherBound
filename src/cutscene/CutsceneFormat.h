/**
 * @file
 * @brief Parser interface for `.cutscene` files.
 * @ingroup cutscene_system
 * @ingroup data_formats
 */

#pragma once

#include "../game_data/Cutscene.h"
#include <iosfwd>
#include <string>
#include <vector>

/**
 * @brief Parser for section-based cutscene files.
 * @ingroup data_formats
 */
namespace CutsceneFormat {

/// Result of parsing a `.cutscene` file.
struct ParseResult {
    Cutscene cutscene;                 ///< Parsed cutscene payload.
    std::vector<std::string> warnings; ///< Non-fatal parse warnings.

    /// Returns whether the cutscene has the required header information.
    bool valid() const { return !cutscene.id.empty(); }
};

/// Parses a cutscene from the current stream position.
ParseResult parse(std::istream &input);

} // namespace CutsceneFormat
