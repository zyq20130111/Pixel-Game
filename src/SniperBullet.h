#pragma once

#include "BulletBase.h"

namespace pixel_world {

class SniperBullet final : public BulletBase {
public:
    SniperBullet(const Vec3& position, const Vec3& velocity,
                 float lifetime);

    void render() const override;
};

}  // namespace pixel_world
