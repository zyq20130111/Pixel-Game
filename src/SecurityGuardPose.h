#pragma once

#include "types.h"

#include <array>
#include <cstddef>

namespace pixel_world {

enum BoneId : std::size_t {
    kRootBone = 0,
    kTorsoBone,
    kHeadBone,
    kLeftUpperArmBone,
    kLeftLowerArmBone,
    kRightUpperArmBone,
    kRightLowerArmBone,
    kLeftUpperLegBone,
    kLeftLowerLegBone,
    kRightUpperLegBone,
    kRightLowerLegBone,
    kWeaponBone,
    kBoneCount,
};

enum class SecurityGuardAnimation {
    Stand,
    Walk,
    Run,
    Aim,
    Fire,
};

using SecurityAnimation = SecurityGuardAnimation;

struct BonePose {
    Vec3 position;
    Vec3 rotationDegrees;
};

using Pose = std::array<BonePose, kBoneCount>;

}  // namespace pixel_world
