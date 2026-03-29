#include "../../src/cutscene/CutsceneLoader.h"
#include <cassert>

int main() {
    const char *paths[] = {
        "assets/data/cutscenes/bart_iver_intro.cutscene",
        "assets/data/cutscenes/first_faculty_aftermath.cutscene",
        "assets/data/cutscenes/rival_intercept.cutscene",
        "assets/data/cutscenes/bart_concordance_reveal.cutscene",
        "assets/data/cutscenes/academy_archive.cutscene",
    };

    for (const char *path : paths) {
        const auto loaded = CutsceneLoader::load(path);
        assert(loaded.opened);
        assert(loaded.valid());
        assert(!loaded.resolvedPath.empty());
        assert(!loaded.cutscene.id.empty());
        assert(!loaded.cutscene.steps.empty());
    }
}
