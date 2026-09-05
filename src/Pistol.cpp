#include "Pistol.h"

#include "PistolBullet.h"
#include "Camera.h"
#include "game_constants.h"
#include "ThreeDUtils.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace pixel_world {

Pistol::Pistol() : muzzleFlashTimer_(0.0f) {}

void Pistol::reset() {
    muzzleFlashTimer_ = 0.0f;
}

void Pistol::update(GLFWwindow* window, const Camera& camera,
                    std::vector<std::unique_ptr<BulletBase>>& bullets,
                    bool& previousFireDown, float dt) {
    muzzleFlashTimer_ = std::max(0.0f, muzzleFlashTimer_ - dt);

    const bool fireDown =
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    if (fireDown && !previousFireDown) {
        if (bullets.size() >= static_cast<std::size_t>(constants::kMaxBullets)) {
            bullets.erase(bullets.begin());
        }

        const Vec3 muzzlePosition = muzzleWorldPosition(camera);
        const Vec3 target =
            camera.position() +
            ThreeDUtils::cameraForward() * constants::kBulletAimDistance;
        const Vec3 direction = ThreeDUtils::normalize(target - muzzlePosition);
        bullets.push_back(std::make_unique<PistolBullet>(
            muzzlePosition, direction * constants::kBulletSpeed,
            constants::kBulletLifetime));
        muzzleFlashTimer_ = constants::kMuzzleFlashDuration;
    }
    previousFireDown = fireDown;
}

void Pistol::render(const Camera& camera) const {
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    const Vec3 root = weaponRoot(camera, muzzleFlashTimer_);
    glTranslatef(root.x, root.y, root.z);
    glScalef(constants::kPistolScale, constants::kPistolScale,
             constants::kPistolScale);

    drawFirstPersonHandBack();

    ThreeDUtils::drawCube({0.0f, -0.02f, 0.0f}, {0.50f, 0.32f, 0.72f},
                          constants::kPistolMetalDark);
    ThreeDUtils::drawCube({0.0f, 0.09f, -0.30f}, {0.42f, 0.22f, 0.76f},
                          constants::kPistolMetal);
    ThreeDUtils::drawCube({0.0f, 0.20f, -0.31f}, {0.28f, 0.07f, 0.62f},
                          constants::kPistolHighlight);

    ThreeDUtils::drawCube({0.0f, -0.01f, -0.77f}, {0.25f, 0.18f, 0.28f},
                          constants::kPistolMetalDark);
    ThreeDUtils::drawCube({0.0f, 0.12f, -0.78f}, {0.14f, 0.10f, 0.08f},
                          constants::kPistolMetal);

    ThreeDUtils::drawViewPivotedCube(
        {0.0f, -0.18f, 0.10f}, {0.0f, -0.38f, 0.05f},
        {0.34f, 0.78f, 0.42f}, -12.0f, constants::kPistolGrip);
    ThreeDUtils::drawViewPivotedCube(
        {0.0f, -0.18f, 0.10f}, {0.0f, -0.40f, 0.22f},
        {0.22f, 0.62f, 0.12f}, -12.0f, constants::kPistolGripDark);

    ThreeDUtils::drawCube({-0.15f, -0.13f, -0.25f}, {0.08f, 0.10f, 0.10f},
                          constants::kPistolMetalDark);
    ThreeDUtils::drawCube({0.15f, -0.13f, -0.25f}, {0.08f, 0.10f, 0.10f},
                          constants::kPistolMetalDark);
    ThreeDUtils::drawCube({0.0f, -0.16f, -0.12f}, {0.10f, 0.18f, 0.10f},
                          constants::kPistolMetalDark);

    drawIronSights(camera.aimAmount());
    drawFirstPersonHandGrip();
    drawMuzzleFlash(muzzleFlashTimer_);

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
}

float Pistol::muzzleFlashTimer() const {
    return muzzleFlashTimer_;
}

