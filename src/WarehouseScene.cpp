#include "WarehouseScene.h"

#include "ThreeDUtils.h"
#include "game_constants.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace pixel_world {
namespace {

using namespace constants;

constexpr float kCameraRadius = 0.42f;
constexpr float kRoomHalfWidth = 11.0f;
constexpr float kRoomHalfDepth = 11.0f;
constexpr float kWallHeight = 5.0f;
constexpr float kWallThickness = 0.45f;
constexpr float kDoorHalfWidth = 1.6f;
constexpr float kDoorHeight = 4.2f;
constexpr float kDoorZ = kRoomHalfDepth;
constexpr float kForkliftDoorCrashZ = 9.4f;
constexpr float kForkliftRadius = 1.0f;
constexpr float kForkliftSpeed = 5.5f;
constexpr float kForkliftReverseSpeed = 3.0f;
constexpr float kForkliftTurnSpeed = 80.0f;
constexpr float kGuardCollisionRadius = 0.62f;
constexpr float kGuardPlayerClearance = 0.92f;
constexpr float kGuardEyeHeight = 2.25f;
constexpr float kGuardSightDistance = 15.0f;
constexpr float kGuardSightHalfAngleCos = 0.30f;

constexpr float kPlatformCenterX = -8.5f;
constexpr float kPlatformCenterZ = -1.0f;
constexpr float kPlatformHalfX = 2.0f;
constexpr float kPlatformHalfZ = 3.0f;
constexpr float kPlatformTopY = 2.5f;
constexpr float kPileTopY = 1.8f;

constexpr Vec3 kForkliftStart{2.5f, 0.0f, 8.0f};
constexpr float kForkliftStartHeading = 180.0f;

constexpr WarehouseScene::Box kPlatformBox{
    {kPlatformCenterX, kPlatformTopY * 0.5f, kPlatformCenterZ},
    {2.0f * kPlatformHalfX, kPlatformTopY, 2.0f * kPlatformHalfZ}};
constexpr WarehouseScene::Box kColumnA{
    {-3.0f, 1.6f, -3.0f}, {1.2f, 3.2f, 1.2f}};
constexpr WarehouseScene::Box kColumnB{
    {3.0f, 1.6f, 3.0f}, {1.2f, 3.2f, 1.2f}};
constexpr WarehouseScene::Box kPileA{
    {-9.0f, 0.9f, -5.2f}, {1.7f, 1.8f, 1.7f}};
constexpr WarehouseScene::Box kPileB{
    {-7.0f, 0.9f, -5.2f}, {1.7f, 1.8f, 1.7f}};
constexpr WarehouseScene::Box kSniperCrateBox{
    {kPlatformCenterX, kPlatformTopY + 0.55f, 0.0f}, {1.2f, 1.1f, 1.2f}};
constexpr WarehouseScene::Box kShelfUpright{
    {9.0f, 1.5f, 0.0f}, {1.2f, 3.0f, 5.0f}};
constexpr WarehouseScene::Box kShelfFallen{
    {7.4f, 0.6f, 0.0f}, {3.0f, 1.2f, 5.6f}};

float distanceSquaredOnFloor(const Vec3& a, const Vec3& b) {
    const float dx = a.x - b.x;
    const float dz = a.z - b.z;
    return dx * dx + dz * dz;
}

bool circleIntersectsBoxImpl(float circleX, float circleZ, float radius,
                             const WarehouseScene::Box& box) {
    const float halfX = box.size.x * 0.5f;
    const float halfZ = box.size.z * 0.5f;
    const float closestX =
        std::clamp(circleX, box.center.x - halfX, box.center.x + halfX);
    const float closestZ =
        std::clamp(circleZ, box.center.z - halfZ, box.center.z + halfZ);
    const float deltaX = circleX - closestX;
    const float deltaZ = circleZ - closestZ;
    return deltaX * deltaX + deltaZ * deltaZ <= radius * radius;
}

}  // namespace

WarehouseScene::WarehouseScene(Language language)
    : sniperRifleTaken_(false),
      ironDoorOpen_(false),
      shelfFallen_(false),
      levelComplete_(false),
      drivingForklift_(false),
      forkliftHeadingDegrees_(kForkliftStartHeading),
      forkliftPosition_(kForkliftStart),
      language_(language),
      difficulty_(Difficulty::Normal),
      sniperFired_(false),
      sniperFiredPosition_{} {}

