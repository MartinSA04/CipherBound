#include "../src/core/CutsceneFormat.h"
#include <cassert>
#include <sstream>

int main() {
    std::istringstream input(R"([header]
id|intro_scene
[steps]
move|player|3|4
say|Professor|Line one;Line two
wait|30
sync
bad_step|ignored
)");

    const CutsceneFormat::ParseResult parsed = CutsceneFormat::parse(input);
    assert(parsed.valid());
    assert(parsed.warnings.size() == 1);
    assert(parsed.cutscene.id == "intro_scene");
    assert(parsed.cutscene.steps.size() == 4);

    assert(parsed.cutscene.steps[0].type == CutsceneStep::Type::move);
    assert(parsed.cutscene.steps[0].target == "player");
    assert(parsed.cutscene.steps[0].x == 3);
    assert(parsed.cutscene.steps[0].y == 4);

    assert(parsed.cutscene.steps[1].type == CutsceneStep::Type::say);
    assert(parsed.cutscene.steps[1].speaker == "Professor");
    assert(parsed.cutscene.steps[1].lines.size() == 2);

    assert(parsed.cutscene.steps[2].type == CutsceneStep::Type::wait);
    assert(parsed.cutscene.steps[2].frames == 30);
    assert(parsed.cutscene.steps[3].type == CutsceneStep::Type::sync);
}
