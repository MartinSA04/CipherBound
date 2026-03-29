/**
 * @file
 * @brief High-level UI facade combining renderer, input, dialogue, and menu helpers.
 * @ingroup app_core
 */

#pragma once
#include "InputManager.h"
#include "OverworldRenderer.h"
#include "Renderer.h"
#include "SpriteFont.h"
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

/// High-level screen family used to switch UI drawing behavior.
enum class ScreenType {
    title,     ///< Title screen.
    overworld, ///< Overworld rendering.
    battle,    ///< Battle presentation.
    menu,      ///< Pause/menu screens.
    bag,       ///< Bag screen.
    party,     ///< Party screen.
    summary,   ///< Daemon summary screen.
    shop,      ///< Shop screen.
    dialogue,  ///< Dialogue/choice overlay.
};

/**
 * @brief Facade for rendering, input polling, dialogue state, and menu navigation helpers.
 * @ingroup app_core
 */
class GameUI {
  public:
    /// Creates the UI and its owned renderer/input helpers.
    GameUI();

    /// Returns the owned renderer.
    Renderer &getRenderer();
    /// Returns the owned input manager.
    InputManager &getInput();
    /// Returns the overworld renderer helper.
    OverworldRenderer &getOverworldRenderer();

    /// Sets the current screen family.
    void setScreen(ScreenType screen);
    /// Returns the current screen family.
    ScreenType getCurrentScreen() const;

    /// Begins a frame.
    void beginFrame();
    /// Ends a frame.
    void endFrame();
    /// Returns whether the window requested close.
    bool shouldClose() const;
    /// Polls input and updates edge-triggered button state.
    void updateInput();

    /// Draws the overworld scene.
    void drawOverworld(const Map &map, const Player &player,
                       const std::vector<std::unique_ptr<NPC>> &npcs);

    /// Loads textures used by battle UI rendering.
    void loadBattleAssets();
    /// Loads the sprite sheet for one daemon species on demand.
    void loadDaemonSprite(const std::string &speciesName);

    /// Draws the party list UI.
    void drawPartyList(const Player &player, int selected);
    /// Draws the bag screen UI.
    void drawBagScreen(const Player &player, const Pokedex &pokedex, int selected);
    /// Draws the trainer journal and current objective.
    void drawPlayerStatsScreen(const Player &player, const std::string &objectiveTitle,
                               const std::vector<std::string> &objectiveLines);
    /// Draws the shop screen UI.
    void drawShopScreen(const Player &player, const Pokedex &pokedex,
                        const std::vector<int> &itemIds, int selected, const std::string &title,
                        const std::string &footerText);
    /// Draws the daemon summary screen.
    void drawSummaryScreen(const Daemon &daemon, const Pokedex &pokedex, int page = 0,
                           int selectedMove = 0);
    /// Draws the move details / move replacement screen for one daemon.
    void drawMoveLearningScreen(const Daemon &daemon, const Pokedex &pokedex, int selectedMove,
                                int newMoveId);

    /// Handles repeated vertical menu navigation.
    void navigateVertical(int &selected, int count);
    /// Handles repeated horizontal menu navigation.
    void navigateHorizontal(int &selected, int count);
    /// Handles repeated linear list navigation.
    void navigateLinear(int &selected, int count);
    /// Handles repeated navigation in a 2x2 grid.
    void navigate2x2(int &selected);

    /// Draws a dialogue box.
    void drawDialogueBox(const std::string &speaker, const std::string &text);
    /// Draws a choice box.
    void drawChoiceBox(const std::vector<std::string> &options, int selected);
    /// Draws the shop quantity prompt.
    void drawShopQuantityBox(int quantity);

    /// Starts a dialogue sequence with preloaded lines.
    void startDialogue(const std::string &speaker, const std::vector<std::string> &lines);
    /// Advances to the next dialogue line and returns whether more lines remain.
    bool advanceDialogueLine();
    /// Returns whether dialogue is currently active.
    bool isDialogueActive() const;
    /// Returns the currently visible dialogue line.
    const std::string &getCurrentDialogueLine() const;
    /// Returns the current dialogue speaker.
    const std::string &getDialogueSpeaker() const;

    /// Replaces the current typewriter text.
    void setDialogueText(const std::string &text);
    /// Advances the typewriter effect and returns whether dialogue handling should continue.
    bool updateTypewriter(const bool inputConfirm);
    /// Returns whether the current text is fully revealed.
    bool isTextFullyRevealed() const;
    /// Instantly reveals the full current dialogue text.
    void revealAllText();

    /// Returns the owned sprite font.
    SpriteFont &getSpriteFont();
    /// Draws the menu selection arrow sprite.
    void drawSelectionArrow(int x, int y, int scale = PIXEL_SCALE);

    /// Draws a sprite-based HP bar.
    void drawSpriteHPBar(int x, int y, int width, int currentHP, int maxHP,
                         int scale = PIXEL_SCALE);
    /// Draws a sprite-based EXP bar.
    void drawSpriteEXPBar(int x, int y, int width, int currentEXP, int maxEXP,
                          int scale = PIXEL_SCALE);

    /// Draws the standard text bar panel.
    void drawTextBar(int panelY);
    /// Draws a narrower text bar variant.
    void drawNarrowTextBar(int x, int y, int srcW, int scale = PIXEL_SCALE);

  private:
    Renderer renderer;                   ///< Owned low-level renderer.
    InputManager input;                  ///< Owned input manager.
    OverworldRenderer overworldRenderer; ///< Overworld rendering helper.
    SpriteFont spriteFont;               ///< Bitmap font renderer.
    ScreenType currentScreen;            ///< Active screen family.

    bool battleAssetsLoaded{false};

    int menuRepeatTimer{0};
    bool menuFirstRepeat{true};
    bool menuDirHeld{false};
    Direction menuLastDir{Direction::up};
    static constexpr int menuInitialDelay = 16;
    static constexpr int menuRepeatDelay = 6;

    std::vector<std::string> dialogueLines;
    int dialogueLineIndex{0};
    std::string dialogueSpeaker;

    std::string typewriterFullText;
    std::size_t typewriterCharsRevealed{0};
    int typewriterFrameCounter{0};
    int typewriterSpeed{2};
    int typewriterFastSpeed{1};
    int typewriterIndicatorTimer{0};

    void drawTextBox(int x, int y, int width, int height, const std::string &text);
};