void WarehouseScene::reset(Difficulty difficulty) {
    difficulty_ = difficulty;
    sniperRifleTaken_ = false;
    ironDoorOpen_ = false;
    shelfFallen_ = false;
    levelComplete_ = false;
    drivingForklift_ = false;
    forkliftHeadingDegrees_ = kForkliftStartHeading;
    forkliftPosition_ = kForkliftStart;
    sniperFired_ = false;
    sniperFiredPosition_ = {};

    guards_[0].setRole(SecurityGuardRole::Guard);
    guards_[0].setDifficulty(difficulty_);
    guards_[0].setWeapon(SecurityGuardWeapon::Baton);
    guards_[0].setPosition({0.0f, 0.0f, -7.0f});
    guards_[0].setPatrolling(true);
    guards_[0].setPatrolWaypoints(
        {{-6.0f, 0.0f, -7.5f}, {6.0f, 0.0f, -7.5f}});
    guards_[0].reset();

    guards_[1].setRole(SecurityGuardRole::Guard);
    guards_[1].setDifficulty(difficulty_);
    guards_[1].setWeapon(SecurityGuardWeapon::Baton);
    guards_[1].setPosition({7.5f, 0.0f, -3.0f});
    guards_[1].setPatrolling(true);
    guards_[1].setPatrolWaypoints(
        {{7.5f, 0.0f, -3.0f}, {7.5f, 0.0f, 3.0f}});
    guards_[1].reset();

    guards_[2].setRole(SecurityGuardRole::Guard);
    guards_[2].setDifficulty(difficulty_);
    guards_[2].setWeapon(SecurityGuardWeapon::Baton);
    guards_[2].setPosition({2.5f, 0.0f, 9.2f});
    guards_[2].setPatrolling(false);
    guards_[2].setPatrolWaypoints({});
    guards_[2].reset();
    guards_[2].setYawDegrees(180.0f);
}

int WarehouseScene::update(float dt, const Vec3& playerPosition) {
    if (levelComplete_) {
        return 0;
    }

    updateDoorAndExit(playerPosition);
    if (levelComplete_) {
        return 0;
    }

    return updateGuards(dt, playerPosition);
}

void WarehouseScene::updateDoorAndExit(const Vec3& playerPosition) {
    if (!ironDoorOpen_) {
        return;
    }

    if (std::abs(playerPosition.x) <= kDoorHalfWidth &&
        playerPosition.z > kRoomHalfDepth + 0.6f) {
        levelComplete_ = true;
    }
}

int WarehouseScene::updateGuards(float dt, const Vec3& playerPosition) {
    sniperFired_ = false;
    int damage = 0;
    for (std::size_t index = 0; index < guards_.size(); ++index) {
        SecurityGuardModel& guard = guards_[index];
        if (!guard.alive()) {
            guard.update(dt);
            continue;
        }

        const bool canSee = canGuardSeePlayer(guard, playerPosition);
        const bool canHit = hasLineOfSight(guard, playerPosition);
        const Vec3 currentGuardPosition = guard.position();
        damage += guard.update(
            dt, playerPosition, canSee, canHit,
            [this, index, &playerPosition, currentGuardPosition](
                const Vec3& position) {
                return guardPositionBlocked(position, currentGuardPosition,
                                            index, playerPosition);
            });
    }
    return damage;
}

bool WarehouseScene::hasLineOfSight(
    const SecurityGuardModel& guard, const Vec3& playerPosition) const {
    if (onPlatform(playerPosition)) {
        return false;
    }
    const Vec3 guardEye{guard.position().x,
                        guard.position().y + kGuardEyeHeight,
                        guard.position().z};
    const Vec3 playerEye{playerPosition.x, playerPosition.y,
                         playerPosition.z};
    float hitT = 0.0f;
    return !segmentHitsGeometry(guardEye, playerEye, hitT);
}

bool WarehouseScene::canGuardSeePlayer(
    const SecurityGuardModel& guard, const Vec3& playerPosition) const {
    const Vec3 toPlayer{playerPosition.x - guard.position().x, 0.0f,
                        playerPosition.z - guard.position().z};
    const float distance = ThreeDUtils::length(toPlayer);
    if (distance <= 0.001f || distance > kGuardSightDistance) {
        return false;
    }

    const Vec3 direction = ThreeDUtils::normalize(toPlayer);
    const float yawRadians = guard.yawDegrees() * kPi / 180.0f;
    const Vec3 facing{std::sin(yawRadians), 0.0f,
                      std::cos(yawRadians)};
    if (ThreeDUtils::dot(direction, facing) < kGuardSightHalfAngleCos) {
        return false;
    }

    return hasLineOfSight(guard, playerPosition);
}

bool WarehouseScene::guardPositionBlocked(
    const Vec3& position, const Vec3& currentPosition,
    std::size_t movingGuardIndex, const Vec3& playerPosition) const {
    (void)currentPosition;
    if (position.x < -kRoomHalfWidth + kGuardCollisionRadius ||
        position.x > kRoomHalfWidth - kGuardCollisionRadius ||
        position.z < -kRoomHalfDepth + kGuardCollisionRadius ||
        position.z > kRoomHalfDepth - kGuardCollisionRadius) {
        return true;
    }

    if (distanceSquaredOnFloor(position, playerPosition) <
        kGuardPlayerClearance * kGuardPlayerClearance) {
        return true;
    }

    const WarehouseScene::Box solidBoxes[] = {
        kPlatformBox, kColumnA, kColumnB, kPileA, kPileB, kSniperCrateBox,
        shelfBox(), forkliftBox()};
    for (const Box& box : solidBoxes) {
        if (circleIntersectsBox(position.x, position.z,
                                kGuardCollisionRadius, box)) {
            return true;
        }
    }

    for (std::size_t index = 0; index < guards_.size(); ++index) {
        if (index == movingGuardIndex || !guards_[index].alive()) {
            continue;
        }
        const Vec3& other = guards_[index].position();
        const float minDistance = kGuardCollisionRadius * 2.0f;
        if (distanceSquaredOnFloor(position, other) <
            minDistance * minDistance) {
            return true;
        }
    }
    return false;
}

