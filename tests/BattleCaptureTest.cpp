#include "../src/battle/BattleCapture.h"
#include "../src/game_data/Item.h"
#include "../src/game_data/Species.h"
#include "../src/state/Daemon.h"
#include <array>
#include <cassert>
#include <random>
#include <string>

namespace {

Species makeSpecies() {
    Species species{};
    species.id = 7;
    species.name = "Bound";
    species.primaryType = ElementType::classical;
    species.secondaryType = ElementType::logic;
    species.baseStats = {30, 15, 15, 15, 15, 15};
    species.catchRate = 255;
    species.baseExpYield = 64;
    return species;
}

} // namespace

int main() {
    const Species species = makeSpecies();
    std::array<MoveSlot, 4> moves = {{{1, 5, 5}, {2, 10, 10}, {-1, 0, 0}, {-1, 0, 0}}};
    Daemon target(species, 8, 120, 9, "Cipher", StatusEffect::entangled, {1, 2, 3, 4, 5, 6},
                  {6, 5, 4, 3, 2, 1}, moves);

    ItemData ultraBall{1, "Ultra Ball", "", ItemCategory::capture, 0, 255};
    std::mt19937 rng(42);

    const BattleCaptureOutcome outcome = BattleCapture::resolve(target, ultraBall, rng);
    assert(outcome.success);
    assert(outcome.shakes == 4);

    Daemon caught = BattleCapture::caughtDaemon(target);
    assert(caught.getSpeciesId() == target.getSpeciesId());
    assert(caught.getLevel() == target.getLevel());
    assert(caught.getExp() == target.getExp());
    assert(caught.getCurrentHP() == target.getCurrentHP());
    assert(caught.getNickname() == target.getNickname());
    assert(caught.getStatus() == target.getStatus());
    assert(caught.getMoves()[0].moveId == target.getMoves()[0].moveId);

    return 0;
}
