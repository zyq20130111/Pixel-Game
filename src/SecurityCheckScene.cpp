#include "SecurityCheckScene.h"

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
constexpr float kRoomHalfWidth = 8.0f;
constexpr float kRoomHalfDepth = 12.0f;
constexpr float kWallHeight = 6.0f;
constexpr float kWallThickness = 0.45f;
constexpr float kGatePassageHalfWidth = 2.2f;
constexpr float kGateZ = 0.0f;
constexpr float kExitZ = -10.4f;
constexpr float kExitHalfWidth = 1.4f;
constexpr float kSwitchDistance = 2.3f;
constexpr Vec3 kSwitchPosition{7.3f, 1.6f, 4.5f};
constexpr float kGuardSightDistance = 15.0f;
constexpr float kGuardCollisionRadius = 0.62f;
constexpr float kGuardPlayerClearance = 0.92f;
constexpr float kGuardEyeHeight = 2.25f;
constexpr float kSniperWalkDuration = 0.9f;
constexpr std::array<float, 3> kSniperSpawnDelays{{0.3f, 1.1f, 1.9f}};
constexpr std::array<Vec3, 3> kSniperStations{{
    {-5.5f, 0.0f, -7.0f}, {0.0f, 0.0f, -8.0f}, {5.5f, 0.0f, -7.0f}}};
constexpr std::array<Vec3, 3> kSniperSpawnPoints{{
    {-5.5f, 0.0f, -11.0f}, {0.0f, 0.0f, -11.2f}, {5.5f, 0.0f, -11.0f}}};

constexpr SecurityCheckScene::Box kSolidBoxes[] = {
    {{-2.2f, 1.6f, 0.0f}, {0.5f, 3.2f, 0.5f}},
    {{2.2f, 1.6f, 0.0f}, {0.5f, 3.2f, 0.5f}},
    {{-5.1f, 0.55f, 0.0f}, {5.8f, 1.1f, 0.5f}},
    {{5.1f, 0.55f, 0.0f}, {5.8f, 1.1f, 0.5f}},
    {{7.3f, 1.6f, 4.5f}, {0.5f, 1.6f, 0.8f}},
};

float distanceSquaredOnFloor(const Vec3& a, const Vec3& b) {
    const float dx = a.x - b.x;
    const float dz = a.z - b.z;
    return dx * dx + dz * dz;
}

bool circleIntersectsBoxImpl(float circleX, float circleZ, float radius,
                             const SecurityCheckScene::Box& box) {
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

}  // namespace

SecurityCheckScene::SecurityCheckScene(Language language)
    : powerOn_(true),
      alarmTriggered_(false),
      passedGate_(false),
      levelComplete_(false),
      alarmFlashTimer_(0.0f),
      alarmElapsed_(0.0f),
      language_(language),
      difficulty_(Difficulty::Normal),
      sniperFired_(false),
      sniperFiredPosition_{} {}

void SecurityCheckScene::reset(Difficulty difficulty) {
    difficulty_ = difficulty;
    powerOn_ = true;
    alarmTriggered_ = false;
    passedGate_ = false;
    levelComplete_ = false;
    alarmFlashTimer_ = 0.0f;
    alarmElapsed_ = 0.0f;
    sniperFired_ = false;
    sniperFiredPosition_ = {};

    guards_[0].setRole(SecurityGuardRole::Guard);
    guards_[0].setDifficulty(difficulty_);
    guards_[0].setWeapon(SecurityGuardWeapon::Baton);
    guards_[0].setPosition({-4.0f, 0.0f, 1.2f});
    guards_[0].setPatrolling(false);
    guards_[0].setPatrolWaypoints({});
    guards_[0].reset();

    guards_[1].setRole(SecurityGuardRole::Guard);
    guards_[1].setDifficulty(difficulty_);
    guards_[1].setWeapon(SecurityGuardWeapon::Baton);
    guards_[1].setPosition({4.0f, 0.0f, 1.2f});
    guards_[1].setPatrolling(false);
    guards_[1].setPatrolWaypoints({});
    guards_[1].reset();

    guards_[2].setRole(SecurityGuardRole::Guard);
    guards_[2].setDifficulty(difficulty_);
    guards_[2].setWeapon(SecurityGuardWeapon::Sniper);
    guards_[2].setPosition({-5.5f, 0.0f, -11.0f});
    guards_[2].setPatrolling(false);
    guards_[2].setPatrolWaypoints({});
    guards_[2].reset();

    guards_[3].setRole(SecurityGuardRole::Guard);
    guards_[3].setDifficulty(difficulty_);
    guards_[3].setWeapon(SecurityGuardWeapon::Sniper);
    guards_[3].setPosition({0.0f, 0.0f, -11.2f});
    guards_[3].setPatrolling(false);
    guards_[3].setPatrolWaypoints({});
    guards_[3].reset();

    guards_[4].setRole(SecurityGuardRole::Guard);
    guards_[4].setDifficulty(difficulty_);
    guards_[4].setWeapon(SecurityGuardWeapon::Sniper);
    guards_[4].setPosition({5.5f, 0.0f, -11.0f});
    guards_[4].setPatrolling(false);
    guards_[4].setPatrolWaypoints({});
    guards_[4].reset();
}

