#pragma once

#include "../game_data/Cutscene.h"

class NPC;
class Player;

namespace TrainerApproachCutscene {

Cutscene build(const NPC &trainer, const Player &player, bool includePreBattleDialogue = false);

} // namespace TrainerApproachCutscene
