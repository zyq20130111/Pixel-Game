#include "ParkingLotScene.h"

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
constexpr float kRoomHalfWidth = 20.0f;
constexpr float kRoomHalfDepth = 10.0f;
constexpr float kWallHeight = 6.0f;
constexpr float kWallThickness = 0.45f;
constexpr float kGateHalfWidth = 3.6f;
constexpr float kElevatorDoorHalfWidth = 2.3f;
constexpr float kElevatorDistance = 3.4f;
constexpr float kGuardEyeHeight = 2.25f;
constexpr float kGuardSightDistance = 15.0f;
constexpr float kGuardCollisionRadius = 0.62f;
constexpr float kGuardPlayerClearance = 0.92f;
constexpr float kGuardAlarmDuration = 4.0f;

constexpr ParkingLotScene::Box kSolidBoxes[] = {
    {{-6.0f, 1.35f, 0.1f}, {1.25f, 2.7f, 1.25f}},
    {{6.0f, 1.35f, 0.1f}, {1.25f, 2.7f, 1.25f}},
    {{-12.0f, 0.70f, -1.8f}, {3.8f, 1.40f, 6.4f}},
    {{12.0f, 1.15f, 1.2f}, {4.4f, 2.30f, 7.8f}},
};

bool circleIntersectsBoxImpl(float circleX, float circleZ, float radius,
                             const ParkingLotScene::Box& box) {
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

void drawWarningStripe(float x, float y, float z, float width, float depth,
                       const Color& color) {
    ThreeDUtils::drawCube({x, y, z}, {width, 0.34f, depth}, color);
}

void drawParkingSlot(float x, float z, float width, float depth) {
    const float y = 0.035f;
    const float line = 0.055f;
    ThreeDUtils::drawCube({x - width * 0.5f, y, z},
                          {line, 0.03f, depth}, kPlazaTrim);
    ThreeDUtils::drawCube({x + width * 0.5f, y, z},
                          {line, 0.03f, depth}, kPlazaTrim);
    ThreeDUtils::drawCube({x, y, z - depth * 0.5f},
                          {width, 0.03f, line}, kPlazaTrim);
}

float distanceSquaredOnFloor(const Vec3& a, const Vec3& b) {
    const float dx = a.x - b.x;
    const float dz = a.z - b.z;
    return dx * dx + dz * dz;
}

}  // namespace

ParkingLotScene::ParkingLotScene(Language language)
    : captainDefeated_(false),
      accessCardObtained_(false),
      elevatorOpen_(false),
      levelComplete_(false),
      elevatorOpenAmount_(0.0f),
      groupAlerted_(false),
      groupAlertTimer_(0.0f),
      accessCardPosition_({0.0f, 1.1f, -6.6f}),
      language_(language),
      difficulty_(Difficulty::Normal),
      guards_{SecurityGuardModel(SecurityGuardRole::Guard),
              SecurityGuardModel(SecurityGuardRole::Guard),
              SecurityGuardModel(SecurityGuardRole::Captain)} {}

void ParkingLotScene::reset(Difficulty difficulty) {
    difficulty_ = difficulty;
    captainDefeated_ = false;
    accessCardObtained_ = false;
    elevatorOpen_ = false;
    levelComplete_ = false;
    elevatorOpenAmount_ = 0.0f;
    groupAlerted_ = false;
    groupAlertTimer_ = 0.0f;
    accessCardPosition_ = {0.0f, 1.1f, -6.6f};

    guards_[0].setRole(SecurityGuardRole::Guard);
    guards_[0].setDifficulty(difficulty_);
    guards_[0].setPosition({-4.2f, 0.0f, 4.8f});
    guards_[0].setPatrolling(true);
    guards_[0].setPatrolWaypoints(
        {{-4.2f, 0.0f, 4.8f}, {-4.2f, 0.0f, -3.4f},
         {4.2f, 0.0f, -3.4f}, {4.2f, 0.0f, 4.8f}});
    guards_[0].reset();

    guards_[1].setRole(SecurityGuardRole::Guard);
    guards_[1].setDifficulty(difficulty_);
    guards_[1].setPosition({12.0f, 0.0f, -3.2f});
    guards_[1].setPatrolling(false);
    guards_[1].setPatrolWaypoints({});
    guards_[1].reset();

    guards_[2].setRole(SecurityGuardRole::Captain);
    guards_[2].setDifficulty(difficulty_);
    guards_[2].setPosition({0.0f, 0.0f, -6.5f});
    guards_[2].setPatrolling(false);
    guards_[2].setPatrolWaypoints({});
    guards_[2].reset();
}

