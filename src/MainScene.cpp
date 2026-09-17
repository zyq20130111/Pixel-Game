#include "MainScene.h"

#include "AppConfig.h"
#include "game_constants.h"
#include "ThreeDUtils.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <string>

namespace pixel_world {
namespace {

using namespace constants;

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

bool numberKeyDown(GLFWwindow* window, int mainKey, int keypadKey) {
    return glfwGetKey(window, mainKey) == GLFW_PRESS ||
           glfwGetKey(window, keypadKey) == GLFW_PRESS;
}

struct InventoryIconPoint {
    float x;
    float y;
};

void drawInventoryPolygon(std::initializer_list<InventoryIconPoint> points,
                          const Color& color, float alpha) {
    glColor4f(color.red, color.green, color.blue, alpha);
    glBegin(GL_POLYGON);
    for (const InventoryIconPoint& point : points) {
        glVertex2f(point.x, point.y);
    }
    glEnd();
}

void drawInventoryKnifeIcon(const Rect& slot, float uiScale, float alpha) {
    const float centerX = slot.x + slot.width * 0.5f;
    const float centerY = slot.y + slot.height * 0.47f;
    glPushMatrix();
    glTranslatef(centerX, centerY, 0.0f);
    glScalef(uiScale, uiScale, 1.0f);
    glRotatef(-18.0f, 0.0f, 0.0f, 1.0f);

    // Dark silhouette first, then the steel and grip inset.
    drawInventoryPolygon(
        {{-35.0f, 0.0f}, {-19.0f, -9.0f}, {8.0f, -7.0f},
         {13.0f, 0.0f}, {8.0f, 7.0f}, {-19.0f, 9.0f}},
        kInkColor, alpha);
    drawInventoryPolygon(
        {{-32.0f, 0.0f}, {-18.0f, -6.0f}, {6.0f, -5.0f},
         {9.0f, 0.0f}, {6.0f, 5.0f}, {-18.0f, 6.0f}},
        kKnifeBlade, alpha);
    ThreeDUtils::drawRect2D({-15.0f, -2.0f, 19.0f, 3.0f},
                            kKnifeBladeEdge, alpha);
    ThreeDUtils::drawRect2D({4.0f, -10.0f, 5.0f, 20.0f},
                            kKnifeGuard, alpha);
    ThreeDUtils::drawRect2D({8.0f, -7.0f, 26.0f, 14.0f},
                            kInkColor, alpha);
    ThreeDUtils::drawRect2D({10.0f, -5.0f, 22.0f, 10.0f},
                            kKnifeGrip, alpha);
    ThreeDUtils::drawRect2D({13.0f, -3.0f, 15.0f, 3.0f},
                            kKnifeGripDark, alpha);
    ThreeDUtils::drawRect2D({31.0f, -6.0f, 5.0f, 12.0f},
                            kKnifeGripDark, alpha);
    glPopMatrix();
}

void drawInventoryPistolIcon(const Rect& slot, float uiScale, float alpha) {
    const float centerX = slot.x + slot.width * 0.5f;
    const float centerY = slot.y + slot.height * 0.47f;
    glPushMatrix();
    glTranslatef(centerX, centerY, 0.0f);
    glScalef(uiScale, uiScale, 1.0f);

    // Compact side profile: slide, barrel, trigger guard and angled grip.
    ThreeDUtils::drawRect2D({-31.0f, -11.0f, 39.0f, 19.0f},
                            kInkColor, alpha);
    ThreeDUtils::drawRect2D({-28.0f, -8.0f, 34.0f, 13.0f},
                            kPistolMetal, alpha);
    ThreeDUtils::drawRect2D({-23.0f, -6.0f, 25.0f, 3.0f},
                            kPistolHighlight, alpha);
    ThreeDUtils::drawRect2D({5.0f, -6.0f, 26.0f, 9.0f},
                            kPistolMetalDark, alpha);
    ThreeDUtils::drawRect2D({8.0f, -4.0f, 22.0f, 4.0f},
                            kPistolHighlight, alpha);

    drawInventoryPolygon(
        {{-4.0f, 7.0f}, {15.0f, 7.0f}, {10.0f, 34.0f},
         {-7.0f, 30.0f}},
        kInkColor, alpha);
    drawInventoryPolygon(
        {{-1.0f, 9.0f}, {12.0f, 9.0f}, {8.0f, 30.0f},
         {-4.0f, 28.0f}},
        kPistolGrip, alpha);
    ThreeDUtils::drawRect2D({0.0f, 12.0f, 7.0f, 14.0f},
                            kPistolGripDark, alpha);

    drawInventoryPolygon(
        {{-4.0f, 6.0f}, {8.0f, 6.0f}, {5.0f, 20.0f},
         {-1.0f, 20.0f}},
        kInkColor, alpha);
    drawInventoryPolygon(
        {{-1.0f, 8.0f}, {5.0f, 8.0f}, {3.0f, 17.0f},
         {1.0f, 17.0f}},
        kPistolMetalDark, alpha);
    glPopMatrix();
}

}  // namespace

MainScene::MainScene()
    : window_(nullptr),
      glfwInitialized_(false),
      framebufferWidth_(0),
      framebufferHeight_(0),
      state_(AppState::MainMenu),
      language_(AppConfig::loadLanguage()),
      camera_(),
      soundManager_(),
      parkingLotScene_(language_),
      knife_(),
      pistol_(),
      mainUI_(language_),
      difficultyUI_(language_),
      loadingUI_(language_),
      selectedDifficulty_(Difficulty::Normal),
      loadingElapsed_(0.0f),
      bullets_(),
      impactEffects_(),
      equippedWeapon_(WeaponType::Knife),
      weaponSwitchTarget_(WeaponType::Knife),
      weaponSwitchElapsed_(0.0f),
      weaponSwitching_(false),
      previousFireDown_(false),
      previousWeapon1Down_(false),
      previousWeapon2Down_(false),
      previousJumpDown_(false),
      previousCrouchDown_(false),
      previousProneDown_(false),
      previousInteractDown_(false),
      previousRestartDown_(false),
      parkingHintTimer_(0.0f),
      playerHealth_(kCharacterMaxHp),
      playerMaxHealth_(kCharacterMaxHp),
      playerDamageFlashTimer_(0.0f),
      playerDown_(false),
      previousEscapeDown_(false),
      exitPromptVisible_(false),
      previousPromptMouseDown_(false),
      promptMouse_{0.0f, 0.0f, false} {}

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
                mainUI_.update(window_, framebufferWidth_, framebufferHeight_);
            if (mainUI_.buttonClicked()) {
                soundManager_.play2D("btnclick.wav", 1.0f);
            }
            if (action == MenuAction::NewGame ||
                action == MenuAction::ContinueGame) {
                state_ = AppState::DifficultySelect;
            } else if (action == MenuAction::Exit) {
                glfwSetWindowShouldClose(window_, GLFW_TRUE);
            }

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            mainUI_.render(framebufferWidth_, framebufferHeight_);
        } else if (state_ == AppState::DifficultySelect) {
            const MenuAction action = difficultyUI_.update(
                window_, framebufferWidth_, framebufferHeight_);
            if (difficultyUI_.buttonClicked()) {
                soundManager_.play2D("btnclick.wav", 1.0f);
            }
            if (action == MenuAction::DifficultyEasy) {
                startLoading(Difficulty::Easy);
            } else if (action == MenuAction::DifficultyNormal) {
                startLoading(Difficulty::Normal);
            } else if (action == MenuAction::DifficultyHard) {
                startLoading(Difficulty::Hard);
            } else if (action == MenuAction::Back) {
                state_ = AppState::MainMenu;
                mainUI_.showMainMenu();
            }

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            difficultyUI_.render(framebufferWidth_, framebufferHeight_);
        } else if (state_ == AppState::Loading) {
            updateLoading(dt);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            loadingUI_.render(framebufferWidth_, framebufferHeight_);
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

        updateWindowTitle(window_, state_, language_);
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
    if (!soundManager_.initialize()) {
        std::cerr << "Failed to initialize audio.\n";
        return false;
    }

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
    updateWindowTitle(window_, state_, language_);
    return true;
}

