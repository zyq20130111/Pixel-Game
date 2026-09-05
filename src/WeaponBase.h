#pragma once

#include "platform.h"

#include <memory>
#include <vector>

namespace pixel_world {

class Camera;
class BulletBase;

class WeaponBase {
public:
    virtual ~WeaponBase();

    virtual void update(GLFWwindow* window, const Camera& camera,
                        std::vector<std::unique_ptr<BulletBase>>& bullets,
                        bool& previousFireDown, float dt) = 0;
    virtual void render(const Camera& camera) const = 0;
    virtual float muzzleFlashTimer() const = 0;
};

}  // namespace pixel_world
