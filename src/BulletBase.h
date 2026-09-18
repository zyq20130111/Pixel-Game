#pragma once

#include "types.h"

namespace pixel_world {

enum class BulletType {
    Pistol,
    Sniper,
};

class BulletBase {
public:
    BulletBase(const Vec3& position, const Vec3& velocity, float lifetime,
               BulletType type);
    virtual ~BulletBase();

    virtual void render() const = 0;

    void update(float dt);
    void expire();
    bool active() const;
    const Vec3& position() const;
    const Vec3& velocity() const;
    float lifetime() const;
    BulletType type() const;
    void setPosition(const Vec3& position);

private:
    Vec3 position_;
    Vec3 velocity_;
    float lifetime_;
    BulletType type_;
};

}  // namespace pixel_world
