#include "../../src/story/StoryAction.h"
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

    StoryAction dialogue = StoryAction::showDialogue("Prof. Bart Iver", {"Line 1", "Line 2"});
    assert(dialogue.is<StoryShowDialogueAction>());
    const auto *dialoguePayload = dialogue.tryGet<StoryShowDialogueAction>();
    assert(dialoguePayload != nullptr);
    assert(dialoguePayload->speaker == "Prof. Bart Iver");
    assert(dialoguePayload->lines.size() == 2);

    const Species starterSpecies{4,
                                 "Abacub",
                                 ElementType::algebraic,
                                 ElementType::algebraic,
                                 GrowthRate::mediumSlow,
                                 {45, 40, 45, 55, 50, 45},
                                 {0, 0, 0, 1, 0, 0},
                                 200,
                                 64,
                                 {},
                                 {}};
    StoryAction naming = StoryAction::promptStarterNickname(
        Daemon(starterSpecies, 5), "Prof. Bart Iver", {"Take care of it."});
    assert(naming.is<StoryPromptStarterNicknameAction>());
    const auto *namingPayload = naming.tryGet<StoryPromptStarterNicknameAction>();
    assert(namingPayload != nullptr);
    assert(namingPayload->daemon.getSpeciesId() == 4);
    assert(namingPayload->speaker == "Prof. Bart Iver");
    assert(namingPayload->lines.size() == 1);

    StoryAction battle = StoryAction::startBattle(trainer);
    assert(battle.is<StoryStartBattleAction>());
    assert(battle.tryGet<StoryStartBattleAction>()->trainer == trainer);

    return 0;
}
