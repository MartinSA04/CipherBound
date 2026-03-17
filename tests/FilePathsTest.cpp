#include "../src/core/CutsceneFormat.h"
#include "../src/core/FilePaths.h"
#include <cassert>
#include <fstream>

int main() {
    const auto path = FilePaths::resolveExistingPath("assets/data/cutscenes/bart_iver_intro.cutscene");
    assert(!path.empty());
    assert(std::filesystem::exists(path));

    std::ifstream input(path);
    assert(input.is_open());

    const auto parsed = CutsceneFormat::parse(input);
    assert(parsed.valid());
    assert(parsed.cutscene.id == "bart_iver_intro");
}
