#include "BulletBase.h"

namespace pixel_world {

BulletBase::BulletBase(const Vec3& position, const Vec3& velocity,
                       float lifetime, BulletType type)
    : position_(position),
      velocity_(velocity),
      lifetime_(lifetime),
      type_(type) {}

BulletBase::~BulletBase() = default;

void BulletBase::update(float dt) {
    position_ = position_ + velocity_ * dt;
    lifetime_ -= dt;
}

void BulletBase::expire() {
    lifetime_ = 0.0f;
}

bool BulletBase::active() const {
    return lifetime_ > 0.0f;
}

const Vec3& BulletBase::position() const {
    return position_;
}

const Vec3& BulletBase::velocity() const {
    return velocity_;
}

float BulletBase::lifetime() const {
    return lifetime_;
}

BulletType BulletBase::type() const {
    return type_;
}

void BulletBase::setPosition(const Vec3& position) {
    position_ = position;
}

}  // namespace pixel_world
