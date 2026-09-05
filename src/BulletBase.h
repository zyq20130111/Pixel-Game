#pragma once

#include "types.h"

namespace pixel_world {

class BulletBase {
public:
    BulletBase(const Vec3& position, const Vec3& velocity, float lifetime);
    virtual ~BulletBase();

    virtual void render() const = 0;

    void update(float dt);
    void expire();
    bool active() const;
    const Vec3& position() const;
    const Vec3& velocity() const;
    float lifetime() const;
    void setPosition(const Vec3& position);

private:
    Vec3 position_;
    Vec3 velocity_;
    float lifetime_;
};

}  // namespace pixel_world
