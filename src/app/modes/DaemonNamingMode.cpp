#include "DaemonNamingMode.h"
#include "../../ui/GameUI.h"
#include "../../ui/InputManager.h"
#include "../../ui/Renderer.h"
#include "../../ui/UiConstants.h"
#include <utility>

namespace {

std::string promptTitle(DaemonNamingPurpose purpose) {
    switch (purpose) {
    case DaemonNamingPurpose::starter:
        return "Name your starter";
    case DaemonNamingPurpose::battleCapture:
        return "Nickname your catch";
    }
    return "Name your daemon";
}

std::string promptText(DaemonNamingPurpose purpose, const Daemon &daemon) {
    const std::string &speciesName = daemon.getSpecies().name;
    switch (purpose) {
    case DaemonNamingPurpose::starter:
        return "Edit the name or press DONE to keep " + speciesName + ".";
    case DaemonNamingPurpose::battleCapture:
        return "You caught " + speciesName + ". Edit the name or keep it.";
    }
    return "Choose a nickname.";
}

} // namespace

DaemonNamingMode::DaemonNamingMode(Daemon daemon, DaemonNamingPurpose purpose,
                                   std::string completionSpeaker,
                                   std::vector<std::string> completionLines,
                                   GameState returnState)
    : daemon(std::move(daemon)), purpose(purpose), completionSpeaker(std::move(completionSpeaker)),
      completionLines(std::move(completionLines)), returnState(returnState) {
    nameEntry.reset(this->daemon.getNickname());
}

void DaemonNamingMode::update(GameContext &ctx, InputManager &input) {
    nameEntry.navigate(input);

    if (input.isCancelPressed()) {
        if (nameEntry.backspace() == NameEntryAction::edited) {
            ctx.playSound(SoundEffect::select);
        }
        return;
    }

    if (!input.isConfirmPressed())
        return;

    switch (nameEntry.activateSelectedKey()) {
    case NameEntryAction::edited:
        ctx.playSound(SoundEffect::select);
        return;
    case NameEntryAction::auxiliary:
        nameEntry.setText(daemon.getSpecies().name);
        ctx.playSound(SoundEffect::select);
        return;
    case NameEntryAction::submit:
        ctx.playSound(SoundEffect::select);
        daemon.setNickname(nameEntry.normalizedTextOr(daemon.getSpecies().name));
        finishNaming(ctx);
        return;
    case NameEntryAction::none:
        return;
    }
}

void DaemonNamingMode::render(GameContext &ctx) {
    Renderer &renderer = ctx.ui.getRenderer();

    renderer.drawFilledRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, TDT4102::Color{22, 30, 70});

    ctx.ui.loadDaemonSprite(daemon.getSpecies().name);
    const std::string spriteId = "daemon_" + daemon.getSpecies().name;
    if (renderer.hasTexture(spriteId)) {
        const int spriteScale = 2;
        const int spriteSize = 80 * spriteScale;
        const int spriteX = WINDOW_WIDTH / 2 - spriteSize / 2;
        const int spriteY = 84;

        renderer.drawFilledRect(spriteX - 16, spriteY - 12, spriteSize + 32, spriteSize + 24,
                                TDT4102::Color{228, 236, 255});
        renderer.drawRect(spriteX - 16, spriteY - 12, spriteSize + 32, spriteSize + 24,
                          TDT4102::Color::transparent, TDT4102::Color{24, 32, 74});
        renderer.drawSpriteRaw(spriteId, spriteX, spriteY, spriteSize, spriteSize);
    }

    renderer.drawText(promptTitle(purpose), WINDOW_WIDTH / 2 - 86, 22,
                      TDT4102::Color{240, 244, 255}, 22);
    renderer.drawText(promptText(purpose, daemon), WINDOW_WIDTH / 2 - 180, 78,
                      TDT4102::Color{190, 198, 230}, 14);
    nameEntry.render(ctx.ui,
                     {.fieldLabel = "Nickname",
                      .footerPrimary = "Arrows move. Z/Enter selects. X/Esc deletes.",
                      .footerSecondary =
                          "DEFAULT restores the species name and DONE confirms the nickname.",
                      .nameBoxY = 270});
}

void DaemonNamingMode::finishNaming(GameContext &ctx) {
    ctx.world.getPlayer().addDaemon(std::move(daemon));

    if (purpose == DaemonNamingPurpose::starter) {
        ctx.world.getPlayer().setFlag("has_starter");
        ctx.pushRequest(
            ModeRequest::dialogue(completionSpeaker, completionLines, nullptr, returnState));
        return;
    }

    ctx.pushRequest(ModeRequest::endBattle());
}