int ParkingLotScene::update(float dt, const Vec3& playerPosition,
                            bool playerFired) {
    std::array<bool, kGuardCount> guardSeesPlayer{};
    bool directThreatDetected = false;
    for (std::size_t index = 0; index < guards_.size(); ++index) {
        guardSeesPlayer[index] =
            guards_[index].alive() &&
            canGuardSeePlayer(guards_[index], playerPosition);
        directThreatDetected =
            directThreatDetected || guardSeesPlayer[index];
    }

    if (playerFired || directThreatDetected) {
        groupAlertTimer_ = kGuardAlarmDuration;
    }
    groupAlertTimer_ = std::max(0.0f, groupAlertTimer_ - dt);
    groupAlerted_ = groupAlertTimer_ > 0.0f;

    int playerDamage = 0;
    for (std::size_t index = 0; index < guards_.size(); ++index) {
        SecurityGuardModel& guard = guards_[index];
        const bool playerDetected =
            guard.alive() &&
            (guardSeesPlayer[index] ||
             (groupAlerted_ && !guard.returning()));
        const bool weaponCanHitPlayer =
            guard.alive() &&
            (guard.weapon() == SecurityGuardWeapon::Sniper
                 ? guardSeesPlayer[index]
                 : canGuardHitPlayer(guard, playerPosition));
        const Vec3 currentGuardPosition = guard.position();
        playerDamage += guard.update(
            dt, playerPosition, playerDetected, weaponCanHitPlayer,
            [this, index, &playerPosition, currentGuardPosition](
                const Vec3& position) {
                return guardPositionBlocked(position, currentGuardPosition,
                                            index, playerPosition);
            });
    }

    if (guards_[2].defeated()) {
        captainDefeated_ = true;
    }
    if (elevatorOpen_) {
        elevatorOpenAmount_ =
            std::min(1.0f, elevatorOpenAmount_ + dt * 1.8f);
        if (elevatorOpenAmount_ >= 1.0f) {
            levelComplete_ = true;
        }
    }
    return playerDamage;
}

void ParkingLotScene::render() const {
    drawFloor();
    drawWalls();
    drawColumns();
    drawParkingLines();
    drawCar();
    drawTruck();
    drawElevator();
    drawRollerDoor();
    drawLighting();

    for (const SecurityGuardModel& guard : guards_) {
        guard.render();
        guard.renderHealthBar();
    }
    drawAccessCard();
}

