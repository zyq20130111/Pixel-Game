#include "PistolBullet.h"

#include "game_constants.h"
#include "platform.h"
#include "ThreeDUtils.h"

#include <cmath>

namespace pixel_world {

PistolBullet::PistolBullet(const Vec3& position, const Vec3& velocity,
                           float lifetime)
    : BulletBase(position, velocity, lifetime) {}

void PistolBullet::render() const {
    glPushMatrix();
    glTranslatef(position().x, position().y, position().z);
    glRotatef(-std::atan(0.22f) * 180.0f / constants::kPi, 1.0f, 0.0f, 0.0f);

    ThreeDUtils::drawCube({0.0f, 0.0f, -0.14f}, {0.08f, 0.08f, 0.24f},
                          constants::kBulletCore);
    ThreeDUtils::drawCube({0.0f, 0.0f, 0.12f}, {0.05f, 0.05f, 0.32f},
                          constants::kBulletTrail);

    glPopMatrix();
}

}  // namespace pixel_world
