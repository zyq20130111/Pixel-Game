#pragma once

#include "ZombieAttackSound.h"
#include "types.h"

namespace pixel_world {

class ZombieModel final {
public:
    ZombieModel();
    ~ZombieModel();

    void reset();
    void update(float dt);
    void setAudioListener(const Vec3& position, const Vec3& forward);
    void render() const;

    const Vec3& position() const;

private:
    struct Impl;

    void buildMeshes();

    float animationPhase_;
    Vec3 position_;
    ZombieAttackSound attackSound_;
    Impl* impl_;
};

}  // namespace pixel_world
