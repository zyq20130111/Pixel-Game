#include "MainScene.h"

#include "game_constants.h"
#include "ThreeDUtils.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

namespace pixel_world {
namespace {

using namespace constants;

}  // namespace

MainScene::MainScene()
    : window_(nullptr),
      glfwInitialized_(false),
      framebufferWidth_(0),
      framebufferHeight_(0),
      state_(AppState::MainMenu),
      camera_(),
      menuCamera_({0.0f, kCameraGroundHeight, 10.0f}),
      character_(),
      pistol_(),
      loginScreen_(),
      bullets_(),
      impactEffects_(),
      previousFireDown_(false),
      previousJumpDown_(false) {}

MainScene::~MainScene() {
    if (window_ != nullptr) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
    if (glfwInitialized_) {
        glfwTerminate();
    }
}

int MainScene::run() {
    if (!initialize()) {
        return EXIT_FAILURE;
    }

    auto last = std::chrono::steady_clock::now();
    while (!glfwWindowShouldClose(window_)) {
        const auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - last).count();
        last = now;
        dt = std::min(dt, kMaxDeltaTime);

        glfwPollEvents();
        glfwGetFramebufferSize(window_, &framebufferWidth_, &framebufferHeight_);

        if (state_ == AppState::MainMenu) {
            const MenuAction action =
                loginScreen_.update(window_, framebufferWidth_, framebufferHeight_);
            if (action == MenuAction::Login) {
                state_ = AppState::Playing;
                resetGame();
                previousFireDown_ =
                    glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_LEFT) ==
                    GLFW_PRESS;
                previousJumpDown_ =
                    glfwGetKey(window_, GLFW_KEY_SPACE) == GLFW_PRESS;
            } else if (action == MenuAction::Exit) {
                glfwSetWindowShouldClose(window_, GLFW_TRUE);
            }

            ThreeDUtils::setProjection(framebufferWidth_, framebufferHeight_);
            menuCamera_.apply();
            renderFrame(menuCamera_, false);
            loginScreen_.render(framebufferWidth_, framebufferHeight_);
        } else {
            updateGameplay(dt);
            ThreeDUtils::setProjection(
                framebufferWidth_, framebufferHeight_,
                ThreeDUtils::currentFieldOfView(camera_.aimAmount()));
            camera_.apply();
            renderFrame(camera_, true);
        }

        updateWindowTitle(window_, state_);
        glfwSwapBuffers(window_);
    }

    return EXIT_SUCCESS;
}

bool MainScene::initialize() {
    if (glfwInit() != GLFW_TRUE) {
        std::cerr << "Failed to initialize GLFW.\n";
        return false;
    }
    glfwInitialized_ = true;

    window_ = glfwCreateWindow(kWindowWidth, kWindowHeight, "Pixel World 3D",
                               nullptr, nullptr);
    if (window_ == nullptr) {
        std::cerr << "Failed to create GLFW window.\n";
        return false;
    }

    glfwMakeContextCurrent(window_);
    glfwSetFramebufferSizeCallback(window_, ThreeDUtils::framebufferSizeCallback);
    glfwSwapInterval(1);
    ThreeDUtils::configureRendering();

    glfwGetFramebufferSize(window_, &framebufferWidth_, &framebufferHeight_);
    ThreeDUtils::framebufferSizeCallback(
        window_, framebufferWidth_, framebufferHeight_);
    updateWindowTitle(window_, state_);
    return true;
}

void MainScene::resetGame() {
    camera_.reset();
    character_.reset();
    pistol_.reset();
    bullets_.clear();
    impactEffects_.clear();
}

void MainScene::updateGameplay(float dt) {
    camera_.update(window_, dt, previousJumpDown_);
    camera_.updateAim(window_, dt);
    character_.update(window_, dt);
    pistol_.update(window_, camera_, bullets_, previousFireDown_, dt);
    updateImpactEffects(dt);
    updateBullets(dt);
}

