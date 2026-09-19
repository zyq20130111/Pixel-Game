#include "SecurityGuardModel.h"

#include "SecurityGuardBaton.h"
#include "SecurityGuardPose.h"
#include "SniperRifle.h"
#include "ThreeDUtils.h"
#include "WeaponBase.h"
#include "game_constants.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <vector>

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
constexpr float kCaptainSniperCooldown = 1.25f;
constexpr float kGuardMaxChaseDistance = 17.0f;

float smoothStep01(float value) {
    const float t = std::clamp(value, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

struct Vertex {
    GLfloat position[3];
    GLfloat color[3];
};

struct Matrix4 {
    GLfloat values[16];
};

Matrix4 identityMatrix() {
    return Matrix4{{1.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 1.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 1.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f}};
}

Matrix4 multiply(const Matrix4& left, const Matrix4& right) {
    Matrix4 result{};
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            float value = 0.0f;
            for (int inner = 0; inner < 4; ++inner) {
                value += left.values[inner * 4 + row] *
                         right.values[column * 4 + inner];
            }
            result.values[column * 4 + row] = value;
        }
    }
    return result;
}

Matrix4 translationMatrix(const Vec3& position) {
    Matrix4 result = identityMatrix();
    result.values[12] = position.x;
    result.values[13] = position.y;
    result.values[14] = position.z;
    return result;
}

Matrix4 rotationXMatrix(float angleDegrees) {
    const float angleRadians = angleDegrees * constants::kPi / 180.0f;
    const float sine = std::sin(angleRadians);
    const float cosine = std::cos(angleRadians);
    Matrix4 result = identityMatrix();
    result.values[5] = cosine;
    result.values[6] = sine;
    result.values[9] = -sine;
    result.values[10] = cosine;
    return result;
}

Matrix4 rotationYMatrix(float angleDegrees) {
    const float angleRadians = angleDegrees * constants::kPi / 180.0f;
    const float sine = std::sin(angleRadians);
    const float cosine = std::cos(angleRadians);
    Matrix4 result = identityMatrix();
    result.values[0] = cosine;
    result.values[2] = -sine;
    result.values[8] = sine;
    result.values[10] = cosine;
    return result;
}

Matrix4 rotationZMatrix(float angleDegrees) {
    const float angleRadians = angleDegrees * constants::kPi / 180.0f;
    const float sine = std::sin(angleRadians);
    const float cosine = std::cos(angleRadians);
    Matrix4 result = identityMatrix();
    result.values[0] = cosine;
    result.values[1] = sine;
    result.values[4] = -sine;
    result.values[5] = cosine;
    return result;
}

Matrix4 boneLocalMatrix(const Vec3& position, const Vec3& rotationDegrees) {
    Matrix4 result = translationMatrix(position);
    result = multiply(result, rotationZMatrix(rotationDegrees.z));
    result = multiply(result, rotationYMatrix(rotationDegrees.y));
    result = multiply(result, rotationXMatrix(rotationDegrees.x));
    return result;
}

Matrix4 scaleMatrix(const Vec3& scale) {
    Matrix4 result = identityMatrix();
    result.values[0] = scale.x;
    result.values[5] = scale.y;
    result.values[10] = scale.z;
    return result;
}

Vec3 transformPoint(const Matrix4& matrix, const Vec3& point) {
    return {
        matrix.values[0] * point.x + matrix.values[4] * point.y +
            matrix.values[8] * point.z + matrix.values[12],
        matrix.values[1] * point.x + matrix.values[5] * point.y +
            matrix.values[9] * point.z + matrix.values[13],
        matrix.values[2] * point.x + matrix.values[6] * point.y +
            matrix.values[10] * point.z + matrix.values[14],
    };
}

Matrix4 transposeMatrix(const Matrix4& matrix) {
    Matrix4 result{};
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            result.values[column * 4 + row] = matrix.values[row * 4 + column];
        }
    }
    return result;
}

class VertexBuffer {
public:
    void addTriangle(const Vec3& a, const Vec3& b, const Vec3& c,
                     const Color& color) {
        vertices_.push_back(makeVertex(a, color));
        vertices_.push_back(makeVertex(b, color));
        vertices_.push_back(makeVertex(c, color));
    }

