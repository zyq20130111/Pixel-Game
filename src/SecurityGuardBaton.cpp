#include "SecurityGuardBaton.h"

#include "ThreeDUtils.h"
#include "game_constants.h"

#include <cmath>

namespace pixel_world {
namespace {

void setBatonPoseBone(Pose& pose, BoneId bone, const Vec3& position,
                      const Vec3& rotationDegrees) {
    pose[bone] = {position, rotationDegrees};
}

}  // namespace

bool SecurityGuardBaton::update(
    GLFWwindow* window, const Camera& camera,
    std::vector<std::unique_ptr<BulletBase>>& bullets,
    SoundManager& soundManager, bool& previousFireDown, float dt) {
    (void)window;
    (void)camera;
    (void)bullets;
    (void)soundManager;
    (void)previousFireDown;
    (void)dt;
    return false;
}

void SecurityGuardBaton::render(const Camera& camera,
                                const WeaponRenderMotion& motion) const {
    (void)camera;
    (void)motion;
}

float SecurityGuardBaton::muzzleFlashTimer() const {
    return 0.0f;
}

void SecurityGuardBaton::renderThirdPerson(const GLfloat* weaponMatrix,
                                           bool showMuzzleFlash) const {
    (void)showMuzzleFlash;
    glPushMatrix();
    glMultMatrixf(weaponMatrix);
    ThreeDUtils::drawCube({0.0f, -0.05f, 0.0f},
                          {0.13f, 0.70f, 0.13f},
                          constants::kPoliceBaton);
    ThreeDUtils::drawCube({0.0f, -0.42f, 0.0f},
                          {0.17f, 0.16f, 0.17f},
                          constants::kPoliceBatonHighlight);
    glPopMatrix();
}

void SecurityGuardBaton::applyThirdPersonPose(
    Pose& pose, SecurityGuardAnimation animation, float phase) const {
    const float gait = std::sin(phase);

    switch (animation) {
        case SecurityGuardAnimation::Stand:
            setBatonPoseBone(pose, kLeftUpperArmBone, {-0.62f, 0.30f, 0.0f},
                             {-8.0f, 0.0f, 0.0f});
            setBatonPoseBone(pose, kLeftLowerArmBone, {0.0f, -0.60f, 0.0f},
                             {-12.0f, 0.0f, 0.0f});
            setBatonPoseBone(pose, kRightUpperArmBone, {0.62f, 0.30f, 0.0f},
                             {-10.0f, 0.0f, 0.0f});
            setBatonPoseBone(pose, kRightLowerArmBone, {0.0f, -0.60f, 0.0f},
                             {-8.0f, 0.0f, 0.0f});
            setBatonPoseBone(pose, kWeaponBone, {0.0f, -0.58f, 0.05f},
                             {0.0f, 0.0f, -15.0f});
            break;

        case SecurityGuardAnimation::Walk:
        case SecurityGuardAnimation::Run: {
            const float armAmplitude =
                animation == SecurityGuardAnimation::Walk ? 22.0f : 52.0f;
            setBatonPoseBone(pose, kLeftUpperArmBone, {-0.62f, 0.30f, 0.0f},
                             {-gait * armAmplitude, 0.0f, 0.0f});
            setBatonPoseBone(pose, kRightUpperArmBone, {0.62f, 0.30f, 0.0f},
                             {gait * armAmplitude, 0.0f, 0.0f});
            setBatonPoseBone(pose, kWeaponBone, {0.0f, -0.58f, 0.05f},
                             {0.0f, 0.0f, -15.0f + gait * 18.0f});
            break;
        }

        case SecurityGuardAnimation::Aim:
            setBatonPoseBone(pose, kTorsoBone, {0.0f, 1.30f, 0.0f},
                             {-5.0f, 0.0f, 0.0f});
            setBatonPoseBone(pose, kHeadBone, {0.0f, 0.88f, 0.0f},
                             {4.0f, 0.0f, 0.0f});
            setBatonPoseBone(pose, kLeftUpperArmBone, {-0.62f, 0.30f, 0.0f},
                             {-28.0f, 0.0f, 0.0f});
            setBatonPoseBone(pose, kLeftLowerArmBone, {0.0f, -0.60f, 0.0f},
                             {-18.0f, 0.0f, 0.0f});
            setBatonPoseBone(pose, kRightUpperArmBone, {0.62f, 0.30f, 0.0f},
                             {-85.0f, 0.0f, 0.0f});
            setBatonPoseBone(pose, kRightLowerArmBone, {0.0f, -0.60f, 0.0f},
                             {-40.0f, 0.0f, 0.0f});
            setBatonPoseBone(pose, kWeaponBone, {0.0f, -0.58f, 0.10f},
                             {0.0f, 0.0f, -30.0f});
            break;

        case SecurityGuardAnimation::Fire: {
            const float swing = std::sin(phase * constants::kPi);
            setBatonPoseBone(pose, kTorsoBone, {0.0f, 1.30f, 0.0f},
                             {-8.0f * swing, 0.0f, 0.0f});
            setBatonPoseBone(pose, kHeadBone, {0.0f, 0.88f, 0.0f},
                             {6.0f * swing, 0.0f, 0.0f});
            setBatonPoseBone(pose, kLeftUpperArmBone, {-0.62f, 0.30f, 0.0f},
                             {-25.0f * swing, 0.0f, 0.0f});
            setBatonPoseBone(pose, kRightUpperArmBone, {0.62f, 0.30f, 0.0f},
                             {-10.0f - 95.0f * swing, 0.0f, 0.0f});
            setBatonPoseBone(pose, kRightLowerArmBone, {0.0f, -0.60f, 0.0f},
                             {-15.0f - 45.0f * swing, 0.0f, 0.0f});
            setBatonPoseBone(pose, kWeaponBone, {0.0f, -0.58f, 0.05f},
                             {0.0f, 0.0f, -70.0f * swing});
            break;
        }
    }
}

}  // namespace pixel_world