bool WarehouseScene::forkliftBlocked(const Vec3& position) const {
    if (position.x < -kRoomHalfWidth + kForkliftRadius ||
        position.x > kRoomHalfWidth - kForkliftRadius ||
        position.z < -kRoomHalfDepth + kForkliftRadius ||
        position.z > kRoomHalfDepth - kForkliftRadius) {
        return true;
    }

    const WarehouseScene::Box solidBoxes[] = {
        kPlatformBox, kColumnA, kColumnB, kPileA, kPileB, kSniperCrateBox,
        shelfBox()};
    for (const Box& box : solidBoxes) {
        if (circleIntersectsBox(position.x, position.z, kForkliftRadius,
                                box)) {
            return true;
        }
    }

    for (const SecurityGuardModel& guard : guards_) {
        if (!guard.alive()) {
            continue;
        }
        const Vec3& other = guard.position();
        const float minDistance = kForkliftRadius + kGuardCollisionRadius;
        if (distanceSquaredOnFloor(position, other) <
            minDistance * minDistance) {
            return true;
        }
    }
    return false;
}

bool WarehouseScene::platformTopContains(const Vec3& position) const {
    const float minX = kPlatformCenterX - kPlatformHalfX;
    const float maxX = kPlatformCenterX + kPlatformHalfX;
    const float minZ = kPlatformCenterZ - kPlatformHalfZ;
    const float maxZ = kPlatformCenterZ + kPlatformHalfZ;
    return position.x >= minX && position.x <= maxX &&
           position.z >= minZ && position.z <= maxZ;
}

Vec3 WarehouseScene::forkliftForward() const {
    const float yawRadians = forkliftHeadingDegrees_ * kPi / 180.0f;
    return {std::sin(yawRadians), 0.0f, -std::cos(yawRadians)};
}

WarehouseScene::Box WarehouseScene::forkliftBox() const {
    return {{forkliftPosition_.x, 1.0f, forkliftPosition_.z},
            {2.4f, 2.0f, 3.8f}};
}

WarehouseScene::Box WarehouseScene::shelfBox() const {
    return shelfFallen_ ? kShelfFallen : kShelfUpright;
}

void WarehouseScene::updateForklift(float dt, bool accelerate, bool reverse,
                                    bool steerLeft, bool steerRight) {
    if (steerLeft) {
        forkliftHeadingDegrees_ += kForkliftTurnSpeed * dt;
    }
    if (steerRight) {
        forkliftHeadingDegrees_ -= kForkliftTurnSpeed * dt;
    }

    float speed = 0.0f;
    if (accelerate) {
        speed = kForkliftSpeed;
    } else if (reverse) {
        speed = -kForkliftReverseSpeed;
    }
    if (speed == 0.0f) {
        return;
    }

    const Vec3 forward = forkliftForward();
    const Vec3 displacement = forward * (speed * dt);
    const float longestAxisMove =
        std::max(std::abs(displacement.x), std::abs(displacement.z));
    const int stepCount =
        std::max(1, static_cast<int>(std::ceil(longestAxisMove / 0.12f)));
    const Vec3 step = displacement * (1.0f / static_cast<float>(stepCount));

    for (int stepIndex = 0; stepIndex < stepCount; ++stepIndex) {
        Vec3 candidate = forkliftPosition_ + step;
        if (!forkliftBlocked(candidate)) {
            forkliftPosition_ = candidate;
            continue;
        }

        if (!shelfFallen_ &&
            circleIntersectsBox(candidate.x, candidate.z, kForkliftRadius,
                                kShelfUpright)) {
            shelfFallen_ = true;
            if (!forkliftBlocked(candidate)) {
                forkliftPosition_ = candidate;
                continue;
            }
        }
        break;
    }

    if (!ironDoorOpen_ && forkliftPosition_.z >= kForkliftDoorCrashZ) {
        ironDoorOpen_ = true;
    }
}