bool ParkingLotScene::cameraPositionBlocked(
    const Vec3& position, const Vec3& currentPosition) const {
    if (position.x < -kRoomHalfWidth + kCameraRadius ||
        position.x > kRoomHalfWidth - kCameraRadius) {
        return true;
    }

    const bool atSouthGate =
        std::abs(position.x) < kGateHalfWidth - kCameraRadius;
    if (position.z > kRoomHalfDepth - kCameraRadius && !atSouthGate) {
        return true;
    }

    if (position.z < -kRoomHalfDepth + kCameraRadius) {
        return true;
    }

    for (const Box& box : kSolidBoxes) {
        if (circleIntersectsBox(position.x, position.z, kCameraRadius,
                                box)) {
            return true;
        }
    }

    if (!elevatorOpen_ &&
        circleIntersectsBox(position.x, position.z, kCameraRadius,
                            {{0.0f, 1.3f, -9.35f},
                             {2.0f * kElevatorDoorHalfWidth, 2.6f, 0.35f}})) {
        return true;
    }

    for (const SecurityGuardModel& guard : guards_) {
        if (!guard.alive()) {
            continue;
        }
        const Vec3& guardPosition = guard.position();
        const Box guardCollisionBox{
            {guardPosition.x, 1.3f, guardPosition.z},
            {1.15f, 2.6f, 1.0f}};
        if (!circleIntersectsBox(position.x, position.z, kCameraRadius,
                                 guardCollisionBox)) {
            continue;
        }

        // A guard can pin the player against its collision box during an
        // attack. Allow movement that increases the distance from that guard
        // so the player can escape, while still blocking movement into it.
        const bool currentPositionOverlaps =
            circleIntersectsBox(currentPosition.x, currentPosition.z,
                                kCameraRadius, guardCollisionBox);
        const float currentDistance =
            distanceSquaredOnFloor(currentPosition, guardPosition);
        const float candidateDistance =
            distanceSquaredOnFloor(position, guardPosition);
        if (!currentPositionOverlaps ||
            candidateDistance <= currentDistance + 0.0001f) {
            return true;
        }
    }
    return false;
}

bool ParkingLotScene::segmentHitsGeometry(const Vec3& start, const Vec3& end,
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
    if (std::abs(start.x) > kGateHalfWidth ||
        std::abs(end.x) > kGateHalfWidth) {
        test({{0.0f, kWallHeight * 0.5f, kRoomHalfDepth},
              {2.0f * kRoomHalfWidth, kWallHeight, kWallThickness}});
    }

    for (const Box& box : kSolidBoxes) {
        test(box);
    }

    if (!elevatorOpen_) {
        test({{0.0f, 1.3f, -9.35f},
              {2.0f * kElevatorDoorHalfWidth, 2.6f, 0.35f}});
    }

    if (hit) {
        hitT = closest;
    }
    return hit;
}

