#include "../../src/cutscene/CutsceneLoader.h"
#include <cassert>

int main() {
    const auto loaded = CutsceneLoader::load("assets/data/cutscenes/bart_iver_intro.cutscene");
    assert(loaded.opened);
    assert(loaded.valid());
    assert(!loaded.resolvedPath.empty());
    assert(loaded.cutscene.id == "bart_iver_intro");
    assert(!loaded.cutscene.steps.empty());
}
