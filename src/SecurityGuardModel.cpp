#include "SecurityGuardModel.h"

#include "ThreeDUtils.h"
#include "game_constants.h"

#include <algorithm>
#include <cmath>

namespace pixel_world {
namespace {

using namespace constants;

constexpr float kGuardHalfWidth = 0.52f;
constexpr float kGuardHalfDepth = 0.44f;
constexpr float kGuardBottom = 0.04f;
constexpr float kGuardTop = 2.72f;
constexpr float kGuardLegTop = 0.92f;
constexpr float kGuardWaistTop = 1.86f;
constexpr float kGuardHealthBarHeight = 3.05f;
constexpr float kDeathDuration = 0.85f;

}  // namespace

SecurityGuardModel::SecurityGuardModel(SecurityGuardRole role)
    : role_(role),
      difficulty_(Difficulty::Normal),
      position_({0.0f, 0.0f, 0.0f}),
      yawDegrees_(180.0f),
      animationPhase_(0.0f),
      deathTimer_(0.0f),
      health_(100),
      maxHealth_(100),
      alive_(true),
      patrolling_(false),
      patrolWaypoints_(),
      patrolWaypointIndex_(0) {
    setDifficulty(difficulty_);
    reset();
}

void SecurityGuardModel::reset() {
    yawDegrees_ = 180.0f;
    animationPhase_ = 0.0f;
    deathTimer_ = 0.0f;
    health_ = maxHealth_;
    alive_ = true;
    patrolWaypointIndex_ = 0;
}

void SecurityGuardModel::update(float dt) {
    if (!alive_) {
        deathTimer_ = std::min(kDeathDuration, deathTimer_ + dt);
        animationPhase_ = 0.0f;
        return;
    }

    if (patrolling_ && !patrolWaypoints_.empty()) {
        const Vec3 target = patrolWaypoints_[patrolWaypointIndex_];
        Vec3 direction{target.x - position_.x, 0.0f,
                       target.z - position_.z};
        const float distance = ThreeDUtils::length(direction);
        if (distance < 0.16f) {
            patrolWaypointIndex_ =
                (patrolWaypointIndex_ + 1) % patrolWaypoints_.size();
        } else {
            direction = ThreeDUtils::normalize(direction);
            constexpr float kPatrolSpeed = 1.7f;
            position_ = position_ + direction * (kPatrolSpeed * dt);
            yawDegrees_ =
                std::atan2(direction.x, direction.z) * 180.0f / kPi;
            animationPhase_ += dt * 8.0f;
        }
    } else {
        animationPhase_ += dt * 1.5f;
    }

}

void SecurityGuardModel::render() const {
    const float deathProgress =
        alive_ ? 0.0f : std::clamp(deathTimer_ / kDeathDuration, 0.0f, 1.0f);
    const float legSwing =
        alive_ && patrolling_ ? std::sin(animationPhase_) * 0.08f : 0.0f;

    const Color uniform =
        role_ == SecurityGuardRole::Captain ? Color{0.42f, 0.16f, 0.12f}
                                             : Color{0.12f, 0.24f, 0.38f};
    const Color uniformLight =
        role_ == SecurityGuardRole::Captain ? Color{0.82f, 0.30f, 0.12f}
                                             : Color{0.24f, 0.48f, 0.64f};
    const Color uniformDark =
        role_ == SecurityGuardRole::Captain ? Color{0.22f, 0.07f, 0.06f}
                                             : Color{0.05f, 0.10f, 0.18f};
    const Color accent =
        role_ == SecurityGuardRole::Captain ? kPoliceBadge : kPoliceBadge;

    glPushMatrix();
    glTranslatef(position_.x, position_.y, position_.z);
    glRotatef(yawDegrees_, 0.0f, 1.0f, 0.0f);
    if (deathProgress > 0.0f) {
        glTranslatef(0.0f, 0.0f, 0.45f * deathProgress);
        glRotatef(-78.0f * deathProgress, 1.0f, 0.0f, 0.0f);
    }

    ThreeDUtils::drawCube({0.0f, 1.48f, 0.0f},
                          {1.06f, 1.12f, 0.68f}, uniform);
    ThreeDUtils::drawCube({0.0f, 1.54f, 0.36f},
                          {0.72f, 0.62f, 0.10f}, uniformLight);
    ThreeDUtils::drawCube({0.0f, 1.12f, 0.39f},
                          {0.86f, 0.14f, 0.10f}, kPoliceBelt);
    ThreeDUtils::drawCube({0.28f, 1.58f, 0.43f},
                          {0.16f, 0.20f, 0.07f}, accent);
    if (role_ == SecurityGuardRole::Captain) {
        ThreeDUtils::drawCube({-0.28f, 1.75f, 0.43f},
                              {0.18f, 0.08f, 0.07f}, accent);
        ThreeDUtils::drawCube({0.28f, 1.75f, 0.43f},
                              {0.18f, 0.08f, 0.07f}, accent);
    }

    ThreeDUtils::drawCube({-0.29f, 0.55f + legSwing, 0.0f},
                          {0.38f, 0.92f, 0.42f}, uniformDark);
    ThreeDUtils::drawCube({0.29f, 0.55f - legSwing, 0.0f},
                          {0.38f, 0.92f, 0.42f}, uniformDark);
    ThreeDUtils::drawCube({-0.29f, 0.08f + legSwing, 0.06f},
                          {0.42f, 0.18f, 0.62f}, kPoliceShoe);
    ThreeDUtils::drawCube({0.29f, 0.08f - legSwing, 0.06f},
                          {0.42f, 0.18f, 0.62f}, kPoliceShoe);

    ThreeDUtils::drawCube({-0.70f, 1.44f, 0.0f},
                          {0.30f, 0.92f, 0.32f}, uniform);
    ThreeDUtils::drawCube({0.70f, 1.44f, 0.0f},
                          {0.30f, 0.92f, 0.32f}, uniform);
    ThreeDUtils::drawCube({-0.70f, 0.96f, 0.05f},
                          {0.26f, 0.30f, 0.32f}, kPoliceSkin);
    ThreeDUtils::drawCube({0.70f, 0.96f, 0.05f},
                          {0.26f, 0.30f, 0.32f}, kPoliceSkin);

    ThreeDUtils::drawCube({0.0f, 2.35f, 0.0f},
                          {0.90f, 0.84f, 0.84f}, kPoliceSkin);
    ThreeDUtils::drawCube({0.0f, 2.79f, -0.02f},
                          {1.00f, 0.26f, 0.96f}, kPoliceCap);
    ThreeDUtils::drawCube({0.0f, 2.65f, 0.43f},
                          {0.66f, 0.12f, 0.28f}, kPoliceCapDark);
    ThreeDUtils::drawCube({0.0f, 2.77f, 0.47f},
                          {0.20f, 0.14f, 0.06f}, accent);
    ThreeDUtils::drawCube({-0.16f, 2.30f, 0.45f},
                          {0.20f, 0.18f, 0.08f}, kPoliceEyeWhite);
    ThreeDUtils::drawCube({0.16f, 2.30f, 0.45f},
                          {0.20f, 0.18f, 0.08f}, kPoliceEyeWhite);
    ThreeDUtils::drawCube({-0.16f, 2.30f, 0.50f},
                          {0.08f, 0.11f, 0.06f}, kPoliceEye);
    ThreeDUtils::drawCube({0.16f, 2.30f, 0.50f},
                          {0.08f, 0.11f, 0.06f}, kPoliceEye);
    ThreeDUtils::drawCube({0.0f, 2.14f, 0.47f},
                          {0.22f, 0.07f, 0.06f}, kPoliceMouth);

    glPopMatrix();
}

void SecurityGuardModel::renderHealthBar() const {
    if (!alive_ && deathTimer_ >= kDeathDuration) {
        return;
    }

    const float healthRatio =
        std::clamp(static_cast<float>(health_) /
                       static_cast<float>(std::max(1, maxHealth_)),
                   0.0f, 1.0f);
    const Color fill =
        healthRatio > 0.55f
            ? kHealthBarFill
            : (healthRatio > 0.25f ? kHealthBarMid : kHealthBarLow);
    const float fullWidth = role_ == SecurityGuardRole::Captain ? 1.42f : 1.16f;
    const float y = position_.y + kGuardHealthBarHeight;
    ThreeDUtils::drawCube({position_.x, y, position_.z},
                          {fullWidth + 0.16f, 0.13f, 0.06f}, kInkColor);
    ThreeDUtils::drawCube({position_.x, y, position_.z + 0.01f},
                          {fullWidth, 0.08f, 0.07f}, kHealthBarBack);
    if (healthRatio > 0.01f) {
        ThreeDUtils::drawCube(
            {position_.x - fullWidth * 0.5f +
                 fullWidth * healthRatio * 0.5f,
             y, position_.z + 0.02f},
            {fullWidth * healthRatio, 0.08f, 0.08f}, fill);
    }
}

void SecurityGuardModel::setRole(SecurityGuardRole role) {
    role_ = role;
    maxHealth_ = maxHealthForDifficulty(difficulty_);
    health_ = maxHealth_;
}

void SecurityGuardModel::setDifficulty(Difficulty difficulty) {
    difficulty_ = difficulty;
    maxHealth_ = maxHealthForDifficulty(difficulty_);
    health_ = maxHealth_;
}

void SecurityGuardModel::setPosition(const Vec3& position) {
    position_ = position;
}

void SecurityGuardModel::setPatrolling(bool patrolling) {
    patrolling_ = patrolling;
}

void SecurityGuardModel::setPatrolWaypoints(
    const std::vector<Vec3>& waypoints) {
    patrolWaypoints_ = waypoints;
    patrolWaypointIndex_ = 0;
}

const Vec3& SecurityGuardModel::position() const {
    return position_;
}

bool SecurityGuardModel::alive() const {
    return alive_;
}

bool SecurityGuardModel::defeated() const {
    return !alive_ && health_ <= 0;
}

bool SecurityGuardModel::isCaptain() const {
    return role_ == SecurityGuardRole::Captain;
}

int SecurityGuardModel::health() const {
    return health_;
}

int SecurityGuardModel::maxHealth() const {
    return maxHealth_;
}

bool SecurityGuardModel::segmentHit(const Vec3& start, const Vec3& end,
                                    float& hitT) const {
    if (!alive_) {
        return false;
    }

    const Vec3 boundsMin{position_.x - kGuardHalfWidth,
                         position_.y + kGuardBottom,
                         position_.z - kGuardHalfDepth};
    const Vec3 boundsMax{position_.x + kGuardHalfWidth,
                         position_.y + kGuardTop,
                         position_.z + kGuardHalfDepth};
    return segmentIntersectsAabb(start, end, boundsMin, boundsMax, hitT);
}

void SecurityGuardModel::applyPistolDamage(const Vec3& hitPosition) {
    if (!alive_) {
        return;
    }

    health_ =
        std::max(0, health_ - pistolDamageForZone(hitZoneForPoint(hitPosition)));
    if (health_ <= 0) {
        alive_ = false;
        deathTimer_ = 0.0f;
    }
}

bool SecurityGuardModel::segmentIntersectsAabb(
    const Vec3& start, const Vec3& end, const Vec3& boundsMin,
    const Vec3& boundsMax, float& hitT) {
    const Vec3 delta = end - start;
    float tMin = 0.0f;
    float tMax = 1.0f;

    const auto clipAxis = [&](float startCoord, float deltaCoord,
                              float minCoord, float maxCoord) {
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

CharacterHitZone SecurityGuardModel::hitZoneForPoint(
    const Vec3& hitPosition) const {
    const float localY = hitPosition.y - position_.y;
    if (localY <= kGuardLegTop) {
        return CharacterHitZone::Legs;
    }
    if (localY <= kGuardWaistTop) {
        return CharacterHitZone::Waist;
    }
    return CharacterHitZone::Head;
}

int SecurityGuardModel::pistolDamageForZone(CharacterHitZone zone) {
    switch (zone) {
        case CharacterHitZone::Legs:
            return kPistolLegDamage;
        case CharacterHitZone::Waist:
            return kPistolWaistDamage;
        case CharacterHitZone::Head:
            return kPistolHeadDamage;
    }
    return kPistolLegDamage;
}

int SecurityGuardModel::maxHealthForDifficulty(
    Difficulty difficulty) const {
    const bool captain = role_ == SecurityGuardRole::Captain;
    switch (difficulty) {
        case Difficulty::Easy:
            return captain ? 140 : 70;
        case Difficulty::Normal:
            return captain ? 180 : 100;
        case Difficulty::Hard:
            return captain ? 240 : 135;
    }
    return captain ? 180 : 100;
}

}  // namespace pixel_world
