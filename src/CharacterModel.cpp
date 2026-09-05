#include "CharacterModel.h"

#include "game_constants.h"
#include "ThreeDUtils.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace pixel_world {

CharacterModel::CharacterModel() {
    reset();
}

void CharacterModel::reset() {
    position_ = {0.0f, 0.0f, 4.2f};
    yawDegrees_ = 0.0f;
    walkPhase_ = 0.0f;
    patrolDirection_ = 1.0f;
    hp_ = constants::kCharacterMaxHp;
    alive_ = true;
    deathTimer_ = 0.0f;
}

void CharacterModel::update(GLFWwindow* window, float dt) {
    if (!alive_) {
        deathTimer_ =
            std::min(constants::kCharacterDeathDuration, deathTimer_ + dt);
        walkPhase_ = 0.0f;
        return;
    }

    Vec3 direction{};
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        direction.z -= 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        direction.z += 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        direction.x -= 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        direction.x += 1.0f;
    }

    const bool manuallyControlled = ThreeDUtils::length(direction) > 0.0f;
    if (manuallyControlled) {
        direction = ThreeDUtils::normalize(direction);
    } else {
        direction = {0.0f, 0.0f, patrolDirection_};
    }

    position_ = position_ + direction * (constants::kCharacterMoveSpeed * dt);
    position_.x = std::clamp(position_.x, constants::kCharacterPathMinX,
                             constants::kCharacterPathMaxX);
    position_.z = std::clamp(position_.z, constants::kCharacterPathMinZ,
                             constants::kCharacterPathMaxZ);

    if (manuallyControlled) {
        yawDegrees_ =
            std::atan2(direction.x, direction.z) * 180.0f / constants::kPi;
        if (std::abs(direction.z) > 0.1f) {
            patrolDirection_ = direction.z > 0.0f ? 1.0f : -1.0f;
        }
    } else if (position_.z >= constants::kCharacterPathMaxZ) {
        position_.z = constants::kCharacterPathMaxZ;
        patrolDirection_ = -1.0f;
        yawDegrees_ = 180.0f;
    } else if (position_.z <= constants::kCharacterPathMinZ) {
        position_.z = constants::kCharacterPathMinZ;
        patrolDirection_ = 1.0f;
        yawDegrees_ = 0.0f;
    }

    walkPhase_ = std::fmod(
        walkPhase_ + dt * constants::kCharacterWalkCycleSpeed,
        2.0f * constants::kPi);
}

