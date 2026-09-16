#pragma once

namespace pixel_world {

enum class FirstPersonHandAnimation {
    PistolFire,
    KnifeSlash,
};

class FirstPersonHands final {
public:
    static void drawBack(FirstPersonHandAnimation animation, float progress);
    static void drawGrip(FirstPersonHandAnimation animation, float progress);
};

}  // namespace pixel_world
