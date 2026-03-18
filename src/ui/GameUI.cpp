#include "GameUI.h"

GameUI::GameUI()
    : renderer(), input(renderer.getWindow()), overworldRenderer(renderer), spriteFont(),
      currentScreen(ScreenType::title) {
    spriteFont.loadTextures(renderer);
    renderer.loadTexture("ui_text_bar", "assets/sprites/ui/text_bar.png");
}

Renderer &GameUI::getRenderer() { return renderer; }
InputManager &GameUI::getInput() { return input; }
OverworldRenderer &GameUI::getOverworldRenderer() { return overworldRenderer; }
SpriteFont &GameUI::getSpriteFont() { return spriteFont; }

void GameUI::setScreen(ScreenType screen) { currentScreen = screen; }
ScreenType GameUI::getCurrentScreen() const { return currentScreen; }

void GameUI::beginFrame() { renderer.beginFrame(); }
void GameUI::endFrame() { renderer.endFrame(); }
bool GameUI::shouldClose() const { return renderer.shouldClose(); }
void GameUI::updateInput() { input.update(); }

void GameUI::drawOverworld(const Map &map, const Player &player,
                           const std::vector<std::unique_ptr<NPC>> &npcs) {
    overworldRenderer.render(map, player, npcs);
}

void GameUI::loadBattleAssets() {
    if (battleAssetsLoaded)
        return;

    renderer.loadTexture("ui_player_info", "assets/sprites/ui/player_creature_info.png");
    renderer.loadTexture("ui_opponent_info", "assets/sprites/ui/opponent_creature_info.png");
    renderer.loadTexture("ui_hp_bar", "assets/sprites/ui/hp_bar.png");
    renderer.loadTexture("ui_exp_bar", "assets/sprites/ui/exp_bar.png");
    renderer.loadTexture("ui_player_base", "assets/sprites/ui/player_battle_base.png");
    renderer.loadTexture("ui_opponent_base", "assets/sprites/ui/opponent_battle_base.png");
    renderer.loadTexture("player_back", "assets/sprites/player/player_back.png");
    renderer.loadTexture("daemon_ball", "assets/sprites/items/daemon_ball.png");

    battleAssetsLoaded = true;
}

void GameUI::loadDaemonSprite(const std::string &speciesName) {
    std::string frontId = "daemon_" + speciesName;
    std::string backId = "daemon_" + speciesName + "_back";
    std::string frontPath = "assets/sprites/daemons/" + speciesName + ".png";
    std::string backPath = "assets/sprites/daemons/" + speciesName + "_back.png";

    if (!renderer.hasTexture(frontId))
        renderer.loadTexture(frontId, frontPath);
    if (!renderer.hasTexture(backId))
        renderer.loadTexture(backId, backPath);
}

void GameUI::navigateVertical(int &selected, int count) {
    if (count <= 0)
        return;

    if (selected < 0 || selected >= count)
        selected = (selected % count + count) % count;

    Direction dir;
    bool dirHeld = input.getMovementDirection(dir);

    if (dirHeld && (dir == Direction::up || dir == Direction::down)) {
        if (!menuDirHeld || dir != menuLastDir) {
            if (dir == Direction::up)
                selected = (selected - 1 + count) % count;
            else
                selected = (selected + 1) % count;
            menuRepeatTimer = 0;
            menuFirstRepeat = true;
            menuDirHeld = true;
            menuLastDir = dir;
        } else {
            menuRepeatTimer++;
            int delay = menuFirstRepeat ? menuInitialDelay : menuRepeatDelay;
            if (menuRepeatTimer >= delay) {
                if (dir == Direction::up)
                    selected = (selected - 1 + count) % count;
                else
                    selected = (selected + 1) % count;
                menuRepeatTimer = 0;
                menuFirstRepeat = false;
            }
        }
    } else {
        menuDirHeld = false;
        menuRepeatTimer = 0;
        menuFirstRepeat = true;
    }
}

void GameUI::navigateHorizontal(int &selected, int count) {
    if (count <= 0)
        return;

    if (selected < 0 || selected >= count)
        selected = (selected % count + count) % count;

    Direction dir;
    bool dirHeld = input.getMovementDirection(dir);

    if (dirHeld && (dir == Direction::left || dir == Direction::right)) {
        if (!menuDirHeld || dir != menuLastDir) {
            if (dir == Direction::left)
                selected = (selected - 1 + count) % count;
            else
                selected = (selected + 1) % count;
            menuRepeatTimer = 0;
            menuFirstRepeat = true;
            menuDirHeld = true;
            menuLastDir = dir;
        } else {
            menuRepeatTimer++;
            int delay = menuFirstRepeat ? menuInitialDelay : menuRepeatDelay;
            if (menuRepeatTimer >= delay) {
                if (dir == Direction::left)
                    selected = (selected - 1 + count) % count;
                else
                    selected = (selected + 1) % count;
                menuRepeatTimer = 0;
                menuFirstRepeat = false;
            }
        }
    } else {
        menuDirHeld = false;
        menuRepeatTimer = 0;
        menuFirstRepeat = true;
    }
}

void GameUI::navigateLinear(int &selected, int count) {
    if (count <= 0)
        return;

    if (selected < 0 || selected >= count)
        selected = (selected % count + count) % count;

    Direction dir;
    bool dirHeld = input.getMovementDirection(dir);

    if (dirHeld) {
        const bool decrement = dir == Direction::down || dir == Direction::left;
        const bool increment = dir == Direction::up || dir == Direction::right;

        if (!decrement && !increment) {
            menuDirHeld = false;
            menuRepeatTimer = 0;
            menuFirstRepeat = true;
            return;
        }

        if (!menuDirHeld || dir != menuLastDir) {
            selected = decrement ? (selected - 1 + count) % count : (selected + 1) % count;
            menuRepeatTimer = 0;
            menuFirstRepeat = true;
            menuDirHeld = true;
            menuLastDir = dir;
        } else {
            menuRepeatTimer++;
            int delay = menuFirstRepeat ? menuInitialDelay : menuRepeatDelay;
            if (menuRepeatTimer >= delay) {
                selected = decrement ? (selected - 1 + count) % count : (selected + 1) % count;
                menuRepeatTimer = 0;
                menuFirstRepeat = false;
            }
        }
    } else {
        menuDirHeld = false;
        menuRepeatTimer = 0;
        menuFirstRepeat = true;
    }
}

void GameUI::navigate2x2(int &sel) {
    if (input.isRightHeld() && (sel % 2) == 0)
        sel += 1;
    else if (input.isLeftHeld() && (sel % 2) == 1)
        sel -= 1;
    else if (input.isDownHeld() && sel < 2)
        sel += 2;
    else if (input.isUpHeld() && sel >= 2)
        sel -= 2;
}
