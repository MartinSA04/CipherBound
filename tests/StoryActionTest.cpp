#include "../src/core/StoryAction.h"
#include <cassert>

int main() {
    NPC *trainer = reinterpret_cast<NPC *>(0x1);

    StoryAction none = StoryAction::none();
    assert(none.is<StoryNoAction>());

    StoryAction choice = StoryAction::showChoice({"Yes", "No"}, "pokeball_1");
    assert(choice.is<StoryShowChoiceAction>());
    const auto *choicePayload = choice.tryGet<StoryShowChoiceAction>();
    assert(choicePayload != nullptr);
    assert(choicePayload->options.size() == 2);
    assert(choicePayload->choiceContext == "pokeball_1");

    StoryAction dialogue =
        StoryAction::showDialogue("Prof. Bart Iver", {"Line 1", "Line 2"});
    assert(dialogue.is<StoryShowDialogueAction>());
    const auto *dialoguePayload = dialogue.tryGet<StoryShowDialogueAction>();
    assert(dialoguePayload != nullptr);
    assert(dialoguePayload->speaker == "Prof. Bart Iver");
    assert(dialoguePayload->lines.size() == 2);

    StoryAction battle = StoryAction::startBattle(trainer);
    assert(battle.is<StoryStartBattleAction>());
    assert(battle.tryGet<StoryStartBattleAction>()->trainer == trainer);

    return 0;
}
