#pragma once
#include "../state/Map.h"
#include "../state/NPC.h"
#include "../state/Player.h"
#include "../state/World.h"
#include "Renderer.h"
#include <array>
#include <vector>

// Source rectangle for a sprite frame
struct SpriteFrame {
    int x, y, w, h;
};

class OverworldRenderer {
  public:
    OverworldRenderer(Renderer &renderer);

    // Load the player spritesheet (call once at init)
    void loadSprites();

    // Load a background image for a map (call when entering a map)
    void loadMapBackground(const std::string &mapId,
                           const std::string &imagePath);
    void loadMapBackgroundOverlay(const std::string &mapId,
                                  const std::string &imagePath);

    void loadMapBackgrounds(const World &world);

    // Render the full overworld scene
    void render(const Map &map, const Player &player,
                const std::vector<std::shared_ptr<NPC>> &npcs);

    // Individual layers (called by render in order)
    void renderMap(const Map &map, int cameraX, int cameraY);
    void renderEntity(const Entity &entity, int cameraX, int cameraY,
                      TDT4102::Color color = TDT4102::Color::red,
                      int pixelOffsetX = 0, int pixelOffsetY = 0);
    void renderPlayer(const Player &player, int cameraX, int cameraY);
    void renderNPC(const NPC &npc, int cameraX, int cameraY);
    void renderMapOverlay(const Map &map, int cameraX, int cameraY);

    // Camera follows the player, centered on screen
    void calculateCamera(const Player &player, const Map &map, int &cameraX,
                         int &cameraY) const;

  private:
    Renderer &renderer;
    bool spritesLoaded{false};

    // Walk frames per direction: [0]=idle, [1]=step1, [2]=idle2, [3]=step2
    // Index by Direction enum: down=0, up=1, left=2, right=3
    static constexpr int DIR_DOWN = 0;
    static constexpr int DIR_UP = 1;
    static constexpr int DIR_LEFT = 2;
    static constexpr int DIR_RIGHT = 3;

    // Sprite frame size in the spritesheet
    static constexpr int SPRITE_W = 32;
    static constexpr int SPRITE_H = 32;

    // 4 frames per direction (idle, step1, idle2, step2)
    std::array<std::array<SpriteFrame, 4>, 4> walkFrames;

    // Get the correct sprite frame for the player's current state
    SpriteFrame getPlayerFrame(const Player &player) const;
    SpriteFrame getNPCFrame(const NPC &npc) const;

    // Direction enum to index
    static int dirToIndex(Direction dir);

    // Map TileType to a placeholder color
    TDT4102::Color getTileColor(TileType type) const;
};
