#include "DialogueChoiceMode.h"
#include "../../game_data/Pokedex.h"
#include "../../state/World.h"
#include "../../story/StoryManager.h"
#include "../../ui/GameUI.h"
#include "../../ui/InputManager.h"

DialogueChoiceMode::DialogueChoiceMode(const std::vector<std::string> &options,
                                       const std::string &context, GameState returnState)
    : choiceOptions(options), choiceContext(context), returnState(returnState) {}

void DialogueChoiceMode::update(GameContext &ctx, InputManager &input) {
    int count = static_cast<int>(choiceOptions.size());
    ctx.ui.navigateVertical(choiceSelected, count);

    if (input.isConfirmPressed()) {
        ctx.playSound(SoundEffect::select);
        ctx.pushRequest(ModeRequest::storyAction(
            ctx.story.onChoiceSelected(choiceContext, choiceSelected, ctx.world, ctx.pokedex)));
    }
}

void DialogueChoiceMode::render(GameContext &ctx) {
    if (returnState == GameState::overworld)
        renderOverworld(ctx);

    // If choosing a starter pokeball, show the Daemon sprite above the dialogue
    if (const auto speciesId = StoryManager::starterSpeciesIdForChoiceContext(choiceContext);
        speciesId.has_value()) {
        const Species &sp = ctx.pokedex.getSpecies(*speciesId);
        ctx.ui.loadDaemonSprite(sp.name);
        std::string spriteId = "daemon_" + sp.name;

        Renderer &r = ctx.ui.getRenderer();
        if (r.hasTexture(spriteId)) {
            int spriteScale = 3; // 80 * 3 = 240px
            int spriteSize = 80 * spriteScale;
            SpriteFont &font = ctx.ui.getSpriteFont();
            int previewPadding = 3 * PIXEL_SCALE;
            int labelGap = 2 * PIXEL_SCALE;
            int labelHeight = 16 * PIXEL_SCALE;
            int labelWidth = font.getTextWidth(sp.name, PIXEL_SCALE);
            int previewWidth =
                std::max(spriteSize + previewPadding * 2, labelWidth + previewPadding * 2);
            int previewHeight =
                spriteSize + labelGap + labelHeight + previewPadding * 2 + PIXEL_SCALE;
            int previewX = WINDOW_WIDTH / 2 - previewWidth / 2;
            int dialoguePanelY = WINDOW_HEIGHT - UI_PANEL_HEIGHT;
            int previewY = dialoguePanelY - previewHeight - 8;
            int spriteX = WINDOW_WIDTH / 2 - spriteSize / 2;
            int spriteY = previewY + previewPadding;

            r.drawFilledRect(previewX + PIXEL_SCALE, previewY + PIXEL_SCALE, previewWidth,
                             previewHeight, TDT4102::Color{48, 58, 84});
            r.drawFilledRect(previewX, previewY, previewWidth, previewHeight,
                             TDT4102::Color{240, 245, 255});
            r.drawRect(previewX, previewY, previewWidth, previewHeight, TDT4102::Color::transparent,
                       TDT4102::Color{60, 70, 100});

            r.drawSpriteRaw(spriteId, spriteX, spriteY, spriteSize, spriteSize);

            int labelY = spriteY + spriteSize + labelGap;
            int labelX = WINDOW_WIDTH / 2 - font.getTextWidth(sp.name, PIXEL_SCALE) / 2;
            font.drawText(r, sp.name, labelX, labelY, PIXEL_SCALE);
        }
    }

    // Draw the last dialogue line underneath
    if (ctx.ui.isDialogueActive())
        ctx.ui.drawDialogueBox(ctx.ui.getDialogueSpeaker(), ctx.ui.getCurrentDialogueLine());

    // Draw the choice box on top
    ctx.ui.drawChoiceBox(choiceOptions, choiceSelected);
}
