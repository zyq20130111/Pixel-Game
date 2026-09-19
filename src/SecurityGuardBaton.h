#pragma once

#include "WeaponBase.h"

namespace pixel_world {

class SecurityGuardBaton final : public WeaponBase {
public:
    bool update(GLFWwindow* window, const Camera& camera,
                std::vector<std::unique_ptr<BulletBase>>& bullets,
                SoundManager& soundManager, bool& previousFireDown,
                float dt) override;
    void render(const Camera& camera,
                const WeaponRenderMotion& motion) const override;
    float muzzleFlashTimer() const override;
    void renderThirdPerson(const GLfloat* weaponMatrix,
                           bool showMuzzleFlash) const override;
    void applyThirdPersonPose(Pose& pose,
                              SecurityGuardAnimation animation,
                              float phase) const override;
};

}  // namespace pixel_world