void MainScene::resetGame() {
    parkingLotScene_.reset(selectedDifficulty_);
    camera_.reset(parkingLotScene_.spawnPosition());
    soundManager_.stopAll();
    soundManager_.setListener(camera_.position(), camera_.forward(),
                              {0.0f, 1.0f, 0.0f});
    glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    camera_.resetLookTracking(window_);
    knife_.reset();
    pistol_.reset();
    equippedWeapon_ = WeaponType::Knife;
    weaponSwitchTarget_ = equippedWeapon_;
    weaponSwitchElapsed_ = 0.0f;
    weaponSwitching_ = false;
    bullets_.clear();
    impactEffects_.clear();
    parkingHintTimer_ = 0.0f;
    playerMaxHealth_ = kCharacterMaxHp;
    playerHealth_ = playerMaxHealth_;
    playerDamageFlashTimer_ = 0.0f;
    playerDown_ = false;
    exitPromptVisible_ = false;
    previousEscapeDown_ =
        glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    previousWeapon1Down_ =
        numberKeyDown(window_, GLFW_KEY_1, GLFW_KEY_KP_1);
    previousWeapon2Down_ =
        numberKeyDown(window_, GLFW_KEY_2, GLFW_KEY_KP_2);
    previousInteractDown_ =
        glfwGetKey(window_, GLFW_KEY_E) == GLFW_PRESS;
    previousRestartDown_ =
        glfwGetKey(window_, GLFW_KEY_R) == GLFW_PRESS;
    previousJumpDown_ =
        glfwGetKey(window_, GLFW_KEY_SPACE) == GLFW_PRESS;
    previousCrouchDown_ =
        glfwGetKey(window_, GLFW_KEY_C) == GLFW_PRESS;
    previousProneDown_ =
        glfwGetKey(window_, GLFW_KEY_Z) == GLFW_PRESS;
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
        soundManager_.play2D("btnclick.wav", 1.0f);
        glfwSetWindowShouldClose(window_, GLFW_TRUE);
    } else if (contains(layout.noButton, promptMouse_.x, promptMouse_.y)) {
        soundManager_.play2D("btnclick.wav", 1.0f);
        exitPromptVisible_ = false;
        glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        camera_.resetLookTracking(window_);
        previousFireDown_ = promptMouse_.pressed;
        previousJumpDown_ =
            glfwGetKey(window_, GLFW_KEY_SPACE) == GLFW_PRESS;
        previousCrouchDown_ =
            glfwGetKey(window_, GLFW_KEY_C) == GLFW_PRESS;
        previousProneDown_ =
            glfwGetKey(window_, GLFW_KEY_Z) == GLFW_PRESS;
    }
}

