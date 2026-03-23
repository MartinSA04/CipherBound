/**
 * @file
 * @brief Low-level rendering wrapper around `AnimationWindow` and texture management.
 * @ingroup app_core
 */

#pragma once
#include "UiConstants.h"
#include <AnimationWindow.h>
#include <Image.h>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>

/**
 * @brief Thin wrapper over `AnimationWindow` with texture caching and sprite helpers.
 * @ingroup app_core
 */
class Renderer {
  public:
    /// Creates the render window and internal texture caches.
    Renderer();

    /// Returns the underlying animation window.
    TDT4102::AnimationWindow &getWindow();

    /// Prepares the frame for drawing.
    void beginFrame();
    /// Presents the current frame.
    void endFrame();
    /// Returns whether the window requested close.
    bool shouldClose() const;

    /// Loads a texture and stores it under a string id.
    void loadTexture(const std::string &id, const std::filesystem::path &path);
    /// Returns a cached texture by id.
    TDT4102::Image &getTexture(const std::string &id);
    /// Returns whether a texture id is cached.
    bool hasTexture(const std::string &id) const;

    /// Draws a tile-aligned sprite in world space using the current camera.
    void drawSprite(const std::string &textureId, int worldX, int worldY, int cameraX, int cameraY,
                    int srcWidth = TILE_SIZE, int srcHeight = TILE_SIZE);
    /// Draws a sprite directly in screen space.
    void drawSpriteRaw(const std::string &textureId, int screenX, int screenY,
                       int width = TILE_SIZE, int height = TILE_SIZE);
    /// Draws a specific source rectangle to a destination rectangle.
    void drawSpriteRegion(const std::string &textureId, int srcX, int srcY, int srcW, int srcH,
                          int dstX, int dstY, int dstW, int dstH, bool flipH = false);

    /// Draws a map tile by sprite id in world space.
    void drawTile(int spriteId, int worldX, int worldY, int cameraX, int cameraY);

    /// Draws immediate-mode text using the window text API.
    void drawText(const std::string &text, int screenX, int screenY,
                  TDT4102::Color color = TDT4102::Color::white, int fontSize = 16);

    /// Draws a rectangle with optional border.
    void drawRect(int x, int y, int w, int h, TDT4102::Color fill,
                  TDT4102::Color border = TDT4102::Color::transparent);

    /// Draws a filled rectangle.
    void drawFilledRect(int x, int y, int w, int h, TDT4102::Color color);

    /// Converts world X to screen X using the camera origin.
    static int worldToScreenX(int worldX, int cameraX);
    /// Converts world Y to screen Y using the camera origin.
    static int worldToScreenY(int worldY, int cameraY);
    /// Returns whether a world-space sprite would be visible on screen.
    static bool isOnScreen(int worldX, int worldY, int cameraX, int cameraY);

  private:
    void markTextureFailed(const std::string &id, const std::string &operation,
                           const std::string &message);

    TDT4102::AnimationWindow window;                                     ///< Backing render window.
    std::unordered_map<std::string, TDT4102::Image> textures;            ///< Loaded texture cache.
    std::unordered_map<std::string, std::filesystem::path> texturePaths; ///< Texture source paths.
    std::unordered_set<std::string> failedTextures; ///< Texture ids that failed to load.
};
