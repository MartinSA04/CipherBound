#include "../../ui/GameUI.h"
#include "../Battle.h"
#include "../ui/BattlePresentationState.h"
#include "BattleMode.h"
#include <cmath>

namespace {

void drawEvolutionSprite(GameUI &ui, const Species &species, int centerX, int centerY,
                         int baseScale, int scaleBonus) {
    ui.loadDaemonSprite(species.name);
    Renderer &renderer = ui.getRenderer();
    const std::string spriteId = "daemon_" + species.name;
    if (!renderer.hasTexture(spriteId))
        return;

    TDT4102::Image &img = renderer.getTexture(spriteId);
    const int scale = baseScale + scaleBonus;
    const int width = img.width * scale;
    const int height = img.height * scale;
    renderer.drawSpriteRaw(spriteId, centerX - width / 2, centerY - height / 2, width, height);
}

void drawEvolutionScene(GameContext &ctx, int sourceSpeciesId, int targetSpeciesId,
                        const std::string &displayName, int evolutionAnimFrame,
                        bool evolutionApplied) {
    static constexpr int introFrame = 120;
    static constexpr int transformFrame = 420;
    static constexpr int totalFrame = 600;

    GameUI &ui = ctx.ui;
    Renderer &renderer = ui.getRenderer();
    SpriteFont &font = ui.getSpriteFont();
    const int scale = PIXEL_SCALE;
    const int centerX = WINDOW_WIDTH / 2;
    const int centerY = (WINDOW_HEIGHT - UI_PANEL_HEIGHT) / 2 - 8 * scale;

    renderer.drawFilledRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, TDT4102::Color{56, 72, 120});
    renderer.drawFilledRect(0, 0, WINDOW_WIDTH, 20 * scale, TDT4102::Color{35, 48, 88});
    font.drawText(renderer, "EVOLUTION", 18 * scale, 4 * scale, scale - 1);

    const float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(evolutionAnimFrame) * 0.13f);
    const float shimmer = 0.5f + 0.5f * std::sin(static_cast<float>(evolutionAnimFrame) * 0.22f);
    const int auraRadius = 28 * scale + static_cast<int>(pulse * 24.0f * scale);
    const int innerRadius = 18 * scale + static_cast<int>(shimmer * 14.0f * scale);
    renderer.drawFilledRect(centerX - auraRadius, centerY - auraRadius, auraRadius * 2,
                            auraRadius * 2, TDT4102::Color{120, 200, 255, 70});
    renderer.drawFilledRect(centerX - innerRadius, centerY - innerRadius, innerRadius * 2,
                            innerRadius * 2, TDT4102::Color{240, 248, 255, 110});

    const Species &sourceSpecies = ctx.pokedex.getSpecies(sourceSpeciesId);
    const Species &targetSpecies = ctx.pokedex.getSpecies(targetSpeciesId);
    const bool flashPhase = ((evolutionAnimFrame / 6) % 2) == 0;
    const bool showTarget =
        evolutionApplied ||
        (evolutionAnimFrame > introFrame && evolutionAnimFrame < transformFrame && !flashPhase);
    const int scaleBonus = 1 + (evolutionAnimFrame / 10) % 2;
    drawEvolutionSprite(ui, showTarget ? targetSpecies : sourceSpecies, centerX, centerY, scale,
                        scaleBonus);

    if (evolutionAnimFrame > introFrame && evolutionAnimFrame < transformFrame && flashPhase) {
        renderer.drawFilledRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
                                TDT4102::Color{255, 255, 255, 90});
    }

    if (evolutionAnimFrame >= totalFrame) {
        ui.drawDialogueBox("", displayName + " evolved into " + targetSpecies.name + "!");
        return;
    }

    const int panelY = WINDOW_HEIGHT - UI_PANEL_HEIGHT;
    ui.drawTextBar(panelY);
    std::string text;
    if (evolutionAnimFrame < introFrame) {
        text = "What? " + displayName + " is evolving!";
    } else if (evolutionAnimFrame < transformFrame) {
        text = displayName + " is surrounded by a strange light!";
    } else {
        text = targetSpecies.name + " is taking shape!";
    }
    font.drawTextPartial(renderer, text, text.size(), 18 * scale, panelY + 18 * scale, scale - 1, 1,
                         WINDOW_WIDTH - 36 * scale);
}

} // namespace