    void addQuad(const Vec3& a, const Vec3& b, const Vec3& c,
                 const Vec3& d, const Color& color) {
        addTriangle(a, b, c, color);
        addTriangle(a, c, d, color);
    }

    void addUnitCube() {
        const Vec3 v0{-0.5f, -0.5f, -0.5f};
        const Vec3 v1{0.5f, -0.5f, -0.5f};
        const Vec3 v2{0.5f, 0.5f, -0.5f};
        const Vec3 v3{-0.5f, 0.5f, -0.5f};
        const Vec3 v4{-0.5f, -0.5f, 0.5f};
        const Vec3 v5{0.5f, -0.5f, 0.5f};
        const Vec3 v6{0.5f, 0.5f, 0.5f};
        const Vec3 v7{-0.5f, 0.5f, 0.5f};

        addQuad(v4, v5, v6, v7, {1.04f, 1.04f, 1.04f});
        addQuad(v0, v3, v2, v1, {0.78f, 0.78f, 0.78f});
        addQuad(v0, v1, v5, v4, {0.90f, 0.90f, 0.90f});
        addQuad(v1, v2, v6, v5, {1.00f, 1.00f, 1.00f});
        addQuad(v2, v3, v7, v6, {0.86f, 0.86f, 0.86f});
        addQuad(v3, v0, v4, v7, {0.94f, 0.94f, 0.94f});
    }

    void drawTransformed(const Matrix4& transform, const Color& tint) const {
        std::vector<Vertex> transformed;
        transformed.reserve(vertices_.size());
        for (const Vertex& vertex : vertices_) {
            const Vec3 position{vertex.position[0], vertex.position[1],
                                vertex.position[2]};
            const Vec3 transformedPosition = transformPoint(transform, position);
            transformed.push_back(
                Vertex{{transformedPosition.x, transformedPosition.y,
                        transformedPosition.z},
                       {vertex.color[0] * tint.red,
                        vertex.color[1] * tint.green,
                        vertex.color[2] * tint.blue}});
        }

        if (transformed.empty()) {
            return;
        }
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        glVertexPointer(3, GL_FLOAT, sizeof(Vertex),
                        transformed.data()->position);
        glColorPointer(3, GL_FLOAT, sizeof(Vertex),
                       transformed.data()->color);
        glDrawArrays(GL_TRIANGLES, 0,
                     static_cast<GLsizei>(transformed.size()));
        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
    }

private:
    static Vertex makeVertex(const Vec3& position, const Color& color) {
        return Vertex{{position.x, position.y, position.z},
                      {color.red, color.green, color.blue}};
    }

    std::vector<Vertex> vertices_;
};

constexpr float kTau = constants::kPi * 2.0f;

struct Bone {
    int parent;
    Vec3 localPosition;
    Vec3 localRotationDegrees;
    Matrix4 worldMatrix;
};

void setPoseBone(Pose& pose, BoneId bone, const Vec3& position,
                 const Vec3& rotationDegrees) {
    pose[bone] = {position, rotationDegrees};
}

Pose makeBasePose() {
    Pose pose{};
    setPoseBone(pose, kRootBone, {}, {});
    setPoseBone(pose, kTorsoBone, {0.0f, 1.30f, 0.0f}, {});
    setPoseBone(pose, kHeadBone, {0.0f, 0.88f, 0.0f}, {});
    setPoseBone(pose, kLeftUpperArmBone, {-0.62f, 0.30f, 0.0f}, {});
    setPoseBone(pose, kLeftLowerArmBone, {0.0f, -0.60f, 0.0f}, {});
    setPoseBone(pose, kRightUpperArmBone, {0.62f, 0.30f, 0.0f}, {});
    setPoseBone(pose, kRightLowerArmBone, {0.0f, -0.60f, 0.0f}, {});
    setPoseBone(pose, kLeftUpperLegBone, {-0.20f, -0.08f, 0.0f}, {});
    setPoseBone(pose, kLeftLowerLegBone, {0.0f, -0.62f, 0.0f}, {});
    setPoseBone(pose, kRightUpperLegBone, {0.20f, -0.08f, 0.0f}, {});
    setPoseBone(pose, kRightLowerLegBone, {0.0f, -0.62f, 0.0f}, {});
    setPoseBone(pose, kWeaponBone, {0.0f, -0.58f, 0.05f}, {});
    return pose;
}

