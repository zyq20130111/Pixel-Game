#include "Camera.h"

#include "game_constants.h"
#include "ThreeDUtils.h"

#include <algorithm>
#include <cmath>

namespace pixel_world {

Camera::Camera() {
    reset();
}

Camera::Camera(const Vec3& position) {
    reset(position);
}

void Camera::reset(const Vec3& position) {
    position_ = position;
    verticalVelocity_ = 0.0f;
    walkPhase_ = 0.0f;
    aimAmount_ = 0.0f;
    grounded_ = true;
    running_ = false;
    moving_ = false;
}

bool Camera::update(GLFWwindow* window, float dt, bool& previousJumpDown) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    Vec3 direction{};
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        direction.z -= 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        direction.z += 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        direction.x -= 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        direction.x += 1.0f;
    }

    moving_ = ThreeDUtils::length(direction) > 0.0f;
    running_ =
        moving_ && (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                    glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);
    if (moving_) {
        direction = ThreeDUtils::normalize(direction);
        const float speed =
            constants::kCameraMoveSpeed *
            (running_ ? constants::kCameraRunSpeedMultiplier : 1.0f);
        position_ = position_ + direction * (speed * dt);
        position_.x =
            std::clamp(position_.x, -constants::kWorldLimit, constants::kWorldLimit);
        position_.z =
            std::clamp(position_.z, -constants::kWorldLimit, constants::kWorldLimit);
        const float cycleSpeed =
            constants::kCharacterWalkCycleSpeed *
            (running_ ? constants::kCameraRunCycleMultiplier : 1.0f);
        walkPhase_ =
            std::fmod(walkPhase_ + dt * cycleSpeed, 2.0f * constants::kPi);
    } else {
        walkPhase_ = 0.0f;
    }

    const bool jumpDown = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    if (jumpDown && !previousJumpDown && grounded_) {
        verticalVelocity_ = constants::kCameraJumpVelocity;
        grounded_ = false;
    }
    previousJumpDown = jumpDown;

    if (!grounded_ || verticalVelocity_ != 0.0f) {
        verticalVelocity_ -= constants::kCameraGravity * dt;
        position_.y += verticalVelocity_ * dt;
        if (position_.y <= constants::kCameraGroundHeight) {
            position_.y = constants::kCameraGroundHeight;
            verticalVelocity_ = 0.0f;
            grounded_ = true;
        }
    }

    return moving_;
}

void Camera::updateAim(GLFWwindow* window, float dt) {
    const bool aimPressed =
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    const float target = aimPressed ? 1.0f : 0.0f;
    const float step = constants::kAimTransitionSpeed * dt;
    if (aimAmount_ < target) {
        aimAmount_ = std::min(target, aimAmount_ + step);
    } else {
        aimAmount_ = std::max(target, aimAmount_ - step);
    }
}

const Vec3& Camera::position() const {
    return position_;
}

float Camera::walkPhase() const {
    return walkPhase_;
}

float Camera::aimAmount() const {
    return aimAmount_;
}

bool Camera::running() const {
    return running_;
}

bool Camera::moving() const {
    return moving_;
}

bool Camera::grounded() const {
    return grounded_;
}

Vec3 Camera::toWorld(const Vec3& viewPosition) const {
    return ThreeDUtils::cameraToWorld(position_, viewPosition);
}

void Camera::apply() const {
    ThreeDUtils::applyCamera(position_);
}

}  // namespace pixel_world
