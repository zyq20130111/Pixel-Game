#include "MainScene.h"

#include "AppConfig.h"
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
      parkingLotScene_(),
      pistol_(),
      mainUI_(language_),
      difficultyUI_(language_),
      loadingUI_(language_),
      selectedDifficulty_(Difficulty::Normal),
      loadingElapsed_(0.0f),
      bullets_(),
      impactEffects_(),
      previousFireDown_(false),
      previousJumpDown_(false),
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
    updateWindowTitle(window_, state_);
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
    pistol_.reset();
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
    previousInteractDown_ =
        glfwGetKey(window_, GLFW_KEY_E) == GLFW_PRESS;
    previousRestartDown_ =
        glfwGetKey(window_, GLFW_KEY_R) == GLFW_PRESS;
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
    }
}

void MainScene::updateGameplay(float dt) {
    if (parkingLotScene_.levelComplete()) {
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
        return;
    }

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
    const bool fireDown =
        glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    const bool playerFired = fireDown && !previousFireDown_;
    pistol_.update(window_, camera_, bullets_, soundManager_,
                   previousFireDown_, dt);
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

void MainScene::renderFrame(const Camera& camera, bool showPistol) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderScene();
    renderImpactEffects();

    for (const std::unique_ptr<BulletBase>& bullet : bullets_) {
        bullet->render();
    }

    if (showPistol) {
        pistol_.render(camera);
        renderCrosshair();
    }

    if (state_ == AppState::Playing) {
        renderParkingHud();
    }
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
    ThreeDUtils::drawText("PARKING LOT  /  OBJECTIVE",
                          panel.x + 14.0f * uiScale,
                          panel.y + 17.0f * uiScale, titleScale, textColor);

    const std::string guardStatus =
        "SECURITY: " + std::to_string(parkingLotScene_.livingGuardCount());
    const std::string captainStatus =
        std::string("CAPTAIN: ") +
        (parkingLotScene_.captainDefeated() ? "DOWN" : "ALIVE");
    const std::string cardStatus =
        std::string("ACCESS CARD: ") +
        (parkingLotScene_.accessCardObtained() ? "OBTAINED" : "REQUIRED");
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
        "HEALTH: " + std::to_string(playerHealth_) + " / " +
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
        prompt = "PLAYER DOWN  /  R: RESTART";
        promptColor = dangerColor;
    } else if (parkingLotScene_.levelComplete()) {
        prompt = "ELEVATOR OPEN  /  LEVEL COMPLETE";
        promptColor = successColor;
    } else if (parkingLotScene_.nearElevator(camera_.position()) &&
               !parkingLotScene_.accessCardObtained()) {
        prompt = parkingHintTimer_ > 0.0f
                     ? "ACCESS CARD REQUIRED"
                     : "E: USE ELEVATOR  (CARD REQUIRED)";
        promptColor = parkingHintTimer_ > 0.0f ? dangerColor : mutedColor;
    } else if (parkingLotScene_.nearElevator(camera_.position())) {
        prompt = "E: USE ELEVATOR";
        promptColor = successColor;
    } else if (!parkingLotScene_.captainDefeated()) {
        prompt = "ELIMINATE THE CAPTAIN";
    } else if (!parkingLotScene_.accessCardObtained()) {
        prompt = "COLLECT THE ACCESS CARD";
    } else {
        prompt = "REACH THE ELEVATOR";
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

void MainScene::updateWindowTitle(GLFWwindow* window, AppState state) {
    static std::string lastTitle;
    std::string title;
    switch (state) {
        case AppState::MainMenu:
            title = "Pixel World 3D | Main Menu";
            break;
        case AppState::DifficultySelect:
            title = "Pixel World 3D | Select Difficulty";
            break;
        case AppState::Loading:
            title = "Pixel World 3D | Loading";
            break;
        case AppState::Playing:
            title =
                "Pixel World 3D | Mouse look | LMB fire | RMB aim | E elevator | "
                "Shift run | Space jump | Arrows walk | WASD move | Esc exit";
            break;
    }
    if (title == lastTitle) {
        return;
    }

    glfwSetWindowTitle(window, title.c_str());
    lastTitle = title;
}

}  // namespace pixel_world
