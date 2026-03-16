#pragma once
#include "../state/Map.h"
#include "../state/NPC.h"
#include "../state/World.h"
#include "../state/player/Player.h"
#include "Renderer.h"
#include <array>
#include <memory>
#include <vector>

struct SpriteFrame {
    int x, y, w, h;
};

class OverworldRenderer {
  public:
    OverworldRenderer(Renderer &renderer);

    void loadSprites();

    void loadMapBackground(const std::string &mapId, const std::string &imagePath);
    void loadMapBackgroundOverlay(const std::string &mapId, const std::string &imagePath);

    void loadMapBackgrounds(const World &world);

    void render(const Map &map, const Player &player,
                const std::vector<std::unique_ptr<NPC>> &npcs);

    void renderMap(const Map &map, int cameraX, int cameraY);
    void renderEntity(const Entity &entity, int cameraX, int cameraY,
                      TDT4102::Color color = TDT4102::Color::red, int pixelOffsetX = 0,
                      int pixelOffsetY = 0);
    void renderPlayer(const Player &player, int cameraX, int cameraY);
    void renderNPC(const NPC &npc, int cameraX, int cameraY);
    void renderMapOverlay(const Map &map, int cameraX, int cameraY);

    void calculateCamera(const Player &player, const Map &map, int &cameraX, int &cameraY) const;

  private:
    Renderer &renderer;
    bool spritesLoaded{false};

    static constexpr int DIR_DOWN = 0;
    static constexpr int DIR_UP = 1;
    static constexpr int DIR_LEFT = 2;
    static constexpr int DIR_RIGHT = 3;

    static constexpr int SPRITE_W = 32;
    static constexpr int SPRITE_H = 32;

    // 4 frames per direction (idle, step1, idle2, step2)
    std::array<std::array<SpriteFrame, 4>, 4> walkFrames;

    SpriteFrame getPlayerFrame(const Player &player) const;
    SpriteFrame getNPCFrame(const NPC &npc) const;

    static int dirToIndex(Direction dir);

    TDT4102::Color getTileColor(TileType type) const;
};
