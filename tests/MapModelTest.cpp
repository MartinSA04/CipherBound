#include "../src/state/Map.h"
#include <cassert>
#include <string>

int main() {
    Map map("test_map", 3, 3);

    assert(map.getId() == "test_map");
    assert(map.getWidth() == 3);
    assert(map.getHeight() == 3);
    assert(map.isInBounds({0, 0}));
    assert(!map.isInBounds({-1, 0}));

    Tile blocked = map.getTile({1, 1});
    blocked.type = TileType::wall;
    blocked.hasCollision = true;
    map.setTile({1, 1}, blocked);
    assert(!map.isWalkable({1, 1}));

    Tile ledge = map.getTile({2, 1});
    ledge.type = TileType::ledgeRight;
    ledge.hasCollision = false;
    map.setTile({2, 1}, ledge);
    assert(map.isWalkable({2, 1}, Direction::right));
    assert(!map.isWalkable({2, 1}, Direction::left));

    map.setOccupied({0, 1}, true);
    assert(!map.isWalkable({0, 1}));

    map.addEncounterSlot({1, 2, 4, 10});
    assert(map.hasWildEncounters());
    assert(map.getEncounterSlots().size() == 1);

    map.addWarp({{0, 0}, "next_map", {2, 2}});
    const WarpPoint *warp = map.getWarp({0, 0});
    assert(warp != nullptr);
    assert(warp->targetMapId == "next_map");
    const Position expectedWarpTarget{2, 2};
    assert(warp->targetPosition == expectedWarpTarget);

    map.setBackgroundImage("bg.png");
    map.setBackgroundImageOverlay("overlay.png");
    assert(map.hasBackgroundImage());
    assert(map.hasBackgroundImageOverlay());
    assert(map.getBackgroundImage() == "bg.png");
    assert(map.getBackgroundImageOverlay() == "overlay.png");

    return 0;
}
