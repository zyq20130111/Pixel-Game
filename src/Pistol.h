#pragma once

#include "WeaponBase.h"
#include "types.h"

namespace pixel_world {

class SoundManager;

class Pistol final : public WeaponBase {
public:
    Pistol();

    void reset();
    void update(GLFWwindow* window, const Camera& camera,
                std::vector<std::unique_ptr<BulletBase>>& bullets,
                SoundManager& soundManager, bool& previousFireDown,
                float dt) override;
    void render(const Camera& camera) const override;
    float muzzleFlashTimer() const override;

private:
    static float bob(bool cameraMoving, bool cameraRunning,
                     float cameraWalkPhase);
    static float sway(bool cameraMoving, bool cameraRunning,
                      float cameraWalkPhase);
    static Vec3 weaponRoot(const Camera& camera, float muzzleFlashTimer);
    static Vec3 muzzleFrontViewPosition(const Camera& camera,
                                        float muzzleFlashTimer);
    static Vec3 muzzleWorldPosition(const Camera& camera);

    static void drawFirstPersonHandBack();
    static void drawFirstPersonHandGrip();
    static void drawMuzzleFlash(float muzzleFlashTimer);
    static void drawIronSights(float aimAmount);

    float muzzleFlashTimer_;
};

}  // namespace pixel_world
