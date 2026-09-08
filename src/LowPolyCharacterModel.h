#pragma once

#include "CharacterModelBase.h"

#include <memory>

namespace pixel_world {

class LowPolyCharacterModel final : public CharacterModelBase {
public:
    LowPolyCharacterModel();
    ~LowPolyCharacterModel() override;

    void reset() override;
    void update(GLFWwindow* window, float dt) override;
    void render() const override;
    void renderHealthBar() const override;
    bool segmentHit(const Vec3& start, const Vec3& end,
                    float& hitT) const override;
    void applyPistolDamage(const Vec3& hitPosition) override;

    const Vec3& position() const override;
    bool alive() const override;

private:
    static bool segmentIntersectsAabb(const Vec3& start, const Vec3& end,
                                      const Vec3& boundsMin,
                                      const Vec3& boundsMax, float& hitT);
    void kill();
    CharacterHitZone hitZoneForPoint(const Vec3& hitPosition) const;
    static int pistolDamageForZone(CharacterHitZone zone);
    void buildMeshes();

    struct Impl;

    Vec3 position_;
    float yawDegrees_;
    float animationPhase_;
    float patrolDirection_;
    int hp_;
    bool alive_;
    float deathTimer_;
    float attackTimer_;
    bool previousAttackDown_;
    bool crouched_;
    bool prone_;
    std::unique_ptr<Impl> impl_;
};

}  // namespace pixel_world
