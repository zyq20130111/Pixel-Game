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

constexpr float kMapScale = 1.45f;
constexpr float kBuildingHeightScale = 1.35f;
constexpr float kDecorationScale = 1.25f;
constexpr float kCameraCollisionRadius = 0.42f;
constexpr float kTreeCollisionHalfExtent = 0.62f * kDecorationScale;
constexpr float kBigHeadCollisionHalfExtent =
    0.50f * kSceneCharacterScaleMultiplier;
constexpr float kPoliceCollisionHalfX =
    0.52f * kSceneCharacterScaleMultiplier;
constexpr float kPoliceCollisionHalfZ =
    0.48f * kSceneCharacterScaleMultiplier;

struct RockPlacement {
    Vec3 base;
    float scale;
};

constexpr std::array<Vec3, 8> kTreeBases{{
    {-9.5f, 0.0f, -10.5f},
    {9.5f, 0.0f, -10.5f},
    {-9.5f, 0.0f, -3.0f},
    {9.5f, 0.0f, -3.0f},
    {-9.5f, 0.0f, 5.0f},
    {9.5f, 0.0f, 5.0f},
    {-9.5f, 0.0f, 11.5f},
    {9.5f, 0.0f, 11.5f},
}};

constexpr std::array<RockPlacement, 4> kRockPlacements{{
    {{-11.0f, 0.0f, 1.5f}, 1.0f},
    {{11.0f, 0.0f, 1.5f}, 1.1f},
    {{-10.5f, 0.0f, 13.0f}, 0.9f},
    {{10.5f, 0.0f, 13.0f}, 0.9f},
}};

struct ExitPromptLayout {
    Rect panel;
    Rect yesButton;
    Rect noButton;
};

bool contains(const Rect& rect, float x, float y) {
    return x >= rect.x && x <= rect.x + rect.width && y >= rect.y &&
           y <= rect.y + rect.height;
}

MouseState getMouseState(GLFWwindow* window, int framebufferWidth,
                         int framebufferHeight) {
    int windowWidth = 1;
    int windowHeight = 1;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);

    double cursorX = 0.0;
    double cursorY = 0.0;
    glfwGetCursorPos(window, &cursorX, &cursorY);

    return {
        static_cast<float>(cursorX) * static_cast<float>(framebufferWidth) /
            static_cast<float>(windowWidth),
        static_cast<float>(cursorY) * static_cast<float>(framebufferHeight) /
            static_cast<float>(windowHeight),
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS,
    };
}

ExitPromptLayout makeExitPromptLayout(int width, int height) {
    const float scale =
        std::min(static_cast<float>(width) /
                     static_cast<float>(kWindowWidth),
                 static_cast<float>(height) /
                     static_cast<float>(kWindowHeight));
    const float panelWidth =
        std::min(520.0f * scale, static_cast<float>(width) - 48.0f);
    const float panelHeight =
        std::min(270.0f * scale, static_cast<float>(height) - 48.0f);
    const float panelX = (static_cast<float>(width) - panelWidth) * 0.5f;
    const float panelY = (static_cast<float>(height) - panelHeight) * 0.5f;
    const float buttonGap = 24.0f * scale;
    const float buttonWidth =
        std::min(170.0f * scale, (panelWidth - 72.0f * scale) * 0.5f);
    const float buttonHeight = std::max(34.0f, 58.0f * scale);
    const float buttonsWidth = buttonWidth * 2.0f + buttonGap;
    const float buttonY =
        panelY + panelHeight - buttonHeight - 42.0f * scale;
    const float yesX = panelX + (panelWidth - buttonsWidth) * 0.5f;

    return {
        {panelX, panelY, panelWidth, panelHeight},
        {yesX, buttonY, buttonWidth, buttonHeight},
        {yesX + buttonWidth + buttonGap, buttonY, buttonWidth, buttonHeight},
    };
}

