#include "../src/game_data/Species.h"
#include "../src/state/Map.h"
#include "../src/state/World.h"
#include <cassert>
#include <string>

namespace {

Species makeSpecies(int id, const std::string &name) {
    Species species{};
    species.id = id;
    species.name = name;
    species.primaryType = ElementType::classical;
    species.secondaryType = ElementType::logic;
    species.baseStats = {20, 10, 10, 10, 10, 10};
    species.catchRate = 120;
    species.baseExpYield = 64;
    return species;
}

} // namespace

int main() {
    World world(42);
    world.addMap("bedroom", Map("bedroom", 8, 8));
    world.addMap("center", Map("center", 8, 8));
    world.addMap("route", Map("route", 8, 8));

    Player player("Ada", {4, 4});
    player.restorePartyDaemon(Daemon(makeSpecies(1, "Starter"), 5));
    world.setPlayer(std::move(player));

    world.setCurrentMap("route");
    world.getMap("route").setOccupied(world.getPlayer().getPosition(), true);
    world.setDefaultRespawnPoint("bedroom", {1, 2}, Direction::left);

    world.getPlayer().getDaemon(0).takeDamage(world.getPlayer().getDaemon(0).getCurrentHP());
    world.respawnPlayerAfterBlackout();

    assert(world.getCurrentMapId() == "bedroom");
    assert((world.getPlayer().getPosition() == Position{1, 2}));
    assert(world.getPlayer().getFacing() == Direction::left);
    assert(world.getPlayer().getDaemon(0).getCurrentHP() == world.getPlayer().getDaemon(0).getMaxHP());

    world.getMap("bedroom").setOccupied(world.getPlayer().getPosition(), false);
    world.setCurrentMap("center");
    world.getPlayer().setPosition({3, 3});
    world.getPlayer().setFacing(Direction::up);
    world.getMap("center").setOccupied(world.getPlayer().getPosition(), true);
    world.markHealingCenterUsed(world.getPlayer().getPosition(), world.getPlayer().getFacing());

    world.getMap("center").setOccupied(world.getPlayer().getPosition(), false);
    world.setCurrentMap("route");
    world.getPlayer().setPosition({5, 5});
    world.getPlayer().setFacing(Direction::down);
    world.getMap("route").setOccupied(world.getPlayer().getPosition(), true);
    world.getPlayer().getDaemon(0).takeDamage(world.getPlayer().getDaemon(0).getCurrentHP());
    world.respawnPlayerAfterBlackout();

    assert(world.getCurrentMapId() == "center");
    assert((world.getPlayer().getPosition() == Position{3, 3}));
    assert(world.getPlayer().getFacing() == Direction::up);
    assert(world.getPlayer().getDaemon(0).getCurrentHP() == world.getPlayer().getDaemon(0).getMaxHP());
}
