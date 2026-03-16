#include "DialogueMode.h"
#include "../../ui/InputManager.h"
#include "../../ui/GameUI.h"
#include "../../state/World.h"
#include "../../data/Pokedex.h"
#include "../../audio/SoundManager.h"
#include "../StoryManager.h"

DialogueMode::DialogueMode(const std::string &speaker,
                           const std::vector<std::string> &lines,
                           std::shared_ptr<NPC> npc,
                           GameState returnState)
    : dialogueNPC(std::move(npc)),
      returnState(returnState),
      savedSpeaker(speaker),
      savedLines(lines)
{
}

void DialogueMode::onEnter(GameContext &ctx)
{
    ctx.ui.startDialogue(savedSpeaker, savedLines);
}

void DialogueMode::update(GameContext &ctx, InputManager &input)
{
    if (ctx.ui.updateTypewriter(input.isConfirmPressed()))
    {
        ctx.playSound(SoundEffect::select);
        if (!ctx.ui.advanceDialogueLine())
        {
            // Dialogue finished — ask StoryManager what to do
            StoryAction action = ctx.story.onDialogueEnd(dialogueNPC, ctx.world);
            dialogueNPC = nullptr;
            ctx.pushRequest(ModeRequest::storyAction(action));
        }
    }
}

void DialogueMode::render(GameContext &ctx)
{
    if (returnState == GameState::overworld)
        renderOverworld(ctx);

    if (ctx.ui.isDialogueActive())
        ctx.ui.drawDialogueBox(ctx.ui.getDialogueSpeaker(), ctx.ui.getCurrentDialogueLine());
}
