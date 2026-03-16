#pragma once
#include "InputManager.h"
#include "OverworldRenderer.h"
#include "Renderer.h"
#include "SpriteFont.h"
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

enum class ScreenType {
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

enum class EXPTickResult {
    inProgress,
    reachedTarget,
    filledBar,
};

class GameUI {
  public:
    GameUI();

    Renderer &getRenderer();
    InputManager &getInput();
    OverworldRenderer &getOverworldRenderer();

    void setScreen(ScreenType screen);
    ScreenType getCurrentScreen() const;

    void beginFrame();
    void endFrame();
    bool shouldClose() const;
    void updateInput();

    void drawOverworld(const Map &map, const Player &player,
                       const std::vector<std::unique_ptr<NPC>> &npcs);

    void loadBattleAssets();
    void loadDaemonSprite(const std::string &speciesName);

    void drawPartyList(const Player &player, int selected);
    void drawBagScreen(const Player &player, const Pokedex &pokedex, int selected);
    void drawSummaryScreen(const Daemon &daemon, const Pokedex &pokedex, int page = 0);

    void navigateVertical(int &selected, int count);
    void navigate2x2(int &selected);

    int playerDisplayHP{0};
    int opponentDisplayHP{0};
    int playerDisplayEXP{0};
    int expAnimFrame{0};
    int expAnimStartEXP{-1};

    int battleIntroFrame{0};
    int battleIntroPhase{0};

    void drawDialogueBox(const std::string &speaker, const std::string &text);
    void drawChoiceBox(const std::vector<std::string> &options, int selected);

    void startDialogue(const std::string &speaker, const std::vector<std::string> &lines);
    bool advanceDialogueLine();
    bool isDialogueActive() const;
    const std::string &getCurrentDialogueLine() const;
    const std::string &getDialogueSpeaker() const;

    bool tickHPAnimation(int targetPlayerHP, int targetOpponentHP, int maxPlayerHP,
                         int maxOpponentHP);
    EXPTickResult tickEXPAnimation(int targetEXP, int expNeeded);

    void setDialogueText(const std::string &text);
    bool updateTypewriter(const bool inputConfirm);
    bool isTextFullyRevealed() const;
    void revealAllText();

    static constexpr int BATTLE_INTRO_DURATION = 90;
    static constexpr int BATTLE_INTRO_SCENE_DURATION = 46;

    SpriteFont &getSpriteFont();
    void drawSelectionArrow(int x, int y, int scale = PIXEL_SCALE);
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

    void drawSpriteHPBar(int x, int y, int width, int currentHP, int maxHP,
                         int scale = PIXEL_SCALE);
    void drawSpriteEXPBar(int x, int y, int width, int currentEXP, int maxEXP,
                          int scale = PIXEL_SCALE);

    void drawTextBar(int panelY);
    void drawNarrowTextBar(int x, int y, int srcW, int scale = PIXEL_SCALE);

  private:
    Renderer renderer;
    InputManager input;
    OverworldRenderer overworldRenderer;
    SpriteFont spriteFont;
    ScreenType currentScreen;

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
