#include "Camera.h"

#include "game_constants.h"
#include "ThreeDUtils.h"

#include <algorithm>
#include <cmath>

namespace pixel_world {

namespace {

float postureHeight(Camera::Posture posture) {
    switch (posture) {
        case Camera::Posture::Crouching:
            return constants::kCameraCrouchHeight;
        case Camera::Posture::Prone:
            return constants::kCameraProneHeight;
        case Camera::Posture::Standing:
        default:
            return constants::kCameraGroundHeight;
    }
}

float postureSpeedMultiplier(Camera::Posture posture) {
    switch (posture) {
        case Camera::Posture::Crouching:
            return constants::kCharacterCrouchSpeedMultiplier;
        case Camera::Posture::Prone:
            return constants::kCharacterProneSpeedMultiplier;
        case Camera::Posture::Standing:
        default:
            return 1.0f;
    }
}

}  // namespace

Camera::Camera() {
    reset();
}

Camera::Camera(const Vec3& position) {
    reset(position);
}

void Camera::reset(const Vec3& position) {
    position_ = position;
    verticalVelocity_ = 0.0f;
    jumpOffset_ = 0.0f;
    currentHeight_ = position.y;
    yawDegrees_ = 0.0f;
    pitchDegrees_ = 0.0f;
    walkPhase_ = 0.0f;
    aimAmount_ = 0.0f;
    grounded_ = true;
    running_ = false;
    moving_ = false;
    posture_ = Posture::Standing;
    lastCursorX_ = 0.0;
    lastCursorY_ = 0.0;
    lookInitialized_ = false;
}

bool Camera::update(GLFWwindow* window, float dt, bool& previousJumpDown) {
    return update(window, dt, previousJumpDown, MovementCollisionTest{});
}

bool Camera::update(GLFWwindow* window, float dt, bool& previousJumpDown,
                    const MovementCollisionTest& collisionTest) {
    Vec3 input{};
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        input.z -= 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        input.z += 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        input.x -= 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        input.x += 1.0f;
    }

    moving_ = ThreeDUtils::length(input) > 0.0f;
    const bool runDown =
        glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
    running_ = moving_ && posture_ == Posture::Standing && runDown;
    if (moving_) {
        input = ThreeDUtils::normalize(input);
        const Vec3 forward =
            ThreeDUtils::cameraForward(yawDegrees_, 0.0f);
        const Vec3 right = ThreeDUtils::cameraRight(yawDegrees_, 0.0f);
        const Vec3 direction = right * input.x + forward * (-input.z);
        const float speed =
            constants::kCameraMoveSpeed * postureSpeedMultiplier(posture_) *
            (running_ ? constants::kCameraRunSpeedMultiplier : 1.0f);
        const Vec3 displacement = direction * (speed * dt);
        const float longestAxisMove =
            std::max(std::abs(displacement.x), std::abs(displacement.z));
        const int stepCount = std::max(
            1, static_cast<int>(std::ceil(longestAxisMove / 0.10f)));
        const Vec3 step = displacement * (1.0f / static_cast<float>(stepCount));

        // Subdivide fast movement so a single frame cannot tunnel through a
        // thin tree trunk or character collider. Resolving each axis
        // independently also lets the camera slide along the obstacle.
        for (int stepIndex = 0; stepIndex < stepCount; ++stepIndex) {
            if (std::abs(step.x) > 0.0001f) {
                Vec3 candidate = position_;
                candidate.x += step.x;
                if (!collisionTest || !collisionTest(candidate)) {
                    position_.x = candidate.x;
                }
            }
            if (std::abs(step.z) > 0.0001f) {
                Vec3 candidate = position_;
                candidate.z += step.z;
                if (!collisionTest || !collisionTest(candidate)) {
                    position_.z = candidate.z;
                }
            }
        }
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
        if (posture_ != Posture::Standing) {
            setPosture(Posture::Standing);
        } else {
            verticalVelocity_ = constants::kCameraJumpVelocity;
            jumpOffset_ = 0.0f;
            grounded_ = false;
        }
    }
    previousJumpDown = jumpDown;

    const float targetHeight = postureHeight(posture_);
    const float transition =
        1.0f - std::exp(-constants::kCameraStanceTransitionSpeed * dt);
    currentHeight_ += (targetHeight - currentHeight_) * transition;
    if (std::abs(targetHeight - currentHeight_) <= 0.001f) {
        currentHeight_ = targetHeight;
    }

    if (!grounded_ || verticalVelocity_ != 0.0f) {
        verticalVelocity_ -= constants::kCameraGravity * dt;
        jumpOffset_ += verticalVelocity_ * dt;
        if (jumpOffset_ <= 0.0f) {
            jumpOffset_ = 0.0f;
            verticalVelocity_ = 0.0f;
            grounded_ = true;
        }
    }
    position_.y = currentHeight_ + jumpOffset_;

    return moving_;
}

void Camera::updateLook(GLFWwindow* window) {
    double cursorX = 0.0;
    double cursorY = 0.0;
    glfwGetCursorPos(window, &cursorX, &cursorY);

    if (!lookInitialized_) {
        lastCursorX_ = cursorX;
        lastCursorY_ = cursorY;
        lookInitialized_ = true;
        return;
    }

    const double deltaX = cursorX - lastCursorX_;
    const double deltaY = cursorY - lastCursorY_;
    lastCursorX_ = cursorX;
    lastCursorY_ = cursorY;

    yawDegrees_ += static_cast<float>(deltaX) *
                   constants::kCameraLookSensitivity;
    pitchDegrees_ = std::clamp(
        pitchDegrees_ - static_cast<float>(deltaY) *
                           constants::kCameraLookSensitivity,
        -constants::kCameraPitchLimitDegrees,
        constants::kCameraPitchLimitDegrees);
}

void Camera::resetLookTracking(GLFWwindow* window) {
    if (window == nullptr) {
        lookInitialized_ = false;
        return;
    }

    int windowWidth = 1;
    int windowHeight = 1;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    lastCursorX_ = static_cast<double>(windowWidth) * 0.5;
    lastCursorY_ = static_cast<double>(windowHeight) * 0.5;
    glfwSetCursorPos(window, lastCursorX_, lastCursorY_);
    lookInitialized_ = false;
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

Vec3 Camera::forward() const {
    return ThreeDUtils::cameraForward(yawDegrees_, pitchDegrees_);
}

float Camera::yawDegrees() const {
    return yawDegrees_;
}

float Camera::pitchDegrees() const {
    return pitchDegrees_;
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

Camera::Posture Camera::posture() const {
    return posture_;
}

void Camera::setPosture(Posture posture) {
    posture_ = posture;
}

Vec3 Camera::toWorld(const Vec3& viewPosition) const {
    return ThreeDUtils::cameraToWorld(position_, viewPosition, yawDegrees_,
                                      pitchDegrees_);
}

void Camera::apply() const {
    ThreeDUtils::applyCamera(position_, yawDegrees_, pitchDegrees_);
}

}  // namespace pixel_world
