#pragma once

constexpr int SRC_TILE_SIZE = 16;
constexpr int PIXEL_SCALE = 4;

constexpr int TILE_SIZE = SRC_TILE_SIZE * PIXEL_SCALE;

constexpr int VIEW_TILES_X = 16;
constexpr int VIEW_TILES_Y = 12;

constexpr int WINDOW_WIDTH = VIEW_TILES_X * TILE_SIZE;
constexpr int WINDOW_HEIGHT = VIEW_TILES_Y * TILE_SIZE;

constexpr int UI_PANEL_HEIGHT = 3 * TILE_SIZE;
