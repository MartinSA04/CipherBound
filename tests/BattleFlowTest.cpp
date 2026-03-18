#include "../src/battle/Battle.h"
#include "../src/battle/BattleRules.h"
#include "../src/game_data/Pokedex.h"
#include "../src/state/player/Player.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <filesystem>
#include <memory>
#include <random>
#include <string>

namespace {

std::filesystem::path assetRoot() {
    std::filesystem::path root = "assets";
    if (!std::filesystem::exists(root / "data" / "species.txt"))
        root = std::filesystem::path("..") / "assets";
    return root;
}

Pokedex loadPokedex() {
    Pokedex pokedex;
    const std::filesystem::path root = assetRoot();
    pokedex.loadSpecies((root / "data" / "species.txt").string());
    pokedex.loadMoves((root / "data" / "moves.txt").string());
    pokedex.loadItems((root / "data" / "items.txt").string());
    return pokedex;
}

std::array<MoveSlot, 4> singleMoveSet(int moveId) {
    return {{{moveId, 15, 15}, {-1, 0, 0}, {-1, 0, 0}, {-1, 0, 0}}};
}

Daemon makeDaemon(const Pokedex &pokedex, int speciesId, int level, int moveId, int currentHP = -1,
                  const std::string &nickname = {}) {
    const Species &species = pokedex.getSpecies(speciesId);
    const Daemon base(species, level);
    const int hp = currentHP >= 0 ? std::min(currentHP, base.getMaxHP()) : base.getCurrentHP();
    return Daemon(species, level, base.getExp(), hp, nickname.empty() ? species.name : nickname,
                  StatusEffect::none, {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0},
                  singleMoveSet(moveId));
}

void drainBattle(Battle &battle) {
    for (int guard = 0; guard < 128; ++guard) {
        switch (battle.getState()) {
        case BattleState::intro:
            battle.finishIntroAnimation();
            break;
        case BattleState::showingMessages:
            battle.advanceMessage();
            break;
        case BattleState::animatingHP:
            battle.finishHPAnimation();
            break;
        case BattleState::animatingEXP:
            battle.finishEXPAnimation();
            break;
        case BattleState::animatingCapture:
            battle.finishCaptureAnimation();
            break;
        case BattleState::animatingAttack:
            battle.finishAttackAnimation();
            break;
        default:
            return;
        }
    }

    assert(false && "Battle did not settle");
}

} // namespace

int main() {
    const Pokedex pokedex = loadPokedex();
    std::mt19937 rng(123);

    Player winner("Winner", {0, 0});
    winner.restorePartyDaemon(makeDaemon(pokedex, 3, 20, 7));
    const Daemon rewardTarget = makeDaemon(pokedex, 1, 1, 1, 1, "Target");
    const int expectedMoney = BattleRules::calculateMoneyReward(rewardTarget, BattleType::wild);

    Battle winBattle(winner, std::make_unique<Daemon>(rewardTarget), BattleType::wild, rng, pokedex);
    winBattle.start();
    drainBattle(winBattle);
    assert(winBattle.getState() == BattleState::choosingAction);

    winBattle.chooseAction(BattleAction::fight);
    winBattle.chooseMove(0);
    drainBattle(winBattle);

    assert(winBattle.getState() == BattleState::victory);
    assert(winBattle.getResult().playerWon);
    assert(winBattle.getResult().moneyGained == expectedMoney);
    assert(winner.getMoney() == expectedMoney);
    assert(expectedMoney == 0);
    assert(winBattle.getResult().expGained > 0);

    Player loser("Loser", {0, 0});
    loser.restorePartyDaemon(makeDaemon(pokedex, 1, 5, 1, 1, "Lead"));
    loser.restorePartyDaemon(makeDaemon(pokedex, 2, 5, 1, 1, "Backup"));

    Battle blackoutBattle(loser, std::make_unique<Daemon>(makeDaemon(pokedex, 3, 40, 7)),
                          BattleType::wild, rng, pokedex);
    blackoutBattle.start();
    drainBattle(blackoutBattle);
    assert(blackoutBattle.getState() == BattleState::choosingAction);

    blackoutBattle.executeOpponentTurn();
    drainBattle(blackoutBattle);
    assert(blackoutBattle.getState() == BattleState::choosingAction);
    assert(loser.getDaemon(0).getNickname() == "Backup");
    assert(!loser.allDaemonsFainted());

    blackoutBattle.executeOpponentTurn();
    drainBattle(blackoutBattle);
    assert(blackoutBattle.getState() == BattleState::defeat);
    assert(loser.allDaemonsFainted());
}