void MainScene::updateBullets(float dt) {
    for (const std::unique_ptr<BulletBase>& bullet : bullets_) {
        const Vec3 previousPosition = bullet->position();
        const Vec3 nextPosition = previousPosition + bullet->velocity() * dt;
        bullet->update(dt);

        bool hit = false;
        ImpactType hitType = ImpactType::Ground;
        float closestHitT = 2.0f;

        float characterHitT = 0.0f;
        if (character_.segmentHit(previousPosition, nextPosition,
                                  characterHitT) &&
            characterHitT < closestHitT) {
            hit = true;
            hitType = ImpactType::Character;
            closestHitT = characterHitT;
        }

        float groundHitT = 0.0f;
        if (segmentHitsGround(previousPosition, nextPosition, groundHitT) &&
            groundHitT < closestHitT) {
            hit = true;
            hitType = ImpactType::Ground;
            closestHitT = groundHitT;
        }

        if (hit) {
            const Vec3 hitPosition =
                pointOnSegment(previousPosition, nextPosition, closestHitT);
            bullet->setPosition(hitPosition);
            bullet->expire();
            spawnImpactEffect(hitPosition, hitType);
            if (hitType == ImpactType::Character) {
                character_.applyPistolDamage(hitPosition);
            }
        }
    }

    bullets_.erase(
        std::remove_if(
            bullets_.begin(), bullets_.end(),
            [](const std::unique_ptr<BulletBase>& bullet) {
                const Vec3& position = bullet->position();
                return !bullet->active() || position.y <= kBulletGroundHitHeight ||
                       std::abs(position.x) > kFarPlane ||
                       std::abs(position.z) > kFarPlane;
            }),
        bullets_.end());
}

void MainScene::updateImpactEffects(float dt) {
    for (ImpactEffect& effect : impactEffects_) {
        effect.age += dt;
    }

    impactEffects_.erase(
        std::remove_if(
            impactEffects_.begin(), impactEffects_.end(),
            [](const ImpactEffect& effect) {
                return effect.age >= effect.duration;
            }),
        impactEffects_.end());
}

void MainScene::renderFrame(const Camera& camera, bool showPistol) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    drawSkybox(camera.position());
    drawSun(camera.position());
    renderScene();
    renderImpactEffects();

    for (const std::unique_ptr<BulletBase>& bullet : bullets_) {
        bullet->render();
    }

    if (showPistol) {
        pistol_.render(camera);
        renderCrosshair();
    }
}

void MainScene::renderScene() const {
    drawGround();
    drawHouse({-5.0f, 0.0f, -3.0f});
    drawHouse({4.8f, 0.0f, -6.0f});

    drawTree({-8.5f, 0.0f, -4.5f});
    drawTree({-3.5f, 0.0f, -8.0f});
    drawTree({2.0f, 0.0f, -4.0f});
    drawTree({6.5f, 0.0f, -1.5f});
    drawTree({-7.0f, 0.0f, 4.0f});

    drawRock({-9.5f, 0.0f, 2.5f}, 1.0f);
    drawRock({-1.0f, 0.0f, 6.0f}, 1.2f);
    drawRock({8.0f, 0.0f, 5.0f}, 0.9f);

    character_.render();
    character_.renderHealthBar();

    for (int i = -2; i <= 2; ++i) {
        ThreeDUtils::drawCube(
            {static_cast<float>(i) * 1.2f,
             0.18f + 0.2f * std::abs(i), 8.5f + i * 0.35f},
            {0.7f, 0.6f, 0.7f},
            ThreeDUtils::shade(kStoneColor, 0.85f + 0.03f * i));
    }
}

void MainScene::renderImpactEffects() const {
    for (const ImpactEffect& effect : impactEffects_) {
        if (effect.type == ImpactType::Ground) {
            renderGroundImpactEffect(effect);
        } else {
            renderCharacterImpactEffect(effect);
        }
    }
}

