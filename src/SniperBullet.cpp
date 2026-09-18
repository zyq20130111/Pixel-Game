#include "SniperBullet.h"

#include "ThreeDUtils.h"
#include "game_constants.h"
#include "platform.h"

namespace pixel_world {
namespace {

void drawTracer(const Vec3& position, const Vec3& velocity) {
    const Vec3 direction = ThreeDUtils::normalize(velocity);
    const Vec3 tail = position - direction * 0.22f;

    glColor3f(constants::kSniperBulletTrail.red,
              constants::kSniperBulletTrail.green,
              constants::kSniperBulletTrail.blue);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glVertex3f(tail.x, tail.y, tail.z);
    glVertex3f(position.x, position.y, position.z);
    glEnd();
}

}  // namespace

SniperBullet::SniperBullet(const Vec3& position, const Vec3& velocity,
                           float lifetime)
    : BulletBase(position, velocity, lifetime, BulletType::Sniper) {}

void SniperBullet::render() const {
    drawTracer(position(), velocity());

    glPushMatrix();
    glTranslatef(position().x, position().y, position().z);
    ThreeDUtils::drawCube({}, {0.055f, 0.055f, 0.055f},
                          constants::kSniperBulletCore);
    glPopMatrix();
}

}  // namespace pixel_world
