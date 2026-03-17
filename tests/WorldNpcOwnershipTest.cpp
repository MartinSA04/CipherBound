#include "../src/game_data/Species.h"
#include "../src/state/Map.h"
#include "../src/state/World.h"
#include <cassert>
#include <memory>
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
    World world(123);
    world.addMap("town", Map("town", 8, 8));
    world.setCurrentMap("town");

    auto trainer =
        std::make_unique<NPC>("npc_trainer", "Trainer", Position{2, 2}, NPCType::trainer);
    trainer->setFacing(Direction::down);
    trainer->setSightRange(4);
    trainer->addDaemon(Daemon(makeSpecies(1, "Alpha"), 5));
    NPC *trainerPtr = trainer.get();
    world.addNPC("town", std::move(trainer));

    for (int i = 0; i < 8; ++i) {
        auto bystander = std::make_unique<NPC>("npc_" + std::to_string(i), "Bystander",
                                               Position{4 + (i % 2), i / 2}, NPCType::normal);
        world.addNPC("town", std::move(bystander));
    }

    assert(world.getNPCs("town").size() == 9);
    assert(world.findNPCById("npc_trainer") == trainerPtr);
    assert(world.findNPCById("town", "npc_trainer") == trainerPtr);
    assert(world.findNPCAt("town", Position{2, 2}) == trainerPtr);

    world.getPlayer().setPosition({2, 5});
    assert(world.NPCSeeingPlayer() == trainerPtr);

    world.setNPCDefeated("npc_trainer");
    assert(world.getPlayer().hasFlag("defeated_npc_trainer"));
    assert(trainerPtr->isDefeated());
    assert(world.NPCSeeingPlayer() == nullptr);

    return 0;
}