void BattleMode::render(GameContext &ctx) {
    if (!ctx.hasBattle())
        return;

    Battle &battle = ctx.battle();
    BattlePresentationState &presentation = ctx.battlePresentation();
    BattleState bs = battle.getState();

    if (bs == BattleState::intro) {
        presentation.introPhase = battle.getIntroPhase();
        if (battle.getType() == BattleType::wild)
            battleRenderer.drawBattleIntroSceneWild(ctx.ui, battle, presentation);
        else
            battleRenderer.drawBattleIntroSceneTrainer(ctx.ui, battle, presentation);
        return;
    }

    if (!battle.isIntroComplete() && bs == BattleState::showingMessages) {
        const int savedFrame = presentation.introFrame;
        presentation.introFrame = BattlePresentationState::introSceneDuration;
        presentation.introPhase = battle.getIntroPhase();
        if (battle.getType() == BattleType::wild)
            battleRenderer.drawBattleIntroSceneWild(ctx.ui, battle, presentation);
        else
            battleRenderer.drawBattleIntroSceneTrainer(ctx.ui, battle, presentation);
        presentation.introFrame = savedFrame;
        ctx.ui.drawDialogueBox("", battle.getMessage());
        return;
    }

    if (bs == BattleState::animatingCapture) {
        battleRenderer.drawCaptureScene(ctx.ui, battle, presentation, captureAnimFrame,
                                        captureAnimDone);
        return;
    }

    if (bs == BattleState::animatingSwitch) {
        battleRenderer.drawPlayerSwitchScene(ctx.ui, battle, presentation, battleAnimFrame);
        return;
    }

    if (!progressionEvents.empty()) {
        const ProgressionEvent &event = progressionEvents.front();
        if (event.type == ProgressionEventType::replaceMove) {
            const Daemon &daemon = ctx.world.getPlayer().getDaemon(event.partyIndex);
            ctx.ui.drawMoveLearningScreen(daemon, ctx.pokedex, progressionSelectedMove,
                                          event.moveId);
        } else if (event.type == ProgressionEventType::evolution) {
            drawEvolutionScene(ctx, event.sourceSpeciesId, event.targetSpeciesId, event.text,
                               evolutionAnimFrame, evolutionApplied);
        } else {
            battleRenderer.drawBattleScene(ctx.ui, battle, presentation, battleAnimFrame,
                                           attackAnimFrame, captureAnimDone);
            ctx.ui.drawDialogueBox("", event.text);
        }
        return;
    }

    battleRenderer.drawBattleScene(ctx.ui, battle, presentation, battleAnimFrame, attackAnimFrame,
                                   captureAnimDone);

    if (bs == BattleState::choosingAction) {
        battleRenderer.drawBattleMenu(ctx.ui, menuSelected);
    } else if (bs == BattleState::choosingMove) {
        battleRenderer.drawMoveSelectScreen(ctx.ui, battle.getPlayerDaemon(), ctx.pokedex,
                                            moveSelected);
    } else if (bs == BattleState::choosingSwitch) {
        if (viewingSummary) {
            const Daemon &daemon = ctx.world.getPlayer().getDaemon(partySelected);
            ctx.ui.drawSummaryScreen(daemon, ctx.pokedex, summaryPage, summaryMoveSelected);
        } else {
            ctx.ui.drawPartyList(ctx.world.getPlayer(), partySelected);
            if (showingPartyAction) {
                ctx.ui.drawChoiceBox({"Summary", "Switch", "Cancel"}, partyActionSelected);
            }
        }
    } else if (bs == BattleState::choosingItem) {
        ctx.ui.drawBagScreen(ctx.world.getPlayer(), ctx.pokedex, bagSelected);
    } else {
        ctx.ui.drawDialogueBox("", battle.getMessage());
    }
}
