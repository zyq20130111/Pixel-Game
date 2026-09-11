#pragma once

#include "types.h"

namespace pixel_world {

class MikoModel final {
public:
    MikoModel();
    ~MikoModel();

    void reset();
    void update(float dt);
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
