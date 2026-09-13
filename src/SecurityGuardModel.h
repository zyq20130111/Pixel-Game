#pragma once

#include "types.h"

#include <vector>

namespace pixel_world {

enum class SecurityGuardRole {
    Guard,
    Captain,
};

class SecurityGuardModel final {
public:
    explicit SecurityGuardModel(
        SecurityGuardRole role = SecurityGuardRole::Guard);

    void reset();
    void update(float dt);
    void render() const;
    void renderHealthBar() const;

    void setRole(SecurityGuardRole role);
    void setDifficulty(Difficulty difficulty);
    void setPosition(const Vec3& position);
    void setPatrolling(bool patrolling);
    void setPatrolWaypoints(const std::vector<Vec3>& waypoints);

    const Vec3& position() const;
    bool alive() const;
    bool defeated() const;
    bool isCaptain() const;
    int health() const;
    int maxHealth() const;

    bool segmentHit(const Vec3& start, const Vec3& end,
                    float& hitT) const;
    void applyPistolDamage(const Vec3& hitPosition);

private:
    static bool segmentIntersectsAabb(const Vec3& start, const Vec3& end,
                                      const Vec3& boundsMin,
                                      const Vec3& boundsMax, float& hitT);
    CharacterHitZone hitZoneForPoint(const Vec3& hitPosition) const;
    static int pistolDamageForZone(CharacterHitZone zone);
    int maxHealthForDifficulty(Difficulty difficulty) const;

    SecurityGuardRole role_;
    Difficulty difficulty_;
    Vec3 position_;
    float yawDegrees_;
    float animationPhase_;
    float deathTimer_;
    int health_;
    int maxHealth_;
    bool alive_;
    bool patrolling_;
    std::vector<Vec3> patrolWaypoints_;
    std::size_t patrolWaypointIndex_;
};

}  // namespace pixel_world
