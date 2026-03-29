#include "../src/state/world/NPCDialogueFormat.h"
#include <cassert>
#include <sstream>

int main() {
    std::istringstream input(R"(# default lines can appear before a header
Hello there.
Mind the archive dust.

[flag quest_done]
You made it back.
Keep the badge visible.

[default]
Fallback line.
)");

    const auto parsed = NPCDialogueFormat::parse(input);
    assert(parsed.valid());
    assert(parsed.warnings.empty());
    assert(parsed.stages.size() == 3);

    assert(parsed.stages[0].requiredFlag.empty());
    assert(parsed.stages[0].lines.size() == 2);
    assert(parsed.stages[0].lines[0] == "Hello there.");
    assert(parsed.stages[0].lines[1] == "Mind the archive dust.");

    assert(parsed.stages[1].requiredFlag == "quest_done");
    assert(parsed.stages[1].lines.size() == 2);
    assert(parsed.stages[1].lines[0] == "You made it back.");

    assert(parsed.stages[2].requiredFlag.empty());
    assert(parsed.stages[2].lines.size() == 1);
    assert(parsed.stages[2].lines[0] == "Fallback line.");
}