void makeSecurityPose(Pose& pose, SecurityAnimation animation, float phase) {
    pose = makeBasePose();

    switch (animation) {
        case SecurityAnimation::Stand: {
            const float breathe = std::sin(phase * 1.2f) * 0.015f;
            setPoseBone(pose, kTorsoBone, {0.0f, 1.30f + breathe, 0.0f}, {});
            break;
        }

        case SecurityAnimation::Walk:
        case SecurityAnimation::Run: {
            const float legAmplitude =
                animation == SecurityAnimation::Walk ? 28.0f : 44.0f;
            const float bob =
                animation == SecurityAnimation::Walk ? 0.025f : 0.055f;
            const float gait = std::sin(phase);
            setPoseBone(pose, kRootBone,
                        {0.0f, bob * std::abs(std::sin(phase * 2.0f)), 0.0f},
                        {});
            setPoseBone(pose, kLeftUpperLegBone, {-0.20f, -0.08f, 0.0f},
                        {gait * legAmplitude, 0.0f, 0.0f});
            setPoseBone(pose, kRightUpperLegBone, {0.20f, -0.08f, 0.0f},
                        {-gait * legAmplitude, 0.0f, 0.0f});
            setPoseBone(pose, kLeftLowerLegBone, {0.0f, -0.62f, 0.0f},
                        {12.0f, 0.0f, 0.0f});
            setPoseBone(pose, kRightLowerLegBone, {0.0f, -0.62f, 0.0f},
                        {12.0f, 0.0f, 0.0f});
            break;
        }

        case SecurityAnimation::Aim:
        case SecurityAnimation::Fire:
            break;
    }
}

void drawBoundPart(const VertexBuffer& buffer, const Bone& bone,
                   const Vec3& offset, const Vec3& size,
                   const Color& tint) {
    const Matrix4 transform =
        multiply(bone.worldMatrix,
                 multiply(translationMatrix(offset), scaleMatrix(size)));
    buffer.drawTransformed(transform, tint);
}

}  // namespace

struct SecurityGuardModel::Impl {
    Impl() : unitCube(), bones{} {
        unitCube.addUnitCube();
        weaponInstance = std::make_unique<SecurityGuardBaton>();
        bones[kRootBone] = {-1, {}, {}, identityMatrix()};
        bones[kTorsoBone] = {kRootBone, {}, {}, identityMatrix()};
        bones[kHeadBone] = {kTorsoBone, {}, {}, identityMatrix()};
        bones[kLeftUpperArmBone] = {kTorsoBone, {}, {}, identityMatrix()};
        bones[kLeftLowerArmBone] = {
            kLeftUpperArmBone, {}, {}, identityMatrix()};
        bones[kRightUpperArmBone] = {kTorsoBone, {}, {}, identityMatrix()};
        bones[kRightLowerArmBone] = {
            kRightUpperArmBone, {}, {}, identityMatrix()};
        bones[kLeftUpperLegBone] = {kTorsoBone, {}, {}, identityMatrix()};
        bones[kLeftLowerLegBone] = {
            kLeftUpperLegBone, {}, {}, identityMatrix()};
        bones[kRightUpperLegBone] = {kTorsoBone, {}, {}, identityMatrix()};
        bones[kRightLowerLegBone] = {
            kRightUpperLegBone, {}, {}, identityMatrix()};
        bones[kWeaponBone] = {
            kRightLowerArmBone, {}, {}, identityMatrix()};
        updateSkeleton(SecurityAnimation::Stand, 0.0f);
    }

    void setWeaponInstance(SecurityGuardWeapon weapon) {
        if (weapon == SecurityGuardWeapon::Sniper) {
            weaponInstance = std::make_unique<SniperRifle>();
        } else {
            weaponInstance = std::make_unique<SecurityGuardBaton>();
        }
        weaponInstance->setViewMode(WeaponViewMode::ThirdPerson);
    }

