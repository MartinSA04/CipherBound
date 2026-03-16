#include "DialogueChoiceMode.h"
#include "../../ui/InputManager.h"
#include "../../ui/GameUI.h"
#include "../../state/World.h"
#include "../../data/Pokedex.h"
#include "../StoryManager.h"
#include "../../audio/SoundManager.h"

DialogueChoiceMode::DialogueChoiceMode(const std::vector<std::string> &options,
                                       const std::string &context,
                                       GameState returnState)
    : choiceOptions(options),
      choiceContext(context),
      returnState(returnState)
{
}

void DialogueChoiceMode::update(GameContext &ctx, InputManager &input)
{
    int count = static_cast<int>(choiceOptions.size());
    ctx.ui.navigateVertical(choiceSelected, count);

    if (input.isConfirmPressed())
    {
        ctx.playSound(SoundEffect::select);
        StoryAction action = ctx.story.onChoiceSelected(choiceContext, choiceSelected,
                                                        ctx.world, ctx.pokedex);
        ctx.pushRequest(ModeRequest::storyAction(action));
    }
}

void DialogueChoiceMode::render(GameContext &ctx)
{
    if (returnState == GameState::overworld)
        renderOverworld(ctx);

    // If choosing a starter pokeball, show the Daemon sprite above the dialogue
    if (choiceContext.starts_with("pokeball_"))
    {
        int speciesId = std::stoi(choiceContext.substr(9));
        if (speciesId > 0 && speciesId <= ctx.pokedex.speciesCount())
        {
            const Species &sp = ctx.pokedex.getSpecies(speciesId);
            ctx.ui.loadDaemonSprite(sp.name);
            std::string spriteId = "daemon_" + sp.name;

            Renderer &r = ctx.ui.getRenderer();
            if (r.hasTexture(spriteId))
            {
                int spriteScale = 3; // 80 * 3 = 240px
                int spriteSize = 80 * spriteScale;
                int spriteX = WINDOW_WIDTH / 2 - spriteSize / 2;
                int panelY = WINDOW_HEIGHT - UI_PANEL_HEIGHT;
                int spriteY = panelY - spriteSize - 8;

                r.drawSpriteRaw(spriteId, spriteX, spriteY, spriteSize, spriteSize);

                // Draw name + type label below sprite
                int labelY = spriteY + spriteSize + 4;
                ctx.ui.getSpriteFont().drawText(r, sp.name,
                    WINDOW_WIDTH / 2 - static_cast<int>(sp.name.size()) * 8 * PIXEL_SCALE / 2,
                    labelY, PIXEL_SCALE);
            }
        }
    }

    // Draw the last dialogue line underneath
    if (ctx.ui.isDialogueActive())
        ctx.ui.drawDialogueBox(ctx.ui.getDialogueSpeaker(), ctx.ui.getCurrentDialogueLine());

    // Draw the choice box on top
    ctx.ui.drawChoiceBox(choiceOptions, choiceSelected);
}
