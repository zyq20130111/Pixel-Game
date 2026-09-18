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
constexpr float kGuardAttackDistance = 1.50f;
constexpr float kGuardStopDistance = 1.34f;
constexpr float kGuardAttackDuration = 0.82f;
constexpr float kGuardAttackLungeStart = 0.10f;
constexpr float kGuardAttackLungeEnd = 0.38f;
constexpr float kGuardAttackHitStart = 0.36f;
constexpr float kGuardAlertDuration = 3.5f;
constexpr float kGuardAttackCooldown = 0.62f;
constexpr float kGuardMaxChaseDistance = 17.0f;

float smoothStep01(float value) {
    const float t = std::clamp(value, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

}  // namespace

SecurityGuardModel::SecurityGuardModel(SecurityGuardRole role)
    : role_(role),
      difficulty_(Difficulty::Normal),
      position_({0.0f, 0.0f, 0.0f}),
      homePosition_({0.0f, 0.0f, 0.0f}),
      yawDegrees_(180.0f),
      animationPhase_(0.0f),
      deathTimer_(0.0f),
      health_(100),
      maxHealth_(100),
      alive_(true),
      patrolling_(false),
      patrolWaypoints_(),
      patrolWaypointIndex_(0),
      state_(SecurityGuardState::Patrol),
      playerDetected_(false),
      alertTimer_(0.0f),
      attackTimer_(0.0f),
      attackCooldown_(0.0f),
      attackHit_(false) {
    setDifficulty(difficulty_);
    reset();
}

void SecurityGuardModel::reset() {
    position_ = homePosition_;
    yawDegrees_ = 0.0f;
    animationPhase_ = 0.0f;
    deathTimer_ = 0.0f;
    health_ = maxHealth_;
    alive_ = true;
    patrolWaypointIndex_ = 0;
    state_ = SecurityGuardState::Patrol;
    playerDetected_ = false;
    alertTimer_ = 0.0f;
    attackTimer_ = 0.0f;
    attackCooldown_ = 0.0f;
    attackHit_ = false;
}

void SecurityGuardModel::update(float dt) {
    update(dt, position_, false, false, MovementCollisionTest{});
}

int SecurityGuardModel::update(
    float dt, const Vec3& playerPosition, bool playerDetected,
    bool weaponCanHitPlayer,
    const MovementCollisionTest& collisionTest) {
    if (!alive_) {
        deathTimer_ = std::min(kDeathDuration, deathTimer_ + dt);
        animationPhase_ = 0.0f;
        state_ = SecurityGuardState::Patrol;
        return 0;
    }

    attackCooldown_ = std::max(0.0f, attackCooldown_ - dt);

    const Vec3 playerDirection{playerPosition.x - position_.x, 0.0f,
                               playerPosition.z - position_.z};
    const float playerDistance = ThreeDUtils::length(playerDirection);

    if (playerDetected) {
        playerDetected_ = true;
        alertTimer_ = kGuardAlertDuration;
        if (state_ == SecurityGuardState::Returning) {
            state_ = SecurityGuardState::Chasing;
        }
    } else if (playerDetected_) {
        alertTimer_ = std::max(0.0f, alertTimer_ - dt);
        if (alertTimer_ <= 0.0f) {
            playerDetected_ = false;
        }
    }

    // A direct sighting can interrupt a return, while a group alarm is
    // filtered by ParkingLotScene until the guard reaches its home position.
    if (state_ == SecurityGuardState::Returning) {
        updateReturning(dt, collisionTest);
        return 0;
    }

    const Vec3 homeDirection{position_.x - homePosition_.x, 0.0f,
                             position_.z - homePosition_.z};
    if (state_ == SecurityGuardState::Chasing && playerDetected_ &&
        ThreeDUtils::length(homeDirection) > kGuardMaxChaseDistance) {
        beginReturning();
        return 0;
    }

    if (state_ == SecurityGuardState::Attacking) {
        const float previousAttackTimer = attackTimer_;
        attackTimer_ += dt;

        const float lungeStart =
            std::max(previousAttackTimer, kGuardAttackLungeStart);
        const float lungeEnd =
            std::min(attackTimer_, kGuardAttackLungeEnd);
        if (lungeEnd > lungeStart) {
            const float lungeDt = lungeEnd - lungeStart;
            const float lungeProgressStart = std::clamp(
                (lungeStart - kGuardAttackLungeStart) /
                    (kGuardAttackLungeEnd - kGuardAttackLungeStart),
                0.0f, 1.0f);
            const float lungeProgressEnd = std::clamp(
                (lungeEnd - kGuardAttackLungeStart) /
                    (kGuardAttackLungeEnd - kGuardAttackLungeStart),
                0.0f, 1.0f);
            const float lungeSpeed =
                (attackLungeSpeedAtProgress(lungeProgressStart) +
                 attackLungeSpeedAtProgress(lungeProgressEnd)) *
                0.5f;
            if (moveTo(playerPosition, lungeSpeed * lungeDt, collisionTest)) {
                animationPhase_ += lungeDt * 24.0f;
            }
        }

        animationPhase_ += dt * 14.0f;
        const Vec3 currentPlayerDirection{
            playerPosition.x - position_.x, 0.0f,
            playerPosition.z - position_.z};
        const float currentPlayerDistance =
            ThreeDUtils::length(currentPlayerDirection);
        if (currentPlayerDistance > 0.001f) {
            yawDegrees_ =
                std::atan2(currentPlayerDirection.x,
                           currentPlayerDirection.z) *
                180.0f / kPi;
        }

        int damage = 0;
        if (!attackHit_ && attackTimer_ >= kGuardAttackHitStart) {
            attackHit_ = true;
            if (weaponCanHitPlayer &&
                currentPlayerDistance <= kGuardAttackDistance + 0.20f) {
                damage = batonDamageForDifficulty(difficulty_);
            }
        }

        if (attackTimer_ >= kGuardAttackDuration) {
            if (playerDetected_) {
                state_ = SecurityGuardState::Chasing;
            } else {
                state_ = SecurityGuardState::Patrol;
            }
            attackTimer_ = 0.0f;
            attackCooldown_ = kGuardAttackCooldown;
            attackHit_ = false;
        }
        return damage;
    }

    if (playerDetected_) {
        if (weaponCanHitPlayer && playerDistance <= kGuardAttackDistance &&
            attackCooldown_ <= 0.0f) {
            state_ = SecurityGuardState::Attacking;
            attackTimer_ = 0.0f;
            attackHit_ = false;
            animationPhase_ = 0.0f;
            if (playerDistance > 0.001f) {
                yawDegrees_ =
                    std::atan2(playerDirection.x, playerDirection.z) *
                    180.0f / kPi;
            }
            return 0;
        }

        state_ = SecurityGuardState::Chasing;
        if (playerDistance > kGuardStopDistance) {
            const float moveDistance =
                std::min(chaseSpeedForDifficulty(difficulty_) * dt,
                         playerDistance - kGuardStopDistance);
            if (moveTo(playerPosition, moveDistance, collisionTest)) {
                animationPhase_ += dt * 10.0f;
            }
        } else {
            animationPhase_ += dt * 3.0f;
        }
        if (playerDistance > 0.001f) {
            yawDegrees_ =
                std::atan2(playerDirection.x, playerDirection.z) * 180.0f /
                kPi;
        }
        return 0;
    }

    state_ = SecurityGuardState::Patrol;
    updatePatrol(dt, collisionTest);
    return 0;
}

void SecurityGuardModel::render() const {
    const float deathProgress =
        alive_ ? 0.0f : std::clamp(deathTimer_ / kDeathDuration, 0.0f, 1.0f);
    const bool attackActive = alive_ &&
                              state_ == SecurityGuardState::Attacking;
    const float attackProgress =
        attackActive
            ? std::clamp(attackTimer_ / kGuardAttackDuration, 0.0f, 1.0f)
            : 0.0f;
    const float attackLungeProgress =
        attackActive
            ? std::clamp((attackProgress - kGuardAttackLungeStart /
                                           kGuardAttackDuration) /
                             ((kGuardAttackLungeEnd -
                               kGuardAttackLungeStart) /
                              kGuardAttackDuration),
                         0.0f, 1.0f)
            : 0.0f;
    const float legSwing =
        alive_ && (patrolling_ || state_ == SecurityGuardState::Chasing ||
                   state_ == SecurityGuardState::Returning)
            ? std::sin(animationPhase_) * 0.08f
            : (attackActive
                   ? std::sin(attackLungeProgress * kPi) * 0.13f
                   : 0.0f);

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
    } else if (attackActive) {
        float attackLean = 0.0f;
        float attackBodyOffset = 0.0f;
        if (attackProgress < 0.20f) {
            const float phase =
                smoothStep01(attackProgress / 0.20f);
            attackLean = -7.0f * phase;
            attackBodyOffset = -0.04f * phase;
        } else if (attackProgress < 0.52f) {
            const float phase =
                smoothStep01((attackProgress - 0.20f) / 0.32f);
            attackLean = -7.0f + 15.0f * phase;
            attackBodyOffset = -0.04f + 0.11f * phase;
        } else {
            const float phase =
                smoothStep01((attackProgress - 0.52f) / 0.48f);
            attackLean = 8.0f * (1.0f - phase);
            attackBodyOffset = 0.07f * (1.0f - phase);
        }
        glTranslatef(0.0f, 0.0f, attackBodyOffset);
        glRotatef(attackLean, 1.0f, 0.0f, 0.0f);
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

    float attackArmLift = 0.0f;
    float attackArmForward = 0.0f;
    if (attackActive) {
        if (attackProgress < 0.20f) {
            const float phase =
                smoothStep01(attackProgress / 0.20f);
            attackArmLift = 0.24f * phase;
            attackArmForward = -0.10f * phase;
        } else if (attackProgress < 0.52f) {
            const float phase =
                smoothStep01((attackProgress - 0.20f) / 0.32f);
            attackArmLift = 0.24f - 0.30f * phase;
            attackArmForward = -0.10f + 0.30f * phase;
        } else {
            const float phase =
                smoothStep01((attackProgress - 0.52f) / 0.48f);
            attackArmLift = -0.06f * (1.0f - phase);
            attackArmForward = 0.20f * (1.0f - phase);
        }
    }

    ThreeDUtils::drawCube({-0.70f, 1.44f, 0.0f},
                          {0.30f, 0.92f, 0.32f}, uniform);
    ThreeDUtils::drawCube(
        {0.70f, 1.44f + attackArmLift, attackArmForward},
        {0.30f, 0.92f, 0.32f}, uniform);
    ThreeDUtils::drawCube({-0.70f, 0.96f, 0.05f},
                          {0.26f, 0.30f, 0.32f}, kPoliceSkin);
    ThreeDUtils::drawCube(
        {0.70f, 0.96f + attackArmLift, 0.05f + attackArmForward},
        {0.26f, 0.30f, 0.32f}, kPoliceSkin);

    glPushMatrix();
    glTranslatef(0.74f, 1.08f + attackArmLift,
                 0.17f + attackArmForward);
    float batonAngle = -24.0f;
    float batonPitch = 0.0f;
    if (attackActive) {
        if (attackProgress < 0.20f) {
            const float phase =
                smoothStep01(attackProgress / 0.20f);
            batonAngle = -24.0f - 94.0f * phase;
            batonPitch = 18.0f * phase;
        } else if (attackProgress < 0.52f) {
            const float phase =
                smoothStep01((attackProgress - 0.20f) / 0.32f);
            batonAngle = -118.0f + 218.0f * phase;
            batonPitch = 18.0f - 88.0f * phase;
        } else {
            const float phase =
                smoothStep01((attackProgress - 0.52f) / 0.48f);
            batonAngle = 100.0f - 124.0f * phase;
            batonPitch = -70.0f * (1.0f - phase);
        }
    }
    glRotatef(batonPitch, 1.0f, 0.0f, 0.0f);
    glRotatef(batonAngle, 0.0f, 0.0f, 1.0f);
    ThreeDUtils::drawCube({0.0f, -0.18f, 0.0f},
                          {0.14f, 0.34f, 0.14f}, kPoliceBaton);
    ThreeDUtils::drawCube({0.0f, -0.48f, 0.0f},
                          {0.18f, 0.34f, 0.18f}, kPoliceBatonHighlight);
    glPopMatrix();

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
    const float y = kGuardHealthBarHeight;

    // Keep the bar in the guard's local space so its width follows the
    // same facing direction as the model.
    glPushMatrix();
    glTranslatef(position_.x, position_.y, position_.z);
    glRotatef(yawDegrees_, 0.0f, 1.0f, 0.0f);

    ThreeDUtils::drawCube({0.0f, y, 0.0f},
                          {fullWidth + 0.16f, 0.13f, 0.06f}, kInkColor);
    ThreeDUtils::drawCube({0.0f, y, 0.01f},
                          {fullWidth, 0.08f, 0.07f}, kHealthBarBack);
    if (healthRatio > 0.01f) {
        ThreeDUtils::drawCube(
            {-fullWidth * 0.5f + fullWidth * healthRatio * 0.5f,
             y, 0.02f},
            {fullWidth * healthRatio, 0.08f, 0.08f}, fill);
    }
    glPopMatrix();
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
    homePosition_ = position;
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

bool SecurityGuardModel::playerDetected() const {
    return playerDetected_;
}

bool SecurityGuardModel::attacking() const {
    return state_ == SecurityGuardState::Attacking;
}

bool SecurityGuardModel::returning() const {
    return state_ == SecurityGuardState::Returning;
}

bool SecurityGuardModel::weaponCanHitPlayer(
    const Vec3& playerPosition) const {
    if (!alive_) {
        return false;
    }

    const Vec3 direction{playerPosition.x - position_.x, 0.0f,
                         playerPosition.z - position_.z};
    return ThreeDUtils::length(direction) <= kGuardAttackDistance + 0.20f;
}

int SecurityGuardModel::health() const {
    return health_;
}

int SecurityGuardModel::maxHealth() const {
    return maxHealth_;
}

float SecurityGuardModel::yawDegrees() const {
    return yawDegrees_;
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

void SecurityGuardModel::applySniperDamage(const Vec3& hitPosition) {
    if (!alive_) {
        return;
    }

    health_ =
        std::max(0, health_ - sniperDamageForZone(hitZoneForPoint(hitPosition)));
    if (health_ <= 0) {
        alive_ = false;
        deathTimer_ = 0.0f;
    }
}

void SecurityGuardModel::applyKnifeDamage(const Vec3& hitPosition) {
    if (!alive_) {
        return;
    }

    health_ =
        std::max(0, health_ - knifeDamageForZone(hitZoneForPoint(hitPosition)));
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

int SecurityGuardModel::sniperDamageForZone(CharacterHitZone zone) {
    switch (zone) {
        case CharacterHitZone::Legs:
            return kSniperLegDamage;
        case CharacterHitZone::Waist:
            return kSniperWaistDamage;
        case CharacterHitZone::Head:
            return kSniperHeadDamage;
    }
    return kSniperLegDamage;
}

int SecurityGuardModel::knifeDamageForZone(CharacterHitZone zone) {
    switch (zone) {
        case CharacterHitZone::Legs:
            return kKnifeLegDamage;
        case CharacterHitZone::Waist:
            return kKnifeWaistDamage;
        case CharacterHitZone::Head:
            return kKnifeHeadDamage;
    }
    return kKnifeLegDamage;
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

int SecurityGuardModel::batonDamageForDifficulty(
    Difficulty difficulty) const {
    int damage = 12;
    switch (difficulty) {
        case Difficulty::Easy:
            damage = 8;
            break;
        case Difficulty::Normal:
            damage = 12;
            break;
        case Difficulty::Hard:
            damage = 18;
            break;
    }
    return role_ == SecurityGuardRole::Captain ? damage + 4 : damage;
}

float SecurityGuardModel::chaseSpeedForDifficulty(
    Difficulty difficulty) const {
    switch (difficulty) {
        case Difficulty::Easy:
            return role_ == SecurityGuardRole::Captain ? 2.35f : 2.05f;
        case Difficulty::Normal:
            return role_ == SecurityGuardRole::Captain ? 2.70f : 2.35f;
        case Difficulty::Hard:
            return role_ == SecurityGuardRole::Captain ? 3.10f : 2.70f;
    }
    return 2.35f;
}

float SecurityGuardModel::attackLungeSpeedForDifficulty(
    Difficulty difficulty) const {
    switch (difficulty) {
        case Difficulty::Easy:
            return role_ == SecurityGuardRole::Captain ? 5.80f : 5.20f;
        case Difficulty::Normal:
            return role_ == SecurityGuardRole::Captain ? 6.60f : 6.00f;
        case Difficulty::Hard:
            return role_ == SecurityGuardRole::Captain ? 7.40f : 6.80f;
    }
    return 6.00f;
}

float SecurityGuardModel::attackLungeSpeedAtProgress(float progress) const {
    const float normalizedProgress = std::clamp(progress, 0.0f, 1.0f);
    const float peakSpeed = attackLungeSpeedForDifficulty(difficulty_);

    if (normalizedProgress < 0.18f) {
        return peakSpeed *
               smoothStep01(normalizedProgress / 0.18f);
    }
    if (normalizedProgress < 0.70f) {
        const float plateauProgress =
            smoothStep01((normalizedProgress - 0.18f) / 0.52f);
        return peakSpeed * (1.12f + 0.08f * plateauProgress);
    }

    return peakSpeed * 1.20f *
           (1.0f -
            smoothStep01((normalizedProgress - 0.70f) / 0.30f));
}

void SecurityGuardModel::updatePatrol(
    float dt, const MovementCollisionTest& collisionTest) {
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
            yawDegrees_ =
                std::atan2(direction.x, direction.z) * 180.0f / kPi;
            constexpr float kPatrolSpeed = 1.7f;
            if (moveTo(target, std::min(kPatrolSpeed * dt, distance),
                       collisionTest)) {
                animationPhase_ += dt * 8.0f;
            }
        }
    } else {
        animationPhase_ += dt * 1.5f;
    }
}

bool SecurityGuardModel::updateReturning(
    float dt, const MovementCollisionTest& collisionTest) {
    const Vec3 direction{homePosition_.x - position_.x, 0.0f,
                         homePosition_.z - position_.z};
    const float distance = ThreeDUtils::length(direction);
    if (distance <= 0.16f) {
        position_ = homePosition_;
        state_ = SecurityGuardState::Patrol;
        playerDetected_ = false;
        alertTimer_ = 0.0f;
        patrolWaypointIndex_ = 0;
        animationPhase_ = 0.0f;
        return true;
    }

    if (distance > 0.001f) {
        yawDegrees_ =
            std::atan2(direction.x, direction.z) * 180.0f / kPi;
    }
    if (moveTo(homePosition_, chaseSpeedForDifficulty(difficulty_) * dt,
               collisionTest)) {
        animationPhase_ += dt * 10.0f;
    }
    return true;
}

void SecurityGuardModel::beginReturning() {
    state_ = SecurityGuardState::Returning;
    playerDetected_ = false;
    alertTimer_ = 0.0f;
    attackTimer_ = 0.0f;
    attackHit_ = false;
}

bool SecurityGuardModel::moveTo(
    const Vec3& target, float maxDistance,
    const MovementCollisionTest& collisionTest) {
    Vec3 direction{target.x - position_.x, 0.0f,
                   target.z - position_.z};
    const float distance = ThreeDUtils::length(direction);
    if (distance <= 0.001f || maxDistance <= 0.0f) {
        return false;
    }

    direction = ThreeDUtils::normalize(direction);
    const Vec3 displacement = direction * std::min(maxDistance, distance);
    const Vec3 candidate = position_ + displacement;
    if (!collisionTest || !collisionTest(candidate)) {
        position_ = candidate;
        return true;
    }

    const Vec3 xCandidate{position_.x + displacement.x, position_.y,
                          position_.z};
    if (!collisionTest(xCandidate)) {
        position_ = xCandidate;
        return true;
    }

    const Vec3 zCandidate{position_.x, position_.y,
                          position_.z + displacement.z};
    if (!collisionTest(zCandidate)) {
        position_ = zCandidate;
        return true;
    }

    const Vec3 slideDirection{-direction.z, 0.0f, direction.x};
    const Vec3 slideCandidate =
        position_ + slideDirection * std::min(maxDistance, distance);
    if (!collisionTest(slideCandidate)) {
        position_ = slideCandidate;
        return true;
    }

    const Vec3 reverseSlideDirection{direction.z, 0.0f, -direction.x};
    const Vec3 reverseSlideCandidate =
        position_ +
        reverseSlideDirection * std::min(maxDistance, distance);
    if (!collisionTest(reverseSlideCandidate)) {
        position_ = reverseSlideCandidate;
        return true;
    }
    return false;
}

}  // namespace pixel_world
