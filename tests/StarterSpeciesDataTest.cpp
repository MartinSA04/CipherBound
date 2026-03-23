#include "../src/common/FilePaths.h"
#include "../src/game_data/Pokedex.h"
#include <cassert>

int main() {
    const std::filesystem::path dataRoot = FilePaths::resolveExistingPath("assets/data");

    Pokedex pokedex;
    pokedex.loadSpecies((dataRoot / "species.txt").string());

    const Species &abacub = pokedex.getSpecies(4);
    assert(abacub.name == "Abacub");
    assert(abacub.primaryType == ElementType::algebraic);
    assert(abacub.secondaryType == ElementType::algebraic);
    assert(abacub.evolutions.size() == 1);
    assert(abacub.evolutions[0].targetSpeciesId == 5);
    assert(abacub.evolutions[0].levelRequired == 16);

    const Species &algursa = pokedex.getSpecies(5);
    assert(algursa.name == "Algursa");
    assert(algursa.evolutions.size() == 1);
    assert(algursa.evolutions[0].targetSpeciesId == 6);
    assert(algursa.evolutions[0].levelRequired == 32);

    const Species &theoremaul = pokedex.getSpecies(6);
    assert(theoremaul.name == "Theoremaul");
    assert(theoremaul.primaryType == ElementType::algebraic);
    assert(theoremaul.secondaryType == ElementType::logic);
    assert(theoremaul.evolutions.empty());

    const Species &bugbit = pokedex.getSpecies(7);
    assert(bugbit.name == "Bugbit");
    assert(bugbit.primaryType == ElementType::digital);
    assert(bugbit.secondaryType == ElementType::digital);
    assert(bugbit.evolutions.size() == 1);
    assert(bugbit.evolutions[0].targetSpeciesId == 8);
    assert(bugbit.evolutions[0].levelRequired == 16);

    const Species &glitchid = pokedex.getSpecies(8);
    assert(glitchid.name == "Glitchid");
    assert(glitchid.evolutions.size() == 1);
    assert(glitchid.evolutions[0].targetSpeciesId == 9);
    assert(glitchid.evolutions[0].levelRequired == 32);

    const Species &fatalisk = pokedex.getSpecies(9);
    assert(fatalisk.name == "Fatalisk");
    assert(fatalisk.primaryType == ElementType::digital);
    assert(fatalisk.secondaryType == ElementType::chaotic);
    assert(fatalisk.evolutions.empty());

    return 0;
}