void CharacterModel::render() const {
    const float deathProgress =
        alive_ ? 0.0f
               : ThreeDUtils::smoothStep(
                     deathTimer_ / constants::kCharacterDeathDuration);

    glPushMatrix();
    glTranslatef(position_.x, position_.y, position_.z);
    glRotatef(yawDegrees_, 0.0f, 1.0f, 0.0f);
    if (!alive_) {
        glTranslatef(0.0f, 0.0f, -0.16f * deathProgress);
        glRotatef(-86.0f * deathProgress, 1.0f, 0.0f, 0.0f);
    }
    glScalef(constants::kCharacterScale, constants::kCharacterScale,
             constants::kCharacterScale);

    const float walkSwing = std::sin(walkPhase_) * 18.0f;
    const float leftLegSwing =
        alive_ ? walkSwing : -8.0f * deathProgress;
    const float rightLegSwing =
        alive_ ? -walkSwing : 8.0f * deathProgress;
    const float leftArmSwing =
        alive_ ? -walkSwing : -70.0f * deathProgress;
    const float rightArmSwing =
        alive_ ? walkSwing : -55.0f * deathProgress;

    ThreeDUtils::drawPivotedCube({-0.20f, 1.55f, 0.05f}, {0.0f, -0.47f, 0.0f},
                                 {0.38f, 0.95f, 0.38f}, leftLegSwing,
                                 constants::kCharacterPants);
    ThreeDUtils::drawPivotedCube({-0.20f, 1.55f, 0.05f}, {0.0f, -1.20f, 0.03f},
                                 {0.34f, 0.70f, 0.34f}, leftLegSwing,
                                 constants::kCharacterBoot);
    ThreeDUtils::drawPivotedCube({0.20f, 1.55f, -0.05f}, {0.0f, -0.47f, 0.0f},
                                 {0.38f, 0.95f, 0.38f}, rightLegSwing,
                                 constants::kCharacterPantsDark);
    ThreeDUtils::drawPivotedCube({0.20f, 1.55f, -0.05f}, {0.0f, -1.20f, -0.03f},
                                 {0.34f, 0.70f, 0.34f}, rightLegSwing,
                                 constants::kCharacterBoot);

    ThreeDUtils::drawCube({0.0f, 1.65f, 0.0f}, {1.05f, 1.35f, 0.65f},
                          constants::kCharacterShirt);
    ThreeDUtils::drawCube({0.18f, 1.67f, -0.18f}, {0.42f, 0.90f, 0.25f},
                          constants::kCharacterShirtDark);
    ThreeDUtils::drawCube({-0.05f, 2.23f, 0.35f}, {0.70f, 0.18f, 0.42f},
                          constants::kCharacterSkin);

    ThreeDUtils::drawPivotedCube({-0.72f, 2.10f, 0.12f}, {0.0f, -0.40f, 0.0f},
                                 {0.28f, 1.00f, 0.28f}, leftArmSwing,
                                 constants::kCharacterShirtDark);
    ThreeDUtils::drawPivotedCube({-0.72f, 2.10f, 0.12f}, {0.0f, -1.10f, 0.02f},
                                 {0.24f, 0.92f, 0.24f}, leftArmSwing,
                                 constants::kCharacterSkin);
    ThreeDUtils::drawPivotedCube({0.72f, 2.10f, -0.12f}, {0.0f, -0.40f, 0.0f},
                                 {0.28f, 1.00f, 0.28f}, rightArmSwing,
                                 constants::kCharacterShirt);
    ThreeDUtils::drawPivotedCube({0.72f, 2.10f, -0.12f}, {0.0f, -1.10f, -0.02f},
                                 {0.24f, 0.92f, 0.24f}, rightArmSwing,
                                 constants::kCharacterSkin);

    ThreeDUtils::drawCube({0.0f, 3.05f, 0.0f}, {1.0f, 1.0f, 1.0f},
                          constants::kCharacterSkin);
    ThreeDUtils::drawCube({0.0f, 3.23f, -0.08f}, {1.08f, 0.48f, 1.08f},
                          constants::kCharacterHair);
    ThreeDUtils::drawCube({-0.15f, 3.01f, 0.52f}, {0.14f, 0.14f, 0.14f},
                          constants::kCharacterEye);
    ThreeDUtils::drawCube({0.15f, 3.01f, 0.52f}, {0.14f, 0.14f, 0.14f},
                          constants::kCharacterEye);
    ThreeDUtils::drawCube({0.0f, 2.87f, 0.54f}, {0.10f, 0.10f, 0.10f},
                          constants::kCharacterCheek);
    ThreeDUtils::drawCube({0.0f, 3.27f, 0.56f}, {0.24f, 0.12f, 0.10f},
                          constants::kCharacterHair);

    glPopMatrix();
}

void CharacterModel::renderHealthBar() const {
    if (!alive_ &&
        deathTimer_ >= constants::kCharacterDeathDuration) {
        return;
    }

    const float healthRatio = std::clamp(
        static_cast<float>(hp_) / static_cast<float>(constants::kCharacterMaxHp),
        0.0f, 1.0f);
    const Color fillColor =
        healthRatio > 0.55f
            ? constants::kHealthBarFill
            : (healthRatio > 0.25f ? constants::kHealthBarMid
                                   : constants::kHealthBarLow);
    const float barScale = 0.5f;
    const float fullWidth = 1.14f * barScale;
    const float fillWidth = fullWidth * healthRatio;
    const float x = position_.x;
    const float y = position_.y + constants::kCharacterHealthBarHeight;
    const float z = position_.z + 0.06f;

    ThreeDUtils::drawCube({x, y, z},
                          {1.32f * barScale, 0.22f * barScale,
                           0.08f * barScale},
                          constants::kInkColor);
    ThreeDUtils::drawCube({x, y, z + 0.01f},
                          {fullWidth, 0.11f * barScale, 0.09f * barScale},
                          constants::kHealthBarBack);
    if (fillWidth > 0.01f) {
        ThreeDUtils::drawCube(
            {x - fullWidth * 0.5f + fillWidth * 0.5f, y, z + 0.02f},
            {fillWidth, 0.11f * barScale, 0.10f * barScale}, fillColor);
    }

    for (int i = 1; i < 5; ++i) {
        ThreeDUtils::drawCube(
            {x - fullWidth * 0.5f +
                 fullWidth * 0.2f * static_cast<float>(i),
             y, z + 0.03f},
            {0.025f * barScale, 0.13f * barScale, 0.11f * barScale},
            constants::kInkColor);
    }
}

