#include "../src/game_data/Species.h"
#include "../src/state/NPC.h"
#include "../src/state/player/Party.h"
#include "../src/state/player/Player.h"
#include <array>
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
    const std::array<Species, 7> species = {
        makeSpecies(1, "One"),   makeSpecies(2, "Two"),  makeSpecies(3, "Three"),
        makeSpecies(4, "Four"),  makeSpecies(5, "Five"), makeSpecies(6, "Six"),
        makeSpecies(7, "Seven"),
    };

    PartyAndPCBoxes storage;
    storage.addDaemon(Daemon(species[0], 5));
    storage.addDaemon(Daemon(species[1], 5));
    assert(storage.partySize() == 2);
    assert(storage.canDeposit());

    storage.depositDaemon(1);
    assert(storage.partySize() == 1);
    assert(storage.getBoxCount(0) == 1);

    storage.withdrawDaemon(0, 0);
    assert(storage.partySize() == 2);
    assert(storage.getBoxCount(0) == 0);

    Player player("Tester", {1, 1});
    player.addItem(42, 3);
    assert(player.getItemCount(42) == 3);
    assert(player.removeItem(42, 2));
    assert(player.getItemCount(42) == 1);

    player.addMoney(250);
    assert(player.getMoney() == 250);
    assert(player.spendMoney(100));
    assert(player.getMoney() == 150);

    player.setFlag("started");
    player.addBadge("badge_alpha");
    assert(player.hasFlag("started"));
    assert(player.hasBadge("badge_alpha"));
    assert(player.badgeCount() == 1);

    for (const Species &entry : species)
        player.addDaemon(Daemon(entry, 5));

    assert(player.partySize() == 6);
    assert(player.getBoxCount(0) == 1);
    assert(player.caughtCount() == 7);
    assert(player.hasCaught(7));
    assert(player.getDaemon(0).getSpeciesId() == 1);

    player.setRespawnPoint("healing_center", {4, 4}, Direction::right);
    assert(player.hasRespawnPoint());
    assert(player.getRespawnMapId() == "healing_center");
    assert((player.getRespawnPosition() == Position{4, 4}));
    assert(player.getRespawnFacing() == Direction::right);

    player.getDaemon(0).takeDamage(player.getDaemon(0).getCurrentHP());
    assert(player.findFirstUsableDaemonIndex() == 1);
    assert(player.hasUsableDaemons());
    assert(!player.allDaemonsFainted());

    for (int i = 1; i < player.partySize(); ++i)
        player.getDaemon(i).takeDamage(player.getDaemon(i).getCurrentHP());
    assert(player.findFirstUsableDaemonIndex() == -1);
    assert(player.allDaemonsFainted());

    player.fullHealParty();
    assert(player.hasUsableDaemons());
    assert(!player.allDaemonsFainted());
    assert(player.getDaemon(0).getCurrentHP() == player.getDaemon(0).getMaxHP());

    player.clearRespawnPoint();
    assert(!player.hasRespawnPoint());

    player.clearBadges();
    player.clearFlags();
    assert(player.getBadges().empty());
    assert(player.getFlags().empty());

    NPC trainer("npc_1", "Trainer", {2, 2}, NPCType::trainer);
    assert(trainer.getSpriteType().empty());
    assert(!trainer.willFight());
    trainer.addDaemon(Daemon(species[0], 5));
    assert(trainer.partySize() == 1);
    assert(trainer.willFight());
    trainer.setDefeated(true);
    assert(!trainer.willFight());
    trainer.setHidden(true);
    assert(trainer.isHidden());

    NPC villager("npc_2", "Villager", {1, 2}, NPCType::normal, "villager_f");
    assert(villager.getSpriteType() == "villager_f");

    return 0;
}
