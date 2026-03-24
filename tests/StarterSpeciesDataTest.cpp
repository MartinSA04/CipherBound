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

    const Species &fibonib = pokedex.getSpecies(10);
    assert(fibonib.name == "Fibonib");
    assert(fibonib.primaryType == ElementType::recursive);
    assert(fibonib.secondaryType == ElementType::recursive);
    assert(fibonib.evolutions.size() == 1);
    assert(fibonib.evolutions[0].targetSpeciesId == 11);
    assert(fibonib.evolutions[0].levelRequired == 16);

    const Species &rabbonacci = pokedex.getSpecies(11);
    assert(rabbonacci.name == "Rabbonacci");
    assert(rabbonacci.primaryType == ElementType::recursive);
    assert(rabbonacci.secondaryType == ElementType::recursive);
    assert(rabbonacci.evolutions.size() == 1);
    assert(rabbonacci.evolutions[0].targetSpeciesId == 12);
    assert(rabbonacci.evolutions[0].levelRequired == 32);

    const Species &mandelhare = pokedex.getSpecies(12);
    assert(mandelhare.name == "Mandelhare");
    assert(mandelhare.primaryType == ElementType::recursive);
    assert(mandelhare.secondaryType == ElementType::chaotic);
    assert(mandelhare.evolutions.empty());

    const Species &isotad = pokedex.getSpecies(13);
    assert(isotad.name == "Isotad");
    assert(isotad.primaryType == ElementType::radioactive);
    assert(isotad.secondaryType == ElementType::radioactive);
    assert(isotad.evolutions.size() == 1);
    assert(isotad.evolutions[0].targetSpeciesId == 14);
    assert(isotad.evolutions[0].levelRequired == 16);

    const Species &raditoad = pokedex.getSpecies(14);
    assert(raditoad.name == "Raditoad");
    assert(raditoad.primaryType == ElementType::radioactive);
    assert(raditoad.secondaryType == ElementType::nuclear);
    assert(raditoad.evolutions.size() == 1);
    assert(raditoad.evolutions[0].targetSpeciesId == 15);
    assert(raditoad.evolutions[0].levelRequired == 32);

    const Species &meltoad = pokedex.getSpecies(15);
    assert(meltoad.name == "Meltoad");
    assert(meltoad.primaryType == ElementType::nuclear);
    assert(meltoad.secondaryType == ElementType::thermal);
    assert(meltoad.evolutions.empty());

    const Species &ohmlet = pokedex.getSpecies(16);
    assert(ohmlet.name == "Ohmlet");
    assert(ohmlet.primaryType == ElementType::electromagnetic);
    assert(ohmlet.secondaryType == ElementType::electromagnetic);
    assert(ohmlet.evolutions.size() == 1);
    assert(ohmlet.evolutions[0].targetSpeciesId == 17);
    assert(ohmlet.evolutions[0].levelRequired == 16);

    const Species &voltridge = pokedex.getSpecies(17);
    assert(voltridge.name == "Voltridge");
    assert(voltridge.primaryType == ElementType::electromagnetic);
    assert(voltridge.secondaryType == ElementType::electromagnetic);
    assert(voltridge.evolutions.size() == 1);
    assert(voltridge.evolutions[0].targetSpeciesId == 18);
    assert(voltridge.evolutions[0].levelRequired == 32);

    const Species &ampheasant = pokedex.getSpecies(18);
    assert(ampheasant.name == "Ampheasant");
    assert(ampheasant.primaryType == ElementType::electromagnetic);
    assert(ampheasant.secondaryType == ElementType::quantum);
    assert(ampheasant.evolutions.empty());

    return 0;
}
