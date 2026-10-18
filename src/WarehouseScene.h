#pragma once

#include "Localization.h"
#include "SecurityGuardModel.h"
#include "types.h"

#include <array>
#include <cstddef>

namespace pixel_world {

class WarehouseScene final {
public:
    static constexpr std::size_t kGuardCount = 3;

    struct Box {
        Vec3 center;
        Vec3 size;
    };

    explicit WarehouseScene(Language language = Language::English);

    void reset(Difficulty difficulty);
    int update(float dt, const Vec3& playerPosition);
    void render() const;

    bool cameraPositionBlocked(const Vec3& position,
                               const Vec3& currentPosition) const;
    bool segmentHitsGeometry(const Vec3& start, const Vec3& end,
                             float& hitT) const;
    bool segmentHitsGuard(const Vec3& start, const Vec3& end, float& hitT,
                          std::size_t& guardIndex) const;
    void applyGuardDamage(std::size_t guardIndex, const Vec3& hitPosition);
    void applyGuardSniperDamage(std::size_t guardIndex,
                                const Vec3& hitPosition);
    void applyGuardKnifeDamage(std::size_t guardIndex,
                               const Vec3& hitPosition);

    Vec3 spawnPosition() const;

    bool nearPlatformClimb(const Vec3& playerPosition) const;
    float groundHeightAt(const Vec3& playerPosition) const;
    bool onPlatform(const Vec3& playerPosition) const;
    bool nearSniperCrate(const Vec3& playerPosition) const;
    bool interactWithSniperCrate(const Vec3& playerPosition);

    bool nearForklift(const Vec3& playerPosition) const;
    bool enterForklift(const Vec3& playerPosition);
    void exitForklift();
    bool drivingForklift() const;
    Vec3 forkliftSeatPosition() const;
    Vec3 forkliftExitPosition() const;
    float forkliftHeadingDegrees() const;
    void updateForklift(float dt, bool accelerate, bool reverse,
                        bool steerLeft, bool steerRight);

    bool nearExit(const Vec3& playerPosition) const;
    bool sniperRifleTaken() const;
    bool ironDoorOpen() const;
    bool shelfFallen() const;
    bool levelComplete() const;
    bool sniperFired() const;
    Vec3 sniperFiredPosition() const;

private:
    static bool circleIntersectsBox(float circleX, float circleZ, float radius,
                                    const Box& box);
    static bool segmentIntersectsAabb(const Vec3& start, const Vec3& end,
                                      const Box& box, float& hitT);

    void updateDoorAndExit(const Vec3& playerPosition);
    int updateGuards(float dt, const Vec3& playerPosition);
    bool hasLineOfSight(const SecurityGuardModel& guard,
                        const Vec3& playerPosition) const;
    bool canGuardSeePlayer(const SecurityGuardModel& guard,
                           const Vec3& playerPosition) const;
    bool guardPositionBlocked(const Vec3& position,
                              const Vec3& currentPosition,
                              std::size_t movingGuardIndex,
                              const Vec3& playerPosition) const;
    bool forkliftBlocked(const Vec3& position) const;
    bool platformTopContains(const Vec3& position) const;
    Box forkliftBox() const;
    Box shelfBox() const;
    Vec3 forkliftForward() const;

    void drawFloor() const;
    void drawWalls() const;
    void drawIronDoor() const;
    void drawPlatform() const;
    void drawCardboardPiles() const;
    void drawSniperCrate() const;
    void drawForklift() const;
    void drawColumns() const;
    void drawShelf() const;
    void drawLighting() const;
    void drawExitMarker() const;
    void drawWorldLabel(const char* label, const Vec3& center,
                        float pixelScale, const Color& color) const;
    void renderGuards() const;

    bool sniperRifleTaken_;
    bool ironDoorOpen_;
    bool shelfFallen_;
    bool levelComplete_;
    bool drivingForklift_;
    float forkliftHeadingDegrees_;
    Vec3 forkliftPosition_;
    Language language_;
    Difficulty difficulty_;
    bool sniperFired_;
    Vec3 sniperFiredPosition_;
    std::array<SecurityGuardModel, kGuardCount> guards_;
};

}  // namespace pixel_world
