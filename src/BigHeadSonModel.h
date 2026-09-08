#pragma once

#include "platform.h"

namespace pixel_world {

class BigHeadSonModel final {
public:
    BigHeadSonModel();
    ~BigHeadSonModel();

    void reset();
    void update(float dt);
    void render() const;
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
    Impl* impl_;
};

}  // namespace pixel_world
