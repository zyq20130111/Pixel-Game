#pragma once

#include "types.h"

#include <functional>
#include <vector>

namespace pixel_world {

enum class SecurityGuardRole {
    Guard,
    Captain,
};

enum class SecurityGuardState {
    Patrol,
    Chasing,
    Attacking,
    Returning,
};

class SecurityGuardModel final {
public:
    using MovementCollisionTest = std::function<bool(const Vec3&)>;

    explicit SecurityGuardModel(
        SecurityGuardRole role = SecurityGuardRole::Guard);

    void reset();
    void update(float dt);
    int update(float dt, const Vec3& playerPosition, bool playerDetected,
               bool weaponCanHitPlayer,
               const MovementCollisionTest& collisionTest);
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
    bool playerDetected() const;
    bool attacking() const;
    bool returning() const;
    bool weaponCanHitPlayer(const Vec3& playerPosition) const;
    int health() const;
    int maxHealth() const;
    float yawDegrees() const;

    bool segmentHit(const Vec3& start, const Vec3& end,
                    float& hitT) const;
    void applyPistolDamage(const Vec3& hitPosition);
    void applySniperDamage(const Vec3& hitPosition);
    void applyKnifeDamage(const Vec3& hitPosition);

private:
    static bool segmentIntersectsAabb(const Vec3& start, const Vec3& end,
                                      const Vec3& boundsMin,
                                      const Vec3& boundsMax, float& hitT);
    CharacterHitZone hitZoneForPoint(const Vec3& hitPosition) const;
    static int pistolDamageForZone(CharacterHitZone zone);
    static int sniperDamageForZone(CharacterHitZone zone);
    static int knifeDamageForZone(CharacterHitZone zone);
    int maxHealthForDifficulty(Difficulty difficulty) const;
    int batonDamageForDifficulty(Difficulty difficulty) const;
    float chaseSpeedForDifficulty(Difficulty difficulty) const;
    float attackLungeSpeedForDifficulty(Difficulty difficulty) const;
    float attackLungeSpeedAtProgress(float progress) const;
    void updatePatrol(float dt, const MovementCollisionTest& collisionTest);
    bool updateReturning(float dt, const MovementCollisionTest& collisionTest);
    void beginReturning();
    bool moveTo(const Vec3& target, float maxDistance,
                const MovementCollisionTest& collisionTest);

    SecurityGuardRole role_;
    Difficulty difficulty_;
    Vec3 position_;
    Vec3 homePosition_;
    float yawDegrees_;
    float animationPhase_;
    float deathTimer_;
    int health_;
    int maxHealth_;
    bool alive_;
    bool patrolling_;
    std::vector<Vec3> patrolWaypoints_;
    std::size_t patrolWaypointIndex_;
    SecurityGuardState state_;
    bool playerDetected_;
    float alertTimer_;
    float attackTimer_;
    float attackCooldown_;
    bool attackHit_;
};

}  // namespace pixel_world
