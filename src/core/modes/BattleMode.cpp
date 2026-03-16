#include "BattleMode.h"

void BattleMode::setTrainerNPCId(const std::string &id) { currentTrainerNPCId = id; }

void BattleMode::setTrainer(NPC *trainer) { battleTrainer = trainer; }

const std::string &BattleMode::getTrainerNPCId() const { return currentTrainerNPCId; }