void drawPromptButton(const Rect& button, const char* label,
                      const Color& baseColor, bool hovered, float uiScale) {
    const Color shadow = ThreeDUtils::shade(baseColor, 0.55f);
    const Color fill =
        hovered ? ThreeDUtils::shade(baseColor, 1.18f) : baseColor;
    const float pixel = std::max(1.0f, 3.0f * uiScale);

    ThreeDUtils::drawRect2D(
        {button.x, button.y + 5.0f * uiScale, button.width, button.height},
        shadow);
    ThreeDUtils::drawRect2D(button, fill);
    ThreeDUtils::drawFrame2D(button, kInkColor, pixel);
    ThreeDUtils::drawRect2D(
        {button.x + pixel, button.y + pixel,
         button.width - 2.0f * pixel, pixel},
        ThreeDUtils::shade(fill, 1.25f));

    const float textScale = std::max(1.0f, 3.0f * uiScale);
    const float textY =
        button.y + (button.height - 7.0f * textScale) * 0.5f;
    const std::string text(label);
    ThreeDUtils::drawText(
        text, button.x + (button.width -
                          ThreeDUtils::textWidth(text, textScale)) *
                             0.5f,
        textY, textScale, Color{1.0f, 1.0f, 0.92f});
}

void drawWorldLabel(const char* label, const Vec3& center, float pixelScale,
                    const Color& color) {
    const std::string text(label);
    glPushMatrix();
    glTranslatef(center.x, center.y, center.z);
    glScalef(1.0f, -1.0f, 1.0f);
    ThreeDUtils::drawText(
        text, -ThreeDUtils::textWidth(text, pixelScale) * 0.5f, 0.0f,
        pixelScale, color);
    glPopMatrix();
}

bool circleIntersectsBox(float circleX, float circleZ, float radius,
                         float boxCenterX, float boxCenterZ, float halfX,
                         float halfZ) {
    const float closestX =
        std::clamp(circleX, boxCenterX - halfX, boxCenterX + halfX);
    const float closestZ =
        std::clamp(circleZ, boxCenterZ - halfZ, boxCenterZ + halfZ);
    const float deltaX = circleX - closestX;
    const float deltaZ = circleZ - closestZ;
    return deltaX * deltaX + deltaZ * deltaZ <= radius * radius;
}

}  // namespace

MainScene::MainScene()
    : window_(nullptr),
      glfwInitialized_(false),
      framebufferWidth_(0),
      framebufferHeight_(0),
      state_(AppState::MainMenu),
      camera_(),
      menuCamera_({0.0f, kCameraGroundHeight, 17.5f}),
      character_(),
      bigHeadSon_(),
      police_(),
      policeCrouched_(),
      zombie_(),
      miko_(),
      pistol_(),
      loginScreen_(),
      bullets_(),
      impactEffects_(),
      previousFireDown_(false),
      previousJumpDown_(false),
      previousEscapeDown_(false),
      exitPromptVisible_(false),
      previousPromptMouseDown_(false),
      promptMouse_{0.0f, 0.0f, false} {
    police_.setPosition({3.0f, 0.0f, 0.6f});
    policeCrouched_.setPosition({4.8f, 0.0f, 0.6f});
    policeCrouched_.setCrouched(true);
    policeCrouched_.setMoving(false);
}

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
            const bool promptWasVisible = exitPromptVisible_;
            updateExitPrompt();
            if (!promptWasVisible && !exitPromptVisible_) {
                updateGameplay(dt);
            }

            ThreeDUtils::setProjection(
                framebufferWidth_, framebufferHeight_,
                ThreeDUtils::currentFieldOfView(camera_.aimAmount()));
            camera_.apply();
            renderFrame(camera_, true);
            if (exitPromptVisible_) {
                renderExitPrompt();
            }
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

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* videoMode =
        monitor != nullptr ? glfwGetVideoMode(monitor) : nullptr;
    int windowWidth = kWindowWidth;
    int windowHeight = kWindowHeight;

    if (videoMode != nullptr) {
        windowWidth = videoMode->width;
        windowHeight = videoMode->height;
        glfwWindowHint(GLFW_RED_BITS, videoMode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS, videoMode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, videoMode->blueBits);
        glfwWindowHint(GLFW_REFRESH_RATE, videoMode->refreshRate);
    } else {
        monitor = nullptr;
    }

    window_ = glfwCreateWindow(windowWidth, windowHeight, "Pixel World 3D",
                               monitor, nullptr);
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
    glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    camera_.resetLookTracking(window_);
    character_.reset();
    bigHeadSon_.reset();
    police_.reset();
    policeCrouched_.reset();
    zombie_.reset();
    zombie_.setAudioListener(camera_.position(), camera_.forward());
    miko_.reset();
    policeCrouched_.setPosition({4.8f, 0.0f, 0.6f});
    policeCrouched_.setCrouched(true);
    policeCrouched_.setMoving(false);
    pistol_.reset();
    bullets_.clear();
    impactEffects_.clear();
    exitPromptVisible_ = false;
    previousEscapeDown_ =
        glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    previousPromptMouseDown_ =
        glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
}

