#pragma once

#include "WeaponBase.h"
#include "types.h"

namespace pixel_world {

class Camera;

class Knife final : public WeaponBase {
public:
    Knife();

    void reset();
    bool update(GLFWwindow* window, const Camera& camera,
                std::vector<std::unique_ptr<BulletBase>>& bullets,
                SoundManager& soundManager, bool& previousFireDown,
                float dt) override;
    void render(const Camera& camera,
                const WeaponRenderMotion& motion) const override;
    float muzzleFlashTimer() const override;

    void attackSegment(const Camera& camera, Vec3& start, Vec3& end) const;

private:
    static float bob(bool cameraMoving, bool cameraRunning,
                     float cameraWalkPhase);
    static float sway(bool cameraMoving, bool cameraRunning,
                      float cameraWalkPhase);
    static float smooth01(float value);
    static float attackProgress(float attackTimer);
    static float slashAmount(float progress);
    static float windupAmount(float progress);
    static Vec3 weaponRoot(const Camera& camera, float attackTimer);

    float attackTimer_;
    bool hitDelivered_;
};

}  // namespace pixel_world
