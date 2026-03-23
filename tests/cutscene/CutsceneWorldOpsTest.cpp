#include "../../src/cutscene/CutsceneWorldOps.h"
#include "../../src/cutscene/CutscenePlayback.h"
#include "../src/state/Map.h"
#include "../src/state/NPC.h"
#include "../src/state/World.h"
#include <cassert>
#include <memory>
#include <vector>

int main() {
    World world(42);
    world.addMap("lab", Map("lab", 6, 6));
    world.setCurrentMap("lab");
    world.setPlayer(Player("Ada", {1, 1}));
    world.getMap("lab").setOccupied({1, 1}, true);
    world.getPlayer().setMoveDelay(1);

    auto npc = std::make_unique<NPC>("guide", "Guide", Position{3, 3}, NPCType::normal);
    npc->setMoveDelay(1);
    world.addNPC("lab", std::move(npc));

    assert((CutsceneWorldOps::tryGetEntityPosition(world, "player") == Position{1, 1}));
    assert((CutsceneWorldOps::tryGetEntityPosition(world, "guide") == Position{3, 3}));
    assert(!CutsceneWorldOps::tryGetEntityPosition(world, "missing").has_value());

    assert((CutsceneWorldOps::adjacentDestination({2, 2}, Direction::up) == Position{2, 1}));

    assert(CutsceneWorldOps::setFacing(world, "guide", Direction::left));
    assert(world.findNPCById("lab", "guide")->getFacing() == Direction::left);
    assert(CutsceneWorldOps::setHidden(world, "guide", true));
    assert(world.findNPCById("lab", "guide")->isHidden());
    assert(CutsceneWorldOps::setHidden(world, "guide", false));
    assert(!world.findNPCById("lab", "guide")->isHidden());

    assert(CutsceneWorldOps::stepEntityToward(world, "player", {2, 1}));
    assert((world.getPlayer().getPosition() == Position{2, 1}));
    assert(world.getMap("lab").getTile({2, 1}).isOccupied);

    assert(CutsceneWorldOps::stepEntityToward(world, "guide", {4, 3}));
    assert((world.findNPCById("lab", "guide")->getPosition() == Position{4, 3}));

    std::vector<CutscenePendingMove> pending = {{"player", {2, 1}}, {"guide", {4, 3}}};
    assert(!CutsceneWorldOps::allMovesComplete(world, pending));
    for (int i = 0; i < 24 && !CutsceneWorldOps::allMovesComplete(world, pending); ++i)
        CutsceneWorldOps::updateAnimations(world);
    assert(CutsceneWorldOps::allMovesComplete(world, pending));

    world.getPlayer().setPosition({2, 2});
    world.getMap("lab").setOccupied({2, 1}, false);
    world.getMap("lab").setOccupied({2, 2}, true);
    world.getPlayer().setMoveDelay(1);
    std::vector<CutscenePendingMove> chained = {{"player", {3, 2}}, {"player", {4, 2}}};
    CutsceneWorldOps::tickMovements(world, chained);
    assert((world.getPlayer().getPosition() == Position{3, 2}));
}