void MainScene::renderCrosshair() const {
    if (framebufferWidth_ <= 0 || framebufferHeight_ <= 0) {
        return;
    }

    const float easedAim = ThreeDUtils::smoothStep(camera_.aimAmount());
    const float uiScale =
        std::min(static_cast<float>(framebufferWidth_) /
                     static_cast<float>(kWindowWidth),
                 static_cast<float>(framebufferHeight_) /
                     static_cast<float>(kWindowHeight));
    const float pixel = std::max(2.0f, std::round(3.0f * uiScale));
    const float thickness = pixel;
    const float armLength = pixel * (5.0f - 1.5f * easedAim);
    const float gap = pixel * (2.0f - 1.35f * easedAim);
    const float centerX = static_cast<float>(framebufferWidth_) * 0.5f;
    const float centerY = static_cast<float>(framebufferHeight_) * 0.5f;
    const float halfThickness = thickness * 0.5f;

    ThreeDUtils::setUiProjection(framebufferWidth_, framebufferHeight_);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const Rect left{centerX - gap - armLength, centerY - halfThickness,
                    armLength, thickness};
    const Rect right{centerX + gap, centerY - halfThickness, armLength,
                     thickness};
    const Rect top{centerX - halfThickness, centerY - gap - armLength,
                   thickness, armLength};
    const Rect bottom{centerX - halfThickness, centerY + gap, thickness,
                      armLength};
    const Rect dot{centerX - halfThickness, centerY - halfThickness, thickness,
                   thickness};
    const std::array<Rect, 5> pieces{left, right, top, bottom, dot};
    for (const Rect& piece : pieces) {
        ThreeDUtils::drawRect2D(
            {piece.x - pixel, piece.y - pixel, piece.width + pixel * 2.0f,
             piece.height + pixel * 2.0f},
            kCrosshairShadow, 0.55f);
    }
    for (const Rect& piece : pieces) {
        ThreeDUtils::drawRect2D(piece, kCrosshairCore, 0.92f);
    }

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    ThreeDUtils::setProjection(
        framebufferWidth_, framebufferHeight_,
        ThreeDUtils::currentFieldOfView(camera_.aimAmount()));
}

void MainScene::drawSkybox(const Vec3& cameraPosition) const {
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);

    const float halfSize = 90.0f;
    const Vec3 center = cameraPosition;
    const Vec3 p000{center.x - halfSize, center.y - halfSize,
                    center.z - halfSize};
    const Vec3 p001{center.x - halfSize, center.y - halfSize,
                    center.z + halfSize};
    const Vec3 p010{center.x - halfSize, center.y + halfSize,
                    center.z - halfSize};
    const Vec3 p011{center.x - halfSize, center.y + halfSize,
                    center.z + halfSize};
    const Vec3 p100{center.x + halfSize, center.y - halfSize,
                    center.z - halfSize};
    const Vec3 p101{center.x + halfSize, center.y - halfSize,
                    center.z + halfSize};
    const Vec3 p110{center.x + halfSize, center.y + halfSize,
                    center.z - halfSize};
    const Vec3 p111{center.x + halfSize, center.y + halfSize,
                    center.z + halfSize};

    const Vec3 leftUpperMid = ThreeDUtils::lerp(p010, p000, 0.33f);
    const Vec3 leftLowerMid = ThreeDUtils::lerp(p010, p000, 0.68f);
    const Vec3 rightUpperMid = ThreeDUtils::lerp(p110, p100, 0.33f);
    const Vec3 rightLowerMid = ThreeDUtils::lerp(p110, p100, 0.68f);
    const Vec3 nearUpperMid = ThreeDUtils::lerp(p011, p001, 0.33f);
    const Vec3 nearLowerMid = ThreeDUtils::lerp(p011, p001, 0.68f);
    const Vec3 farUpperMid = ThreeDUtils::lerp(p111, p101, 0.33f);
    const Vec3 farLowerMid = ThreeDUtils::lerp(p111, p101, 0.68f);

    ThreeDUtils::drawFace(p010, p110, rightUpperMid, leftUpperMid, kSkyTop);
    ThreeDUtils::drawFace(leftUpperMid, rightUpperMid, rightLowerMid,
                          leftLowerMid, kSkyUpper);
    ThreeDUtils::drawFace(leftLowerMid, rightLowerMid, p100, p000,
                          kSkyMiddle);

    ThreeDUtils::drawFace(p011, p111, farUpperMid, nearUpperMid, kSkyTop);
    ThreeDUtils::drawFace(nearUpperMid, farUpperMid, farLowerMid, nearLowerMid,
                          kSkyUpper);
    ThreeDUtils::drawFace(nearLowerMid, farLowerMid, p101, p001,
                          kSkyHorizon);

    ThreeDUtils::drawFace(p010, p011, nearUpperMid, leftUpperMid, kSkyTop);
    ThreeDUtils::drawFace(leftUpperMid, nearUpperMid, nearLowerMid,
                          leftLowerMid, kSkyMiddle);
    ThreeDUtils::drawFace(leftLowerMid, nearLowerMid, p001, p000,
                          kSkyHorizon);

    ThreeDUtils::drawFace(p110, p111, farUpperMid, rightUpperMid, kSkyTop);
    ThreeDUtils::drawFace(rightUpperMid, farUpperMid, farLowerMid,
                          rightLowerMid, kSkyUpper);
    ThreeDUtils::drawFace(rightLowerMid, farLowerMid, p101, p100,
                          kSkyHorizon);

    ThreeDUtils::drawFace(p010, p011, p111, p110, kSkyTop);
    ThreeDUtils::drawFace(p000, p100, p101, p001, kSkyBottom);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}

