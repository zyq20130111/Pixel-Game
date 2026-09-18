#include "SniperRifle.h"

#include "Camera.h"
#include "FirstPersonHands.h"
#include "SniperBullet.h"
#include "SoundManager.h"
#include "ThreeDUtils.h"
#include "game_constants.h"

#include <algorithm>
#include <cmath>

namespace pixel_world {
namespace {

using namespace constants;

void drawPart(const Vec3& center, const Vec3& size, const Color& color) {
    ThreeDUtils::drawCube(center, size, color);
}

}  // namespace

SniperRifle::SniperRifle() : muzzleFlashTimer_(0.0f) {}

void SniperRifle::reset() {
    muzzleFlashTimer_ = 0.0f;
}

bool SniperRifle::update(GLFWwindow* window, const Camera& camera,
                         std::vector<std::unique_ptr<BulletBase>>& bullets,
                         SoundManager& soundManager, bool& previousFireDown,
                         float dt) {
    muzzleFlashTimer_ = std::max(0.0f, muzzleFlashTimer_ - dt);

    const bool fireDown =
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    bool fired = false;
    if (fireDown && !previousFireDown) {
        if (bullets.size() >= static_cast<std::size_t>(kMaxBullets)) {
            bullets.erase(bullets.begin());
        }

        const Vec3 muzzlePosition = muzzleWorldPosition(camera);
        const Vec3 target =
            camera.position() + camera.forward() * kSniperBulletAimDistance;
        const Vec3 direction =
            ThreeDUtils::normalize(target - muzzlePosition);
        bullets.push_back(std::make_unique<SniperBullet>(
            muzzlePosition, direction * kSniperBulletSpeed,
            kSniperBulletLifetime));
        muzzleFlashTimer_ = kMuzzleFlashDuration;
        soundManager.play2D("pistol.wav", 1.0f);
        fired = true;
    }
    previousFireDown = fireDown;
    return fired;
}

void SniperRifle::render(const Camera& camera,
                         const WeaponRenderMotion& motion) const {
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    glTranslatef(motion.translation.x, motion.translation.y,
                 motion.translation.z);
    glRotatef(motion.rotationDegrees.z, 0.0f, 0.0f, 1.0f);
    glRotatef(motion.rotationDegrees.y, 0.0f, 1.0f, 0.0f);
    glRotatef(motion.rotationDegrees.x, 1.0f, 0.0f, 0.0f);

    const Vec3 root = weaponRoot(camera, muzzleFlashTimer_);
    glTranslatef(root.x, root.y, root.z);
    glScalef(kPistolScale, kPistolScale, kPistolScale);

    const float handAnimation =
        muzzleFlashTimer_ > 0.0f
            ? muzzleFlashTimer_ / kMuzzleFlashDuration
            : 0.0f;
    FirstPersonHands::drawBack(FirstPersonHandAnimation::PistolFire,
                               handAnimation);

    drawMuzzleFlash(muzzleFlashTimer_);

    // A long receiver, heavy barrel, wooden stock and raised optic make the
    // weapon silhouette distinct from the compact pistol.
    drawPart({0.0f, -0.04f, -0.26f}, {0.50f, 0.28f, 0.78f},
             kSniperMetalDark);
    drawPart({0.0f, 0.06f, -0.30f}, {0.42f, 0.16f, 0.62f},
             kSniperMetal);
    drawPart({0.0f, 0.11f, -0.64f}, {0.30f, 0.09f, 0.22f},
             kSniperHighlight);

    drawPart({0.0f, -0.15f, 0.40f}, {0.38f, 0.34f, 0.78f},
             kSniperStock);
    drawPart({0.0f, -0.23f, 0.72f}, {0.44f, 0.24f, 0.36f},
             kSniperStockDark);
    drawPart({0.0f, -0.02f, 0.06f}, {0.25f, 0.12f, 0.24f},
             kSniperMetal);

    drawPart({0.0f, 0.12f, -1.36f}, {0.15f, 0.15f, 1.60f},
             kSniperBarrel);
    drawPart({0.0f, 0.12f, -2.02f}, {0.22f, 0.22f, 0.18f},
             kSniperBarrelDark);

    drawPart({0.0f, 0.30f, -0.34f}, {0.17f, 0.17f, 0.76f},
             kSniperScope);
    drawPart({0.0f, 0.30f, -0.78f}, {0.25f, 0.23f, 0.10f},
             kSniperScopeMount);
    drawPart({0.0f, 0.30f, 0.10f}, {0.25f, 0.23f, 0.10f},
             kSniperScopeMount);
    drawPart({0.0f, 0.30f, -0.77f}, {0.11f, 0.11f, 0.04f},
             kSniperLens);
    drawScope(camera.aimAmount());

    drawPart({-0.22f, -0.22f, -0.23f}, {0.08f, 0.22f, 0.10f},
             kSniperMetalDark);
    drawPart({0.22f, -0.22f, -0.23f}, {0.08f, 0.22f, 0.10f},
             kSniperMetalDark);
    drawPart({0.0f, -0.32f, -0.20f}, {0.06f, 0.30f, 0.06f},
             kSniperMetalDark);

    FirstPersonHands::drawGrip(FirstPersonHandAnimation::PistolFire,
                               handAnimation);

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
}

float SniperRifle::muzzleFlashTimer() const {
    return muzzleFlashTimer_;
}

float SniperRifle::bob(bool cameraMoving, bool cameraRunning,
                       float cameraWalkPhase) {
    const float runScale = cameraRunning ? 1.55f : 1.0f;
    return cameraMoving
               ? std::sin(cameraWalkPhase * 2.0f) * 0.028f * runScale
               : 0.0f;
}

float SniperRifle::sway(bool cameraMoving, bool cameraRunning,
                        float cameraWalkPhase) {
    const float runScale = cameraRunning ? 1.35f : 1.0f;
    return cameraMoving
               ? std::cos(cameraWalkPhase) * 0.022f * runScale
               : 0.0f;
}

Vec3 SniperRifle::weaponRoot(const Camera& camera, float muzzleFlashTimer) {
    const float recoil =
        muzzleFlashTimer > 0.0f
            ? muzzleFlashTimer / kMuzzleFlashDuration
            : 0.0f;
    const float swayAmount =
        sway(camera.moving(), camera.running(), camera.walkPhase()) *
        (1.0f - 0.78f * camera.aimAmount());
    const float bobAmount =
        bob(camera.moving(), camera.running(), camera.walkPhase()) *
        (1.0f - 0.88f * camera.aimAmount());
    return {
        kSniperWeaponBase.x +
            (kSniperWeaponAimBase.x - kSniperWeaponBase.x) *
                camera.aimAmount() +
            swayAmount,
        kSniperWeaponBase.y +
            (kSniperWeaponAimBase.y - kSniperWeaponBase.y) *
                camera.aimAmount() +
            bobAmount + 0.032f * recoil,
        kSniperWeaponBase.z +
            (kSniperWeaponAimBase.z - kSniperWeaponBase.z) *
                camera.aimAmount() +
            0.10f * recoil,
    };
}

Vec3 SniperRifle::muzzleFrontViewPosition(const Camera& camera,
                                          float muzzleFlashTimer) {
    Vec3 muzzle = weaponRoot(camera, muzzleFlashTimer);
    muzzle = muzzle + kSniperMuzzleLocal * kPistolScale;
    muzzle.z -= kMuzzleFlashForwardOffset * kPistolScale;
    return muzzle;
}

Vec3 SniperRifle::muzzleWorldPosition(const Camera& camera) {
    Vec3 bulletOrigin = muzzleFrontViewPosition(camera, 0.0f);
    bulletOrigin.z = std::max(bulletOrigin.z, -kSniperBulletOriginDepth);
    return camera.toWorld(bulletOrigin);
}

void SniperRifle::drawMuzzleFlash(float muzzleFlashTimer) {
    if (muzzleFlashTimer <= 0.0f) {
        return;
    }

    const float flashScale =
        0.60f + 0.40f * (muzzleFlashTimer / kMuzzleFlashDuration);
    const Vec3 flashCenter{
        kSniperMuzzleLocal.x,
        kSniperMuzzleLocal.y,
        kSniperMuzzleLocal.z - kMuzzleFlashForwardOffset};
    drawPart(flashCenter, {0.34f * flashScale, 0.34f * flashScale,
                           0.56f * flashScale},
             kMuzzleFlashOuter);
    drawPart({flashCenter.x, flashCenter.y, flashCenter.z - 0.20f},
             {0.20f * flashScale, 0.20f * flashScale,
              0.50f * flashScale},
             kMuzzleFlashCore);
}

void SniperRifle::drawScope(float aimAmount) {
    if (aimAmount <= 0.55f) {
        return;
    }

    drawPart({0.0f, 0.30f, -0.80f}, {0.035f, 0.035f, 0.08f},
             kSniperLensGlow);
}

}  // namespace pixel_world
