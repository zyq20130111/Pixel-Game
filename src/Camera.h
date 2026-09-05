#pragma once

#include "platform.h"
#include "types.h"

namespace pixel_world {

class Camera {
public:
    Camera();
    explicit Camera(const Vec3& position);

    void reset(const Vec3& position = {0.0f, 2.8f, 10.0f});
    bool update(GLFWwindow* window, float dt, bool& previousJumpDown);
    void updateAim(GLFWwindow* window, float dt);

    const Vec3& position() const;
    float walkPhase() const;
    float aimAmount() const;
    bool running() const;
    bool moving() const;
    bool grounded() const;

    Vec3 toWorld(const Vec3& viewPosition) const;
    void apply() const;

private:
    Vec3 position_;
    float verticalVelocity_;
    float walkPhase_;
    float aimAmount_;
    bool grounded_;
    bool running_;
    bool moving_;
};

}  // namespace pixel_world
