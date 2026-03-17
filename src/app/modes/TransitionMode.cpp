#include "TransitionMode.h"
#include "../../audio/MusicManager.h"
#include "../../state/World.h"
#include "../../state/player/Player.h"
#include "../../ui/GameUI.h"
#include "../../ui/InputManager.h"
#include "../../ui/Renderer.h"
#include "../../story/StoryManager.h"

TransitionMode::TransitionMode(const std::string &targetMapId, const Position &targetSpawn)
    : targetMapId(targetMapId), targetSpawn(targetSpawn) {}

void TransitionMode::update(GameContext &ctx, InputManager & /*input*/) {
    ctx.world.getPlayer().updateAnimation();
    int animFrames = ctx.world.getPlayer().getAnimationFrame();

    if (animFrames == 0) {
        if (targetMapId.empty()) {
            ctx.pushRequest(ModeRequest::changeState(GameState::overworld));
        } else {
            // Load the new map
            ctx.world.setCurrentMap(targetMapId);
            ctx.world.getPlayer().setPosition(targetSpawn);
            ctx.world.getPlayer().startAnimation(ctx.world.getPlayer().getFacing());
            fadeOut = false;

            MusicTrack mapTrack = MusicManager::trackForMap(targetMapId);
            ctx.music.play(mapTrack, ctx.ui.getRenderer().getWindow());

            // Check if entering this map triggers a cutscene
            StoryAction mapAction =
                ctx.story.checkMapEnter(ctx.world.getCurrentMapId(), ctx.world.getPlayer());
            if (!mapAction.is<StoryNoAction>())
                ctx.pushRequest(ModeRequest::storyAction(mapAction));

            targetMapId.clear();
        }
    }
}

void TransitionMode::render(GameContext &ctx) {
    renderOverworld(ctx);
    Renderer &r = ctx.ui.getRenderer();
    int moveDelay = ctx.world.getPlayer().getMoveDelay();
    int transitionAlpha =
        static_cast<int>(static_cast<float>(ctx.world.getPlayer().getAnimationFrame()) /
                         static_cast<float>(moveDelay) * 255.0f);
    int alpha = fadeOut ? 255 - transitionAlpha : transitionAlpha;
    r.drawFilledRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
                     TDT4102::Color{0, 0, 0, static_cast<unsigned char>(alpha)});
}