bool WarehouseScene::cameraPositionBlocked(
    const Vec3& position, const Vec3& currentPosition) const {
    if (position.x < -kRoomHalfWidth + kCameraRadius ||
        position.x > kRoomHalfWidth - kCameraRadius) {
        return true;
    }

    if (position.z < -kRoomHalfDepth + kCameraRadius) {
        return true;
    }

    if (position.z > kRoomHalfDepth - kCameraRadius) {
        const bool insideDoorway =
            ironDoorOpen_ && std::abs(position.x) <= kDoorHalfWidth &&
            position.z <= kRoomHalfDepth + 1.8f;
        if (!insideDoorway) {
            return true;
        }
    }

    if (circleIntersectsBox(position.x, position.z, kCameraRadius,
                            kPlatformBox)) {
        const bool alreadyOnPlatform =
            platformTopContains(currentPosition) &&
            currentPosition.y >=
                kPlatformTopY + constants::kCameraProneHeight - 0.05f;
        const bool feetAbovePlatform =
            position.y >=
            kPlatformTopY + constants::kCameraGroundHeight - 0.05f;
        if (!alreadyOnPlatform && !feetAbovePlatform) {
            return true;
        }
        if (circleIntersectsBox(position.x, position.z, kCameraRadius,
                                kSniperCrateBox)) {
            return true;
        }
    }

    const bool nearPile =
        circleIntersectsBox(position.x, position.z, kCameraRadius, kPileA) ||
        circleIntersectsBox(position.x, position.z, kCameraRadius, kPileB);
    if (nearPile) {
        const bool alreadyOnPile =
            (circleIntersectsBox(currentPosition.x, currentPosition.z,
                                 kCameraRadius, kPileA) ||
             circleIntersectsBox(currentPosition.x, currentPosition.z,
                                 kCameraRadius, kPileB)) &&
            currentPosition.y >=
                kPileTopY + constants::kCameraProneHeight - 0.05f;
        const bool feetAbovePile =
            position.y >=
            kPileTopY + constants::kCameraGroundHeight - 0.05f;
        if (!alreadyOnPile && !feetAbovePile) {
            return true;
        }
    }

    const WarehouseScene::Box groundBoxes[] = {
        kColumnA, kColumnB, shelfBox(), forkliftBox()};
    for (const Box& box : groundBoxes) {
        if (circleIntersectsBox(position.x, position.z, kCameraRadius, box)) {
            return true;
        }
    }

    for (const SecurityGuardModel& guard : guards_) {
        if (!guard.alive()) {
            continue;
        }
        const Vec3& guardPosition = guard.position();
        if (circleIntersectsBox(
                position.x, position.z, kCameraRadius,
                {{guardPosition.x, 1.3f, guardPosition.z},
                 {1.15f, 2.6f, 1.0f}})) {
            return true;
        }
    }
    return false;
}

bool WarehouseScene::segmentHitsGeometry(const Vec3& start, const Vec3& end,
                                         float& hitT) const {
    bool hit = false;
    float closest = 2.0f;
    const auto test = [&](const Box& box) {
        float candidate = 0.0f;
        if (segmentIntersectsAabb(start, end, box, candidate) &&
            candidate < closest) {
            closest = candidate;
            hit = true;
        }
    };

    test({{-kRoomHalfWidth, kWallHeight * 0.5f, 0.0f},
          {kWallThickness, kWallHeight, 2.0f * kRoomHalfDepth}});
    test({{kRoomHalfWidth, kWallHeight * 0.5f, 0.0f},
          {kWallThickness, kWallHeight, 2.0f * kRoomHalfDepth}});
    test({{0.0f, kWallHeight * 0.5f, -kRoomHalfDepth},
          {2.0f * kRoomHalfWidth, kWallHeight, kWallThickness}});

    const float sideWidth = kRoomHalfWidth - kDoorHalfWidth;
    test({{-(kDoorHalfWidth + sideWidth * 0.5f), kWallHeight * 0.5f,
           kDoorZ},
          {sideWidth, kWallHeight, kWallThickness}});
    test({{kDoorHalfWidth + sideWidth * 0.5f, kWallHeight * 0.5f, kDoorZ},
          {sideWidth, kWallHeight, kWallThickness}});

    if (!ironDoorOpen_) {
        test({{0.0f, kDoorHeight * 0.5f, kDoorZ},
              {2.0f * kDoorHalfWidth, kDoorHeight, 0.35f}});
    }

    test(kPlatformBox);
    test(kColumnA);
    test(kColumnB);
    test(kPileA);
    test(kPileB);
    test(kSniperCrateBox);
    test(shelfBox());
    test(forkliftBox());

    if (hit) {
        hitT = closest;
    }
    return hit;
}

bool WarehouseScene::circleIntersectsBox(float circleX, float circleZ,
                                         float radius, const Box& box) {
    return circleIntersectsBoxImpl(circleX, circleZ, radius, box);
}

bool WarehouseScene::segmentIntersectsAabb(const Vec3& start, const Vec3& end,
                                           const Box& box, float& hitT) {
    const Vec3 half{box.size.x * 0.5f, box.size.y * 0.5f,
                    box.size.z * 0.5f};
    const Vec3 min{box.center.x - half.x, box.center.y - half.y,
                   box.center.z - half.z};
    const Vec3 max{box.center.x + half.x, box.center.y + half.y,
                   box.center.z + half.z};
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

    if (!clipAxis(start.x, delta.x, min.x, max.x) ||
        !clipAxis(start.y, delta.y, min.y, max.y) ||
        !clipAxis(start.z, delta.z, min.z, max.z)) {
        return false;
    }

    hitT = tMin;
    return true;
}

bool WarehouseScene::nearPlatformClimb(const Vec3& playerPosition) const {
    if (onPlatform(playerPosition)) {
        return false;
    }
    const float distanceA =
        distanceSquaredOnFloor(playerPosition, kPileA.center);
    const float distanceB =
        distanceSquaredOnFloor(playerPosition, kPileB.center);
    constexpr float kClimbDistance = 2.3f;
    return distanceA <= kClimbDistance * kClimbDistance ||
           distanceB <= kClimbDistance * kClimbDistance;
}

