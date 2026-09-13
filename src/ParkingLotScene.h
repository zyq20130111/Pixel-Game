#pragma once

#include "SecurityGuardModel.h"

#include <array>
#include <cstddef>

namespace pixel_world {

class ParkingLotScene final {
public:
    static constexpr std::size_t kGuardCount = 3;

    struct Box {
        Vec3 center;
        Vec3 size;
    };

    ParkingLotScene();

    void reset(Difficulty difficulty);
    int update(float dt, const Vec3& playerPosition,
               bool playerFired = false);
    void render() const;

    bool cameraPositionBlocked(const Vec3& position,
                               const Vec3& currentPosition) const;
    bool segmentHitsGeometry(const Vec3& start, const Vec3& end,
                             float& hitT) const;
    bool segmentHitsGuard(const Vec3& start, const Vec3& end, float& hitT,
                          std::size_t& guardIndex) const;
    void applyGuardDamage(std::size_t guardIndex, const Vec3& hitPosition);

    bool tryCollectAccessCard(const Vec3& playerPosition);
    bool interactWithElevator(const Vec3& playerPosition);
    bool nearElevator(const Vec3& playerPosition) const;

    Vec3 spawnPosition() const;
    const SecurityGuardModel& guard(std::size_t index) const;
    int livingGuardCount() const;
    bool captainDefeated() const;
    bool accessCardObtained() const;
    bool elevatorOpen() const;
    bool levelComplete() const;

private:
    static bool circleIntersectsBox(float circleX, float circleZ, float radius,
                                    const Box& box);
    static bool segmentIntersectsAabb(const Vec3& start, const Vec3& end,
                                      const Box& box, float& hitT);
    static Vec3 pointOnSegment(const Vec3& start, const Vec3& end, float t);
    bool canGuardSeePlayer(const SecurityGuardModel& guard,
                           const Vec3& playerPosition) const;
    bool guardPositionBlocked(const Vec3& position,
                              const Vec3& currentPosition,
                              std::size_t movingGuardIndex,
                              const Vec3& playerPosition) const;

    void drawFloor() const;
    void drawWalls() const;
    void drawColumns() const;
    void drawParkingLines() const;
    void drawCar() const;
    void drawTruck() const;
    void drawElevator() const;
    void drawRollerDoor() const;
    void drawLighting() const;
    void drawAccessCard() const;
    void drawWorldLabel(const char* label, const Vec3& center,
                        float pixelScale, const Color& color) const;

    bool captainDefeated_;
    bool accessCardObtained_;
    bool elevatorOpen_;
    bool levelComplete_;
    float elevatorOpenAmount_;
    bool groupAlerted_;
    Vec3 accessCardPosition_;
    Difficulty difficulty_;
    std::array<SecurityGuardModel, kGuardCount> guards_;
};

}  // namespace pixel_world