void MainScene::drawSun(const Vec3& cameraPosition) const {
    const Vec3 sunCenter{cameraPosition.x - 14.0f,
                         cameraPosition.y + 12.0f,
                         cameraPosition.z - 30.0f};

    ThreeDUtils::drawCube({sunCenter.x + 0.2f, sunCenter.y + 0.2f,
                           sunCenter.z},
                          {3.2f, 3.2f, 0.6f}, kSunRay);
    ThreeDUtils::drawCube({sunCenter.x, sunCenter.y, sunCenter.z},
                          {2.5f, 2.5f, 0.6f}, kSunOuter);
    ThreeDUtils::drawCube({sunCenter.x - 0.15f, sunCenter.y - 0.15f,
                           sunCenter.z},
                          {1.6f, 1.6f, 0.6f}, kSunCore);
    ThreeDUtils::drawCube({sunCenter.x - 0.30f, sunCenter.y - 0.30f,
                           sunCenter.z},
                          {0.8f, 0.8f, 0.6f}, kSunHighlight);
}

void MainScene::drawGround() const {
    for (int z = -12; z <= 12; ++z) {
        for (int x = -12; x <= 12; ++x) {
            Color color = ((x + z) & 1) == 0 ? kGrassA : kGrassB;
            if (std::abs(x) <= 1 && z < 10) {
                color = ((x + z) & 1) == 0 ? kPathColor : kPathDark;
            }
            if (z == 11 || z == 12) {
                color = ((x + z) & 1) == 0
                            ? kWaterColor
                            : ThreeDUtils::shade(kWaterColor, 0.8f);
            }
            ThreeDUtils::drawCube({static_cast<float>(x), -0.09f,
                                   static_cast<float>(z)},
                                  {0.98f, 0.18f, 0.98f}, color);
        }
    }
}

void MainScene::drawTree(const Vec3& base) const {
    ThreeDUtils::drawCube({base.x, base.y + 0.55f, base.z},
                          {0.5f, 1.2f, 0.5f}, kTrunkColor);
    ThreeDUtils::drawCube({base.x, base.y + 1.45f, base.z},
                          {1.7f, 1.0f, 1.7f}, kLeafDarkColor);
    ThreeDUtils::drawCube({base.x, base.y + 2.15f, base.z},
                          {1.3f, 0.9f, 1.3f}, kLeafColor);
    ThreeDUtils::drawCube({base.x, base.y + 2.80f, base.z},
                          {0.9f, 0.8f, 0.9f}, kLeafDarkColor);
}

void MainScene::drawRock(const Vec3& base, float scale) const {
    ThreeDUtils::drawCube(
        {base.x, base.y + 0.22f * scale, base.z},
        {0.7f * scale, 0.45f * scale, 0.8f * scale}, kStoneColor);
    ThreeDUtils::drawCube(
        {base.x + 0.12f * scale, base.y + 0.40f * scale,
         base.z - 0.08f * scale},
        {0.45f * scale, 0.25f * scale, 0.35f * scale}, kStoneDark);
}

void MainScene::drawHouse(const Vec3& base) const {
    ThreeDUtils::drawCube({base.x, base.y + 0.85f, base.z},
                          {2.4f, 1.7f, 2.0f}, kHouseWall);
    ThreeDUtils::drawCube({base.x, base.y + 2.05f, base.z},
                          {2.8f, 0.7f, 2.4f}, kHouseRoof);
    ThreeDUtils::drawCube({base.x, base.y + 2.35f, base.z},
                          {1.2f, 0.5f, 1.2f}, kHouseRoofDark);
    ThreeDUtils::drawCube({base.x - 0.55f, base.y + 0.75f,
                           base.z + 1.01f},
                          {0.28f, 0.45f, 0.08f}, kPathColor);
}

