#pragma once

#include "SecurityGuardPose.h"
#include "platform.h"
#include "types.h"

#include <memory>
#include <vector>

namespace pixel_world {

class Camera;
class BulletBase;
class SoundManager;

struct WeaponRenderMotion {
    Vec3 translation{};
    Vec3 rotationDegrees{};
};

enum class WeaponViewMode {
    FirstPerson,
    ThirdPerson,
};

class WeaponBase {
public:
    virtual ~WeaponBase();

    void setViewMode(WeaponViewMode mode);
    WeaponViewMode viewMode() const;

    virtual bool update(GLFWwindow* window, const Camera& camera,
                        std::vector<std::unique_ptr<BulletBase>>& bullets,
                        SoundManager& soundManager, bool& previousFireDown,
                        float dt) = 0;
    virtual void render(const Camera& camera,
                        const WeaponRenderMotion& motion) const = 0;
    virtual float muzzleFlashTimer() const = 0;

    virtual void updateThirdPerson(float dt, bool attackActive,
                                   float attackTimer);
    virtual void renderThirdPerson(const GLfloat* weaponMatrix,
                                   bool showMuzzleFlash) const;
    virtual void applyThirdPersonPose(Pose& pose,
                                      SecurityGuardAnimation animation,
                                      float phase) const;
    virtual int thirdPersonWeaponBoneParent() const;

protected:
    WeaponViewMode viewMode_ = WeaponViewMode::FirstPerson;
};

}  // namespace pixel_world
