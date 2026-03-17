#pragma once
#include <algorithm>
#include <cmath>

namespace BattleAnimationUtils {

inline int interpolateLinearInt(int startValue, int endValue, int frame, int totalFrames) {
    if (totalFrames <= 0)
        return endValue;

    const int clampedFrame = std::clamp(frame, 0, totalFrames);
    const double t = static_cast<double>(clampedFrame) / static_cast<double>(totalFrames);
    const int interpolated = static_cast<int>(std::lround(
        static_cast<double>(startValue) + static_cast<double>(endValue - startValue) * t));

    return std::clamp(interpolated, std::min(startValue, endValue), std::max(startValue, endValue));
}

} // namespace BattleAnimationUtils
