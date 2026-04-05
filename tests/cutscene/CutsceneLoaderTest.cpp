#include "../../src/cutscene/CutsceneLoader.h"
#include <cassert>

int main() {
    const char *paths[] = {
        "assets/data/cutscenes/bart_iver_intro.cutscene",
        "assets/data/cutscenes/first_faculty_aftermath.cutscene",
        "assets/data/cutscenes/rival_intercept.cutscene",
        "assets/data/cutscenes/bart_concordance_reveal.cutscene",
        "assets/data/cutscenes/academy_archive.cutscene",
        "assets/data/cutscenes/pewter_arrival.cutscene",
        "assets/data/cutscenes/natural_sciences_lockdown.cutscene",
        "assets/data/cutscenes/applied_physics_aftermath.cutscene",
        "assets/data/cutscenes/resonance_lab_reveal.cutscene",
        "assets/data/cutscenes/bart_resonance_followup.cutscene",
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
