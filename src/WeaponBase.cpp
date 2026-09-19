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

}  // namespace pixel_world
