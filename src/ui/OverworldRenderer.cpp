#include "OverworldRenderer.h"

OverworldRenderer::OverworldRenderer(Renderer &renderer)
    : renderer(renderer)
{
    // Walk frame source rectangles from player_sheet.png
    // 4x4 grid: rows = down/up/left/right, cols = 4 walk cycle frames
    // Each cell is 32x32 pixels
    for (int dir = 0; dir < 4; ++dir)
    {
        for (int frame = 0; frame < 4; ++frame)
        {
            walkFrames[dir][frame] = {
                frame * SPRITE_W, // x
                dir * SPRITE_H,   // y
                SPRITE_W,         // w
                SPRITE_H          // h
            };
        }
    }
}

void OverworldRenderer::loadSprites()
{
    if (!spritesLoaded)
    {
        renderer.loadTexture("player", "assets/sprites/player/player_sheet.png");
        renderer.loadTexture("prof_bart_iver", "assets/sprites/npcs/bart_iver/bart_iver_sheet.png");
        spritesLoaded = true;
    }
}

void OverworldRenderer::loadMapBackground(const std::string &mapId, const std::string &imagePath)
{
    std::string texId = "map_bg_" + mapId;
    if (!renderer.hasTexture(texId))
    {
        renderer.loadTexture(texId, imagePath);
    }
}

void OverworldRenderer::loadMapBackgroundOverlay(const std::string &mapId, const std::string &imagePath)
{
    std::string texId = "map_bg_o_" + mapId;
    if (!renderer.hasTexture(texId))
    {
        renderer.loadTexture(texId, imagePath);
    }
}

void OverworldRenderer::loadMapBackgrounds(const World &world)
{
    for (const auto &mapId : world.getMapIds())
    {
        const Map &map = world.getMap(mapId);
        if (map.hasBackgroundImage())
        {
            loadMapBackground(map.getId(), map.getBackgroundImage());
        }
        if (map.hasBackgroundImageOverlay())
        {
            loadMapBackgroundOverlay(map.getId(), map.getBackgroundImageOverlay());
        }
    }
}

int OverworldRenderer::dirToIndex(Direction dir)
{
    switch (dir)
    {
    case Direction::down:
        return DIR_DOWN;
    case Direction::up:
        return DIR_UP;
    case Direction::left:
        return DIR_LEFT;
    case Direction::right:
        return DIR_RIGHT;
    default:
        return DIR_DOWN;
    }
}

SpriteFrame OverworldRenderer::getPlayerFrame(const Player &player) const
{
    int dirIdx = dirToIndex(player.getFacing());

    if (player.isMoving())
    {
        // 4-frame walk cycle: 0-1-2-3, pick based on walkFrame
        int step = player.getWalkFrame() % 4;
        return walkFrames[dirIdx][step];
    }
    else
    {
        // Idle: frame 0
        return walkFrames[dirIdx][0];
    }
}

SpriteFrame OverworldRenderer::getNPCFrame(const NPC &npc) const
{
    int dirIdx = dirToIndex(npc.getFacing());

    if (npc.isWalking())
    {
        // 4-frame walk cycle: 0-1-2-3, pick based on walkFrame
        int step = npc.getWalkFrame() % 4;
        return walkFrames[dirIdx][step];
    }
    else
    {
        // Idle: frame 0
        return walkFrames[dirIdx][0];
    }
}

void OverworldRenderer::render(const Map &map, const Player &player,
                               const std::vector<std::shared_ptr<NPC>> &npcs)
{
    int cameraX, cameraY;
    calculateCamera(player, map, cameraX, cameraY);

    renderMap(map, cameraX, cameraY);
    renderNPCs(npcs, cameraX, cameraY);
    renderPlayer(player, cameraX, cameraY);
    renderMapOverlay(map, cameraX, cameraY);
}

