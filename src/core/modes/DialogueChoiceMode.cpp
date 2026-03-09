#include "DialogueChoiceMode.h"
#include "../../ui/InputManager.h"
#include "../../ui/GameUI.h"
#include "../../world/World.h"
#include "../../data/Pokedex.h"
#include "../StoryManager.h"

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
        StoryAction action = ctx.story.onChoiceSelected(choiceContext, choiceSelected,
                                                        ctx.world, ctx.pokedex);
        ctx.pushRequest(ModeRequest::storyAction(action));
    }
}

void DialogueChoiceMode::render(GameContext &ctx)
{
    if (returnState == GameState::overworld)
        renderOverworld(ctx);

    // Draw the last dialogue line underneath
    if (ctx.ui.isDialogueActive())
        ctx.ui.drawDialogueBox(ctx.ui.getDialogueSpeaker(), ctx.ui.getCurrentDialogueLine());

    // Draw the choice box on top
    ctx.ui.drawChoiceBox(choiceOptions, choiceSelected);
}
