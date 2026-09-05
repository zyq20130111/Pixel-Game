#pragma once

#include "BulletBase.h"

namespace pixel_world {

class PistolBullet final : public BulletBase {
public:
    PistolBullet(const Vec3& position, const Vec3& velocity,
                 float lifetime);

    void render() const override;
};

}  // namespace pixel_world
