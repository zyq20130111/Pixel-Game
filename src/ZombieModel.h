#pragma once

#include "types.h"

namespace pixel_world {

class SoundManager;

class ZombieModel final {
public:
    ZombieModel();
    ~ZombieModel();

    void reset();
    void update(float dt, SoundManager& soundManager);
    void render() const;

    const Vec3& position() const;

private:
    struct Impl;

    void buildMeshes();

    float animationPhase_;
    Vec3 position_;
    Impl* impl_;
};

}  // namespace pixel_world