void MainScene::updateExitPrompt() {
    const bool escapeDown =
        glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    if (escapeDown && !previousEscapeDown_ && !exitPromptVisible_) {
        exitPromptVisible_ = true;
        glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        camera_.resetLookTracking(window_);
        previousPromptMouseDown_ =
            glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    }
    previousEscapeDown_ = escapeDown;

    if (!exitPromptVisible_) {
        return;
    }

    promptMouse_ =
        getMouseState(window_, framebufferWidth_, framebufferHeight_);
    const bool clicked = promptMouse_.pressed && !previousPromptMouseDown_;
    previousPromptMouseDown_ = promptMouse_.pressed;
    if (!clicked) {
        return;
    }

    const ExitPromptLayout layout =
        makeExitPromptLayout(framebufferWidth_, framebufferHeight_);
    if (contains(layout.yesButton, promptMouse_.x, promptMouse_.y)) {
        glfwSetWindowShouldClose(window_, GLFW_TRUE);
    } else if (contains(layout.noButton, promptMouse_.x, promptMouse_.y)) {
        exitPromptVisible_ = false;
        glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        camera_.resetLookTracking(window_);
        previousFireDown_ = promptMouse_.pressed;
        previousJumpDown_ =
            glfwGetKey(window_, GLFW_KEY_SPACE) == GLFW_PRESS;
    }
}

