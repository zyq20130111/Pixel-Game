#pragma once

#include "types.h"

#include <memory>

namespace pixel_world {

class PoliceModel final {
public:
    PoliceModel();
    ~PoliceModel();

    void reset();
    void update(float dt);
    void render() const;
    void setPosition(const Vec3& position);
    void setCrouched(bool crouched);
    void setProne(bool prone);
    void setMoving(bool moving);

private:
    struct Impl;

    void buildMeshes();

    float animationPhase_;
    bool crouched_;
    bool prone_;
    bool moving_;
    Vec3 position_;
    std::unique_ptr<Impl> impl_;
};

}  // namespace pixel_world
