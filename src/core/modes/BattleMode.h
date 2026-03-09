#pragma once
#include "../GameMode.h"
#include <string>
#include <memory>

class Battle;
class NPC;

class BattleMode : public GameMode
{
public:
    void update(GameContext &ctx, InputManager &input) override;
    void render(GameContext &ctx) override;

    void setTrainerNPCId(const std::string &id);
    void setTrainer(std::shared_ptr<NPC> trainer);
    const std::string &getTrainerNPCId() const;

private:
    void updateBattleIntroAnim(GameContext &ctx);

    std::string currentTrainerNPCId;
    std::shared_ptr<NPC> battleTrainer;

    int menuSelected{0};
    int moveSelected{0};
    int partySelected{0};
    int bagSelected{0};
};
