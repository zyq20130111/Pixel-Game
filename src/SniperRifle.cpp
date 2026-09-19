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

void drawAngledPart(const Vec3& center, const Vec3& size,
                    const Vec3& rotationDegrees, const Color& color) {
    ThreeDUtils::drawViewOrientedCube(center, size, rotationDegrees, color);
}

void drawSupportHand(FirstPersonHandAnimation animation, float progress) {
    glPushMatrix();
    glTranslatef(-0.08f, -0.04f, -0.44f);
    glRotatef(-5.0f, 1.0f, 0.0f, 0.0f);
    glRotatef(4.0f, 0.0f, 1.0f, 0.0f);
    FirstPersonHands::drawBack(animation, progress);
    glPopMatrix();
}

void drawStock() {
    drawAngledPart({0.0f, -0.18f, 0.62f}, {0.48f, 0.32f, 0.62f},
                   {-4.0f, 0.0f, 0.0f}, kSniperStock);
    drawPart({0.0f, -0.23f, 0.94f}, {0.54f, 0.40f, 0.16f},
             kSniperStockDark);
    drawPart({0.0f, 0.03f, 0.42f}, {0.38f, 0.12f, 0.42f},
             ThreeDUtils::shade(kSniperStock, 1.12f));
    drawPart({0.0f, -0.33f, 0.38f}, {0.28f, 0.12f, 0.42f},
             kSniperStockDark);

    drawAngledPart({0.0f, -0.43f, 0.05f}, {0.26f, 0.62f, 0.24f},
                   {-12.0f, 0.0f, 0.0f}, kSniperStockDark);
    drawAngledPart({0.0f, -0.39f, 0.00f}, {0.16f, 0.36f, 0.12f},
                   {-12.0f, 0.0f, 0.0f}, kSniperStock);
}

void drawReceiverAndMagazine() {
    drawPart({0.0f, -0.04f, -0.24f}, {0.54f, 0.30f, 0.74f},
             kSniperMetalDark);
    drawPart({0.0f, 0.07f, -0.35f}, {0.46f, 0.16f, 0.70f},
             kSniperMetal);
    drawPart({0.0f, 0.16f, -0.57f}, {0.32f, 0.07f, 0.26f},
             kSniperHighlight);
    drawPart({0.0f, 0.15f, -0.12f}, {0.36f, 0.06f, 0.28f},
             ThreeDUtils::shade(kSniperHighlight, 0.86f));

    drawPart({0.28f, 0.04f, -0.36f}, {0.07f, 0.10f, 0.22f},
             kSniperBarrelDark);
    drawPart({-0.28f, 0.04f, -0.36f}, {0.07f, 0.10f, 0.22f},
             kSniperBarrelDark);
    drawAngledPart({0.34f, -0.04f, -0.14f}, {0.07f, 0.26f, 0.07f},
                   {0.0f, 0.0f, -18.0f}, kSniperMetalDark);
    drawPart({0.39f, -0.18f, -0.11f}, {0.14f, 0.10f, 0.12f},
             kSniperMetal);

    drawAngledPart({0.0f, -0.34f, -0.25f}, {0.30f, 0.42f, 0.26f},
                   {-4.0f, 0.0f, 0.0f}, kSniperMetalDark);
    drawPart({0.0f, -0.53f, -0.22f}, {0.24f, 0.08f, 0.22f},
             ThreeDUtils::shade(kSniperMetalDark, 0.72f));

    drawPart({0.0f, -0.20f, -0.18f}, {0.26f, 0.08f, 0.10f},
             kSniperBarrelDark);
    drawAngledPart({0.0f, -0.31f, -0.13f}, {0.06f, 0.30f, 0.06f},
                   {0.0f, 0.0f, -10.0f}, kSniperMetalDark);
}