    void updateSkeleton(SecurityAnimation animation, float phase) {
        Pose pose{};
        makeSecurityPose(pose, animation, phase);
        weaponInstance->applyThirdPersonPose(pose, animation, phase);
        for (std::size_t i = 0; i < kBoneCount; ++i) {
            bones[i].localPosition = pose[i].position;
            bones[i].localRotationDegrees = pose[i].rotationDegrees;
        }
        bones[kWeaponBone].parent = weaponInstance->thirdPersonWeaponBoneParent();
        for (std::size_t i = 0; i < kBoneCount; ++i) {
            const Matrix4 local = boneLocalMatrix(
                bones[i].localPosition, bones[i].localRotationDegrees);
            bones[i].worldMatrix =
                bones[i].parent < 0
                    ? local
                    : multiply(
                          bones[static_cast<std::size_t>(bones[i].parent)]
                              .worldMatrix,
                          local);
        }
    }

    VertexBuffer unitCube;
    std::array<Bone, kBoneCount> bones;
    std::unique_ptr<WeaponBase> weaponInstance;
};

SecurityGuardModel::SecurityGuardModel(SecurityGuardRole role)
    : role_(role),
      weapon_(role == SecurityGuardRole::Captain ? SecurityGuardWeapon::Sniper
                                                  : SecurityGuardWeapon::Baton),
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
      attackHit_(false),
      impl_(std::make_unique<Impl>()) {
    impl_->setWeaponInstance(weapon_);
    setDifficulty(difficulty_);
    reset();
}