float Pistol::bob(bool cameraMoving, bool cameraRunning,
                  float cameraWalkPhase) {
    const float runScale = cameraRunning ? 1.75f : 1.0f;
    return cameraMoving
               ? std::sin(cameraWalkPhase * 2.0f) * 0.035f * runScale
               : 0.0f;
}

float Pistol::sway(bool cameraMoving, bool cameraRunning,
                   float cameraWalkPhase) {
    const float runScale = cameraRunning ? 1.45f : 1.0f;
    return cameraMoving
               ? std::cos(cameraWalkPhase) * 0.025f * runScale
               : 0.0f;
}

Vec3 Pistol::weaponRoot(const Camera& camera, float muzzleFlashTimer) {
    const float recoil =
        muzzleFlashTimer > 0.0f
            ? muzzleFlashTimer / constants::kMuzzleFlashDuration
            : 0.0f;
    const float swayAmount =
        sway(camera.moving(), camera.running(), camera.walkPhase()) *
        (1.0f - 0.7f * camera.aimAmount());
    const float bobAmount =
        bob(camera.moving(), camera.running(), camera.walkPhase()) *
        (1.0f - 0.85f * camera.aimAmount());
    return {
        constants::kPistolWeaponBase.x +
            (constants::kPistolWeaponAimBase.x -
             constants::kPistolWeaponBase.x) *
                camera.aimAmount() +
            swayAmount,
        constants::kPistolWeaponBase.y +
            (constants::kPistolWeaponAimBase.y -
             constants::kPistolWeaponBase.y) *
                camera.aimAmount() +
            bobAmount + 0.025f * recoil,
        constants::kPistolWeaponBase.z +
            (constants::kPistolWeaponAimBase.z -
             constants::kPistolWeaponBase.z) *
                camera.aimAmount() +
            0.08f * recoil,
    };
}

Vec3 Pistol::muzzleFrontViewPosition(const Camera& camera,
                                     float muzzleFlashTimer) {
    Vec3 muzzle = weaponRoot(camera, muzzleFlashTimer);
    muzzle = muzzle + constants::kPistolMuzzleLocal * constants::kPistolScale;
    muzzle.z -= constants::kMuzzleFlashForwardOffset * constants::kPistolScale;
    return muzzle;
}

Vec3 Pistol::muzzleWorldPosition(const Camera& camera) {
    return camera.toWorld(muzzleFrontViewPosition(camera, 0.0f));
}

void Pistol::drawFirstPersonHandBack() {
    ThreeDUtils::drawViewOrientedCube(
        {0.48f, -1.08f, 0.70f}, {0.42f, 0.92f, 0.38f},
        {-24.0f, 0.0f, -10.0f}, constants::kPistolSleeveDark);
    ThreeDUtils::drawViewOrientedCube(
        {0.32f, -0.82f, 0.55f}, {0.48f, 0.38f, 0.42f},
        {-18.0f, 0.0f, -8.0f}, constants::kPistolSleeve);
    ThreeDUtils::drawViewOrientedCube(
        {0.16f, -0.66f, 0.41f}, {0.38f, 0.24f, 0.34f},
        {-12.0f, 0.0f, -6.0f}, constants::kPistolHandShadow);
    ThreeDUtils::drawViewOrientedCube(
        {0.02f, -0.56f, 0.35f}, {0.54f, 0.42f, 0.42f},
        {-8.0f, 0.0f, 0.0f}, constants::kPistolHand);
    ThreeDUtils::drawCube({-0.03f, -0.40f, 0.54f}, {0.36f, 0.10f, 0.10f},
                          constants::kPistolHandLight);
    ThreeDUtils::drawCube({0.23f, -0.66f, 0.48f}, {0.20f, 0.18f, 0.16f},
                          constants::kPistolHandShadow);
}