void MainScene::updateGameplay(float dt) {
    const bool crouchDown = glfwGetKey(window_, GLFW_KEY_C) == GLFW_PRESS;
    const bool proneDown = glfwGetKey(window_, GLFW_KEY_Z) == GLFW_PRESS;
    const bool crouchPressed = crouchDown && !previousCrouchDown_;
    const bool pronePressed = proneDown && !previousProneDown_;
    if (parkingLotScene_.levelComplete()) {
        previousCrouchDown_ = crouchDown;
        previousProneDown_ = proneDown;
        return;
    }

    const bool restartDown = glfwGetKey(window_, GLFW_KEY_R) == GLFW_PRESS;
    if (playerDown_) {
        if (restartDown && !previousRestartDown_) {
            resetGame();
            previousRestartDown_ = restartDown;
            return;
        }
        previousRestartDown_ = restartDown;
        previousCrouchDown_ = crouchDown;
        previousProneDown_ = proneDown;
        return;
    }

    if (crouchPressed || pronePressed) {
        if (camera_.posture() == Camera::Posture::Standing) {
            camera_.setPosture(crouchPressed ? Camera::Posture::Crouching
                                              : Camera::Posture::Prone);
        } else {
            camera_.setPosture(Camera::Posture::Standing);
        }
    }

    updateWeaponSelection(dt);
    camera_.updateLook(window_);
    camera_.update(
        window_, dt, previousJumpDown_,
        [this](const Vec3& position) {
            return cameraPositionBlocked(position, camera_.position());
        });
    camera_.updateAim(window_, dt);
    soundManager_.setListener(camera_.position(), camera_.forward(),
                              {0.0f, 1.0f, 0.0f});
    soundManager_.update();
    bool playerAttacked = false;
    if (equippedWeapon_ == WeaponType::Knife) {
        if (weaponSwitching_) {
            // Let an in-progress slash finish during a weapon change, while
            // preventing a held mouse button from starting a new attack.
            previousFireDown_ = true;
        }
        playerAttacked =
            knife_.update(window_, camera_, bullets_, soundManager_,
                          previousFireDown_, dt);
    } else {
        if (weaponSwitching_) {
            previousFireDown_ = true;
        }
        playerAttacked =
            pistol_.update(window_, camera_, bullets_, soundManager_,
                           previousFireDown_, dt);
    }
    if (playerAttacked && equippedWeapon_ == WeaponType::Knife) {
        performKnifeAttack();
    }
    const bool playerFired =
        playerAttacked && equippedWeapon_ == WeaponType::Pistol;
    const int playerDamage =
        parkingLotScene_.update(dt, camera_.position(), playerFired);
    if (playerDamage > 0) {
        playerHealth_ = std::max(0, playerHealth_ - playerDamage);
        playerDamageFlashTimer_ = 0.32f;
        if (playerHealth_ <= 0) {
            playerDown_ = true;
            previousFireDown_ = false;
        }
    }
    playerDamageFlashTimer_ =
        std::max(0.0f, playerDamageFlashTimer_ - dt);
    parkingLotScene_.tryCollectAccessCard(camera_.position());
    parkingHintTimer_ = std::max(0.0f, parkingHintTimer_ - dt);
    const bool interactDown =
        glfwGetKey(window_, GLFW_KEY_E) == GLFW_PRESS;
    if (interactDown && !previousInteractDown_) {
        if (!parkingLotScene_.interactWithElevator(camera_.position()) &&
            parkingLotScene_.nearElevator(camera_.position()) &&
            !parkingLotScene_.accessCardObtained()) {
            parkingHintTimer_ = 2.4f;
        }
    }
    previousInteractDown_ = interactDown;
    previousRestartDown_ = restartDown;
    previousCrouchDown_ = crouchDown;
    previousProneDown_ = proneDown;
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

        std::size_t guardIndex = 0;
        float guardHitT = 0.0f;
        if (parkingLotScene_.segmentHitsGuard(previousPosition, nextPosition,
                                               guardHitT, guardIndex) &&
            guardHitT < closestHitT) {
            hit = true;
            hitType = ImpactType::Character;
            closestHitT = guardHitT;
        }

        float geometryHitT = 0.0f;
        if (parkingLotScene_.segmentHitsGeometry(previousPosition, nextPosition,
                                                  geometryHitT) &&
            geometryHitT < closestHitT) {
            hit = true;
            hitType = ImpactType::Geometry;
            closestHitT = geometryHitT;
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
                parkingLotScene_.applyGuardDamage(guardIndex, hitPosition);
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

bool MainScene::cameraPositionBlocked(const Vec3& position,
                                       const Vec3& currentPosition) const {
    return parkingLotScene_.cameraPositionBlocked(position, currentPosition);
}

void MainScene::renderFrame(const Camera& camera, bool showWeapon) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderScene();
    renderImpactEffects();

    for (const std::unique_ptr<BulletBase>& bullet : bullets_) {
        bullet->render();
    }

    if (showWeapon) {
        renderEquippedWeapon(camera);
        renderCrosshair();
    }

    if (state_ == AppState::Playing) {
        renderParkingHud();
        renderWeaponInventory();
    }
}

void MainScene::renderWeapon(const Camera& camera, WeaponType weapon,
                             const WeaponRenderMotion& motion) const {
    if (weapon == WeaponType::Knife) {
        knife_.render(camera, motion);
    } else {
        pistol_.render(camera, motion);
    }
}

void MainScene::renderEquippedWeapon(const Camera& camera) const {
    if (!weaponSwitching_) {
        renderWeapon(camera, equippedWeapon_, {});
        return;
    }

    const float progress = std::clamp(
        weaponSwitchElapsed_ / kWeaponSwitchDuration, 0.0f, 1.0f);
    const float eased = ThreeDUtils::smoothStep(progress);
    const WeaponRenderMotion outgoing{
        {0.0f, -1.05f * eased, 0.12f * eased},
        {7.0f * eased, -8.0f * eased, 6.0f * eased}};
    const float entering = 1.0f - eased;
    const WeaponRenderMotion incoming{
        {0.0f, -1.05f * entering, 0.16f * entering},
        {-7.0f * entering, 8.0f * entering, -6.0f * entering}};

    renderWeapon(camera, equippedWeapon_, outgoing);
    renderWeapon(camera, weaponSwitchTarget_, incoming);
}

void MainScene::renderScene() const {
    parkingLotScene_.render();
}

void MainScene::startLoading(Difficulty difficulty) {
    selectedDifficulty_ = difficulty;
    loadingElapsed_ = 0.0f;
    loadingUI_.setProgress(0.0f);
    state_ = AppState::Loading;
    glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void MainScene::updateLoading(float dt) {
    // Keep the loading screen visible long enough to communicate the scene
    // transition, then initialize the current playable scene.
    loadingElapsed_ += dt;
    const float loadingDuration = 1.25f;
    loadingUI_.setProgress(loadingElapsed_ / loadingDuration);
    if (loadingElapsed_ < loadingDuration) {
        return;
    }

    resetGame();
    state_ = AppState::Playing;
    previousFireDown_ =
        glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    previousJumpDown_ =
        glfwGetKey(window_, GLFW_KEY_SPACE) == GLFW_PRESS;
    previousInteractDown_ =
        glfwGetKey(window_, GLFW_KEY_E) == GLFW_PRESS;
    previousCrouchDown_ =
        glfwGetKey(window_, GLFW_KEY_C) == GLFW_PRESS;
    previousProneDown_ =
        glfwGetKey(window_, GLFW_KEY_Z) == GLFW_PRESS;
}

void MainScene::updateWeaponSelection(float dt) {
    const bool weapon1Down =
        numberKeyDown(window_, GLFW_KEY_1, GLFW_KEY_KP_1);
    const bool weapon2Down =
        numberKeyDown(window_, GLFW_KEY_2, GLFW_KEY_KP_2);

    if (weapon1Down && !previousWeapon1Down_) {
        requestWeapon(WeaponType::Knife);
    } else if (weapon2Down && !previousWeapon2Down_) {
        requestWeapon(WeaponType::Pistol);
    }

    previousWeapon1Down_ = weapon1Down;
    previousWeapon2Down_ = weapon2Down;

    if (!weaponSwitching_) {
        return;
    }

    weaponSwitchElapsed_ += dt;
    if (weaponSwitchElapsed_ < kWeaponSwitchDuration) {
        return;
    }

    equippedWeapon_ = weaponSwitchTarget_;
    weaponSwitchElapsed_ = kWeaponSwitchDuration;
    weaponSwitching_ = false;
    previousFireDown_ =
        glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
}

void MainScene::requestWeapon(WeaponType weapon) {
    if (weapon == equippedWeapon_ || weapon == weaponSwitchTarget_) {
        return;
    }

    weaponSwitchTarget_ = weapon;
    weaponSwitchElapsed_ = 0.0f;
    weaponSwitching_ = true;
}

void MainScene::renderExitPrompt() const {
    const UiText& text = uiText(language_);
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
        text.exitPromptTitle, layout.panel, std::max(1.0f, 4.0f * uiScale),
        kInkColor, 42.0f * uiScale);
    ThreeDUtils::drawCenteredText(
        text.exitPromptSubtitle, layout.panel,
        std::max(1.0f, 2.4f * uiScale),
        ThreeDUtils::shade(kInkColor, 1.2f), 104.0f * uiScale);

    drawPromptButton(layout.yesButton, text.confirmButton, kButtonExit,
                     contains(layout.yesButton, promptMouse_.x, promptMouse_.y),
                     uiScale);
    drawPromptButton(layout.noButton, text.cancelButton, kButtonStart,
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
        } else if (effect.type == ImpactType::Geometry) {
            renderGeometryImpactEffect(effect);
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

void MainScene::renderParkingHud() const {
    if (framebufferWidth_ <= 0 || framebufferHeight_ <= 0) {
        return;
    }

    const float uiScale =
        std::min(static_cast<float>(framebufferWidth_) /
                     static_cast<float>(kWindowWidth),
                 static_cast<float>(framebufferHeight_) /
                     static_cast<float>(kWindowHeight));
    const float margin = 22.0f * uiScale;
    const float panelWidth = std::min(350.0f * uiScale,
                                      static_cast<float>(framebufferWidth_) -
                                          margin * 2.0f);
    const float panelHeight = 196.0f * uiScale;
    const Rect panel{margin, margin, panelWidth, panelHeight};
    const Color panelColor{0.03f, 0.07f, 0.11f};
    const Color panelAccent{0.16f, 0.64f, 0.72f};
    const Color textColor{0.95f, 0.98f, 0.92f};
    const Color mutedColor{0.65f, 0.78f, 0.78f};
    const Color dangerColor{1.0f, 0.42f, 0.28f};
    const Color successColor{0.42f, 1.0f, 0.58f};
    const ParkingText& labels = parkingText(language_);

    ThreeDUtils::setUiProjection(framebufferWidth_, framebufferHeight_);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (playerDamageFlashTimer_ > 0.0f) {
        const float flashAlpha =
            std::min(0.28f, playerDamageFlashTimer_ * 0.95f);
        ThreeDUtils::drawRect2D(
            {0.0f, 0.0f, static_cast<float>(framebufferWidth_),
             static_cast<float>(framebufferHeight_)},
            dangerColor, flashAlpha);
    }

    ThreeDUtils::drawRect2D(
        {panel.x + 6.0f * uiScale, panel.y + 6.0f * uiScale, panel.width,
         panel.height},
        kInkColor, 0.45f);
    ThreeDUtils::drawRect2D(panel, panelColor, 0.88f);
    ThreeDUtils::drawRect2D(
        {panel.x, panel.y, panel.width, 7.0f * uiScale}, panelAccent, 0.95f);
    ThreeDUtils::drawFrame2D(panel, kInkColor, std::max(2.0f, 3.0f * uiScale));

    const float textScale = std::max(1.0f, 2.0f * uiScale);
    const float titleScale = std::max(1.0f, 2.5f * uiScale);
    ThreeDUtils::drawText(labels.title,
                          panel.x + 14.0f * uiScale,
                          panel.y + 17.0f * uiScale, titleScale, textColor);

    const std::string guardStatus =
        std::string(labels.securityLabel) +
        std::to_string(parkingLotScene_.livingGuardCount());
    const std::string captainStatus =
        std::string(labels.captainLabel) +
        (parkingLotScene_.captainDefeated() ? labels.captainDown
                                             : labels.captainAlive);
    const std::string cardStatus =
        std::string(labels.accessCardLabel) +
        (parkingLotScene_.accessCardObtained() ? labels.accessCardObtained
                                               : labels.accessCardRequired);
    ThreeDUtils::drawText(guardStatus, panel.x + 14.0f * uiScale,
                          panel.y + 51.0f * uiScale, textScale,
                          parkingLotScene_.livingGuardCount() == 0
                              ? successColor
                              : textColor);
    ThreeDUtils::drawText(captainStatus, panel.x + 170.0f * uiScale,
                          panel.y + 51.0f * uiScale, textScale,
                          parkingLotScene_.captainDefeated() ? successColor
                                                             : dangerColor);
    ThreeDUtils::drawText(cardStatus, panel.x + 14.0f * uiScale,
                          panel.y + 77.0f * uiScale, textScale,
                          parkingLotScene_.accessCardObtained() ? successColor
                                                                 : mutedColor);

    const std::string healthStatus =
        std::string(labels.healthLabel) + std::to_string(playerHealth_) + " / " +
        std::to_string(playerMaxHealth_);
    const float healthRatio =
        static_cast<float>(playerHealth_) /
        static_cast<float>(std::max(1, playerMaxHealth_));
    const Color healthColor =
        healthRatio > 0.55f
            ? successColor
            : (healthRatio > 0.25f ? Color{1.0f, 0.78f, 0.20f}
                                   : dangerColor);
    ThreeDUtils::drawText(healthStatus, panel.x + 14.0f * uiScale,
                          panel.y + 103.0f * uiScale, textScale,
                          healthColor);

    const Rect healthBar{
        panel.x + 14.0f * uiScale,
        panel.y + 124.0f * uiScale,
        panel.width - 28.0f * uiScale,
        13.0f * uiScale};
    ThreeDUtils::drawRect2D(
        {healthBar.x + 2.0f * uiScale,
         healthBar.y + 3.0f * uiScale,
         healthBar.width,
         healthBar.height},
        kInkColor, 0.42f);
    ThreeDUtils::drawRect2D(healthBar, kHealthBarBack, 0.95f);
    if (healthRatio > 0.0f) {
        ThreeDUtils::drawRect2D(
            {healthBar.x,
             healthBar.y,
             healthBar.width * healthRatio,
             healthBar.height},
            healthColor, 0.98f);
        ThreeDUtils::drawRect2D(
            {healthBar.x,
             healthBar.y,
             healthBar.width * healthRatio,
             std::max(1.0f, 2.0f * uiScale)},
            ThreeDUtils::shade(healthColor, 1.22f), 0.95f);
    }
    ThreeDUtils::drawFrame2D(
        healthBar, kInkColor, std::max(1.0f, 2.0f * uiScale));

    std::string prompt;
    Color promptColor = mutedColor;
    if (playerDown_) {
        prompt = labels.playerDownPrompt;
        promptColor = dangerColor;
    } else if (parkingLotScene_.levelComplete()) {
        prompt = labels.levelCompletePrompt;
        promptColor = successColor;
    } else if (parkingLotScene_.nearElevator(camera_.position()) &&
               !parkingLotScene_.accessCardObtained()) {
        prompt = parkingHintTimer_ > 0.0f
                     ? labels.elevatorCardPrompt
                     : std::string(labels.elevatorPrompt) +
                           "  (" + labels.elevatorCardHint + ")";
        promptColor = parkingHintTimer_ > 0.0f ? dangerColor : mutedColor;
    } else if (parkingLotScene_.nearElevator(camera_.position())) {
        prompt = labels.elevatorPrompt;
        promptColor = successColor;
    } else if (!parkingLotScene_.captainDefeated()) {
        prompt = labels.eliminateCaptainPrompt;
    } else if (!parkingLotScene_.accessCardObtained()) {
        prompt = labels.collectAccessCardPrompt;
    } else {
        prompt = labels.reachElevatorPrompt;
    }
    ThreeDUtils::drawText(prompt, panel.x + 14.0f * uiScale,
                          panel.y + 151.0f * uiScale, textScale, promptColor);

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    ThreeDUtils::setProjection(
        framebufferWidth_, framebufferHeight_,
        ThreeDUtils::currentFieldOfView(camera_.aimAmount()));
}

void MainScene::renderWeaponInventory() const {
    if (framebufferWidth_ <= 0 || framebufferHeight_ <= 0) {
        return;
    }

    const float uiScale =
        std::min(static_cast<float>(framebufferWidth_) /
                     static_cast<float>(kWindowWidth),
                 static_cast<float>(framebufferHeight_) /
                     static_cast<float>(kWindowHeight));
    const float slotSize = std::max(58.0f, 78.0f * uiScale);
    const float gap = std::max(6.0f, 10.0f * uiScale);
    const float margin = std::max(18.0f, 26.0f * uiScale);
    const float totalWidth = slotSize * 2.0f + gap;
    const float startX =
        static_cast<float>(framebufferWidth_) - margin - totalWidth;
    const float startY =
        static_cast<float>(framebufferHeight_) - margin - slotSize;
    const Rect knifeSlot{startX, startY, slotSize, slotSize};
    const Rect pistolSlot{startX + slotSize + gap, startY, slotSize, slotSize};

    ThreeDUtils::setUiProjection(framebufferWidth_, framebufferHeight_);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const auto drawSlot = [&](const Rect& slot, bool selected) {
        const Color fill = selected ? Color{0.10f, 0.30f, 0.34f}
                                    : Color{0.03f, 0.07f, 0.11f};
        const Color outline = selected ? kSunCore : kInkColor;
        ThreeDUtils::drawRect2D(
            {slot.x + 5.0f * uiScale, slot.y + 6.0f * uiScale,
             slot.width, slot.height},
            kInkColor, 0.45f);
        ThreeDUtils::drawRect2D(slot, fill, selected ? 0.96f : 0.84f);
        ThreeDUtils::drawRect2D(
            {slot.x, slot.y, slot.width, 5.0f * uiScale},
            selected ? kSunCore : kPanelBottom, 0.95f);
        ThreeDUtils::drawFrame2D(
            slot, outline, std::max(2.0f, (selected ? 4.0f : 2.0f) * uiScale));
    };

    drawSlot(knifeSlot, equippedWeapon_ == WeaponType::Knife);
    drawSlot(pistolSlot, equippedWeapon_ == WeaponType::Pistol);
    drawInventoryKnifeIcon(
        knifeSlot, uiScale,
        equippedWeapon_ == WeaponType::Knife ? 1.0f : 0.76f);
    drawInventoryPistolIcon(
        pistolSlot, uiScale,
        equippedWeapon_ == WeaponType::Pistol ? 1.0f : 0.76f);

    const float keyScale = std::max(1.0f, 2.0f * uiScale);
    const Color keyColor{0.95f, 0.98f, 0.92f};
    ThreeDUtils::drawText("1", knifeSlot.x + 8.0f * uiScale,
                          knifeSlot.y + 9.0f * uiScale, keyScale, keyColor);
    ThreeDUtils::drawText("2", pistolSlot.x + 8.0f * uiScale,
                          pistolSlot.y + 9.0f * uiScale, keyScale, keyColor);

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    ThreeDUtils::setProjection(
        framebufferWidth_, framebufferHeight_,
        ThreeDUtils::currentFieldOfView(camera_.aimAmount()));
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

void MainScene::performKnifeAttack() {
    Vec3 start{};
    Vec3 end{};
    knife_.attackSegment(camera_, start, end);

    bool hit = false;
    ImpactType hitType = ImpactType::Geometry;
    float closestHitT = 2.0f;

    std::size_t guardIndex = 0;
    float guardHitT = 0.0f;
    if (parkingLotScene_.segmentHitsGuard(start, end, guardHitT, guardIndex) &&
        guardHitT < closestHitT) {
        hit = true;
        hitType = ImpactType::Character;
        closestHitT = guardHitT;
    }

    float geometryHitT = 0.0f;
    if (parkingLotScene_.segmentHitsGeometry(start, end, geometryHitT) &&
        geometryHitT < closestHitT) {
        hit = true;
        hitType = ImpactType::Geometry;
        closestHitT = geometryHitT;
    }

    if (!hit) {
        return;
    }

    const Vec3 hitPosition = pointOnSegment(start, end, closestHitT);
    spawnImpactEffect(hitPosition, hitType);
    if (hitType == ImpactType::Character) {
        parkingLotScene_.applyGuardKnifeDamage(guardIndex, hitPosition);
    }
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

void MainScene::renderGeometryImpactEffect(
    const ImpactEffect& effect) const {
    const float t =
        std::clamp(effect.age / effect.duration, 0.0f, 1.0f);
    const float life = 1.0f - t;
    const float spread = 0.14f + 0.42f * t;
    const Vec3& base = effect.position;

    // Geometry impacts must stay at the actual AABB intersection point.
    // Unlike a ground impact, the surface can be at any world-space height.
    ThreeDUtils::drawCube(base, {0.16f * life, 0.16f * life, 0.16f * life},
                          kImpactSpark);

    const std::array<Vec3, 6> particles{
        Vec3{0.62f, 0.12f, 0.08f},  Vec3{-0.56f, 0.18f, -0.04f},
        Vec3{0.08f, 0.58f, 0.16f},  Vec3{-0.10f, -0.46f, 0.12f},
        Vec3{0.16f, 0.08f, -0.62f}, Vec3{-0.12f, 0.04f, 0.54f},
    };
    for (std::size_t i = 0; i < particles.size(); ++i) {
        const Vec3& particle = particles[i];
        const float size = (i % 2 == 0 ? 0.10f : 0.075f) * life;
        const Color color = i % 2 == 0 ? kImpactSpark : kImpactDust;
        ThreeDUtils::drawCube(
            {base.x + particle.x * spread,
             base.y + particle.y * spread,
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

void MainScene::updateWindowTitle(GLFWwindow* window, AppState state,
                                  Language language) {
    static std::string lastTitle;
    std::string title;
    const bool chinese = language == Language::Chinese;
    switch (state) {
        case AppState::MainMenu:
            title = chinese ? "Pixel World 3D | \xE4\xB8\xBB\xE8\x8F\x9C\xE5\x8D\x95"
                            : "Pixel World 3D | Main Menu";
            break;
        case AppState::DifficultySelect:
            title = chinese
                        ? "Pixel World 3D | \xE9\x80\x89\xE6\x8B\xA9\xE9\x9A\xBE\xE5\xBA\xA6"
                        : "Pixel World 3D | Select Difficulty";
            break;
        case AppState::Loading:
            title = chinese ? "Pixel World 3D | \xE5\x8A\xA0\xE8\xBD\xBD\xE4\xB8\xAD"
                            : "Pixel World 3D | Loading";
            break;
        case AppState::Playing:
            title = chinese
                        ? "Pixel World 3D | \xE5\x81\x9C\xE8\xBD\xA6\xE5\x9C\xBA"
                        : "Pixel World 3D | Mouse look | LMB slash | RMB aim | "
                          "E elevator | Shift run | Space jump/stand | C crouch | "
                          "Z prone | Arrows walk | WASD move | Esc exit";
            break;
    }
    if (title == lastTitle) {
        return;
    }

    glfwSetWindowTitle(window, title.c_str());
    lastTitle = title;
}

}  // namespace pixel_world
