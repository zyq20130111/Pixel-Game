#pragma once

#include "platform.h"
#include "types.h"

#include <functional>

namespace pixel_world {

class Camera {
public:
    using MovementCollisionTest = std::function<bool(const Vec3&)>;

    enum class Posture {
        Standing,
        Crouching,
        Prone,
    };

    Camera();
    explicit Camera(const Vec3& position);

    void reset(const Vec3& position = {0.0f, 2.8f, 17.5f});
    bool update(GLFWwindow* window, float dt, bool& previousJumpDown);
    bool update(GLFWwindow* window, float dt, bool& previousJumpDown,
                const MovementCollisionTest& collisionTest);
    void updateLook(GLFWwindow* window);
    void resetLookTracking(GLFWwindow* window);
    void updateAim(GLFWwindow* window, float dt);

    const Vec3& position() const;
    Vec3 forward() const;
    float yawDegrees() const;
    float pitchDegrees() const;
    float walkPhase() const;
    float aimAmount() const;
    bool running() const;
    bool moving() const;
    bool grounded() const;
    Posture posture() const;
    void setPosture(Posture posture);

    Vec3 toWorld(const Vec3& viewPosition) const;
    void apply() const;

private:
    Vec3 position_;
    float verticalVelocity_;
    float yawDegrees_;
    float pitchDegrees_;
    float walkPhase_;
    float aimAmount_;
    bool grounded_;
    bool running_;
    bool moving_;
    Posture posture_;
    double lastCursorX_;
    double lastCursorY_;
    bool lookInitialized_;
};

}  // namespace pixel_world
