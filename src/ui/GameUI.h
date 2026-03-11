#pragma once
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include "Renderer.h"
#include "OverworldRenderer.h"
#include "InputManager.h"
#include "SpriteFont.h"
#include "Menu.h"
#include "../battle/Battle.h"

enum class ScreenType
{
    title,
    overworld,
    battle,
    menu,
    bag,
    party,
    summary,
    shop,
    dialogue,
};

enum class EXPTickResult
{
    inProgress,
    reachedTarget,
    filledBar,
};

class GameUI
{
public:
    GameUI();

    // Access subsystems
    Renderer &getRenderer();
    InputManager &getInput();
    OverworldRenderer &getOverworldRenderer();

    void setScreen(ScreenType screen);
    ScreenType getCurrentScreen() const;

    // Frame management
    void beginFrame();
    void endFrame();
    bool shouldClose() const;
    void updateInput();

    // Overworld
    void drawOverworld(const Map &map, const Player &player, const std::vector<std::shared_ptr<NPC>> &npcs);

    // Load battle UI sprite assets
    void loadBattleAssets();

    // Load Daemon front/back sprites by species name
    void loadDaemonSprite(const std::string &speciesName);

    // Menus
    void drawPartyList(const Player &player, int selected);
    void drawBagScreen(const Player &player, const Pokedex &pokedex, int selected);

    // Navigation helpers
    void navigateVertical(int &selected, int count);
    void navigate2x2(int &selected);

    // HP/EXP bar animation display values
    int playerDisplayHP{0};
    int opponentDisplayHP{0};
    int playerDisplayEXP{0};
    int expAnimFrame{0};
    int expAnimStartEXP{-1};

    // Battle intro animation frame
    int battleIntroFrame{0};
    int battleIntroPhase{0};



    // Dialogue box
    void drawDialogueBox(const std::string &speaker, const std::string &text);
    void drawChoiceBox(const std::vector<std::string> &options, int selected);

    // Multi-line dialogue management
    void startDialogue(const std::string &speaker, const std::vector<std::string> &lines);
    bool advanceDialogueLine(); // advance to next line; returns true if more lines remain
    bool isDialogueActive() const;
    const std::string &getCurrentDialogueLine() const;
    const std::string &getDialogueSpeaker() const;

    // HP/EXP animation tick helpers
    bool tickHPAnimation(int targetPlayerHP, int targetOpponentHP,
                         int maxPlayerHP, int maxOpponentHP);
    EXPTickResult tickEXPAnimation(int targetEXP, int expNeeded);

    // Typewriter text control
    void setDialogueText(const std::string &text);  // Start revealing new text
    bool updateTypewriter(const bool inputConfirm); // Call once per frame to advance reveal, returns true if game should continue.
    bool isTextFullyRevealed() const;               // True when all chars shown
    void revealAllText();                           // Skip to end instantly

    // Battle intro transition constants
    static constexpr int BATTLE_INTRO_DURATION = 90;       // total frames for the intro (60fps)
    static constexpr int BATTLE_INTRO_SCENE_DURATION = 46; // frames per intro animation phase (60fps)

    // Sprite font access (for custom screen rendering)
    SpriteFont &getSpriteFont();

    // Draw a pixel-art selection arrow at the given position
    void drawSelectionArrow(int x, int y, int scale = PIXEL_SCALE);

    // Battle scene helpers (used by BattleMode rendering)
    struct BattleBaseGeometry {
        int x, y, w, h;
    };
    void drawOpponentInfoBar(const Daemon *opponentDaemon, int offsetX = 0);
    void drawPlayerInfoBar(const Daemon *playerDaemon, int offsetX = 0);
    void drawOpponentDaemon(const Daemon *opponentDaemon, int offsetX = 0, int offsetY = 0);
    void drawPlayerDaemon(const Daemon *playerDaemon, int offsetX = 0, int offsetY = 0);
    void drawPlayerBackSprite(int x, int y, int dstW, int dstH, int frame);
    void drawBattleBackground();
    void drawPlayerBackOnBase(int offsetX = 0, int frame = 0);
    void drawPlayerSendOutPhase(const Daemon *playerDaemon, float t);
    BattleBaseGeometry getPlayerBaseGeometry() const;
    BattleBaseGeometry getOpponentBaseGeometry() const;
    void drawPlayerBase();
    void drawOpponentBase(int offsetX = 0);
    void drawOpponentTrainer(const NPC *opponent, int offsetX = 0);

    // Sprite-based HP/EXP bar drawing (used by multiple modes)
    void drawSpriteHPBar(int x, int y, int width, int currentHP, int maxHP, int scale = PIXEL_SCALE);
    void drawSpriteEXPBar(int x, int y, int width, int currentEXP, int maxEXP, int scale = PIXEL_SCALE);

    // Draw the text_bar.png background across the bottom panel
    void drawTextBar(int panelY);

    // Draw a narrower text_bar using partial sprite rendering
    void drawNarrowTextBar(int x, int y, int srcW, int scale = PIXEL_SCALE);

private:
    Renderer renderer;
    InputManager input;
    OverworldRenderer overworldRenderer;
    SpriteFont spriteFont;
    ScreenType currentScreen;

    bool battleAssetsLoaded{false};

    // Menu navigation repeat delay
    int menuRepeatTimer{0};
    bool menuFirstRepeat{true};
    bool menuDirHeld{false};
    Direction menuLastDir{Direction::up};
    static constexpr int menuInitialDelay = 16;
    static constexpr int menuRepeatDelay = 6;

    // Multi-line dialogue state
    std::vector<std::string> dialogueLines;
    int dialogueLineIndex{0};
    std::string dialogueSpeaker;

    // Typewriter effect state
    std::string typewriterFullText;  // The complete text to reveal
    int typewriterCharsRevealed{0};  // How many characters are currently visible
    int typewriterFrameCounter{0};   // Frame counter for timing
    int typewriterSpeed{2};          // Frames per character (normal speed, 60fps)
    int typewriterFastSpeed{1};      // Frames per character (fast/held)
    int typewriterIndicatorTimer{0}; // Continuous frame counter for indicator animation

    // Dialogue box helpers
    void drawTextBox(int x, int y, int width, int height, const std::string &text);
};
