#include "../src/app/GameMode.h"
#include <cassert>

int main() {
    NPC *trainer = reinterpret_cast<NPC *>(0x1);

    ModeRequest enterBattle = ModeRequest::enterBattleMode();
    assert(std::holds_alternative<EnterBattleModeRequest>(enterBattle.payload));

    ModeRequest dialogue =
        ModeRequest::dialogue("Cipher", {"Line 1", "Line 2"}, trainer, GameState::menu);
    assert(std::holds_alternative<StartDialogueRequest>(dialogue.payload));
    const auto &dialoguePayload = std::get<StartDialogueRequest>(dialogue.payload);
    assert(dialoguePayload.speaker == "Cipher");
    assert(dialoguePayload.lines.size() == 2);
    assert(dialoguePayload.npc == trainer);
    assert(dialoguePayload.returnState == GameState::menu);

    const Species species{7,
                          "Bound",
                          ElementType::classical,
                          ElementType::logic,
                          GrowthRate::mediumFast,
                          {30, 15, 15, 15, 15, 15},
                          {0, 0, 0, 0, 0, 0},
                          255,
                          64,
                          {},
                          {}};
    ModeRequest naming = ModeRequest::daemonNaming(
        Daemon(species, 8), DaemonNamingPurpose::starter, "Prof. Bart Iver", {"Line 3"});
    assert(std::holds_alternative<StartDaemonNamingRequest>(naming.payload));
    const auto &namingPayload = std::get<StartDaemonNamingRequest>(naming.payload);
    assert(namingPayload.daemon.getSpeciesId() == 7);
    assert(namingPayload.purpose == DaemonNamingPurpose::starter);
    assert(namingPayload.completionSpeaker == "Prof. Bart Iver");
    assert(namingPayload.completionLines.size() == 1);
    assert(namingPayload.completionLines[0] == "Line 3");

    ModeMailbox mailbox;
    mailbox.push(ModeRequest::changeState(GameState::party));
    mailbox.push(ModeRequest::trainerBattle(trainer));
    mailbox.push(ModeRequest::openShop("Viridian Mart", "Clerk", {1, 5}));

    std::vector<ModeRequest> drained = mailbox.drain();
    assert(drained.size() == 3);
    assert(std::holds_alternative<ChangeStateRequest>(drained[0].payload));
    assert(std::get<ChangeStateRequest>(drained[0].payload).targetState == GameState::party);
    assert(std::holds_alternative<StartTrainerBattleRequest>(drained[1].payload));
    assert(std::get<StartTrainerBattleRequest>(drained[1].payload).npc == trainer);
    assert(std::holds_alternative<OpenShopRequest>(drained[2].payload));
    const auto &shopPayload = std::get<OpenShopRequest>(drained[2].payload);
    assert(shopPayload.title == "Viridian Mart");
    assert(shopPayload.shopkeeperName == "Clerk");
    assert(shopPayload.itemIds.size() == 2);
    assert(shopPayload.itemIds[0] == 1);
    assert(shopPayload.itemIds[1] == 5);

    assert(mailbox.drain().empty());
    return 0;
}
