#include "TransitionMode.h"
#include "../../audio/MusicManager.h"
#include "../../state/World.h"
#include "../../state/player/Player.h"
#include "../../story/StoryManager.h"
#include "../../ui/GameUI.h"
#include "../../ui/InputManager.h"
#include "../../ui/Renderer.h"
#include <algorithm>

TransitionMode::TransitionMode(const std::string &targetMapId, const Position &targetSpawn)
    : kind(Kind::mapWarp), targetMapId(targetMapId), targetSpawn(targetSpawn) {}

TransitionMode::TransitionMode(Kind kind) : kind(kind) {}

void TransitionMode::onEnter(GameContext &ctx) {
    phase = Phase::fadingOut;
    phaseFrame = 0;
    phaseDuration = ctx.world.getPlayer().getMoveDelay();
    renderBattleBackdrop = (kind == Kind::blackoutRespawn && ctx.hasBattle());
}

void TransitionMode::completeFadeOut(GameContext &ctx) {
    if (kind == Kind::blackoutRespawn) {
        ctx.clearBattle();
        ctx.world.respawnPlayerAfterBlackout();
        renderBattleBackdrop = false;
    } else {
        ctx.world.setCurrentMap(targetMapId);
        ctx.world.getPlayer().setPosition(targetSpawn);
        ctx.world.getPlayer().startAnimation(ctx.world.getPlayer().getFacing());

        // Check if entering this map triggers a cutscene
        StoryAction mapAction =
            ctx.story.checkMapEnter(ctx.world.getCurrentMapId(), ctx.world.getPlayer());
        if (!mapAction.is<StoryNoAction>())
            ctx.pushRequest(ModeRequest::storyAction(mapAction));
    }

    phase = Phase::fadingIn;
    phaseFrame = 0;
    phaseDuration = ctx.world.getPlayer().getMoveDelay();

    MusicTrack mapTrack = MusicManager::trackForMap(ctx.world.getCurrentMapId());
    ctx.music.play(mapTrack, ctx.ui.getRenderer().getWindow());
}

void TransitionMode::update(GameContext &ctx, InputManager & /*input*/) {
    ctx.world.getPlayer().updateAnimation();

    ++phaseFrame;
    if (phaseFrame < phaseDuration)
        return;

    if (phase == Phase::fadingOut) {
        completeFadeOut(ctx);
    } else {
        ctx.pushRequest(ModeRequest::changeState(GameState::overworld));
    }
}

void TransitionMode::render(GameContext &ctx) {
    if (renderBattleBackdrop && ctx.hasBattle()) {
        battleRenderer.drawBattleScene(ctx.ui, ctx.battle(), ctx.battlePresentation(), 0, 0, false);
    } else {
        renderOverworld(ctx);
    }
    Renderer &r = ctx.ui.getRenderer();
    const float progress =
        static_cast<float>(phaseFrame) / static_cast<float>(std::max(1, phaseDuration));
    const int alpha = phase == Phase::fadingOut ? static_cast<int>(255.0f * progress)
                                                : static_cast<int>(255.0f * (1.0f - progress));
    r.drawFilledRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
                     TDT4102::Color{0, 0, 0, static_cast<unsigned char>(alpha)});
}