bool ParkingLotScene::segmentHitsGuard(const Vec3& start, const Vec3& end,
                                       float& hitT,
                                       std::size_t& guardIndex) const {
    bool hit = false;
    float closest = 2.0f;
    std::size_t closestIndex = 0;
    for (std::size_t index = 0; index < guards_.size(); ++index) {
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

void ParkingLotScene::applyGuardDamage(std::size_t guardIndex,
                                       const Vec3& hitPosition) {
    if (guardIndex >= guards_.size()) {
        return;
    }
    guards_[guardIndex].applyPistolDamage(hitPosition);
    if (guards_[2].defeated()) {
        captainDefeated_ = true;
    }
}

void ParkingLotScene::applyGuardSniperDamage(
    std::size_t guardIndex, const Vec3& hitPosition) {
    if (guardIndex >= guards_.size()) {
        return;
    }
    guards_[guardIndex].applySniperDamage(hitPosition);
    if (guards_[2].defeated()) {
        captainDefeated_ = true;
    }
}

void ParkingLotScene::applyGuardKnifeDamage(std::size_t guardIndex,
                                            const Vec3& hitPosition) {
    if (guardIndex >= guards_.size()) {
        return;
    }
    guards_[guardIndex].applyKnifeDamage(hitPosition);
    if (guards_[2].defeated()) {
        captainDefeated_ = true;
    }
}

bool ParkingLotScene::tryCollectAccessCard(const Vec3& playerPosition) {
    if (!captainDefeated_ || accessCardObtained_) {
        return false;
    }

    if (distanceSquaredOnFloor(playerPosition, accessCardPosition_) >
        1.9f * 1.9f) {
        return false;
    }

    accessCardObtained_ = true;
    return true;
}

bool ParkingLotScene::interactWithElevator(const Vec3& playerPosition) {
    if (!nearElevator(playerPosition) || !accessCardObtained_) {
        return false;
    }

    elevatorOpen_ = true;
    return true;
}

bool ParkingLotScene::nearElevator(const Vec3& playerPosition) const {
    return distanceSquaredOnFloor(playerPosition, {0.0f, 0.0f, -8.6f}) <=
           kElevatorDistance * kElevatorDistance;
}

Vec3 ParkingLotScene::spawnPosition() const {
    return {0.0f, kCameraGroundHeight, 7.3f};
}

const SecurityGuardModel& ParkingLotScene::guard(std::size_t index) const {
    return guards_.at(index);
}

int ParkingLotScene::livingGuardCount() const {
    int count = 0;
    for (const SecurityGuardModel& guard : guards_) {
        if (guard.alive()) {
            ++count;
        }
    }
    return count;
}

bool ParkingLotScene::captainDefeated() const {
    return captainDefeated_;
}

bool ParkingLotScene::accessCardObtained() const {
    return accessCardObtained_;
}

bool ParkingLotScene::elevatorOpen() const {
    return elevatorOpen_;
}

bool ParkingLotScene::levelComplete() const {
    return levelComplete_;
}

bool ParkingLotScene::circleIntersectsBox(float circleX, float circleZ,
                                          float radius, const Box& box) {
    return circleIntersectsBoxImpl(circleX, circleZ, radius, box);
}

bool ParkingLotScene::segmentIntersectsAabb(const Vec3& start, const Vec3& end,
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

Vec3 ParkingLotScene::pointOnSegment(const Vec3& start, const Vec3& end,
                                     float t) {
    return start + (end - start) * t;
}

bool ParkingLotScene::canGuardSeePlayer(
    const SecurityGuardModel& guard, const Vec3& playerPosition) const {
    const Vec3 toPlayer{playerPosition.x - guard.position().x, 0.0f,
                        playerPosition.z - guard.position().z};
    const float distance = ThreeDUtils::length(toPlayer);
    if (distance <= 0.001f || distance > kGuardSightDistance) {
        return false;
    }

    const Vec3 direction = ThreeDUtils::normalize(toPlayer);
    const float yawRadians =
        guard.yawDegrees() * kPi / 180.0f;
    const Vec3 facing{std::sin(yawRadians), 0.0f,
                      std::cos(yawRadians)};
    constexpr float kCosSightHalfAngle =
        0.17364817766693033f;  // cos(80 degrees), 160 degree field of view
    if (ThreeDUtils::dot(direction, {facing.x, 0.0f, facing.z}) <
        kCosSightHalfAngle) {
        return false;
    }

    const Vec3 guardEye{guard.position().x, guard.position().y +
                                             kGuardEyeHeight,
                        guard.position().z};
    const Vec3 playerEye{playerPosition.x, playerPosition.y,
                         playerPosition.z};
    float hitT = 0.0f;
    return !segmentHitsGeometry(guardEye, playerEye, hitT);
}

bool ParkingLotScene::canGuardHitPlayer(
    const SecurityGuardModel& guard, const Vec3& playerPosition) const {
    if (!guard.weaponCanHitPlayer(playerPosition)) {
        return false;
    }

    const Vec3 weaponPosition{guard.position().x, guard.position().y + 1.08f,
                              guard.position().z};
    float hitT = 0.0f;
    return !segmentHitsGeometry(weaponPosition, playerPosition, hitT);
}

bool ParkingLotScene::guardPositionBlocked(
    const Vec3& position, const Vec3& currentPosition,
    std::size_t movingGuardIndex,
    const Vec3& playerPosition) const {
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

    for (const Box& box : kSolidBoxes) {
        if (!circleIntersectsBox(position.x, position.z,
                                 kGuardCollisionRadius, box)) {
            continue;
        }

        const bool currentPositionOverlaps =
            circleIntersectsBox(currentPosition.x, currentPosition.z,
                                kGuardCollisionRadius, box);
        const float currentDistance =
            distanceSquaredOnFloor(currentPosition, box.center);
        const float candidateDistance =
            distanceSquaredOnFloor(position, box.center);

        // A guard can spawn just behind a vehicle with its padded collision
        // radius slightly inside the vehicle bounds. Let it move outward a
        // little at a time until it is clear, but never allow movement
        // deeper into the obstacle.
        if (!currentPositionOverlaps ||
            candidateDistance <= currentDistance + 0.0001f) {
            return true;
        }
    }

    if (!elevatorOpen_ &&
        circleIntersectsBox(position.x, position.z, kGuardCollisionRadius,
                            {{0.0f, 1.3f, -9.35f},
                             {2.0f * kElevatorDoorHalfWidth, 2.6f, 0.35f}})) {
        return true;
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

void ParkingLotScene::drawFloor() const {
    ThreeDUtils::drawCube({0.0f, -0.12f, 0.0f},
                          {2.0f * kRoomHalfWidth, 0.24f,
                           2.0f * kRoomHalfDepth},
                          {0.20f, 0.23f, 0.25f});

    for (int z = -9; z <= 9; ++z) {
        ThreeDUtils::drawCube(
            {0.0f, 0.015f, static_cast<float>(z)},
            {39.2f, 0.025f, 0.025f},
            (z % 2 == 0) ? Color{0.24f, 0.27f, 0.28f}
                         : Color{0.18f, 0.21f, 0.22f});
    }
    for (int x = -19; x <= 19; ++x) {
        if (x % 2 == 0) {
            ThreeDUtils::drawCube(
                {static_cast<float>(x), 0.016f, 0.0f},
                {0.025f, 0.025f, 19.2f}, Color{0.24f, 0.27f, 0.28f});
        }
    }
}

void ParkingLotScene::drawWalls() const {
    const Color wall{0.27f, 0.30f, 0.32f};
    const Color wallTrim{0.38f, 0.42f, 0.43f};
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
    ThreeDUtils::drawCube({-11.8f, kWallHeight * 0.5f, kRoomHalfDepth},
                          {16.4f, kWallHeight, kWallThickness}, wall);
    ThreeDUtils::drawCube({11.8f, kWallHeight * 0.5f, kRoomHalfDepth},
                          {16.4f, kWallHeight, kWallThickness}, wall);
    ThreeDUtils::drawCube({0.0f, kWallHeight - 0.20f, kRoomHalfDepth},
                          {7.2f, 0.40f, kWallThickness}, wallTrim);

    ThreeDUtils::drawCube({0.0f, 6.15f, 0.0f},
                          {2.0f * kRoomHalfWidth, 0.30f,
                           2.0f * kRoomHalfDepth},
                          {0.12f, 0.15f, 0.17f});
}

void ParkingLotScene::drawColumns() const {
    const std::array<Vec3, 2> columns{{{-6.0f, 0.0f, 0.1f},
                                       {6.0f, 0.0f, 0.1f}}};
    for (std::size_t index = 0; index < columns.size(); ++index) {
        const Vec3& column = columns[index];
        ThreeDUtils::drawCube({column.x, 1.35f, column.z},
                              {1.25f, 2.7f, 1.25f}, kStoneDark);
        drawWarningStripe(column.x, 0.65f, column.z + 0.64f, 1.05f, 0.08f,
                          kSunCore);
        drawWarningStripe(column.x, 1.18f, column.z + 0.64f, 1.05f, 0.08f,
                          kInkColor);
        drawWarningStripe(column.x, 1.71f, column.z + 0.64f, 1.05f, 0.08f,
                          kSunCore);
        drawWorldLabel(index == 0 ? "01" : "02",
                       {column.x, 2.82f, column.z + 0.66f}, 0.052f,
                       kSignColor);
    }
}

void ParkingLotScene::drawParkingLines() const {
    for (int index = 0; index < 4; ++index) {
        drawParkingSlot(-12.0f, -6.3f + index * 2.0f, 3.8f, 1.65f);
        drawParkingSlot(12.0f, -6.3f + index * 2.0f, 4.2f, 1.65f);
    }

    ThreeDUtils::drawCube({0.0f, 0.04f, 7.55f},
                          {0.14f, 0.04f, 2.9f}, kPlazaTrim);
    ThreeDUtils::drawCube({-0.42f, 0.04f, 6.35f},
                          {0.85f, 0.04f, 0.14f}, kPlazaTrim);
    ThreeDUtils::drawCube({0.42f, 0.04f, 6.35f},
                          {0.85f, 0.04f, 0.14f}, kPlazaTrim);
}

void ParkingLotScene::drawCar() const {
    const ParkingText& text = parkingText(language_);
    const float x = -12.0f;
    const float z = -1.8f;
    ThreeDUtils::drawCube({x, 0.72f, z}, {3.4f, 0.78f, 5.7f},
                          {0.18f, 0.42f, 0.62f});
    ThreeDUtils::drawCube({x, 1.25f, z + 0.15f}, {2.5f, 0.66f, 2.40f},
                          {0.24f, 0.56f, 0.76f});
    ThreeDUtils::drawCube({x, 1.28f, z - 1.42f}, {2.55f, 0.28f, 0.58f},
                          {0.09f, 0.22f, 0.35f});
    ThreeDUtils::drawCube({x, 1.28f, z + 1.60f}, {2.55f, 0.28f, 0.58f},
                          {0.09f, 0.22f, 0.35f});
    for (const float wheelX : {-1.58f, 1.58f}) {
        for (const float wheelZ : {-1.75f, 1.75f}) {
            ThreeDUtils::drawCube({x + wheelX, 0.28f, z + wheelZ},
                                  {0.30f, 0.48f, 0.72f}, kInkColor);
        }
    }
    drawWorldLabel(text.carLabel, {x, 2.02f, z + 0.1f}, 0.045f, kSignColor);
}

void ParkingLotScene::drawTruck() const {
    const ParkingText& text = parkingText(language_);
    const float x = 12.0f;
    const float z = 1.2f;
    ThreeDUtils::drawCube({x, 1.05f, z}, {4.0f, 1.65f, 7.0f},
                          {0.44f, 0.28f, 0.12f});
    ThreeDUtils::drawCube({x, 2.05f, z + 1.15f}, {3.65f, 1.00f, 3.05f},
                          {0.56f, 0.38f, 0.16f});
    ThreeDUtils::drawCube({x, 2.08f, z + 2.74f}, {2.85f, 0.44f, 0.10f},
                          {0.86f, 0.72f, 0.28f});
    for (const float wheelX : {-1.72f, 1.72f}) {
        for (const float wheelZ : {-1.95f, 1.95f}) {
            ThreeDUtils::drawCube({x + wheelX, 0.35f, z + wheelZ},
                                  {0.38f, 0.60f, 0.86f}, kInkColor);
        }
    }
    drawWorldLabel(text.truckLabel, {x, 2.95f, z + 1.0f}, 0.045f,
                   kSignColor);
}

void ParkingLotScene::drawElevator() const {
    const ParkingText& text = parkingText(language_);
    const Color frame{0.20f, 0.24f, 0.28f};
    const Color elevatorWall{0.33f, 0.37f, 0.40f};
    ThreeDUtils::drawCube({-3.5f, 2.4f, -9.48f},
                          {2.4f, 4.8f, 0.50f}, elevatorWall);
    ThreeDUtils::drawCube({3.5f, 2.4f, -9.48f},
                          {2.4f, 4.8f, 0.50f}, elevatorWall);
    ThreeDUtils::drawCube({0.0f, 5.0f, -9.48f},
                          {4.6f, 0.55f, 0.50f}, frame);
    ThreeDUtils::drawCube({0.0f, 1.30f, -9.30f},
                          {4.35f, 2.60f, 0.08f}, kInkColor);

    const float doorOffset = 1.18f * elevatorOpenAmount_;
    ThreeDUtils::drawCube({-1.05f - doorOffset, 1.42f, -9.22f},
                          {2.05f, 2.85f, 0.16f}, {0.42f, 0.46f, 0.48f});
    ThreeDUtils::drawCube({1.05f + doorOffset, 1.42f, -9.22f},
                          {2.05f, 2.85f, 0.16f}, {0.42f, 0.46f, 0.48f});
    ThreeDUtils::drawCube({0.0f, 3.15f, -9.12f},
                          {3.35f, 0.30f, 0.10f},
                          elevatorOpen_ ? kButtonStart : kSignColor);
    drawWorldLabel(text.elevatorLabel, {0.0f, 3.20f, -9.06f}, 0.045f,
                   kInkColor);

    const Color readerLight =
        accessCardObtained_ ? kButtonStart : kButtonExit;
    ThreeDUtils::drawCube({2.55f, 1.48f, -9.05f},
                          {0.26f, 0.60f, 0.16f}, kInkColor);
    ThreeDUtils::drawCube({2.55f, 1.66f, -8.94f},
                          {0.12f, 0.12f, 0.05f}, readerLight);
    ThreeDUtils::drawCube({2.55f, 1.36f, -8.94f},
                          {0.12f, 0.18f, 0.05f}, kHouseWindowLight);
    drawWorldLabel(text.cardReaderLabel, {2.55f, 2.0f, -8.92f}, 0.040f,
                   kSignColor);
}

void ParkingLotScene::drawRollerDoor() const {
    const ParkingText& text = parkingText(language_);
    const float doorTop = 4.65f;
    ThreeDUtils::drawCube({0.0f, doorTop, 9.35f},
                          {7.4f, 0.42f, 0.52f}, kStoneDark);
    ThreeDUtils::drawCube({-3.72f, 2.25f, 9.35f},
                          {0.42f, 4.8f, 0.52f}, kStoneDark);
    ThreeDUtils::drawCube({3.72f, 2.25f, 9.35f},
                          {0.42f, 4.8f, 0.52f}, kStoneDark);
    ThreeDUtils::drawCube({0.0f, 2.20f, 9.22f},
                          {7.1f, 4.45f, 0.18f}, {0.22f, 0.25f, 0.27f});
    for (int stripe = 0; stripe < 12; ++stripe) {
        const float x = -3.20f + stripe * 0.58f;
        ThreeDUtils::drawCube(
            {x, 2.25f, 9.10f}, {0.44f, 4.0f, 0.07f},
            (stripe % 2 == 0) ? kStoneDark : kSignColor);
    }
    drawWorldLabel(text.entryLabel, {0.0f, 4.90f, 9.00f}, 0.050f,
                   kSignColor);
}

void ParkingLotScene::drawLighting() const {
    const std::array<float, 5> lightX{{-14.0f, -7.0f, 0.0f, 7.0f, 14.0f}};
    for (const float x : lightX) {
        ThreeDUtils::drawCube({x, 5.72f, 0.0f},
                              {2.4f, 0.12f, 0.22f}, kInkColor);
        ThreeDUtils::drawCube({x, 5.64f, 0.0f},
                              {1.72f, 0.08f, 0.12f}, kHouseWindowLight);
    }
}

void ParkingLotScene::drawAccessCard() const {
    const ParkingText& text = parkingText(language_);
    if (!captainDefeated_ || accessCardObtained_) {
        return;
    }

    const float bob = 0.10f + std::sin(elevatorOpenAmount_ * 10.0f) * 0.04f;
    ThreeDUtils::drawCube({accessCardPosition_.x, accessCardPosition_.y + bob,
                           accessCardPosition_.z},
                          {0.52f, 0.08f, 0.32f}, kSunCore);
    ThreeDUtils::drawCube({accessCardPosition_.x, accessCardPosition_.y + bob,
                           accessCardPosition_.z + 0.01f},
                          {0.24f, 0.09f, 0.18f}, kHouseWindow);
    drawWorldLabel(text.accessCardWorldLabel,
                   {accessCardPosition_.x, accessCardPosition_.y + 0.32f,
                    accessCardPosition_.z},
                   0.040f, kSignColor);
}

void ParkingLotScene::drawWorldLabel(const char* label, const Vec3& center,
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
