#include "WeaponBase.h"

namespace pixel_world {

WeaponBase::~WeaponBase() = default;

void WeaponBase::setViewMode(WeaponViewMode mode) {
    viewMode_ = mode;
}

WeaponViewMode WeaponBase::viewMode() const {
    return viewMode_;
}

void WeaponBase::updateThirdPerson(float dt, bool attackActive,
                                   float attackTimer) {
    (void)dt;
    (void)attackActive;
    (void)attackTimer;
}

void WeaponBase::renderThirdPerson(const GLfloat* weaponMatrix,
                                   bool showMuzzleFlash) const {
    (void)weaponMatrix;
    (void)showMuzzleFlash;
}

void WeaponBase::applyThirdPersonPose(Pose& pose,
                                      SecurityGuardAnimation animation,
                                      float phase) const {
    (void)pose;
    (void)animation;
    (void)phase;
}

int WeaponBase::thirdPersonWeaponBoneParent() const {
    return kRightLowerArmBone;
}

}  // namespace pixel_world