bool MainScene::segmentHitsGround(const Vec3& start, const Vec3& end,
                                  float& hitT) {
    const float deltaY = end.y - start.y;
    if (std::abs(deltaY) <= 0.0001f) {
        return false;
    }

    const float t = (kBulletGroundHitHeight - start.y) / deltaY;
    if (t < 0.0f || t > 1.0f || end.y > kBulletGroundHitHeight) {
        return false;
    }

    hitT = t;
    return true;
}

Vec3 MainScene::pointOnSegment(const Vec3& start, const Vec3& end, float t) {
    return start + (end - start) * t;
}

void MainScene::spawnImpactEffect(const Vec3& position, ImpactType type) {
    if (impactEffects_.size() >=
        static_cast<std::size_t>(kMaxImpactEffects)) {
        impactEffects_.erase(impactEffects_.begin());
    }

    const float duration =
        type == ImpactType::Ground ? kGroundImpactDuration
                                   : kCharacterImpactDuration;
    impactEffects_.push_back({position, type, 0.0f, duration});
}

void MainScene::renderGroundImpactEffect(const ImpactEffect& effect) const {
    const float t =
        std::clamp(effect.age / effect.duration, 0.0f, 1.0f);
    const float life = 1.0f - t;
    const float spread = 0.18f + 0.55f * t;
    const Vec3 base{effect.position.x, kBulletGroundHitHeight + 0.02f,
                    effect.position.z};

    ThreeDUtils::drawCube(base, {0.46f * life, 0.04f, 0.12f},
                          kImpactDustDark);
    ThreeDUtils::drawCube(base, {0.12f, 0.04f, 0.46f * life},
                          kImpactDustDark);

    const std::array<Vec3, 5> particles{
        Vec3{0.65f, 0.42f, 0.02f}, Vec3{-0.55f, 0.30f, 0.32f},
        Vec3{0.18f, 0.52f, -0.70f}, Vec3{-0.28f, 0.24f, -0.52f},
        Vec3{0.48f, 0.18f, 0.46f},
    };
    for (std::size_t i = 0; i < particles.size(); ++i) {
        const Vec3& particle = particles[i];
        const float size = (i % 2 == 0 ? 0.13f : 0.10f) * life;
        const Color color = i == 0 ? kImpactSpark : kImpactDust;
        ThreeDUtils::drawCube(
            {base.x + particle.x * spread,
             base.y + 0.04f + particle.y * t,
             base.z + particle.z * spread},
            {size, size, size}, color);
    }
}

void MainScene::renderCharacterImpactEffect(
    const ImpactEffect& effect) const {
    const float t =
        std::clamp(effect.age / effect.duration, 0.0f, 1.0f);
    const float life = 1.0f - t;
    const float spread = 0.08f + 0.34f * t;
    const Vec3& base = effect.position;

    ThreeDUtils::drawCube(base, {0.22f * life, 0.22f * life, 0.08f},
                          kImpactSpark);

    const std::array<Vec3, 5> particles{
        Vec3{-0.60f, 0.15f, 0.06f}, Vec3{0.46f, 0.22f, -0.04f},
        Vec3{-0.18f, 0.58f, 0.03f}, Vec3{0.18f, -0.36f, 0.02f},
        Vec3{0.55f, -0.08f, -0.05f},
    };
    for (std::size_t i = 0; i < particles.size(); ++i) {
        const Vec3& particle = particles[i];
        const float size = (i % 2 == 0 ? 0.12f : 0.09f) * life;
        const Color color =
            i % 2 == 0 ? kCharacterHitSpark : kImpactSpark;
        ThreeDUtils::drawCube(
            {base.x + particle.x * spread,
             base.y + particle.y * spread,
             base.z + particle.z * spread},
            {size, size, size}, color);
    }
}

void MainScene::updateWindowTitle(GLFWwindow* window, AppState state) {
    static std::string lastTitle;
    const std::string title =
        state == AppState::MainMenu
            ? "Pixel World 3D | Main Menu"
            : "Pixel World 3D | LMB fire | RMB aim | Shift run | Space jump | Arrows walk | WASD camera | Esc quit";
    if (title == lastTitle) {
        return;
    }

    glfwSetWindowTitle(window, title.c_str());
    lastTitle = title;
}

}  // namespace pixel_world