void OverworldRenderer::calculateCamera(const Player &player, const Map &map,
                                        int &cameraX, int &cameraY) const
{
    // Center camera on player (in pixels, accounting for animation offset)
    Position pos = player.getPosition();
    int playerPixelX = pos.x * TILE_SIZE + player.getPixelOffsetX();
    int playerPixelY = pos.y * TILE_SIZE + player.getPixelOffsetY();

    // Camera in pixels, centered on player
    int camPixelX = playerPixelX - (VIEW_TILES_X / 2) * TILE_SIZE;
    int camPixelY = playerPixelY - (VIEW_TILES_Y / 2) * TILE_SIZE;

    // Clamp to map bounds (in pixels)
    int mapPixelW = map.getWidth() * TILE_SIZE;
    int mapPixelH = map.getHeight() * TILE_SIZE;
    int viewPixelW = VIEW_TILES_X * TILE_SIZE;
    int viewPixelH = VIEW_TILES_Y * TILE_SIZE;

    if (camPixelX < 0)
        camPixelX = 0;
    if (camPixelY < 0)
        camPixelY = 0;
    if (camPixelX > mapPixelW - viewPixelW)
        camPixelX = mapPixelW - viewPixelW;
    if (camPixelY > mapPixelH - viewPixelH)
        camPixelY = mapPixelH - viewPixelH;

    // Handle maps smaller than viewport
    if (mapPixelW < viewPixelW)
        camPixelX = -(viewPixelW - mapPixelW) / 2;
    if (mapPixelH < viewPixelH)
        camPixelY = -(viewPixelH - mapPixelH) / 2;

    cameraX = camPixelX;
    cameraY = camPixelY;
}

void OverworldRenderer::renderMap(const Map &map, int cameraX, int cameraY)
{
    // Texture ID for this map's background
    std::string bgTexId = "map_bg_" + map.getId();
    bool hasBg = map.hasBackgroundImage() && renderer.hasTexture(bgTexId);

    if (hasBg)
    {
        // Draw the entire background as a single scaled image
        int dstW = map.getWidth() * TILE_SIZE;
        int dstH = map.getHeight() * TILE_SIZE;

        renderer.drawSpriteRaw(bgTexId, -cameraX, -cameraY, dstW, dstH);
    }
    else
    {
        // Fallback: draw colored tiles
        int startTileX = cameraX / TILE_SIZE;
        int startTileY = cameraY / TILE_SIZE;
        if (cameraX < 0)
            startTileX = cameraX / TILE_SIZE - 1;
        if (cameraY < 0)
            startTileY = cameraY / TILE_SIZE - 1;

        for (int y = startTileY; y < startTileY + VIEW_TILES_Y + 2; ++y)
        {
            for (int x = startTileX; x < startTileX + VIEW_TILES_X + 2; ++x)
            {
                Position pos{x, y};
                if (!map.isInBounds(pos))
                    continue;

                int sx = x * TILE_SIZE - cameraX;
                int sy = y * TILE_SIZE - cameraY;

                const Tile &tile = map.getTile(pos);
                TDT4102::Color color = getTileColor(tile.type);
                renderer.drawFilledRect(sx, sy, TILE_SIZE, TILE_SIZE, color);
            }
        }
    }
}

void OverworldRenderer::renderMapOverlay(const Map &map, int cameraX, int cameraY)
{
    std::string overlayTexId = "map_bg_o_" + map.getId();
    if (!map.hasBackgroundImageOverlay() || !renderer.hasTexture(overlayTexId))
        return;

    int dstW = map.getWidth() * TILE_SIZE;
    int dstH = map.getHeight() * TILE_SIZE;

    renderer.drawSpriteRaw(overlayTexId, -cameraX, -cameraY, dstW, dstH);
}

void OverworldRenderer::renderEntity(const Entity &entity, int cameraX, int cameraY,
                                     TDT4102::Color color, int pixelOffsetX, int pixelOffsetY)
{
    Position pos = entity.getPosition();
    int sx = pos.x * TILE_SIZE + pixelOffsetX - cameraX;
    int sy = pos.y * TILE_SIZE + pixelOffsetY - cameraY;

    // Draw entity as a colored circle (placeholder until sprites)
    renderer.getWindow().draw_circle({sx + TILE_SIZE / 2, sy + TILE_SIZE / 2},
                                     TILE_SIZE / 2 - 2, color);
}