void drawForeEndAndBarrel() {
    drawPart({0.0f, -0.02f, -0.88f}, {0.40f, 0.22f, 0.76f},
             ThreeDUtils::shade(kSniperStock, 0.82f));
    drawPart({0.0f, 0.08f, -0.90f}, {0.34f, 0.10f, 0.82f},
             kSniperMetalDark);
    drawPart({0.0f, 0.14f, -0.92f}, {0.28f, 0.06f, 0.74f},
             kSniperHighlight);

    for (const float z : {-1.18f, -0.96f, -0.74f}) {
        drawPart({-0.23f, -0.03f, z}, {0.06f, 0.09f, 0.13f},
                 kSniperBarrelDark);
        drawPart({0.23f, -0.03f, z}, {0.06f, 0.09f, 0.13f},
                 kSniperBarrelDark);
    }

    drawPart({0.0f, 0.12f, -1.47f}, {0.13f, 0.13f, 1.28f},
             kSniperBarrel);
    drawPart({0.0f, 0.12f, -1.06f}, {0.20f, 0.20f, 0.10f},
             kSniperBarrelDark);
    drawPart({0.0f, 0.12f, -1.52f}, {0.18f, 0.18f, 0.10f},
             kSniperBarrelDark);

    drawPart({0.0f, 0.12f, -2.08f}, {0.28f, 0.24f, 0.22f},
             kSniperBarrelDark);
    drawPart({-0.17f, 0.12f, -2.08f}, {0.07f, 0.11f, 0.13f},
             kSniperHighlight);
    drawPart({0.17f, 0.12f, -2.08f}, {0.07f, 0.11f, 0.13f},
             kSniperHighlight);

    drawAngledPart({-0.17f, -0.31f, -1.08f}, {0.05f, 0.48f, 0.05f},
                   {0.0f, 0.0f, -12.0f}, kSniperMetalDark);
    drawAngledPart({0.17f, -0.31f, -1.08f}, {0.05f, 0.48f, 0.05f},
                   {0.0f, 0.0f, 12.0f}, kSniperMetalDark);
    drawPart({0.0f, -0.51f, -1.08f}, {0.54f, 0.05f, 0.10f},
             kSniperMetalDark);
}

void drawOptic() {
    drawPart({0.0f, 0.24f, -0.34f}, {0.48f, 0.08f, 0.82f},
             kSniperScopeMount);
    drawPart({0.0f, 0.31f, -0.70f}, {0.24f, 0.18f, 0.10f},
             kSniperScopeMount);
    drawPart({0.0f, 0.31f, 0.06f}, {0.24f, 0.18f, 0.10f},
             kSniperScopeMount);

    drawPart({0.0f, 0.42f, -0.34f}, {0.22f, 0.22f, 0.86f},
             kSniperScope);
    drawPart({0.0f, 0.42f, -0.86f}, {0.34f, 0.30f, 0.22f},
             kSniperScope);
    drawPart({0.0f, 0.42f, 0.22f}, {0.32f, 0.28f, 0.20f},
             kSniperScope);
    drawPart({0.0f, 0.42f, -0.98f}, {0.24f, 0.22f, 0.05f},
             kSniperLens);
    drawPart({0.0f, 0.42f, 0.32f}, {0.20f, 0.18f, 0.04f},
             ThreeDUtils::shade(kSniperLens, 0.72f));

    drawPart({0.0f, 0.61f, -0.32f}, {0.16f, 0.16f, 0.14f},
             kSniperScopeMount);
    drawPart({0.19f, 0.42f, -0.31f}, {0.10f, 0.16f, 0.13f},
             kSniperScopeMount);
}

}  // namespace

SniperRifle::SniperRifle() : muzzleFlashTimer_(0.0f) {}

void SniperRifle::reset() {
    muzzleFlashTimer_ = 0.0f;
}

void SniperRifle::drawRifleModel() {
    drawStock();
    drawReceiverAndMagazine();
    drawForeEndAndBarrel();
    drawOptic();
}

void SniperRifle::updateThirdPerson(float dt, bool attackActive,
                                    float attackTimer) {
    (void)attackActive;
    (void)attackTimer;
    muzzleFlashTimer_ = std::max(0.0f, muzzleFlashTimer_ - dt);
}

void SniperRifle::renderThirdPerson(const GLfloat* weaponMatrix,
                                    bool showMuzzleFlash) const {
    glPushMatrix();
    glMultMatrixf(weaponMatrix);
    glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
    glScalef(kPistolScale, kPistolScale, kPistolScale);
    drawRifleModel();
    glPopMatrix();

    if (!showMuzzleFlash) {
        return;
    }

    glPushMatrix();
    glMultMatrixf(weaponMatrix);
    ThreeDUtils::drawCube({0.0f, 0.08f, 1.53f},
                          {0.23f, 0.23f, 0.37f},
                          kMuzzleFlashOuter);
    ThreeDUtils::drawCube({0.0f, 0.08f, 1.67f},
                          {0.13f, 0.13f, 0.33f},
                          kMuzzleFlashCore);
    glPopMatrix();
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
    drawSupportHand(FirstPersonHandAnimation::PistolFire, handAnimation);

    drawMuzzleFlash(muzzleFlashTimer_);

    drawRifleModel();
    drawScope(camera.aimAmount());

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

    const float glow =
        ThreeDUtils::smoothStep((aimAmount - 0.55f) / 0.45f);
    drawPart({0.0f, 0.42f, -1.01f},
             {0.07f + 0.04f * glow, 0.07f + 0.04f * glow, 0.05f},
             kSniperLensGlow);
    drawPart({0.0f, 0.42f, -1.04f}, {0.018f, 0.20f, 0.025f},
             kCrosshairCore);
    drawPart({0.0f, 0.42f, -1.04f}, {0.20f, 0.018f, 0.025f},
             kCrosshairCore);
}

}  // namespace pixel_world
