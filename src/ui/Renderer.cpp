#include "Renderer.h"

Renderer::Renderer()
    : window{50, 50, WINDOW_WIDTH, WINDOW_HEIGHT, "CipherBound"}
{
    window.setBackgroundColor(TDT4102::Color::black);
    window.hide_cursor();
}

TDT4102::AnimationWindow &Renderer::getWindow()
{
    return window;
}

void Renderer::beginFrame()
{
    // AnimationWindow clears automatically unless keep_previous_frame is on
}

void Renderer::endFrame()
{
    window.next_frame();
}

bool Renderer::shouldClose() const
{
    return window.should_close();
}

// --- Texture management ---

void Renderer::loadTexture(const std::string &id, const std::filesystem::path &path)
{
    textures.emplace(id, TDT4102::Image{path});
}

TDT4102::Image &Renderer::getTexture(const std::string &id)
{
    return textures.at(id);
}

bool Renderer::hasTexture(const std::string &id) const
{
    return textures.count(id) > 0;
}

// --- Drawing ---

int Renderer::worldToScreenX(int worldX, int cameraX)
{
    return (worldX - cameraX) * TILE_SIZE;
}

int Renderer::worldToScreenY(int worldY, int cameraY)
{
    return (worldY - cameraY) * TILE_SIZE;
}

bool Renderer::isOnScreen(int worldX, int worldY, int cameraX, int cameraY)
{
    int sx = worldToScreenX(worldX, cameraX);
    int sy = worldToScreenY(worldY, cameraY);
    return sx >= -TILE_SIZE && sx < WINDOW_WIDTH + TILE_SIZE &&
           sy >= -TILE_SIZE && sy < WINDOW_HEIGHT + TILE_SIZE;
}

void Renderer::drawSprite(const std::string &textureId, int worldX, int worldY,
                          int cameraX, int cameraY, int srcWidth, int srcHeight)
{
    if (!isOnScreen(worldX, worldY, cameraX, cameraY))
        return;

    int sx = worldToScreenX(worldX, cameraX);
    int sy = worldToScreenY(worldY, cameraY);

    if (hasTexture(textureId))
    {
        window.draw_image({sx, sy}, textures.at(textureId), srcWidth, srcHeight);
    }
    else
    {
        // Fallback: draw a colored square
        window.draw_rectangle({sx, sy}, TILE_SIZE, TILE_SIZE, TDT4102::Color::magenta);
    }
}

void Renderer::drawSpriteRaw(const std::string &textureId, int screenX, int screenY,
                             int width, int height)
{
    if (hasTexture(textureId))
    {
        window.draw_image({screenX, screenY}, textures.at(textureId), width, height);
    }
}

void Renderer::drawSpriteRegion(const std::string &textureId,
                                int srcX, int srcY, int srcW, int srcH,
                                int dstX, int dstY, int dstW, int dstH,
                                bool flipH)
{
    if (hasTexture(textureId))
    {
        window.draw_image_region({dstX, dstY}, textures.at(textureId), dstW, dstH,
                                 {srcX, srcY}, srcW, srcH,
                                 flipH ? TDT4102::FlipImage::HORIZONTAL : TDT4102::FlipImage::NONE);
    }
}

void Renderer::drawTile(int spriteId, int worldX, int worldY, int cameraX, int cameraY)
{
    if (!isOnScreen(worldX, worldY, cameraX, cameraY))
        return;

    int sx = worldToScreenX(worldX, cameraX);
    int sy = worldToScreenY(worldY, cameraY);

    // Map spriteId to a color for now (placeholder until tilesets are loaded)
    TDT4102::Color color = TDT4102::Color::dark_green;
    switch (spriteId)
    {
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

void Renderer::drawText(const std::string &text, int screenX, int screenY,
                        TDT4102::Color color, unsigned int fontSize)
{
    window.draw_text({screenX, screenY}, text, color, fontSize);
}

void Renderer::drawRect(int x, int y, int w, int h,
                        TDT4102::Color fill, TDT4102::Color border)
{
    window.draw_rectangle({x, y}, w, h, fill, border);
}

void Renderer::drawFilledRect(int x, int y, int w, int h, TDT4102::Color color)
{
    window.draw_rectangle({x, y}, w, h, color);
}
