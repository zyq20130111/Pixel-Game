#pragma once

#include "Localization.h"
#include "SecurityGuardModel.h"
#include "types.h"

#include <array>
#include <cstddef>

namespace pixel_world {

class SecurityCheckScene final {
public:
    struct Box {
        Vec3 center;
        Vec3 size;
    };

    static constexpr std::size_t kGuardCount = 5;

    explicit SecurityCheckScene(Language language = Language::English);

    void reset(Difficulty difficulty);
    int update(float dt, const Vec3& playerPosition);
    void render() const;

    bool cameraPositionBlocked(const Vec3& position,
                               const Vec3& currentPosition) const;
    bool segmentHitsGeometry(const Vec3& start, const Vec3& end,
                             float& hitT) const;

    bool nearSwitch(const Vec3& playerPosition) const;
    bool interactWithSwitch(const Vec3& playerPosition);

    Vec3 spawnPosition() const;
    bool powerOn() const;
    bool alarmTriggered() const;
    bool levelComplete() const;
    float alarmFlashAmount() const;
    bool segmentHitsGuard(const Vec3& start, const Vec3& end, float& hitT,
                         std::size_t& guardIndex) const;
    void applyGuardDamage(std::size_t guardIndex, const Vec3& hitPosition);
    void applyGuardSniperDamage(std::size_t guardIndex,
                             const Vec3& hitPosition);
    void applyGuardKnifeDamage(std::size_t guardIndex,
                               const Vec3& hitPosition);
    bool sniperFired() const;
    Vec3 sniperFiredPosition() const;

private:
    static bool circleIntersectsBox(float circleX, float circleZ, float radius,
                                    const Box& box);
    static bool segmentIntersectsAabb(const Vec3& start, const Vec3& end,
                                      const Box& box, float& hitT);

    void drawFloor() const;
    void drawWalls() const;
    void drawSecurityGate() const;
    void drawPowerSwitch() const;
    void drawExit() const;
    void drawLighting() const;
    void drawWorldLabel(const char* label, const Vec3& center,
                        float pixelScale, const Color& color) const;

    int updateGuards(float dt, const Vec3& playerPosition);
    bool hasLineOfSight(const SecurityGuardModel& guard,
                        const Vec3& playerPosition) const;
    bool canGateGuardSeePlayer(const SecurityGuardModel& guard,
                               const Vec3& playerPosition) const;
    bool canSniperSeePlayer(const SecurityGuardModel& guard,
                            const Vec3& playerPosition) const;
    bool guardVisible(std::size_t index) const;
    bool guardPositionBlocked(const Vec3& position,
                              const Vec3& currentPosition,
                              std::size_t movingGuardIndex,
                              const Vec3& playerPosition) const;
    void renderGuards() const;

    bool powerOn_;
    bool alarmTriggered_;
    bool passedGate_;
    bool levelComplete_;
    float alarmFlashTimer_;
    float alarmElapsed_;
    Language language_;
    Difficulty difficulty_;
    std::array<SecurityGuardModel, kGuardCount> guards_;
    bool sniperFired_;
    Vec3 sniperFiredPosition_;
};

}  // namespace pixel_world