int SecurityCheckScene::update(float dt, const Vec3& playerPosition) {
    if (alarmTriggered_) {
        alarmFlashTimer_ += dt;
        alarmElapsed_ += dt;
    } else if (!levelComplete_) {
        if (!passedGate_) {
            const bool insidePassage =
                playerPosition.z <= kGateZ + 0.05f &&
                std::abs(playerPosition.x) <= kGatePassageHalfWidth + 0.3f;
            if (insidePassage) {
                if (powerOn_) {
                    alarmTriggered_ = true;
                    alarmFlashTimer_ = 0.0f;
                    alarmElapsed_ = 0.0f;
                } else {
                    passedGate_ = true;
                }
            }
        }

        if (passedGate_ && playerPosition.z < kExitZ) {
            levelComplete_ = true;
        }
    }

    if (levelComplete_) {
        return 0;
    }

    return updateGuards(dt, playerPosition);
}

void SecurityCheckScene::render() const {
    drawFloor();
    drawWalls();
    drawSecurityGate();
    drawPowerSwitch();
    drawExit();
    drawLighting();
    renderGuards();
}

int SecurityCheckScene::updateGuards(float dt,
                                     const Vec3& playerPosition) {
    sniperFired_ = false;
    int damage = 0;
    for (std::size_t index = 0; index < guards_.size(); ++index) {
        SecurityGuardModel& guard = guards_[index];
        if (!guardVisible(index)) {
            continue;
        }

        if (guard.weapon() == SecurityGuardWeapon::Sniper) {
            const std::size_t sniperIndex = index - 2;
            const float walkProgress = std::clamp(
                (alarmElapsed_ - kSniperSpawnDelays[sniperIndex]) /
                    kSniperWalkDuration,
                0.0f, 1.0f);
            guard.setPosition(ThreeDUtils::lerp(
                kSniperSpawnPoints[sniperIndex],
                kSniperStations[sniperIndex], walkProgress));

            const bool canSee = canSniperSeePlayer(guard, playerPosition);
            const bool wasAttacking = guard.attacking();
            damage += guard.update(dt, playerPosition, canSee, canSee,
                                   [](const Vec3&) { return false; });
            if (guard.attacking() && !wasAttacking) {
                sniperFired_ = true;
                sniperFiredPosition_ = guard.position();
            }
        } else {
            const bool canSee =
                canGateGuardSeePlayer(guard, playerPosition);
            const bool canHit = hasLineOfSight(guard, playerPosition);
            const Vec3 currentPosition = guard.position();
            damage += guard.update(
                dt, playerPosition, canSee, canHit,
                [this, index, currentPosition, &playerPosition](
                    const Vec3& position) {
                    return guardPositionBlocked(position, currentPosition,
                                                index, playerPosition);
                });
        }
    }
    return damage;
}

bool SecurityCheckScene::hasLineOfSight(
    const SecurityGuardModel& guard, const Vec3& playerPosition) const {
    const Vec3 guardEye{guard.position().x,
                        guard.position().y + kGuardEyeHeight,
                        guard.position().z};
    const Vec3 playerEye{playerPosition.x, playerPosition.y,
                         playerPosition.z};
    float hitT = 0.0f;
    return !segmentHitsGeometry(guardEye, playerEye, hitT);
}

bool SecurityCheckScene::canGateGuardSeePlayer(
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
    constexpr float kCosSightHalfAngle = 0.5f;
    if (ThreeDUtils::dot(direction, facing) < kCosSightHalfAngle) {
        return false;
    }

    return hasLineOfSight(guard, playerPosition);
}