float WarehouseScene::groundHeightAt(const Vec3& playerPosition) const {
    float groundHeight = 0.0f;
    if (platformTopContains(playerPosition)) {
        groundHeight = std::max(groundHeight, kPlatformTopY);
    }
    if (circleIntersectsBox(playerPosition.x, playerPosition.z, kCameraRadius,
                            kPileA) ||
        circleIntersectsBox(playerPosition.x, playerPosition.z, kCameraRadius,
                            kPileB)) {
        groundHeight = std::max(groundHeight, kPileTopY);
    }
    return groundHeight;
}

bool WarehouseScene::onPlatform(const Vec3& playerPosition) const {
    return platformTopContains(playerPosition) &&
           playerPosition.y >=
               kPlatformTopY + constants::kCameraProneHeight - 0.05f;
}

bool WarehouseScene::nearSniperCrate(const Vec3& playerPosition) const {
    if (!onPlatform(playerPosition)) {
        return false;
    }
    constexpr float kCrateDistance = 2.2f;
    return distanceSquaredOnFloor(playerPosition, kSniperCrateBox.center) <=
           kCrateDistance * kCrateDistance;
}

bool WarehouseScene::interactWithSniperCrate(const Vec3& playerPosition) {
    if (sniperRifleTaken_ || !nearSniperCrate(playerPosition)) {
        return false;
    }
    sniperRifleTaken_ = true;
    return true;
}

bool WarehouseScene::nearForklift(const Vec3& playerPosition) const {
    if (drivingForklift_ || onPlatform(playerPosition)) {
        return false;
    }
    constexpr float kForkliftDistance = 2.6f;
    return distanceSquaredOnFloor(playerPosition, forkliftPosition_) <=
           kForkliftDistance * kForkliftDistance;
}

bool WarehouseScene::enterForklift(const Vec3& playerPosition) {
    if (drivingForklift_ || !nearForklift(playerPosition)) {
        return false;
    }
    drivingForklift_ = true;
    return true;
}

void WarehouseScene::exitForklift() {
    drivingForklift_ = false;
}

bool WarehouseScene::drivingForklift() const {
    return drivingForklift_;
}

Vec3 WarehouseScene::forkliftSeatPosition() const {
    const Vec3 forward = forkliftForward();
    return {forkliftPosition_.x - forward.x * 1.6f,
            kCameraGroundHeight + 0.8f,
            forkliftPosition_.z - forward.z * 1.6f};
}

Vec3 WarehouseScene::forkliftExitPosition() const {
    const Vec3 forward = forkliftForward();
    const Vec3 right{-forward.z, 0.0f, forward.x};
    Vec3 candidate{forkliftPosition_.x + right.x * 1.8f,
                    kCameraGroundHeight,
                    forkliftPosition_.z + right.z * 1.8f};
    const bool blocked =
        circleIntersectsBox(candidate.x, candidate.z,
                            kCameraRadius + 0.10f, forkliftBox()) ||
        candidate.x < -kRoomHalfWidth + kCameraRadius ||
        candidate.x > kRoomHalfWidth - kCameraRadius ||
        candidate.z < -kRoomHalfDepth + kCameraRadius ||
        candidate.z > kRoomHalfDepth - kCameraRadius;
    if (blocked) {
        candidate = {forkliftPosition_.x, kCameraGroundHeight,
                     forkliftPosition_.z - 2.8f};
    }
    candidate.x = std::clamp(candidate.x, -kRoomHalfWidth + kCameraRadius,
                             kRoomHalfWidth - kCameraRadius);
    candidate.z = std::clamp(candidate.z, -kRoomHalfDepth + kCameraRadius,
                             kRoomHalfDepth - kCameraRadius);
    return candidate;
}

float WarehouseScene::forkliftHeadingDegrees() const {
    return forkliftHeadingDegrees_;
}

bool WarehouseScene::nearExit(const Vec3& playerPosition) const {
    return ironDoorOpen_ && std::abs(playerPosition.x) <= kDoorHalfWidth &&
           playerPosition.z > kRoomHalfDepth - 2.2f;
}

bool WarehouseScene::sniperRifleTaken() const {
    return sniperRifleTaken_;
}

bool WarehouseScene::ironDoorOpen() const {
    return ironDoorOpen_;
}

bool WarehouseScene::shelfFallen() const {
    return shelfFallen_;
}

bool WarehouseScene::levelComplete() const {
    return levelComplete_;
}

bool WarehouseScene::sniperFired() const {
    return sniperFired_;
}

Vec3 WarehouseScene::sniperFiredPosition() const {
    return sniperFiredPosition_;
}

Vec3 WarehouseScene::spawnPosition() const {
    return {0.0f, kCameraGroundHeight, -9.0f};
}

bool WarehouseScene::segmentHitsGuard(const Vec3& start, const Vec3& end,
                                      float& hitT,
                                      std::size_t& guardIndex) const {
    bool hit = false;
    float closest = 2.0f;
    std::size_t closestIndex = 0;
    for (std::size_t index = 0; index < guards_.size(); ++index) {
        if (!guards_[index].alive()) {
            continue;
        }
        float candidate = 0.0f;
        if (guards_[index].segmentHit(start, end, candidate) &&
            candidate < closest) {
            hit = true;
            closest = candidate;
            closestIndex = index;
        }
    }

    if (hit) {
        hitT = closest;
        guardIndex = closestIndex;
    }
    return hit;
}