void OverworldRenderer::renderPlayer(const Player &player, int cameraX, int cameraY)
{
    Position pos = player.getPosition();
    int sx = pos.x * TILE_SIZE + player.getPixelOffsetX() - cameraX;
    int sy = pos.y * TILE_SIZE + player.getPixelOffsetY() - cameraY;

    if (renderer.hasTexture("player"))
    {
        SpriteFrame frame = getPlayerFrame(player);

        // Scale sprite to 2x tile size so character fills one tile nicely
        // (the 32x32 source has ~7px padding on each side)
        int dstW = TILE_SIZE * 2;
        int dstH = TILE_SIZE * 2;

        // Center horizontally on the tile, anchor bottom to tile bottom
        int offsetX = (TILE_SIZE - dstW) / 2;
        int offsetY = TILE_SIZE - dstH;

        renderer.drawSpriteRegion("player",
                                  frame.x, frame.y, frame.w, frame.h,
                                  sx + offsetX, sy + offsetY, dstW, dstH,
                                  false);
    }
    else
    {
        // Placeholder: blue circle for player
        renderer.getWindow().draw_circle({sx + TILE_SIZE / 2, sy + TILE_SIZE / 2},
                                         TILE_SIZE / 2 - 2, TDT4102::Color::blue);
    }
}

void OverworldRenderer::renderNPCs(const std::vector<std::shared_ptr<NPC>> &npcs, int cameraX, int cameraY)
{
    for (const auto &npc : npcs)
    {

        Position pos = npc->getPosition();
        int sx = pos.x * TILE_SIZE + npc->getPixelOffsetX() - cameraX;
        int sy = pos.y * TILE_SIZE + npc->getPixelOffsetY() - cameraY;

        if (npc->isHidden())
            continue;

        if (renderer.hasTexture(npc->getId()))
        {
            SpriteFrame frame = getNPCFrame(*npc);

            // Scale sprite to 2x tile size so character fills one tile nicely
            // (the 32x32 source has ~7px padding on each side)
            int dstW = TILE_SIZE * 2;
            int dstH = TILE_SIZE * 2;

            // Center horizontally on the tile, anchor bottom to tile bottom
            int offsetX = (TILE_SIZE - dstW) / 2;
            int offsetY = TILE_SIZE - dstH;

            renderer.drawSpriteRegion(npc->getId(),
                                      frame.x, frame.y, frame.w, frame.h,
                                      sx + offsetX, sy + offsetY, dstW, dstH,
                                      false);
            continue;
        }

        TDT4102::Color color = TDT4102::Color::red;
        switch (npc->getType())
        {
        case NPCType::trainer:
        case NPCType::gymLeader:
            color = TDT4102::Color::red;
            break;
        case NPCType::shopkeeper:
            color = TDT4102::Color::orange;
            break;
        case NPCType::healer:
            color = TDT4102::Color::cyan;
            break;
        case NPCType::questGiver:
            color = TDT4102::Color::yellow;
            break;
        default:
            color = TDT4102::Color::red;
            break;
        }

        renderEntity(*npc, cameraX, cameraY, color,
                     npc->getPixelOffsetX(), npc->getPixelOffsetY());
    }
}

TDT4102::Color OverworldRenderer::getTileColor(TileType type) const
{
    switch (type)
    {
    case TileType::grass:
        return TDT4102::Color{100, 200, 80};
    case TileType::tallGrass:
        return TDT4102::Color{50, 160, 50};
    case TileType::water:
        return TDT4102::Color{60, 120, 220};
    case TileType::sand:
        return TDT4102::Color{220, 200, 130};
    case TileType::path:
        return TDT4102::Color{180, 160, 120};
    case TileType::mountain:
        return TDT4102::Color{120, 100, 80};
    case TileType::wall:
        return TDT4102::Color{60, 60, 60};
    case TileType::door:
        return TDT4102::Color{160, 100, 40};
    case TileType::ledgeUp:
    case TileType::ledgeDown:
    case TileType::ledgeLeft:
    case TileType::ledgeRight:
        return TDT4102::Color{80, 140, 60};
    default:
        return TDT4102::Color::magenta;
    }
}
