#include "../src/common/FilePaths.h"
#include "../src/game_data/Pokedex.h"
#include "../src/state/Map.h"
#include "../src/state/World.h"
#include <algorithm>
#include <cassert>
#include <vector>

int main() {
    const std::filesystem::path dataRoot = FilePaths::resolveExistingPath("assets/data");

    Pokedex pokedex;
    pokedex.loadSpecies((dataRoot / "species.txt").string());
    pokedex.loadMoves((dataRoot / "moves.txt").string());
    pokedex.loadItems((dataRoot / "items.txt").string());

    World world(42);
    world.generate(pokedex);

    std::vector<std::string> mapIds = world.getMapIds();
    std::sort(mapIds.begin(), mapIds.end());

    const std::vector<std::string> expectedMapIds = {
        "bart_iver_lab",
        "neighbor_house_1f",
        "pallet_town",
        "player_house_1f",
        "player_house_2f",
        "route_1",
        "viridian_center",
        "viridian_mart",
        "viridian_town",
    };
    assert(mapIds == expectedMapIds);

    assert(world.getCurrentMapId() == "player_house_2f");
    assert((world.getPlayer().getPosition() == Position{2, 3}));
    assert(world.getMap(world.getCurrentMapId()).getTile(world.getPlayer().getPosition()).isOccupied);

    const WarpPoint *playerHouseWarp = world.getMap("pallet_town").getWarp({6, 7});
    assert(playerHouseWarp != nullptr);
    assert(playerHouseWarp->targetMapId == "player_house_1f");
    assert((playerHouseWarp->targetPosition == Position{4, 8}));

    const WarpPoint *stairsWarp = world.getMap("player_house_2f").getWarp({9, 2});
    assert(stairsWarp != nullptr);
    assert(stairsWarp->targetMapId == "player_house_1f");
    assert((stairsWarp->targetPosition == Position{10, 2}));
}