bool CharacterModel::segmentHit(const Vec3& start, const Vec3& end,
                                float& hitT) const {
    if (!alive_) {
        return false;
    }

    const Vec3 boundsMin{
        position_.x - constants::kCharacterHitHalfWidth,
        position_.y + constants::kCharacterHitBottom,
        position_.z - constants::kCharacterHitHalfDepth};
    const Vec3 boundsMax{
        position_.x + constants::kCharacterHitHalfWidth,
        position_.y + constants::kCharacterHitTop,
        position_.z + constants::kCharacterHitHalfDepth};
    return segmentIntersectsAabb(start, end, boundsMin, boundsMax, hitT);
}

void CharacterModel::applyPistolDamage(const Vec3& hitPosition) {
    if (!alive_) {
        return;
    }

    hp_ = std::max(
        0, hp_ - pistolDamageForZone(hitZoneForPoint(hitPosition)));
    if (hp_ <= 0) {
        kill();
    }
}

const Vec3& CharacterModel::position() const {
    return position_;
}

bool CharacterModel::alive() const {
    return alive_;
}

bool CharacterModel::segmentIntersectsAabb(const Vec3& start, const Vec3& end,
                                           const Vec3& boundsMin,
                                           const Vec3& boundsMax, float& hitT) {
    const Vec3 delta = end - start;
    float tMin = 0.0f;
    float tMax = 1.0f;

    const auto clipAxis = [&](float startCoord, float deltaCoord, float minCoord,
                              float maxCoord) {
        if (std::abs(deltaCoord) <= 0.0001f) {
            return startCoord >= minCoord && startCoord <= maxCoord;
        }

        float enter = (minCoord - startCoord) / deltaCoord;
        float exit = (maxCoord - startCoord) / deltaCoord;
        if (enter > exit) {
            std::swap(enter, exit);
        }

        tMin = std::max(tMin, enter);
        tMax = std::min(tMax, exit);
        return tMin <= tMax;
    };

    if (!clipAxis(start.x, delta.x, boundsMin.x, boundsMax.x) ||
        !clipAxis(start.y, delta.y, boundsMin.y, boundsMax.y) ||
        !clipAxis(start.z, delta.z, boundsMin.z, boundsMax.z)) {
        return false;
    }

    hitT = tMin;
    return true;
}

void CharacterModel::kill() {
    if (!alive_) {
        return;
    }

    hp_ = 0;
    alive_ = false;
    deathTimer_ = 0.0f;
    walkPhase_ = 0.0f;
}

CharacterHitZone CharacterModel::hitZoneForPoint(const Vec3& hitPosition) const {
    const float localHitY = hitPosition.y - position_.y;
    if (localHitY >= constants::kCharacterWaistHitTop) {
        return CharacterHitZone::Head;
    }
    if (localHitY >= constants::kCharacterLegHitTop) {
        return CharacterHitZone::Waist;
    }
    return CharacterHitZone::Legs;
}

int CharacterModel::pistolDamageForZone(CharacterHitZone zone) {
    switch (zone) {
        case CharacterHitZone::Head:
            return constants::kPistolHeadDamage;
        case CharacterHitZone::Waist:
            return constants::kPistolWaistDamage;
        case CharacterHitZone::Legs:
            return constants::kPistolLegDamage;
    }
    return 0;
}

}  // namespace pixel_world
