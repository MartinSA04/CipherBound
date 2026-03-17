#include "../src/battle/ui/BattleAnimationUtils.h"
#include <cassert>

int main() {
    constexpr int totalFrames = 120;

    assert(BattleAnimationUtils::interpolateLinearInt(0, 120, 0, totalFrames) == 0);
    assert(BattleAnimationUtils::interpolateLinearInt(0, 120, totalFrames, totalFrames) == 120);

    const int earlyFrameValue = BattleAnimationUtils::interpolateLinearInt(10, 70, 1, totalFrames);
    assert(earlyFrameValue > 10);

    const int midpointValue =
        BattleAnimationUtils::interpolateLinearInt(0, 120, totalFrames / 2, totalFrames);
    assert(midpointValue >= 59 && midpointValue <= 61);

    const int descendingValue =
        BattleAnimationUtils::interpolateLinearInt(80, 20, totalFrames / 4, totalFrames);
    assert(descendingValue < 80);
    assert(descendingValue > 20);

    return 0;
}