void WarehouseScene::applyGuardDamage(std::size_t guardIndex,
                                      const Vec3& hitPosition) {
    if (guardIndex >= guards_.size()) {
        return;
    }
    guards_[guardIndex].applyPistolDamage(hitPosition);
}

void WarehouseScene::applyGuardSniperDamage(std::size_t guardIndex,
                                            const Vec3& hitPosition) {
    if (guardIndex >= guards_.size()) {
        return;
    }
    guards_[guardIndex].applySniperDamage(hitPosition);
}

void WarehouseScene::applyGuardKnifeDamage(std::size_t guardIndex,
                                           const Vec3& hitPosition) {
    if (guardIndex >= guards_.size()) {
        return;
    }
    guards_[guardIndex].applyKnifeDamage(hitPosition);
}

void WarehouseScene::render() const {
    drawFloor();
    drawWalls();
    drawIronDoor();
    drawPlatform();
    drawCardboardPiles();
    drawSniperCrate();
    drawColumns();
    drawShelf();
    drawForklift();
    drawLighting();
    drawExitMarker();
    renderGuards();
}

void WarehouseScene::renderGuards() const {
    for (const SecurityGuardModel& guard : guards_) {
        guard.render();
    }
}

void WarehouseScene::drawFloor() const {
    ThreeDUtils::drawCube({0.0f, -0.12f, 0.0f},
                          {2.0f * kRoomHalfWidth, 0.24f,
                           2.0f * kRoomHalfDepth},
                          {0.19f, 0.22f, 0.24f});

    for (int z = -10; z <= 10; ++z) {
        ThreeDUtils::drawCube(
            {0.0f, 0.015f, static_cast<float>(z)},
            {2.0f * kRoomHalfWidth - 0.4f, 0.025f, 0.025f},
            (z % 2 == 0) ? Color{0.23f, 0.26f, 0.27f}
                         : Color{0.17f, 0.20f, 0.21f});
    }
    for (int x = -10; x <= 10; ++x) {
        if (x % 2 == 0) {
            ThreeDUtils::drawCube(
                {static_cast<float>(x), 0.016f, 0.0f},
                {0.025f, 0.025f, 2.0f * kRoomHalfDepth - 0.4f},
                {0.23f, 0.26f, 0.27f});
        }
    }
}

void WarehouseScene::drawWalls() const {
    const Color wall{0.24f, 0.27f, 0.29f};
    const Color wallTrim{0.35f, 0.39f, 0.41f};
    ThreeDUtils::drawCube({-kRoomHalfWidth, kWallHeight * 0.5f, 0.0f},
                          {kWallThickness, kWallHeight,
                           2.0f * kRoomHalfDepth},
                          wall);
    ThreeDUtils::drawCube({kRoomHalfWidth, kWallHeight * 0.5f, 0.0f},
                          {kWallThickness, kWallHeight,
                           2.0f * kRoomHalfDepth},
                          wall);
    ThreeDUtils::drawCube({0.0f, kWallHeight * 0.5f, -kRoomHalfDepth},
                          {2.0f * kRoomHalfWidth, kWallHeight,
                           kWallThickness},
                          wall);

    const float sideWidth = kRoomHalfWidth - kDoorHalfWidth;
    ThreeDUtils::drawCube(
        {-(kDoorHalfWidth + sideWidth * 0.5f), kWallHeight * 0.5f, kDoorZ},
        {sideWidth, kWallHeight, kWallThickness}, wall);
    ThreeDUtils::drawCube(
        {kDoorHalfWidth + sideWidth * 0.5f, kWallHeight * 0.5f, kDoorZ},
        {sideWidth, kWallHeight, kWallThickness}, wall);

    ThreeDUtils::drawCube({0.0f, kWallHeight - 0.18f, -kRoomHalfDepth},
                          {2.0f * kRoomHalfWidth, 0.36f, kWallThickness},
                          wallTrim);
    ThreeDUtils::drawCube({0.0f, kWallHeight - 0.18f, kDoorZ},
                          {2.0f * kRoomHalfWidth, 0.36f, kWallThickness},
                          wallTrim);
}

void WarehouseScene::drawIronDoor() const {
    const WarehouseText& text = warehouseText(language_);
    const Color frame{0.16f, 0.19f, 0.22f};
    for (const float x : {-kDoorHalfWidth, kDoorHalfWidth}) {
        ThreeDUtils::drawCube({x, kDoorHeight * 0.5f, kDoorZ},
                              {0.38f, kDoorHeight, 0.30f}, frame);
    }
    ThreeDUtils::drawCube({0.0f, kDoorHeight + 0.18f, kDoorZ},
                          {2.0f * kDoorHalfWidth + 0.5f, 0.30f, 0.30f},
                          frame);

    if (ironDoorOpen_) {
        drawWorldLabel(text.ironDoorLabel, {0.0f, 5.3f, kDoorZ - 0.4f},
                       0.045f, kSignColor);
        return;
    }

    ThreeDUtils::drawCube({0.0f, kDoorHeight * 0.5f, kDoorZ},
                          {2.0f * kDoorHalfWidth - 0.16f, kDoorHeight,
                           0.18f},
                          {0.38f, 0.32f, 0.24f});
    ThreeDUtils::drawCube({0.0f, kDoorHeight * 0.62f, kDoorZ - 0.04f},
                          {1.2f, 0.14f, 0.10f}, kInkColor);
    drawWorldLabel(text.ironDoorLabel, {0.0f, 5.3f, kDoorZ - 0.4f},
                   0.045f, kSignColor);
}

