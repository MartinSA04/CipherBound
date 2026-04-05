/**
 * @file
 * @brief Parser for external NPC dialogue files.
 * @ingroup data_formats
 * @ingroup world_state
 */

#pragma once

#include "../NPC.h"
#include <iosfwd>
#include <string>
#include <vector>

namespace NPCDialogueFormat {

/// Result of parsing an external NPC dialogue file.
struct ParseResult {
    std::vector<DialogueStage> stages; ///< Dialogue stages in author-defined order.
    std::vector<std::string> warnings; ///< Non-fatal parse warnings.

    /// Returns whether at least one dialogue stage was parsed.
    bool valid() const { return !stages.empty(); }
};

/// Parses an NPC dialogue file from the current stream position.
ParseResult parse(std::istream &input);

} // namespace NPCDialogueFormat
