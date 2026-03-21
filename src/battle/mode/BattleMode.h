/**
 * @file
 * @brief Main battle gameplay mode.
 * @ingroup battle_system
 */

#pragma once
#include "../../app/GameMode.h"
#include "../ui/BattleRenderer.h"
#include <deque>
#include <string>

/// Mode that presents the active battle and handles player combat choices.
class BattleMode : public GameMode {
  public:
    /// Updates battle menus, animations, and message progression.
    void update(GameContext &ctx, InputManager &input) override;
    /// Renders the battle presentation layer.
    void render(GameContext &ctx) override;

  private:
    enum class ProgressionEventType {
        message,
        replaceMove,
    };

    struct ProgressionEvent {
        ProgressionEventType type{ProgressionEventType::message};
        int partyIndex{-1};
        int moveId{-1};
        std::string text;
    };

    void updateBattleIntroAnim(GameContext &ctx);
    void updateCaptureAnim(GameContext &ctx);
    void updateSwitchAnim(GameContext &ctx);
    bool updateProgressionSequence(GameContext &ctx, InputManager &input);
    bool queueDaemonProgression(GameContext &ctx, int partyIndex, bool resolveAllLevels);
    bool queueParticipantProgression(GameContext &ctx);

    int menuSelected{0};             ///< Current top-level battle menu selection.
    int moveSelected{0};             ///< Current move selection.
    int partySelected{0};            ///< Current party selection.
    int bagSelected{0};              ///< Current bag selection.

    bool viewingSummary{false};      ///< Whether the party summary overlay is open.
    bool showingPartyAction{false};  ///< Whether the party action submenu is open.
    int partyActionSelected{0};      ///< Current party action submenu selection.
    int summaryPage{0};              ///< Current summary page.
    int summaryMoveSelected{0};      ///< Current move selection on the summary move page.

    int captureAnimFrame{0};         ///< Capture animation frame counter.
    bool captureAnimDone{false};     ///< Whether the current capture animation finished.

    int attackAnimFrame{0};          ///< Attack animation frame counter.

    int battleAnimFrame{0};          ///< Idle battle bob/frame counter.

    bool expSoundPlayed{false};      ///< Prevents repeated EXP sound retriggering.
    bool progressionFinishesExpAnimation{false}; ///< Whether to resume battle after the queue.
    int progressionSelectedMove{0};  ///< Current move selection for move replacement.
    std::deque<ProgressionEvent> progressionEvents; ///< Pending local level-up / learn-move flow.

    BattleRenderer battleRenderer;
};