void Pistol::drawFirstPersonHandGrip() {
    const std::array<float, 4> fingerX{-0.20f, -0.06f, 0.08f, 0.21f};
    const std::array<float, 4> fingerLength{0.43f, 0.50f, 0.48f, 0.38f};

    ThreeDUtils::drawViewOrientedCube(
        {-0.31f, -0.49f, 0.31f}, {0.18f, 0.30f, 0.46f},
        {-16.0f, 0.0f, 18.0f}, constants::kPistolHand);
    ThreeDUtils::drawViewOrientedCube(
        {-0.23f, -0.68f, 0.43f}, {0.16f, 0.18f, 0.20f},
        {-8.0f, 0.0f, 12.0f}, constants::kPistolHandShadow);

    for (std::size_t i = 0; i < fingerX.size(); ++i) {
        const Color fingerColor =
            i == fingerX.size() - 1 ? constants::kPistolHandShadow
                                    : constants::kPistolHand;
        ThreeDUtils::drawViewPivotedCube(
            {fingerX[i], -0.30f, 0.47f},
            {0.0f, -fingerLength[i] * 0.5f, 0.02f},
            {0.11f, fingerLength[i], 0.14f}, -24.0f, fingerColor);
        ThreeDUtils::drawCube({fingerX[i], -0.29f, 0.55f},
                              {0.12f, 0.10f, 0.08f},
                              constants::kPistolHandLight);
    }

    ThreeDUtils::drawViewOrientedCube(
        {-0.21f, -0.23f, 0.02f}, {0.12f, 0.32f, 0.12f},
        {18.0f, 0.0f, 8.0f}, constants::kPistolHand);
}

void Pistol::drawMuzzleFlash(float muzzleFlashTimer) {
    if (muzzleFlashTimer <= 0.0f) {
        return;
    }

    const float flashScale =
        0.55f + 0.45f * (muzzleFlashTimer / constants::kMuzzleFlashDuration);
    const Vec3 flashCenter{
        constants::kPistolMuzzleLocal.x,
        constants::kPistolMuzzleLocal.y,
        constants::kPistolMuzzleLocal.z -
            constants::kMuzzleFlashForwardOffset};

    ThreeDUtils::drawCube(flashCenter,
                          {0.44f * flashScale, 0.18f * flashScale, 0.10f},
                          constants::kMuzzleFlashOuter);
    ThreeDUtils::drawCube(flashCenter,
                          {0.18f * flashScale, 0.44f * flashScale, 0.10f},
                          constants::kMuzzleFlashOuter);
    ThreeDUtils::drawCube(
        {flashCenter.x, flashCenter.y, flashCenter.z - 0.05f},
        {0.24f * flashScale, 0.24f * flashScale, 0.12f},
        constants::kMuzzleFlashCore);
    ThreeDUtils::drawCube(
        {flashCenter.x - 0.22f * flashScale, flashCenter.y,
         flashCenter.z + 0.03f},
        {0.10f, 0.10f, 0.08f}, constants::kMuzzleFlashCore);
    ThreeDUtils::drawCube(
        {flashCenter.x + 0.22f * flashScale, flashCenter.y,
         flashCenter.z + 0.03f},
        {0.10f, 0.10f, 0.08f}, constants::kMuzzleFlashOuter);
}

void Pistol::drawIronSights(float aimAmount) {
    const float sightLift = 0.03f * ThreeDUtils::smoothStep(aimAmount);

    ThreeDUtils::drawCube({-0.13f, 0.27f + sightLift, -0.10f},
                          {0.08f, 0.16f, 0.08f},
                          constants::kPistolMetalDark);
    ThreeDUtils::drawCube({0.13f, 0.27f + sightLift, -0.10f},
                          {0.08f, 0.16f, 0.08f},
                          constants::kPistolMetalDark);
    ThreeDUtils::drawCube({0.0f, 0.30f + sightLift, -0.86f},
                          {0.07f, 0.18f, 0.07f},
                          constants::kPistolMetalDark);
    if (aimAmount > 0.45f) {
        ThreeDUtils::drawCube({0.0f, 0.40f + sightLift, -0.87f},
                              {0.04f, 0.04f, 0.04f},
                              constants::kCrosshairCore);
    }
}

}  // namespace pixel_world
