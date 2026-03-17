#include "../src/data/Pokedex.h"
#include "../src/save/SaveManager.h"
#include "../src/state/Map.h"
#include "../src/state/NPC.h"
#include "../src/state/World.h"
#include <cassert>
#include <filesystem>
#include <memory>

namespace {

Daemon makeDaemon(const Pokedex &pokedex, int speciesId, int level) {
    return Daemon(pokedex.getSpecies(speciesId), level);
}

} // namespace

int main() {
    std::filesystem::path assetRoot = "assets";
    if (!std::filesystem::exists(assetRoot / "data" / "species.txt"))
        assetRoot = std::filesystem::path("..") / "assets";

    Pokedex pokedex;
    pokedex.loadSpecies((assetRoot / "data" / "species.txt").string());
    pokedex.loadMoves((assetRoot / "data" / "moves.txt").string());
    pokedex.loadItems((assetRoot / "data" / "items.txt").string());

    World world(42);
    world.addMap("lab", Map("lab", 8, 8));
    world.addMap("other", Map("other", 8, 8));
    world.setCurrentMap("lab");

    auto trainer = std::make_unique<NPC>("trainer_1", "Trainer", Position{2, 2}, NPCType::trainer);
    trainer->setDefeated(true);
    world.addNPC("lab", std::move(trainer));

    Player original("Ada", {3, 4});
    original.setFacing(Direction::left);
    original.addItem(5, 7);
    original.addBadge("badge_alpha");
    original.setFlag("intro_done");
    original.markSeen(1);
    original.markSeen(3);
    original.markCaught(1);
    original.restorePartyDaemon(makeDaemon(pokedex, 1, 5));
    original.restorePartyDaemon(makeDaemon(pokedex, 2, 6));
    assert(original.restoreBoxDaemon(3, makeDaemon(pokedex, 3, 8)));
    original.setCurrentBox(3);
    world.setPlayer(std::move(original));
    world.getMap("lab").setOccupied(world.getPlayer().getPosition(), true);

    SaveManager saveManager;
    const auto path =
        std::filesystem::temp_directory_path() / "cipherbound_save_manager_roundtrip.sav";
    assert(saveManager.saveGame(path.string(), world.getPlayer(), world));

    world.setCurrentMap("other");
    world.getPlayer().clearParty();
    world.getPlayer().clearPCBoxes();
    world.getPlayer().clearInventory();
    world.getPlayer().clearFlags();
    world.getPlayer().clearBadges();
    world.getPlayer().clearDaemondex();
    world.findNPCById("lab", "trainer_1")->setDefeated(false);

    assert(saveManager.loadGame(path.string(), world.getPlayer(), world, pokedex));

    assert(world.getCurrentMapId() == "lab");
    assert(world.getPlayer().getName() == "Ada");
    assert((world.getPlayer().getPosition() == Position{3, 4}));
    assert(world.getPlayer().getFacing() == Direction::left);
    assert(world.getPlayer().getItemCount(5) == 7);
    assert(world.getPlayer().hasBadge("badge_alpha"));
    assert(world.getPlayer().hasFlag("intro_done"));
    assert(world.getPlayer().partySize() == 2);
    assert(world.getPlayer().getBoxCount(0) == 0);
    assert(world.getPlayer().getBoxCount(3) == 1);
    assert(world.getPlayer().getCurrentBox() == 3);
    assert(world.getPlayer().hasSeen(3));
    assert(world.getPlayer().hasCaught(1));
    assert(world.findNPCById("lab", "trainer_1")->isDefeated());
    assert(world.getMap("lab").getTile({3, 4}).isOccupied);

    std::filesystem::remove(path);
}