SecurityGuardModel::~SecurityGuardModel() = default;

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
    impl_->weaponInstance->updateThirdPerson(
        dt, state_ == SecurityGuardState::Attacking, attackTimer_);

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

    if (weapon_ == SecurityGuardWeapon::Sniper) {
        return updateCaptain(dt, playerPosition, playerDetected_,
                             weaponCanHitPlayer, collisionTest);
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

int SecurityGuardModel::updateCaptain(
    float dt, const Vec3& playerPosition, bool playerDetected,
    bool weaponCanHitPlayer,
    const MovementCollisionTest& collisionTest) {
    const Vec3 playerDirection{playerPosition.x - position_.x, 0.0f,
                               playerPosition.z - position_.z};
    const float playerDistance = ThreeDUtils::length(playerDirection);

    if (playerDistance > 0.001f) {
        yawDegrees_ =
            std::atan2(playerDirection.x, playerDirection.z) * 180.0f / kPi;
    }

    if (state_ == SecurityGuardState::Attacking) {
        attackTimer_ += dt;
        animationPhase_ += dt * 8.0f;

        int damage = 0;
        if (!attackHit_ && attackTimer_ >= kGuardAttackHitStart) {
            attackHit_ = true;
            if (weaponCanHitPlayer && playerDetected) {
                damage = captainSniperDamageForDifficulty(difficulty_);
            }
        }

        if (attackTimer_ >= kGuardAttackDuration) {
            attackTimer_ = 0.0f;
            attackCooldown_ = kCaptainSniperCooldown;
            attackHit_ = false;
            state_ = (playerDetected && weaponCanHitPlayer)
                         ? SecurityGuardState::Aiming
                         : SecurityGuardState::Patrol;
        }
        return damage;
    }

    if (playerDetected && weaponCanHitPlayer) {
        if (attackCooldown_ <= 0.0f) {
            state_ = SecurityGuardState::Attacking;
            attackTimer_ = 0.0f;
            attackHit_ = false;
            animationPhase_ = 0.0f;
        } else {
            state_ = SecurityGuardState::Aiming;
            animationPhase_ += dt * 2.0f;
        }
        return 0;
    }

    if (playerDetected) {
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

    SecurityAnimation animation = SecurityAnimation::Stand;
    float phase = animationPhase_;
    if (!alive_) {
        animation = SecurityAnimation::Stand;
        phase = 0.0f;
    } else if (attackActive) {
        animation = SecurityAnimation::Fire;
        phase = attackProgress;
    } else if (state_ == SecurityGuardState::Aiming) {
        animation = SecurityAnimation::Aim;
    } else if (state_ == SecurityGuardState::Chasing) {
        animation = SecurityAnimation::Run;
    } else if (state_ == SecurityGuardState::Returning || patrolling_) {
        animation = SecurityAnimation::Walk;
    }
    impl_->updateSkeleton(animation, phase);

    const Color uniform =
        role_ == SecurityGuardRole::Captain ? Color{0.42f, 0.16f, 0.12f}
                                             : Color{0.12f, 0.24f, 0.38f};
    const Color uniformLight =
        role_ == SecurityGuardRole::Captain ? Color{0.82f, 0.30f, 0.12f}
                                             : Color{0.24f, 0.48f, 0.64f};
    const Color uniformDark =
        role_ == SecurityGuardRole::Captain ? Color{0.22f, 0.07f, 0.06f}
                                             : Color{0.05f, 0.10f, 0.18f};

    glPushMatrix();
    glTranslatef(position_.x, position_.y, position_.z);
    glRotatef(yawDegrees_, 0.0f, 1.0f, 0.0f);
    if (deathProgress > 0.0f) {
        glTranslatef(0.0f, 0.0f, 0.45f * deathProgress);
        glRotatef(-78.0f * deathProgress, 1.0f, 0.0f, 0.0f);
    }

    renderHumanModel(uniform, uniformLight, uniformDark);
    renderWeaponModel(attackActive, attackTimer_);

    glPopMatrix();
}

void SecurityGuardModel::renderHumanModel(
    const Color& uniform, const Color& uniformLight,
    const Color& uniformDark) const {
    const Bone& torso = impl_->bones[kTorsoBone];
    const Bone& head = impl_->bones[kHeadBone];
    const Bone& leftUpperArm = impl_->bones[kLeftUpperArmBone];
    const Bone& leftLowerArm = impl_->bones[kLeftLowerArmBone];
    const Bone& rightUpperArm = impl_->bones[kRightUpperArmBone];
    const Bone& rightLowerArm = impl_->bones[kRightLowerArmBone];
    const Bone& leftUpperLeg = impl_->bones[kLeftUpperLegBone];
    const Bone& leftLowerLeg = impl_->bones[kLeftLowerLegBone];
    const Bone& rightUpperLeg = impl_->bones[kRightUpperLegBone];
    const Bone& rightLowerLeg = impl_->bones[kRightLowerLegBone];

    drawBoundPart(impl_->unitCube, torso, {0.0f, -0.16f, 0.0f},
                  {1.02f, 1.04f, 0.62f}, uniform);
    drawBoundPart(impl_->unitCube, torso, {0.0f, 0.26f, 0.03f},
                  {0.70f, 0.18f, 0.44f}, uniformLight);
    drawBoundPart(impl_->unitCube, torso, {0.0f, -0.62f, 0.28f},
                  {0.80f, 0.12f, 0.16f}, kPoliceBelt);
    drawBoundPart(impl_->unitCube, torso, {0.28f, 0.34f, 0.32f},
                  {0.15f, 0.15f, 0.06f}, kPoliceBadge);

    drawBoundPart(impl_->unitCube, leftUpperLeg, {0.0f, -0.30f, 0.0f},
                  {0.34f, 0.60f, 0.40f}, uniformDark);
    drawBoundPart(impl_->unitCube, rightUpperLeg, {0.0f, -0.30f, 0.0f},
                  {0.34f, 0.60f, 0.40f}, uniformDark);
    drawBoundPart(impl_->unitCube, leftLowerLeg, {0.0f, -0.26f, 0.05f},
                  {0.30f, 0.52f, 0.36f}, uniformDark);
    drawBoundPart(impl_->unitCube, rightLowerLeg, {0.0f, -0.26f, 0.05f},
                  {0.30f, 0.52f, 0.36f}, uniformDark);
    drawBoundPart(impl_->unitCube, leftLowerLeg, {0.0f, -0.55f, 0.10f},
                  {0.40f, 0.16f, 0.60f}, kPoliceShoe);
    drawBoundPart(impl_->unitCube, rightLowerLeg, {0.0f, -0.55f, 0.10f},
                  {0.40f, 0.16f, 0.60f}, kPoliceShoe);

    drawBoundPart(impl_->unitCube, leftUpperArm, {0.0f, -0.33f, 0.0f},
                  {0.28f, 0.66f, 0.30f}, uniform);
    drawBoundPart(impl_->unitCube, rightUpperArm, {0.0f, -0.33f, 0.0f},
                  {0.28f, 0.66f, 0.30f}, uniform);
    drawBoundPart(impl_->unitCube, leftLowerArm, {0.0f, -0.28f, 0.02f},
                  {0.24f, 0.56f, 0.26f}, kPoliceSkin);
    drawBoundPart(impl_->unitCube, rightLowerArm, {0.0f, -0.28f, 0.02f},
                  {0.24f, 0.56f, 0.26f}, kPoliceSkin);

    drawBoundPart(impl_->unitCube, head, {0.0f, 0.13f, 0.07f},
                  {0.78f, 0.76f, 0.76f}, kPoliceSkin);
    drawBoundPart(impl_->unitCube, head, {0.0f, 0.52f, -0.03f},
                  {0.86f, 0.28f, 0.86f}, kPoliceCap);
    drawBoundPart(impl_->unitCube, head, {0.0f, 0.36f, 0.42f},
                  {0.64f, 0.08f, 0.22f}, kPoliceCapDark);
    drawBoundPart(impl_->unitCube, head, {-0.20f, 0.08f, 0.45f},
                  {0.17f, 0.12f, 0.05f}, kPoliceEyeWhite);
    drawBoundPart(impl_->unitCube, head, {0.20f, 0.08f, 0.45f},
                  {0.17f, 0.12f, 0.05f}, kPoliceEyeWhite);
    drawBoundPart(impl_->unitCube, head, {-0.20f, 0.08f, 0.49f},
                  {0.07f, 0.09f, 0.05f}, kPoliceEye);
    drawBoundPart(impl_->unitCube, head, {0.20f, 0.08f, 0.49f},
                  {0.07f, 0.09f, 0.05f}, kPoliceEye);
    drawBoundPart(impl_->unitCube, head, {0.0f, -0.18f, 0.45f},
                  {0.22f, 0.06f, 0.05f}, kPoliceMouth);

    if (weapon_ == SecurityGuardWeapon::Sniper) {
        drawBoundPart(impl_->unitCube, leftLowerArm, {0.0f, -0.50f, 0.02f},
                      {0.22f, 0.16f, 0.22f}, kPoliceShoe);
        drawBoundPart(impl_->unitCube, rightLowerArm, {0.0f, -0.50f, 0.02f},
                      {0.22f, 0.16f, 0.22f}, kPoliceShoe);
    }
}

void SecurityGuardModel::renderWeaponModel(bool attackActive,
                                           float attackTimer) const {
    const bool showMuzzleFlash =
        attackActive && attackTimer >= kGuardAttackHitStart &&
        attackTimer <= kGuardAttackHitStart + 0.12f;
    impl_->weaponInstance->renderThirdPerson(
        impl_->bones[kWeaponBone].worldMatrix.values, showMuzzleFlash);
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
    weapon_ = role == SecurityGuardRole::Captain ? SecurityGuardWeapon::Sniper
                                                  : SecurityGuardWeapon::Baton;
    impl_->setWeaponInstance(weapon_);
    maxHealth_ = maxHealthForDifficulty(difficulty_);
    health_ = maxHealth_;
}

void SecurityGuardModel::setDifficulty(Difficulty difficulty) {
    difficulty_ = difficulty;
    maxHealth_ = maxHealthForDifficulty(difficulty_);
    health_ = maxHealth_;
}

void SecurityGuardModel::setWeapon(SecurityGuardWeapon weapon) {
    weapon_ = weapon;
    impl_->setWeaponInstance(weapon_);
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

SecurityGuardWeapon SecurityGuardModel::weapon() const {
    return weapon_;
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

int SecurityGuardModel::captainSniperDamageForDifficulty(
    Difficulty difficulty) const {
    switch (difficulty) {
        case Difficulty::Easy:
            return 18;
        case Difficulty::Normal:
            return 24;
        case Difficulty::Hard:
            return 30;
    }
    return 24;
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
