#include "Renderer.h"
#include "../common/FilePaths.h"
#include <iostream>

Renderer::Renderer() : window{50, 50, WINDOW_WIDTH, WINDOW_HEIGHT, "CipherBound"} {
    window.setBackgroundColor(TDT4102::Color::black);
    window.hide_cursor();
}

TDT4102::AnimationWindow &Renderer::getWindow() { return window; }

void Renderer::beginFrame() {
    // AnimationWindow clears automatically unless keep_previous_frame is on
}

void Renderer::endFrame() { window.next_frame(); }

bool Renderer::shouldClose() const { return window.should_close(); }

void Renderer::loadTexture(const std::string &id, const std::filesystem::path &path) {
    if (hasTexture(id) || failedTextures.count(id) > 0)
        return;

    const std::filesystem::path resolved = FilePaths::resolveExistingPath(path);
    if (!std::filesystem::exists(resolved)) {
        markTextureFailed(id, "register", "file not found: " + resolved.string());
        return;
    }

    try {
        textures.emplace(id, TDT4102::Image{resolved});
        texturePaths[id] = resolved;
    } catch (const std::exception &e) {
        markTextureFailed(id, "register", e.what());
    }
}

TDT4102::Image &Renderer::getTexture(const std::string &id) { return textures.at(id); }

bool Renderer::hasTexture(const std::string &id) const { return textures.count(id) > 0; }

int Renderer::worldToScreenX(int worldX, int cameraX) { return (worldX - cameraX) * TILE_SIZE; }

int Renderer::worldToScreenY(int worldY, int cameraY) { return (worldY - cameraY) * TILE_SIZE; }

bool Renderer::isOnScreen(int worldX, int worldY, int cameraX, int cameraY) {
    int sx = worldToScreenX(worldX, cameraX);
    int sy = worldToScreenY(worldY, cameraY);
    return sx >= -TILE_SIZE && sx < WINDOW_WIDTH + TILE_SIZE && sy >= -TILE_SIZE &&
           sy < WINDOW_HEIGHT + TILE_SIZE;
}

void Renderer::drawSprite(const std::string &textureId, int worldX, int worldY, int cameraX,
                          int cameraY, int srcWidth, int srcHeight) {
    if (!isOnScreen(worldX, worldY, cameraX, cameraY))
        return;

    int sx = worldToScreenX(worldX, cameraX);
    int sy = worldToScreenY(worldY, cameraY);

    if (hasTexture(textureId)) {
        try {
            window.draw_image({sx, sy}, textures.at(textureId), srcWidth, srcHeight);
            return;
        } catch (const std::exception &e) {
            markTextureFailed(textureId, "draw", e.what());
        }
    }

    // Fallback: draw a colored square
    window.draw_rectangle({sx, sy}, TILE_SIZE, TILE_SIZE, TDT4102::Color::magenta);
}

void Renderer::drawSpriteRaw(const std::string &textureId, int screenX, int screenY, int width,
                             int height) {
    if (hasTexture(textureId)) {
        try {
            window.draw_image({screenX, screenY}, textures.at(textureId), width, height);
            return;
        } catch (const std::exception &e) {
            markTextureFailed(textureId, "draw", e.what());
        }
    }

    if (width > 0 && height > 0)
        window.draw_rectangle({screenX, screenY}, width, height, TDT4102::Color::magenta);
}

void Renderer::drawSpriteRegion(const std::string &textureId, int srcX, int srcY, int srcW,
                                int srcH, int dstX, int dstY, int dstW, int dstH, bool flipH) {
    if (hasTexture(textureId)) {
        try {
            window.draw_image_region({dstX, dstY}, textures.at(textureId), dstW, dstH,
                                     {srcX, srcY}, srcW, srcH,
                                     flipH ? TDT4102::FlipImage::HORIZONTAL
                                           : TDT4102::FlipImage::NONE);
            return;
        } catch (const std::exception &e) {
            markTextureFailed(textureId, "draw region", e.what());
        }
    }

    if (dstW > 0 && dstH > 0)
        window.draw_rectangle({dstX, dstY}, dstW, dstH, TDT4102::Color::magenta);
}

void Renderer::markTextureFailed(const std::string &id, const std::string &operation,
                                 const std::string &message) {
    std::cerr << "Renderer: failed to " << operation << " texture '" << id << "'";
    if (const auto it = texturePaths.find(id); it != texturePaths.end())
        std::cerr << " from '" << it->second.string() << "'";
    std::cerr << ": " << message << "\n";

    textures.erase(id);
    texturePaths.erase(id);
    failedTextures.insert(id);
}

void Renderer::drawTile(int spriteId, int worldX, int worldY, int cameraX, int cameraY) {
    if (!isOnScreen(worldX, worldY, cameraX, cameraY))
        return;

    int sx = worldToScreenX(worldX, cameraX);
    int sy = worldToScreenY(worldY, cameraY);

    // Map spriteId to a color for now (placeholder until tilesets are loaded)
    TDT4102::Color color = TDT4102::Color::dark_green;
    switch (spriteId) {
    case 0:
        color = TDT4102::Color::green;
        break; // grass
    case 1:
        color = TDT4102::Color::dark_green;
        break; // tall grass
    case 2:
        color = TDT4102::Color::blue;
        break; // water
    case 3:
        color = TDT4102::Color::yellow;
        break; // sand
    case 4:
        color = TDT4102::Color::light_gray;
        break; // path
    case 5:
        color = TDT4102::Color::dark_gray;
        break; // mountain
    case 6:
        color = TDT4102::Color::black;
        break; // wall
    case 7:
        color = TDT4102::Color::brown;
        break; // door
    default:
        color = TDT4102::Color::magenta;
        break;
    }

    window.draw_rectangle({sx, sy}, TILE_SIZE, TILE_SIZE, color);
}

void Renderer::drawText(const std::string &text, int screenX, int screenY, TDT4102::Color color,
                        int fontSize) {
    int clampedFontSize = std::max(fontSize, 1);
    window.draw_text({screenX, screenY}, text, color, static_cast<unsigned int>(clampedFontSize));
}

void Renderer::drawRect(int x, int y, int w, int h, TDT4102::Color fill, TDT4102::Color border) {
    window.draw_rectangle({x, y}, w, h, fill, border);
}

void Renderer::drawFilledRect(int x, int y, int w, int h, TDT4102::Color color) {
    window.draw_rectangle({x, y}, w, h, color);
}
