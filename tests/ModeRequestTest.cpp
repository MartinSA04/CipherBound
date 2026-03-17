#include "../src/app/GameMode.h"
#include <cassert>

int main() {
    NPC *trainer = reinterpret_cast<NPC *>(0x1);

    ModeRequest enterBattle = ModeRequest::enterBattleMode();
    assert(std::holds_alternative<EnterBattleModeRequest>(enterBattle.payload));

    ModeRequest dialogue = ModeRequest::dialogue("Cipher", {"Line 1", "Line 2"}, trainer,
                                                 GameState::menu);
    assert(std::holds_alternative<StartDialogueRequest>(dialogue.payload));
    const auto &dialoguePayload = std::get<StartDialogueRequest>(dialogue.payload);
    assert(dialoguePayload.speaker == "Cipher");
    assert(dialoguePayload.lines.size() == 2);
    assert(dialoguePayload.npc == trainer);
    assert(dialoguePayload.returnState == GameState::menu);

    ModeMailbox mailbox;
    mailbox.push(ModeRequest::changeState(GameState::party));
    mailbox.push(ModeRequest::trainerBattle(trainer));

    std::vector<ModeRequest> drained = mailbox.drain();
    assert(drained.size() == 2);
    assert(std::holds_alternative<ChangeStateRequest>(drained[0].payload));
    assert(std::get<ChangeStateRequest>(drained[0].payload).targetState == GameState::party);
    assert(std::holds_alternative<StartTrainerBattleRequest>(drained[1].payload));
    assert(std::get<StartTrainerBattleRequest>(drained[1].payload).npc == trainer);

    assert(mailbox.drain().empty());
    return 0;
}