void WarehouseScene::drawPlatform() const {
    const WarehouseText& text = warehouseText(language_);
    const Color deck{0.48f, 0.40f, 0.24f};
    const Color frame{0.20f, 0.22f, 0.24f};
    const Color railing{0.82f, 0.60f, 0.18f};

    ThreeDUtils::drawCube({kPlatformCenterX, 0.18f, kPlatformCenterZ},
                          {2.0f * kPlatformHalfX, 0.36f,
                           2.0f * kPlatformHalfZ},
                          frame);
    for (const float x :
         {kPlatformCenterX - kPlatformHalfX + 0.35f,
          kPlatformCenterX + kPlatformHalfX - 0.35f}) {
        for (const float z :
             {kPlatformCenterZ - kPlatformHalfZ + 0.35f,
              kPlatformCenterZ + kPlatformHalfZ - 0.35f}) {
            ThreeDUtils::drawCube({x, kPlatformTopY * 0.5f, z},
                                  {0.28f, kPlatformTopY, 0.28f}, frame);
        }
    }
    ThreeDUtils::drawCube(
        {kPlatformCenterX, kPlatformTopY + 0.07f, kPlatformCenterZ},
        {2.0f * kPlatformHalfX, 0.14f, 2.0f * kPlatformHalfZ}, deck);

    for (const float z : {kPlatformCenterZ - kPlatformHalfZ,
                          kPlatformCenterZ + kPlatformHalfZ}) {
        ThreeDUtils::drawCube(
            {kPlatformCenterX, kPlatformTopY + 0.55f, z},
            {2.0f * kPlatformHalfX, 0.06f, 0.06f}, railing);
    }
    for (const float x : {kPlatformCenterX - kPlatformHalfX,
                          kPlatformCenterX + kPlatformHalfX}) {
        ThreeDUtils::drawCube(
            {x, kPlatformTopY + 0.55f, kPlatformCenterZ},
            {0.06f, 0.06f, 2.0f * kPlatformHalfZ}, railing);
    }

    drawWorldLabel(text.platformLabel,
                   {kPlatformCenterX, 4.7f, kPlatformCenterZ}, 0.040f,
                   kSignColor);
}

void WarehouseScene::drawCardboardPiles() const {
    const Color cardboard{0.68f, 0.52f, 0.28f};
    const Color cardboardDark{0.50f, 0.38f, 0.20f};
    for (const WarehouseScene::Box& pile : {kPileA, kPileB}) {
        ThreeDUtils::drawCube(
            {pile.center.x, 0.32f, pile.center.z},
            {pile.size.x, 0.64f, pile.size.z}, cardboardDark);
        ThreeDUtils::drawCube(
            {pile.center.x, 0.95f, pile.center.z},
            {pile.size.x * 0.88f, 0.62f, pile.size.z * 0.88f}, cardboard);
        ThreeDUtils::drawCube(
            {pile.center.x, 1.52f, pile.center.z},
            {pile.size.x * 0.72f, 0.52f, pile.size.z * 0.72f},
            cardboardDark);
    }
}

void WarehouseScene::drawSniperCrate() const {
    const WarehouseText& text = warehouseText(language_);
    const Color crate{0.56f, 0.34f, 0.16f};
    const Color crateDark{0.38f, 0.22f, 0.10f};
    ThreeDUtils::drawCube(kSniperCrateBox.center, kSniperCrateBox.size,
                          crate);
    ThreeDUtils::drawCube(
        {kSniperCrateBox.center.x, kSniperCrateBox.center.y + 0.08f,
         kSniperCrateBox.center.z},
        {kSniperCrateBox.size.x * 0.9f, 0.10f,
         kSniperCrateBox.size.z * 0.9f},
        sniperRifleTaken_ ? Color{0.20f, 0.42f, 0.24f} : crateDark);
    drawWorldLabel(text.crateLabel,
                   {kSniperCrateBox.center.x,
                    kSniperCrateBox.center.y + 0.95f,
                    kSniperCrateBox.center.z},
                   0.035f, kSignColor);
}

void WarehouseScene::drawColumns() const {
    const Color column{0.30f, 0.33f, 0.35f};
    const Color columnTop{0.40f, 0.44f, 0.46f};
    for (const WarehouseScene::Box& box : {kColumnA, kColumnB}) {
        ThreeDUtils::drawCube(box.center, box.size, column);
        ThreeDUtils::drawCube(
            {box.center.x, box.center.y + box.size.y * 0.5f + 0.10f,
             box.center.z},
            {box.size.x + 0.30f, 0.22f, box.size.z + 0.30f}, columnTop);
    }
}

