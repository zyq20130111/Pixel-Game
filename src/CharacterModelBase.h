#pragma once

#include "platform.h"
#include "types.h"

namespace pixel_world {

class CharacterModelBase {
public:
    virtual ~CharacterModelBase();

    virtual void reset() = 0;
    virtual void update(GLFWwindow* window, float dt) = 0;
    virtual void render() const = 0;
    virtual void renderHealthBar() const = 0;
    virtual bool segmentHit(const Vec3& start, const Vec3& end,
                            float& hitT) const = 0;
    virtual void applyPistolDamage(const Vec3& hitPosition) = 0;

    virtual const Vec3& position() const = 0;
    virtual bool alive() const = 0;
};

}  // namespace pixel_world
