#pragma once
#include <AnimationWindow.h>
#include <Image.h>
#include <filesystem>
#include <string>
#include <unordered_map>

constexpr int SRC_TILE_SIZE = 16;
constexpr int PIXEL_SCALE = 4;

constexpr int TILE_SIZE = SRC_TILE_SIZE * PIXEL_SCALE;

constexpr int VIEW_TILES_X = 16;
constexpr int VIEW_TILES_Y = 12;

constexpr int WINDOW_WIDTH = VIEW_TILES_X * TILE_SIZE;
constexpr int WINDOW_HEIGHT = VIEW_TILES_Y * TILE_SIZE;

constexpr int UI_PANEL_HEIGHT = 3 * TILE_SIZE;

class Renderer {
  public:
    Renderer();

    TDT4102::AnimationWindow &getWindow();

    void beginFrame();
    void endFrame();
    bool shouldClose() const;

    void loadTexture(const std::string &id, const std::filesystem::path &path);
    TDT4102::Image &getTexture(const std::string &id);
    bool hasTexture(const std::string &id) const;

    void drawSprite(const std::string &textureId, int worldX, int worldY, int cameraX, int cameraY,
                    int srcWidth = TILE_SIZE, int srcHeight = TILE_SIZE);
    void drawSpriteRaw(const std::string &textureId, int screenX, int screenY,
                       int width = TILE_SIZE, int height = TILE_SIZE);
    void drawSpriteRegion(const std::string &textureId, int srcX, int srcY, int srcW, int srcH,
                          int dstX, int dstY, int dstW, int dstH, bool flipH = false);

    void drawTile(int spriteId, int worldX, int worldY, int cameraX, int cameraY);

    void drawText(const std::string &text, int screenX, int screenY,
                  TDT4102::Color color = TDT4102::Color::white, int fontSize = 16);

    void drawRect(int x, int y, int w, int h, TDT4102::Color fill,
                  TDT4102::Color border = TDT4102::Color::transparent);

    void drawFilledRect(int x, int y, int w, int h, TDT4102::Color color);

    static int worldToScreenX(int worldX, int cameraX);
    static int worldToScreenY(int worldY, int cameraY);
    static bool isOnScreen(int worldX, int worldY, int cameraX, int cameraY);

  private:
    TDT4102::AnimationWindow window;
    std::unordered_map<std::string, TDT4102::Image> textures;
};
