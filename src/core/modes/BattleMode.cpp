#include "BattleMode.h"

void BattleMode::setTrainerNPCId(const std::string &id) { currentTrainerNPCId = id; }

void BattleMode::setTrainer(std::shared_ptr<NPC> trainer) { battleTrainer = std::move(trainer); }

const std::string &BattleMode::getTrainerNPCId() const { return currentTrainerNPCId; }
