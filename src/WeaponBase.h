#pragma once

#include "platform.h"
#include "types.h"

#include <memory>
#include <vector>

namespace pixel_world {

class Camera;
class BulletBase;
class SoundManager;

struct WeaponRenderMotion {
    Vec3 translation{};
    Vec3 rotationDegrees{};
};

class WeaponBase {
public:
    virtual ~WeaponBase();

    virtual bool update(GLFWwindow* window, const Camera& camera,
                        std::vector<std::unique_ptr<BulletBase>>& bullets,
                        SoundManager& soundManager, bool& previousFireDown,
                        float dt) = 0;
    virtual void render(const Camera& camera,
                        const WeaponRenderMotion& motion) const = 0;
    virtual float muzzleFlashTimer() const = 0;
};

}  // namespace pixel_world
