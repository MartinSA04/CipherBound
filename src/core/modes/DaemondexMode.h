#pragma once
#include "../GameMode.h"

class DaemondexMode : public GameMode
{
public:
    void update(GameContext &ctx, InputManager &input) override;
    void render(GameContext &ctx) override;

private:
    enum class SubState
    {
        list,
        detail,
    };

    SubState subState{SubState::list};
    int selected{0};
    int scrollOffset{0};

    static constexpr int VISIBLE_ROWS = 8;

    void drawList(GameContext &ctx);
    void drawDetail(GameContext &ctx);
};