void WarehouseScene::drawShelf() const {
    const WarehouseText& text = warehouseText(language_);
    const Color shelf{0.32f, 0.38f, 0.44f};
    const Color shelfDark{0.22f, 0.27f, 0.32f};
    const Box box = shelfBox();
    ThreeDUtils::drawCube(box.center, box.size, shelf);
    if (shelfFallen_) {
        ThreeDUtils::drawCube(
            {box.center.x, box.center.y + box.size.y * 0.5f + 0.08f,
             box.center.z},
            {box.size.x * 0.7f, 0.14f, box.size.z * 0.7f}, shelfDark);
    } else {
        for (const float z : {-1.6f, 0.0f, 1.6f}) {
            ThreeDUtils::drawCube({box.center.x, 2.55f, z},
                                  {box.size.x - 0.16f, 0.14f,
                                   box.size.z - 2.2f},
                                  shelfDark);
        }
    }
    drawWorldLabel(text.shelfLabel,
                   {box.center.x, box.center.y + box.size.y * 0.5f + 1.1f,
                    box.center.z},
                   0.035f, kSignColor);
}

void WarehouseScene::drawForklift() const {
    const WarehouseText& text = warehouseText(language_);
    const Color body{0.96f, 0.54f, 0.10f};
    const Color bodyDark{0.38f, 0.18f, 0.06f};
    const Color mast{0.16f, 0.20f, 0.24f};
    const Color fork{0.20f, 0.24f, 0.28f};
    const Color tire{0.07f, 0.09f, 0.12f};

    glPushMatrix();
    glTranslatef(forkliftPosition_.x, 0.0f, forkliftPosition_.z);
    glRotatef(180.0f - forkliftHeadingDegrees_, 0.0f, 1.0f, 0.0f);

    ThreeDUtils::drawCube({0.0f, 0.72f, -0.2f}, {2.2f, 1.1f, 3.4f}, body);
    ThreeDUtils::drawCube({0.0f, 1.45f, -0.4f}, {2.0f, 0.36f, 0.7f}, bodyDark);
    for (const float x : {-0.95f, 0.95f}) {
        ThreeDUtils::drawCube({x, 1.72f, -0.4f}, {0.14f, 0.9f, 0.14f}, mast);
        ThreeDUtils::drawCube({x, 0.30f, -0.4f}, {0.34f, 0.34f, 0.34f}, tire);
    }
    ThreeDUtils::drawCube({0.0f, 2.12f, -0.4f}, {2.1f, 0.12f, 0.16f}, mast);
    ThreeDUtils::drawCube({0.0f, 2.10f, -0.1f}, {2.3f, 0.08f, 0.7f}, mast);
    for (const float x : {-0.55f, 0.55f}) {
        ThreeDUtils::drawCube({x, 0.42f, 1.75f}, {0.14f, 0.10f, 0.6f}, fork);
    }
    for (const float z : {-1.6f, 1.4f}) {
        for (const float x : {-1.0f, 1.0f}) {
            ThreeDUtils::drawCube({x, 0.28f, z}, {0.5f, 0.56f, 0.5f}, tire);
        }
    }
    glPopMatrix();

    drawWorldLabel(text.forkliftLabel,
                   {forkliftPosition_.x, 2.9f, forkliftPosition_.z}, 0.040f,
                   kSignColor);
}

void WarehouseScene::drawLighting() const {
    const std::array<float, 3> lightX{{-4.0f, 0.0f, 4.0f}};
    for (const float x : lightX) {
        ThreeDUtils::drawCube({x, kWallHeight - 0.22f, 0.0f},
                              {2.4f, 0.12f, 0.22f}, kInkColor);
        ThreeDUtils::drawCube({x, kWallHeight - 0.30f, 0.0f},
                              {1.72f, 0.08f, 0.12f}, kHouseWindowLight);
    }
}

void WarehouseScene::drawExitMarker() const {
    const WarehouseText& text = warehouseText(language_);
    const Color stair{0.30f, 0.38f, 0.40f};
    const Color stepLight{0.22f, 0.70f, 0.34f};
    const float z = kDoorZ + 0.8f;
    for (int step = 0; step < 4; ++step) {
        ThreeDUtils::drawCube(
            {0.0f, 0.22f + 0.28f * step, z + 0.42f * step},
            {2.0f * kDoorHalfWidth - 0.5f, 0.24f, 0.52f},
            stair);
    }
    ThreeDUtils::drawCube(
        {0.0f, 1.45f, z + 1.55f},
        {2.0f * kDoorHalfWidth - 0.5f, 0.16f, 0.5f},
        ironDoorOpen_ ? stepLight : Color{0.42f, 0.46f, 0.48f});
    drawWorldLabel(text.exitLabel, {0.0f, 3.6f, z + 0.2f}, 0.045f,
                   kSignColor);
}

void WarehouseScene::drawWorldLabel(const char* label, const Vec3& center,
                                    float pixelScale,
                                    const Color& color) const {
    glPushMatrix();
    glTranslatef(center.x, center.y, center.z);
    glScalef(1.0f, -1.0f, 1.0f);
    const std::string text(label);
    ThreeDUtils::drawText(
        text, -ThreeDUtils::textWidth(text, pixelScale) * 0.5f, 0.0f,
        pixelScale, color);
    glPopMatrix();
}

}  // namespace pixel_world