void MainScene::updateGameplay(float dt) {
    camera_.updateLook(window_);
    camera_.update(
        window_, dt, previousJumpDown_,
        [this](const Vec3& position) {
            return cameraPositionBlocked(position);
        });
    camera_.updateAim(window_, dt);
    character_.update(window_, dt);
    bigHeadSon_.update(dt);
    police_.update(dt);
    policeCrouched_.update(dt);
    zombie_.setAudioListener(camera_.position(), camera_.forward());
    zombie_.update(dt);
    miko_.update(dt);
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

bool MainScene::cameraPositionBlocked(const Vec3& position) const {
    const auto collidesWithBox = [&](float centerX, float centerZ, float halfX,
                                     float halfZ) {
        return circleIntersectsBox(position.x, position.z,
                                   kCameraCollisionRadius, centerX, centerZ,
                                   halfX, halfZ);
    };

    for (const Vec3& base : kTreeBases) {
        const float x = base.x * kMapScale;
        const float z = base.z * kMapScale;
        if (collidesWithBox(x, z, kTreeCollisionHalfExtent,
                            kTreeCollisionHalfExtent)) {
            return true;
        }
    }

    for (const RockPlacement& rock : kRockPlacements) {
        const float decorationScale = rock.scale * kDecorationScale;
        const float x = rock.base.x * kMapScale + 0.04f * decorationScale;
        const float z = rock.base.z * kMapScale - 0.03f * decorationScale;
        if (collidesWithBox(x, z, 0.42f * decorationScale,
                            0.46f * decorationScale)) {
            return true;
        }
    }

    // The portal opening remains passable, while its two solid side pillars
    // block the player like the geometry shown on screen.
    const float portalZ = 12.4f * kMapScale;
    const float portalPostX = 1.18f * kMapScale;
    const float portalPostHalfX = 0.24f * kMapScale;
    const float portalPostHalfZ = 0.25f * kMapScale;
    if (collidesWithBox(-portalPostX, portalZ, portalPostHalfX,
                        portalPostHalfZ) ||
        collidesWithBox(portalPostX, portalZ, portalPostHalfX,
                        portalPostHalfZ)) {
        return true;
    }

    // Collision boxes for the visible characters are deliberately a little
    // wider than their meshes so the first-person camera cannot overlap them.
    if (collidesWithBox(-3.0f, 0.6f, kBigHeadCollisionHalfExtent,
                        kBigHeadCollisionHalfExtent) ||
        collidesWithBox(character_.position().x, character_.position().z,
                        kCharacterHitHalfWidth, kCharacterHitHalfDepth) ||
        collidesWithBox(police_.position().x, police_.position().z,
                        kPoliceCollisionHalfX, kPoliceCollisionHalfZ) ||
        collidesWithBox(policeCrouched_.position().x,
                        policeCrouched_.position().z, kPoliceCollisionHalfX,
                        kPoliceCollisionHalfZ) ||
        collidesWithBox(zombie_.position().x, zombie_.position().z,
                        kZombieCollisionHalfWidth,
                        kZombieCollisionHalfDepth) ||
        collidesWithBox(miko_.position().x, miko_.position().z,
                        kMikoCollisionHalfWidth,
                        kMikoCollisionHalfDepth)) {
        return true;
    }

    return false;
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

    drawRoad({0.0f, 0.04f, -3.8f}, {1.45f, 0.12f, 4.2f});
    drawRoad({0.0f, 0.04f, -5.6f}, {10.5f, 0.12f, 1.35f});
    drawRoad({-4.8f, 0.04f, -6.7f}, {1.30f, 0.12f, 2.6f});
    drawRoad({4.8f, 0.04f, -6.7f}, {1.30f, 0.12f, 2.6f});

    drawRoad({0.0f, 0.04f, 7.3f}, {1.50f, 0.12f, 11.0f});
    drawRoad({0.0f, 0.04f, 5.7f}, {10.5f, 0.12f, 1.35f});
    drawRoad({-4.8f, 0.04f, 4.7f}, {1.30f, 0.12f, 2.45f});
    drawRoad({4.8f, 0.04f, 4.7f}, {1.30f, 0.12f, 2.45f});
    drawRoad({0.0f, 0.04f, 9.9f}, {10.5f, 0.12f, 1.35f});
    drawRoad({-4.8f, 0.04f, 9.2f}, {1.30f, 0.12f, 2.45f});
    drawRoad({4.8f, 0.04f, 9.2f}, {1.30f, 0.12f, 2.45f});
    drawRoad({0.0f, 0.04f, 12.3f}, {1.50f, 0.12f, 3.4f});

    drawCentralPlaza();
    //drawPixelStatue();
    drawDungeonPortal();

    drawCloud({-7.5f, 7.2f, -10.0f}, 1.0f);
    drawCloud({6.5f, 8.3f, -18.0f}, 1.25f);
    drawCloud({12.0f, 6.4f, 1.0f}, 0.8f);

    drawTree({-9.5f, 0.0f, -10.5f});
    drawTree({9.5f, 0.0f, -10.5f});
    drawTree({-9.5f, 0.0f, -3.0f});
    drawTree({9.5f, 0.0f, -3.0f});
    drawTree({-9.5f, 0.0f, 5.0f});
    drawTree({9.5f, 0.0f, 5.0f});
    drawTree({-9.5f, 0.0f, 11.5f});
    drawTree({9.5f, 0.0f, 11.5f});

    drawRock({-11.0f, 0.0f, 1.5f}, 1.0f);
    drawRock({11.0f, 0.0f, 1.5f}, 1.1f);
    drawRock({-10.5f, 0.0f, 13.0f}, 0.9f);
    drawRock({10.5f, 0.0f, 13.0f}, 0.9f);

    character_.render();
    character_.renderHealthBar();
    bigHeadSon_.render();
    police_.render();
    policeCrouched_.render();
    zombie_.render();
    miko_.render();
}

void MainScene::renderExitPrompt() const {
    const ExitPromptLayout layout =
        makeExitPromptLayout(framebufferWidth_, framebufferHeight_);
    const float uiScale =
        std::min(static_cast<float>(framebufferWidth_) /
                     static_cast<float>(kWindowWidth),
                 static_cast<float>(framebufferHeight_) /
                     static_cast<float>(kWindowHeight));

    ThreeDUtils::setUiProjection(framebufferWidth_, framebufferHeight_);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ThreeDUtils::drawRect2D(
        {0.0f, 0.0f, static_cast<float>(framebufferWidth_),
         static_cast<float>(framebufferHeight_)},
        kInkColor, 0.52f);
    ThreeDUtils::drawRect2D(
        {layout.panel.x + 8.0f * uiScale, layout.panel.y + 8.0f * uiScale,
         layout.panel.width, layout.panel.height},
        kInkColor, 0.35f);
    ThreeDUtils::drawRect2D(layout.panel, kPanelBottom, 0.98f);
    ThreeDUtils::drawRect2D(
        {layout.panel.x, layout.panel.y, layout.panel.width,
         layout.panel.height * 0.42f},
        kPanelTop, 0.98f);
    ThreeDUtils::drawFrame2D(
        layout.panel, kInkColor, std::max(2.0f, 5.0f * uiScale));

    ThreeDUtils::drawCenteredText(
        "EXIT GAME", layout.panel, std::max(1.0f, 4.0f * uiScale),
        kInkColor, 42.0f * uiScale);
    ThreeDUtils::drawCenteredText(
        "ARE YOU SURE", layout.panel, std::max(1.0f, 2.4f * uiScale),
        ThreeDUtils::shade(kInkColor, 1.2f), 104.0f * uiScale);

    drawPromptButton(layout.yesButton, "YES", kButtonExit,
                     contains(layout.yesButton, promptMouse_.x, promptMouse_.y),
                     uiScale);
    drawPromptButton(layout.noButton, "NO", kButtonStart,
                     contains(layout.noButton, promptMouse_.x, promptMouse_.y),
                     uiScale);

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    ThreeDUtils::setProjection(
        framebufferWidth_, framebufferHeight_,
        ThreeDUtils::currentFieldOfView(camera_.aimAmount()));
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

    // Keep the farthest skybox corner inside the far clipping plane.  With a
    // 90-unit half-size, diagonal view rays reached roughly 156 units while
    // kFarPlane is 120, exposing the black clear color as triangular gaps.
    const float halfSize = kFarPlane * 0.5f;
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
    for (int z = -21; z <= 21; ++z) {
        for (int x = -21; x <= 21; ++x) {
            Color color = ((x + z) & 1) == 0 ? kGrassA : kGrassB;
            if (std::abs(x) >= 20 || std::abs(z) >= 20) {
                color = ((x + z) & 1) == 0
                            ? kWaterColor
                            : ThreeDUtils::shade(kWaterColor, 0.8f);
            }
            ThreeDUtils::drawCube({static_cast<float>(x), -0.09f,
                                   static_cast<float>(z)},
                                  {0.98f, 0.18f, 0.98f}, color);
            if (std::abs(x) >= 13 || std::abs(z) >= 14) {
                ThreeDUtils::drawCube(
                    {static_cast<float>(x) - 0.12f, 0.025f,
                     static_cast<float>(z) - 0.05f},
                    {0.40f, 0.025f, 0.07f}, kWaterHighlight);
            }
        }
    }
}

void MainScene::drawTree(const Vec3& base) const {
    const float x = base.x * kMapScale;
    const float z = base.z * kMapScale;
    const float s = kDecorationScale;
    ThreeDUtils::drawCube({x, base.y + 0.55f * s, z},
                          {0.5f * s, 1.2f * s, 0.5f * s}, kTrunkColor);
    ThreeDUtils::drawCube({x, base.y + 1.45f * s, z},
                          {1.7f * s, 1.0f * s, 1.7f * s}, kLeafDarkColor);
    ThreeDUtils::drawCube({x, base.y + 2.15f * s, z},
                          {1.3f * s, 0.9f * s, 1.3f * s}, kLeafColor);
    ThreeDUtils::drawCube({x, base.y + 2.80f * s, z},
                          {0.9f * s, 0.8f * s, 0.9f * s}, kLeafDarkColor);
    ThreeDUtils::drawCube({x - 0.56f * s, base.y + 1.72f * s,
                           z + 0.12f * s},
                          {0.40f * s, 0.34f * s, 0.40f * s}, kLeafColor);
    ThreeDUtils::drawCube({x + 0.54f * s, base.y + 2.18f * s,
                           z - 0.08f * s},
                          {0.34f * s, 0.34f * s, 0.34f * s}, kLeafDarkColor);
    ThreeDUtils::drawCube({x - 0.22f * s, base.y + 2.38f * s,
                           z + 0.48f * s},
                          {0.16f * s, 0.16f * s, 0.16f * s}, kSunCore);
}

void MainScene::drawRock(const Vec3& base, float scale) const {
    const float x = base.x * kMapScale;
    const float z = base.z * kMapScale;
    const float s = scale * kDecorationScale;
    ThreeDUtils::drawCube(
        {x, base.y + 0.22f * s, z},
        {0.7f * s, 0.45f * s, 0.8f * s}, kStoneColor);
    ThreeDUtils::drawCube(
        {x + 0.12f * s, base.y + 0.40f * s, z - 0.08f * s},
        {0.45f * s, 0.25f * s, 0.35f * s}, kStoneDark);
}

void MainScene::drawRoad(const Vec3& center, const Vec3& size) const {
    const Vec3 scaledCenter{center.x * kMapScale, center.y,
                            center.z * kMapScale};
    const Vec3 scaledSize{size.x * kMapScale, size.y, size.z * kMapScale};
    ThreeDUtils::drawCube(
        {scaledCenter.x, scaledCenter.y - 0.01f, scaledCenter.z},
        {scaledSize.x + 0.16f, scaledSize.y + 0.04f,
         scaledSize.z + 0.16f},
        kPathDark);
    ThreeDUtils::drawCube(scaledCenter, scaledSize, kPathColor);
    if (scaledSize.x > scaledSize.z) {
        ThreeDUtils::drawCube(
            {scaledCenter.x, scaledCenter.y + scaledSize.y * 0.56f,
             scaledCenter.z - scaledSize.z * 0.16f},
            {scaledSize.x * 0.72f, scaledSize.y * 0.14f,
             scaledSize.z * 0.10f},
            kPlazaTrim);
    } else {
        ThreeDUtils::drawCube(
            {scaledCenter.x + scaledSize.x * 0.16f,
             scaledCenter.y + scaledSize.y * 0.56f, scaledCenter.z},
            {scaledSize.x * 0.10f, scaledSize.y * 0.14f,
             scaledSize.z * 0.72f},
            kPlazaTrim);
    }
}

void MainScene::drawCentralPlaza() const {
    const float sx = kMapScale;
    ThreeDUtils::drawCube({0.0f, 0.12f, 0.0f},
                          {5.8f * sx, 0.26f, 4.1f * sx}, kPlazaColor);
    ThreeDUtils::drawCube({0.0f, 0.27f, 0.0f},
                          {5.25f * sx, 0.08f, 3.55f * sx}, kPlazaTrim);
    ThreeDUtils::drawCube({0.0f, 0.33f, 0.0f},
                          {4.85f * sx, 0.08f, 3.15f * sx}, kPlazaColor);
    ThreeDUtils::drawCube({0.0f, 0.39f, 0.0f},
                          {2.10f * sx, 0.06f, 2.10f * sx}, kHouseWindow);
    drawWorldLabel("PLAZA", {0.0f, 0.46f, 1.0f * sx}, 0.060f, kInkColor);
}

void MainScene::drawPixelStatue() const {
    const float sx = kMapScale;
    const float sy = kBuildingHeightScale;
    const float z = -0.35f * sx;
    ThreeDUtils::drawCube({0.0f, 0.50f * sy, z},
                          {1.85f * sx, 0.45f * sy, 1.85f * sx}, kStoneDark);
    ThreeDUtils::drawCube({0.0f, 0.78f * sy, z},
                          {1.55f * sx, 0.12f * sy, 1.55f * sx}, kPlazaTrim);
    ThreeDUtils::drawCube({0.0f, 1.35f * sy, z},
                          {0.85f * sx, 1.05f * sy, 0.62f * sx}, kStatueColor);
    ThreeDUtils::drawCube({0.0f, 2.05f * sy, z},
                          {0.95f * sx, 0.78f * sy, 0.82f * sx},
                          kCharacterSkin);
    ThreeDUtils::drawCube({0.0f, 2.42f * sy, z},
                          {1.02f * sx, 0.30f * sy, 0.90f * sx},
                          kCharacterHair);
    ThreeDUtils::drawCube({-0.68f * sx, 1.38f * sy, z},
                          {0.30f * sx, 0.90f * sy, 0.34f * sx},
                          kStatueColor);
    ThreeDUtils::drawCube({0.68f * sx, 1.38f * sy, z},
                          {0.30f * sx, 0.90f * sy, 0.34f * sx},
                          kStatueColor);
    ThreeDUtils::drawCube({-0.22f * sx, 0.98f * sy, z},
                          {0.32f * sx, 0.75f * sy, 0.40f * sx},
                          kStatueAccent);
    ThreeDUtils::drawCube({0.22f * sx, 0.98f * sy, z},
                          {0.32f * sx, 0.75f * sy, 0.40f * sx},
                          kStatueAccent);
    ThreeDUtils::drawCube({-0.18f * sx, 2.08f * sy, 0.08f * sx},
                          {0.20f * sx, 0.16f * sy, 0.08f * sx},
                          kCharacterEyeWhite);
    ThreeDUtils::drawCube({0.18f * sx, 2.08f * sy, 0.08f * sx},
                          {0.20f * sx, 0.16f * sy, 0.08f * sx},
                          kCharacterEyeWhite);
    ThreeDUtils::drawCube({-0.18f * sx, 2.08f * sy, 0.13f * sx},
                          {0.08f * sx, 0.10f * sy, 0.05f * sx},
                          kCharacterEye);
    ThreeDUtils::drawCube({0.18f * sx, 2.08f * sy, 0.13f * sx},
                          {0.08f * sx, 0.10f * sy, 0.05f * sx},
                          kCharacterEye);
}

void MainScene::drawDungeonPortal() const {
    const float sx = kMapScale;
    const float sy = kBuildingHeightScale;
    const float portalZ = 12.4f * sx;
    ThreeDUtils::drawCube({0.0f, 0.12f, portalZ},
                          {3.7f * sx, 0.24f, 2.8f * sx}, kPortalFrame);
    ThreeDUtils::drawCube({0.0f, 0.25f, portalZ},
                          {3.25f * sx, 0.08f, 2.35f * sx}, kPlazaTrim);
    ThreeDUtils::drawCube({-1.18f * sx, 1.45f * sy, portalZ},
                          {0.48f * sx, 2.75f * sy, 0.50f * sx},
                          kPortalFrame);
    ThreeDUtils::drawCube({1.18f * sx, 1.45f * sy, portalZ},
                          {0.48f * sx, 2.75f * sy, 0.50f * sx},
                          kPortalFrame);
    ThreeDUtils::drawCube({0.0f, 2.78f * sy, portalZ},
                          {2.85f * sx, 0.48f * sy, 0.50f * sx},
                          kPortalFrame);
    ThreeDUtils::drawCube({0.0f, 1.45f * sy, portalZ + 0.28f * sx},
                          {1.82f * sx, 2.20f * sy, 0.08f * sx},
                          kPortalGlow);
    ThreeDUtils::drawCube({0.0f, 1.45f * sy, portalZ + 0.34f * sx},
                          {1.38f * sx, 1.78f * sy, 0.06f * sx}, kInkColor);
    ThreeDUtils::drawCube({0.0f, 1.45f * sy, portalZ + 0.39f * sx},
                          {1.08f * sx, 1.48f * sy, 0.04f * sx},
                          kPortalGlow);
    ThreeDUtils::drawCube({0.0f, 3.32f * sy, portalZ + 0.28f * sx},
                          {2.75f * sx, 0.78f * sy, 0.08f * sx}, kSignColor);
    drawWorldLabel("DUNGEON",
                   {0.0f, 3.39f * sy, portalZ + 0.34f * sx}, 0.050f,
                   kInkColor);
    drawWorldLabel("ENTER",
                   {0.0f, 0.78f * sy, portalZ + 0.34f * sx}, 0.045f,
                   kSignColor);
}

void MainScene::drawCloud(const Vec3& base, float scale) const {
    ThreeDUtils::drawCube(
        {base.x, base.y - 0.14f * scale, base.z},
        {2.15f * scale, 0.56f * scale, 0.72f * scale}, kCloudShadow);
    ThreeDUtils::drawCube(
        {base.x - 0.62f * scale, base.y + 0.08f * scale, base.z},
        {1.0f * scale, 0.75f * scale, 0.84f * scale}, kCloudColor);
    ThreeDUtils::drawCube(
        {base.x + 0.05f * scale, base.y + 0.24f * scale, base.z},
        {1.24f * scale, 0.92f * scale, 0.94f * scale}, kCloudColor);
    ThreeDUtils::drawCube(
        {base.x + 0.78f * scale, base.y + 0.02f * scale, base.z},
        {0.90f * scale, 0.68f * scale, 0.78f * scale}, kCloudColor);
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
            : "Pixel World 3D | Mouse look | LMB fire | RMB aim | E attack | Shift run | Space jump | Arrows walk | WASD move | Esc exit";
    if (title == lastTitle) {
        return;
    }

    glfwSetWindowTitle(window, title.c_str());
    lastTitle = title;
}

}  // namespace pixel_world