bool SecurityCheckScene::canSniperSeePlayer(
    const SecurityGuardModel& guard, const Vec3& playerPosition) const {
    return hasLineOfSight(guard, playerPosition);
}

bool SecurityCheckScene::guardVisible(std::size_t index) const {
    if (index < 2) {
        return true;
    }
    return alarmTriggered_ &&
           alarmElapsed_ >= kSniperSpawnDelays[index - 2];
}

bool SecurityCheckScene::guardPositionBlocked(
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

    for (const Box& box : kSolidBoxes) {
        if (circleIntersectsBox(position.x, position.z,
                                kGuardCollisionRadius, box)) {
            return true;
        }
    }

    for (std::size_t index = 0; index < guards_.size(); ++index) {
        if (index == movingGuardIndex || !guardVisible(index)) {
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

void SecurityCheckScene::renderGuards() const {
    for (std::size_t index = 0; index < guards_.size(); ++index) {
        if (guardVisible(index)) {
            guards_[index].render();
        }
    }
}
bool SecurityCheckScene::cameraPositionBlocked(
    const Vec3& position, const Vec3& currentPosition) const {
    (void)currentPosition;
    if (position.x < -kRoomHalfWidth + kCameraRadius ||
        position.x > kRoomHalfWidth - kCameraRadius ||
        position.z < -kRoomHalfDepth + kCameraRadius ||
        position.z > kRoomHalfDepth - kCameraRadius) {
        return true;
    }

    for (const Box& box : kSolidBoxes) {
        if (circleIntersectsBox(position.x, position.z, kCameraRadius,
                                box)) {
            return true;
        }
    }

    for (std::size_t index = 0; index < guards_.size(); ++index) {
        if (!guardVisible(index)) {
            continue;
        }
        const SecurityGuardModel& guard = guards_[index];
        if (!guard.alive()) {
            continue;
        }
        const Vec3& guardPosition = guard.position();
        if (circleIntersectsBox(position.x, position.z, kCameraRadius,
                                {{guardPosition.x, 1.3f, guardPosition.z},
                                 {1.15f, 2.6f, 1.0f}})) {
            return true;
        }
    }
    return false;
}

bool SecurityCheckScene::segmentHitsGeometry(const Vec3& start,
                                             const Vec3& end,
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
    test({{0.0f, kWallHeight * 0.5f, kRoomHalfDepth},
          {2.0f * kRoomHalfWidth, kWallHeight, kWallThickness}});

    const float frontSegmentWidth = kRoomHalfWidth - kExitHalfWidth;
    test({{-(kExitHalfWidth + frontSegmentWidth * 0.5f), kWallHeight * 0.5f,
           -kRoomHalfDepth},
          {frontSegmentWidth, kWallHeight, kWallThickness}});
    test({{kExitHalfWidth + frontSegmentWidth * 0.5f, kWallHeight * 0.5f,
           -kRoomHalfDepth},
          {frontSegmentWidth, kWallHeight, kWallThickness}});

    for (const Box& box : kSolidBoxes) {
        test(box);
    }

    if (hit) {
        hitT = closest;
    }
    return hit;
}

bool SecurityCheckScene::circleIntersectsBox(float circleX, float circleZ,
                                             float radius, const Box& box) {
    return circleIntersectsBoxImpl(circleX, circleZ, radius, box);
}

bool SecurityCheckScene::segmentIntersectsAabb(const Vec3& start,
                                               const Vec3& end,
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

bool SecurityCheckScene::nearSwitch(const Vec3& playerPosition) const {
    return distanceSquaredOnFloor(playerPosition, kSwitchPosition) <=
           kSwitchDistance * kSwitchDistance;
}

bool SecurityCheckScene::interactWithSwitch(const Vec3& playerPosition) {
    if (!nearSwitch(playerPosition) || alarmTriggered_ || levelComplete_) {
        return false;
    }

    powerOn_ = !powerOn_;
    return true;
}

Vec3 SecurityCheckScene::spawnPosition() const {
    return {0.0f, kCameraGroundHeight, 10.2f};
}

bool SecurityCheckScene::powerOn() const {
    return powerOn_;
}

bool SecurityCheckScene::alarmTriggered() const {
    return alarmTriggered_;
}

bool SecurityCheckScene::levelComplete() const {
    return levelComplete_;
}

float SecurityCheckScene::alarmFlashAmount() const {
    if (!alarmTriggered_) {
        return 0.0f;
    }
    return 0.5f + 0.5f * std::sin(alarmFlashTimer_ * 9.0f);
}

bool SecurityCheckScene::segmentHitsGuard(const Vec3& start, const Vec3& end,
                                          float& hitT,
                                          std::size_t& guardIndex) const {
    bool hit = false;
    float closest = 2.0f;
    std::size_t closestIndex = 0;
    for (std::size_t index = 0; index < guards_.size(); ++index) {
        if (!guardVisible(index) || !guards_[index].alive()) {
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

void SecurityCheckScene::applyGuardDamage(std::size_t guardIndex,
                                          const Vec3& hitPosition) {
    if (guardIndex >= guards_.size()) {
        return;
    }
    guards_[guardIndex].applyPistolDamage(hitPosition);
}

void SecurityCheckScene::applyGuardSniperDamage(std::size_t guardIndex,
                                                const Vec3& hitPosition) {
    if (guardIndex >= guards_.size()) {
        return;
    }
    guards_[guardIndex].applySniperDamage(hitPosition);
}

void SecurityCheckScene::applyGuardKnifeDamage(std::size_t guardIndex,
                                               const Vec3& hitPosition) {
    if (guardIndex >= guards_.size()) {
        return;
    }
    guards_[guardIndex].applyKnifeDamage(hitPosition);
}

bool SecurityCheckScene::sniperFired() const {
    return sniperFired_;
}

Vec3 SecurityCheckScene::sniperFiredPosition() const {
    return sniperFiredPosition_;
}

void SecurityCheckScene::drawFloor() const {
    ThreeDUtils::drawCube({0.0f, -0.12f, 0.0f},
                          {2.0f * kRoomHalfWidth, 0.24f,
                           2.0f * kRoomHalfDepth},
                          {0.20f, 0.23f, 0.25f});

    for (int z = -11; z <= 11; ++z) {
        ThreeDUtils::drawCube(
            {0.0f, 0.015f, static_cast<float>(z)},
            {15.2f, 0.025f, 0.025f},
            (z % 2 == 0) ? Color{0.24f, 0.27f, 0.28f}
                         : Color{0.18f, 0.21f, 0.22f});
    }
    for (int x = -7; x <= 7; ++x) {
        if (x % 2 == 0) {
            ThreeDUtils::drawCube(
                {static_cast<float>(x), 0.016f, 0.0f},
                {0.025f, 0.025f, 23.2f}, Color{0.24f, 0.27f, 0.28f});
        }
    }

    ThreeDUtils::drawCube({0.0f, 0.05f, 0.0f},
                          {2.0f * kGatePassageHalfWidth + 0.6f, 0.06f, 0.35f},
                          {0.96f, 0.86f, 0.24f});
}

void SecurityCheckScene::drawWalls() const {
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
    ThreeDUtils::drawCube({0.0f, kWallHeight * 0.5f, kRoomHalfDepth},
                          {2.0f * kRoomHalfWidth, kWallHeight,
                           kWallThickness},
                          wall);

    const float frontSegmentWidth = kRoomHalfWidth - kExitHalfWidth;
    ThreeDUtils::drawCube(
        {-(kExitHalfWidth + frontSegmentWidth * 0.5f), kWallHeight * 0.5f,
         -kRoomHalfDepth},
        {frontSegmentWidth, kWallHeight, kWallThickness}, wall);
    ThreeDUtils::drawCube(
        {kExitHalfWidth + frontSegmentWidth * 0.5f, kWallHeight * 0.5f,
         -kRoomHalfDepth},
        {frontSegmentWidth, kWallHeight, kWallThickness}, wall);

    ThreeDUtils::drawCube({0.0f, kWallHeight - 0.20f, kRoomHalfDepth},
                          {2.0f * kRoomHalfWidth, 0.40f, kWallThickness},
                          wallTrim);
    ThreeDUtils::drawCube({0.0f, 6.15f, 0.0f},
                          {2.0f * kRoomHalfWidth, 0.30f,
                           2.0f * kRoomHalfDepth},
                          {0.12f, 0.15f, 0.17f});
}

void SecurityCheckScene::drawSecurityGate() const {
    const SecurityText& text = securityText(language_);
    const Color frame{0.16f, 0.20f, 0.24f};
    const Color pillarColor{0.24f, 0.28f, 0.30f};
    for (const float x : {-kGatePassageHalfWidth, kGatePassageHalfWidth}) {
        ThreeDUtils::drawCube({x, 1.6f, 0.0f}, {0.50f, 3.2f, 0.50f},
                              pillarColor);
        drawWarningStripe(x, 0.62f, 0.28f, 0.44f, 0.08f, kSunCore);
        drawWarningStripe(x, 1.10f, 0.28f, 0.44f, 0.08f, kInkColor);
        drawWarningStripe(x, 1.58f, 0.28f, 0.44f, 0.08f, kSunCore);
    }

    ThreeDUtils::drawCube({0.0f, 3.24f, 0.0f},
                          {2.0f * kGatePassageHalfWidth + 0.5f, 0.32f, 0.50f},
                          frame);
    ThreeDUtils::drawCube({0.0f, 3.40f, 0.0f},
                          {0.42f, 0.34f, 0.20f},
                          powerOn_ ? kButtonExit : kButtonStart);

    drawWorldLabel(text.gateLabel, {0.0f, 3.62f, 0.0f}, 0.045f, kSignColor);
}

void SecurityCheckScene::drawPowerSwitch() const {
    const SecurityText& text = securityText(language_);
    const Color cabinet{0.16f, 0.20f, 0.24f};
    ThreeDUtils::drawCube({kSwitchPosition.x, 1.55f, kSwitchPosition.z},
                          {0.50f, 1.50f, 0.80f}, cabinet);
    ThreeDUtils::drawCube(
        {kSwitchPosition.x, 1.75f, kSwitchPosition.z + 0.02f},
        {0.20f, 0.20f, 0.12f}, kInkColor);
    ThreeDUtils::drawCube(
        {kSwitchPosition.x, 1.90f, kSwitchPosition.z + 0.04f},
        {0.16f, 0.26f, 0.06f}, powerOn_ ? kButtonExit : kButtonStart);
    ThreeDUtils::drawCube(
        {kSwitchPosition.x, 1.30f, kSwitchPosition.z + 0.04f},
        {0.30f, 0.22f, 0.04f}, kSignColor);

    drawWorldLabel(text.switchLabel,
                   {kSwitchPosition.x, 2.55f, kSwitchPosition.z}, 0.045f,
                   kSignColor);
    drawWorldLabel(powerOn_ ? text.powerOn : text.powerOff,
                   {kSwitchPosition.x, 2.18f, kSwitchPosition.z}, 0.040f,
                   powerOn_ ? kButtonExit : kButtonStart);
}

void SecurityCheckScene::drawExit() const {
    const SecurityText& text = securityText(language_);
    ThreeDUtils::drawCube({0.0f, 5.2f, -kRoomHalfDepth + 0.08f},
                          {2.0f * kExitHalfWidth + 0.5f, 0.42f, 0.10f},
                          kStoneDark);
    ThreeDUtils::drawCube({-kExitHalfWidth, 2.6f, -kRoomHalfDepth + 0.10f},
                          {0.40f, 4.8f, 0.10f}, kStoneDark);
    ThreeDUtils::drawCube({kExitHalfWidth, 2.6f, -kRoomHalfDepth + 0.10f},
                          {0.40f, 4.8f, 0.10f}, kStoneDark);
    ThreeDUtils::drawCube({0.0f, 1.2f, -kRoomHalfDepth + 0.18f},
                          {2.0f * kExitHalfWidth - 0.3f, 2.4f, 0.06f},
                          levelComplete_ ? kButtonStart : kInkColor);
    drawWorldLabel(text.exitLabel, {0.0f, 5.42f, -kRoomHalfDepth + 0.05f},
                   0.050f, kSignColor);
}

void SecurityCheckScene::drawLighting() const {
    const std::array<float, 3> lightX{{-4.0f, 0.0f, 4.0f}};
    for (const float x : lightX) {
        ThreeDUtils::drawCube({x, 5.72f, 0.0f},
                              {2.4f, 0.12f, 0.22f}, kInkColor);
        ThreeDUtils::drawCube({x, 5.64f, 0.0f},
                              {1.72f, 0.08f, 0.12f}, kHouseWindowLight);
    }
}

void SecurityCheckScene::drawWorldLabel(const char* label, const Vec3& center,
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