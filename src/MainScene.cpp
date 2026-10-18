#include "MainScene.h"

#include "AppConfig.h"
#include "game_constants.h"
#include "ThreeDUtils.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <string>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#elif defined(__linux__)
#include <limits.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_NATIVE_INCLUDE_NONE
#include <GLFW/glfw3native.h>
#include <imm.h>
#endif

namespace pixel_world {
namespace {

using namespace constants;

std::filesystem::path executableDirectory() {
#ifdef _WIN32
    std::wstring modulePath(32768, L'\0');
    const DWORD length = GetModuleFileNameW(
        nullptr, modulePath.data(), static_cast<DWORD>(modulePath.size()));
    if (length > 0 && length < modulePath.size()) {
        modulePath.resize(length);
        return std::filesystem::path(modulePath).parent_path();
    }
#elif defined(__linux__)
    char modulePath[PATH_MAX] = {};
    const ssize_t length =
        readlink("/proc/self/exe", modulePath, sizeof(modulePath) - 1);
    if (length > 0) {
        modulePath[length] = '\0';
        return std::filesystem::path(modulePath).parent_path();
    }
#endif
    return {};
}

std::filesystem::path saveFilePath() {
    const std::filesystem::path moduleDirectory = executableDirectory();
    if (!moduleDirectory.empty()) {
        return moduleDirectory / "save.dat";
    }
    return std::filesystem::current_path() / "save.dat";
}

constexpr float kSniperScopeOverlayStart = 0.55f;
constexpr float kSniperScopeOverlayFade = 0.45f;
constexpr float kWarehouseJumpVelocity = 7.6f;

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

#ifdef _WIN32
void disableImeForWindow(GLFWwindow* window) {
    if (window == nullptr) {
        return;
    }

    const HWND handle = glfwGetWin32Window(window);
    if (handle != nullptr) {
        ImmAssociateContext(handle, nullptr);
    }
}
#endif

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

void drawRing(float centerX, float centerY, float innerRadius,
              float outerRadius, const Color& color, float alpha,
              int segments = 32) {
    glColor4f(color.red, color.green, color.blue, alpha);
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= segments; ++i) {
        const float angle =
            2.0f * constants::kPi * static_cast<float>(i) /
            static_cast<float>(segments);
        const float cosAngle = std::cos(angle);
        const float sinAngle = std::sin(angle);
        glVertex2f(centerX + cosAngle * outerRadius,
                   centerY + sinAngle * outerRadius);
        glVertex2f(centerX + cosAngle * innerRadius,
                   centerY + sinAngle * innerRadius);
    }
    glEnd();
}

void drawFilledCircle(float centerX, float centerY, float radius,
                      const Color& color, float alpha, int segments = 24) {
    glColor4f(color.red, color.green, color.blue, alpha);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(centerX, centerY);
    for (int i = 0; i <= segments; ++i) {
        const float angle =
            2.0f * constants::kPi * static_cast<float>(i) /
            static_cast<float>(segments);
        glVertex2f(centerX + std::cos(angle) * radius,
                   centerY + std::sin(angle) * radius);
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

void drawInventorySniperIcon(const Rect& slot, float uiScale, float alpha) {
    const float centerX = slot.x + slot.width * 0.5f;
    const float centerY = slot.y + slot.height * 0.47f;
    glPushMatrix();
    glTranslatef(centerX, centerY, 0.0f);
    glScalef(uiScale, uiScale, 1.0f);

    ThreeDUtils::drawRect2D({-40.0f, -8.0f, 22.0f, 17.0f},
                            kInkColor, alpha);
    ThreeDUtils::drawRect2D({-36.0f, -5.0f, 30.0f, 11.0f},
                            kSniperStock, alpha);
    ThreeDUtils::drawRect2D({-16.0f, -12.0f, 40.0f, 20.0f},
                            kInkColor, alpha);
    ThreeDUtils::drawRect2D({-13.0f, -9.0f, 35.0f, 14.0f},
                            kSniperMetal, alpha);
    ThreeDUtils::drawRect2D({20.0f, -6.0f, 30.0f, 8.0f},
                            kInkColor, alpha);
    ThreeDUtils::drawRect2D({22.0f, -4.0f, 30.0f, 4.0f},
                            kSniperBarrel, alpha);
    ThreeDUtils::drawRect2D({-6.0f, -18.0f, 21.0f, 6.0f},
                            kInkColor, alpha);
    ThreeDUtils::drawRect2D({-3.0f, -16.0f, 15.0f, 3.0f},
                            kSniperScope, alpha);
    ThreeDUtils::drawRect2D({-8.0f, 8.0f, 6.0f, 17.0f},
                            kInkColor, alpha);
    ThreeDUtils::drawRect2D({-5.0f, 10.0f, 4.0f, 12.0f},
                            kSniperMetalDark, alpha);
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
      securityCheckScene_(language_),
      warehouseScene_(language_),
      activeScene_(SceneType::ParkingLot),
      loadingDestination_(SceneType::ParkingLot),
      knife_(),
      pistol_(),
      sniperRifle_(),
      mainUI_(language_),
      difficultyUI_(language_),
      loadingUI_(language_),
      selectedDifficulty_(Difficulty::Normal),
      loadingElapsed_(0.0f),
      bullets_(),
      impactEffects_(),
      inventoryWeapons_(),
      checkpointWeapons_(),
      checkpointScene_(SceneType::ParkingLot),
      checkpointLoaded_(false),
      equippedWeapon_(WeaponType::Knife),
      weaponSwitchTarget_(WeaponType::Knife),
      weaponSwitchElapsed_(0.0f),
      weaponSwitching_(false),
      previousFireDown_(false),
      previousWeapon1Down_(false),
      previousWeapon2Down_(false),
      previousWeapon3Down_(false),
      previousJumpDown_(false),
      previousCrouchDown_(false),
      previousProneDown_(false),
      previousInteractDown_(false),
      previousRestartDown_(false),
      previousFpsToggleDown_(false),
      securityAlarmWasTriggered_(false),
      parkingHintTimer_(0.0f),
      playerHealth_(kCharacterMaxHp),
      playerMaxHealth_(kCharacterMaxHp),
      playerDamageFlashTimer_(0.0f),
      playerDown_(false),
      previousEscapeDown_(false),
      exitPromptVisible_(false),
      deathPromptVisible_(false),
      previousPromptMouseDown_(false),
      promptMouse_{0.0f, 0.0f, false},
      fpsDisplayVisible_(false),
      fps_(0.0f),
      fpsAccumulator_(0.0f),
      fpsFrameCount_(0) {
    loadCheckpointFromFile();
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
                mainUI_.update(window_, framebufferWidth_, framebufferHeight_);
            if (mainUI_.buttonClicked()) {
                soundManager_.play2D("btnclick.wav", 1.0f);
            }
            if (action == MenuAction::NewGame) {
                state_ = AppState::DifficultySelect;
            } else if (action == MenuAction::ContinueGame) {
                if (checkpointLoaded_) {
                    restoreCheckpoint();
                    startLoading(checkpointScene_, selectedDifficulty_);
                } else {
                    state_ = AppState::DifficultySelect;
                }
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
                startLoading(SceneType::ParkingLot, Difficulty::Easy);
            } else if (action == MenuAction::DifficultyNormal) {
                startLoading(SceneType::ParkingLot, Difficulty::Normal);
            } else if (action == MenuAction::DifficultyHard) {
                startLoading(SceneType::ParkingLot, Difficulty::Hard);
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
            if (!deathPromptVisible_) {
                updateExitPrompt();
            }
            if (!promptWasVisible && !exitPromptVisible_ &&
                !deathPromptVisible_) {
                updateGameplay(dt);
            }
            if (playerDown_ && !deathPromptVisible_ &&
                !exitPromptVisible_) {
                deathPromptVisible_ = true;
                glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                camera_.resetLookTracking(window_);
                previousPromptMouseDown_ =
                    glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_LEFT) ==
                    GLFW_PRESS;
            }
            if (deathPromptVisible_) {
                updateDeathPrompt();
            }

            ThreeDUtils::setProjection(
                framebufferWidth_, framebufferHeight_,
                ThreeDUtils::currentFieldOfView(camera_.aimAmount()));
            camera_.apply();
            renderFrame(camera_, true);
            if (exitPromptVisible_) {
                renderExitPrompt();
            } else if (deathPromptVisible_) {
                renderDeathPrompt();
            }
        }

        updateWindowTitle(window_, state_, language_, activeScene_);
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

    glfwWindowHint(GLFW_SAMPLES, 4);
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
#ifdef _WIN32
    disableImeForWindow(window_);
#endif
    glfwSetFramebufferSizeCallback(window_, ThreeDUtils::framebufferSizeCallback);
    glfwSwapInterval(1);
    ThreeDUtils::configureRendering();

    glfwGetFramebufferSize(window_, &framebufferWidth_, &framebufferHeight_);
    ThreeDUtils::framebufferSizeCallback(
        window_, framebufferWidth_, framebufferHeight_);
    updateWindowTitle(window_, state_, language_, activeScene_);
    return true;
}

void MainScene::resetGame() {
    activeScene_ = checkpointScene_;
    if (activeScene_ == SceneType::SecurityCheck) {
        securityCheckScene_.reset(selectedDifficulty_);
        camera_.reset(securityCheckScene_.spawnPosition());
    } else if (activeScene_ == SceneType::Warehouse) {
        warehouseScene_.reset(selectedDifficulty_);
        camera_.reset(warehouseScene_.spawnPosition());
        camera_.setJumpVelocity(kWarehouseJumpVelocity);
        camera_.setYawDegrees(180.0f);
    } else {
        parkingLotScene_.reset(selectedDifficulty_);
        camera_.reset(parkingLotScene_.spawnPosition());
    }
    soundManager_.stopAll();
    soundManager_.setListener(camera_.position(), camera_.forward(),
                              {0.0f, 1.0f, 0.0f});
    glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    camera_.resetLookTracking(window_);
    knife_.reset();
    pistol_.reset();
    sniperRifle_.reset();
    restoreCheckpoint();
    weaponSwitchTarget_ = equippedWeapon_;
    weaponSwitchElapsed_ = 0.0f;
    weaponSwitching_ = false;
    bullets_.clear();
    impactEffects_.clear();
    parkingHintTimer_ = 0.0f;
    securityAlarmWasTriggered_ = false;
    playerMaxHealth_ = kCharacterMaxHp;
    playerHealth_ = playerMaxHealth_;
    playerDamageFlashTimer_ = 0.0f;
    playerDown_ = false;
    exitPromptVisible_ = false;
    deathPromptVisible_ = false;
    previousEscapeDown_ =
        glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    previousWeapon1Down_ =
        numberKeyDown(window_, GLFW_KEY_1, GLFW_KEY_KP_1);
    previousWeapon2Down_ =
        numberKeyDown(window_, GLFW_KEY_2, GLFW_KEY_KP_2);
    previousWeapon3Down_ =
        numberKeyDown(window_, GLFW_KEY_3, GLFW_KEY_KP_3);
    previousInteractDown_ =
        glfwGetKey(window_, GLFW_KEY_E) == GLFW_PRESS;
    previousRestartDown_ =
        glfwGetKey(window_, GLFW_KEY_R) == GLFW_PRESS;
    previousFpsToggleDown_ =
        glfwGetKey(window_, GLFW_KEY_F1) == GLFW_PRESS;
    previousJumpDown_ =
        glfwGetKey(window_, GLFW_KEY_SPACE) == GLFW_PRESS;
    previousCrouchDown_ =
        glfwGetKey(window_, GLFW_KEY_C) == GLFW_PRESS;
    previousProneDown_ =
        glfwGetKey(window_, GLFW_KEY_Z) == GLFW_PRESS;
    previousPromptMouseDown_ =
        glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
}

int MainScene::weaponId(WeaponType weapon) {
    switch (weapon) {
        case WeaponType::Knife:
            return 1;
        case WeaponType::Pistol:
            return 2;
        case WeaponType::Sniper:
            return 3;
    }
    return 0;
}

MainScene::WeaponType MainScene::weaponFromId(int id) {
    switch (id) {
        case 1:
            return WeaponType::Knife;
        case 2:
            return WeaponType::Pistol;
        case 3:
            return WeaponType::Sniper;
    }
    return WeaponType::Knife;
}

std::string MainScene::weaponDescription(WeaponType weapon,
                                         Language language) {
    if (language == Language::Chinese) {
        switch (weapon) {
            case WeaponType::Knife:
                return "\xE5\x8C\x95\xE9\xA6\x96";
            case WeaponType::Pistol:
                return "\xE6\x89\x8B\xE6\x9E\xAA";
            case WeaponType::Sniper:
                return "\xE7\x8B\x99\xE5\x87\xBB\xE6\x9E\xAA";
        }
    }

    switch (weapon) {
        case WeaponType::Knife:
            return "Knife";
        case WeaponType::Pistol:
            return "Pistol";
        case WeaponType::Sniper:
            return "Sniper Rifle";
    }
    return "Knife";
}

void MainScene::saveCheckpoint(SceneType scene) {
    checkpointScene_ = scene;
    checkpointWeapons_.clear();

    if (scene == SceneType::ParkingLot) {
        checkpointWeapons_.push_back(
            {weaponId(WeaponType::Knife),
             weaponDescription(WeaponType::Knife, language_)});
    } else {
        for (WeaponType weapon : inventoryWeapons_) {
            checkpointWeapons_.push_back(
                {weaponId(weapon), weaponDescription(weapon, language_)});
        }
    }
    if (checkpointWeapons_.empty()) {
        checkpointWeapons_.push_back(
            {weaponId(WeaponType::Knife),
             weaponDescription(WeaponType::Knife, language_)});
    }

    int sceneId = 0;
    switch (checkpointScene_) {
        case SceneType::ParkingLot:
            sceneId = 1;
            break;
        case SceneType::SecurityCheck:
            sceneId = 2;
            break;
        case SceneType::Warehouse:
            sceneId = 3;
            break;
    }

    std::ofstream output(saveFilePath());
    if (!output) {
        return;
    }
    output << "scene=" << sceneId << '\n';
    output << "difficulty=" << static_cast<int>(selectedDifficulty_) << '\n';
    for (const SavedWeapon& weapon : checkpointWeapons_) {
        output << "weapon=" << weapon.id << ',' << weapon.description << '\n';
    }
    checkpointLoaded_ = true;
}

bool MainScene::loadCheckpointFromFile() {
    std::ifstream input(saveFilePath());
    if (!input) {
        return false;
    }

    int sceneId = 0;
    int difficultyId = -1;
    std::vector<SavedWeapon> weapons;
    std::string line;
    try {
        while (std::getline(input, line)) {
            if (line.rfind("scene=", 0) == 0) {
                sceneId = std::stoi(line.substr(6));
            } else if (line.rfind("difficulty=", 0) == 0) {
                difficultyId = std::stoi(line.substr(11));
            } else if (line.rfind("weapon=", 0) == 0) {
                const std::string payload = line.substr(7);
                const std::size_t separator = payload.find(',');
                const int id =
                    std::stoi(payload.substr(0, separator));
                const std::string description =
                    separator == std::string::npos
                        ? std::string()
                        : payload.substr(separator + 1);
                weapons.push_back({id, description});
            }
        }
    } catch (...) {
        return false;
    }

    if (sceneId < 1 || sceneId > 3) {
        return false;
    }
    switch (sceneId) {
        case 1:
            checkpointScene_ = SceneType::ParkingLot;
            break;
        case 2:
            checkpointScene_ = SceneType::SecurityCheck;
            break;
        case 3:
            checkpointScene_ = SceneType::Warehouse;
            break;
        default:
            return false;
    }

    selectedDifficulty_ =
        difficultyId == 0
            ? Difficulty::Easy
            : (difficultyId == 2 ? Difficulty::Hard : Difficulty::Normal);

    if (weapons.empty()) {
        weapons.push_back(
            {weaponId(WeaponType::Knife),
             weaponDescription(WeaponType::Knife, language_)});
    }
    checkpointWeapons_ = weapons;

    inventoryWeapons_.clear();
    for (const SavedWeapon& weapon : checkpointWeapons_) {
        inventoryWeapons_.push_back(weaponFromId(weapon.id));
    }
    if (inventoryWeapons_.empty()) {
        inventoryWeapons_.push_back(WeaponType::Knife);
    }
    equippedWeapon_ = inventoryWeapons_.front();
    checkpointLoaded_ = true;
    return true;
}

void MainScene::restoreCheckpoint() {
    inventoryWeapons_.clear();
    for (const SavedWeapon& saved : checkpointWeapons_) {
        inventoryWeapons_.push_back(weaponFromId(saved.id));
    }
    if (inventoryWeapons_.empty()) {
        inventoryWeapons_.push_back(WeaponType::Knife);
    }
    equippedWeapon_ = inventoryWeapons_.front();
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

void MainScene::updateDeathPrompt() {
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
        deathPromptVisible_ = false;
        resetGame();
    } else if (contains(layout.noButton, promptMouse_.x, promptMouse_.y)) {
        soundManager_.play2D("btnclick.wav", 1.0f);
        soundManager_.stopAll();
        deathPromptVisible_ = false;
        exitPromptVisible_ = false;
        state_ = AppState::MainMenu;
        mainUI_.showMainMenu();
        glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

void MainScene::updateGameplay(float dt) {
    const bool fpsToggleDown =
        glfwGetKey(window_, GLFW_KEY_F1) == GLFW_PRESS;
    if (fpsToggleDown && !previousFpsToggleDown_) {
        fpsDisplayVisible_ = !fpsDisplayVisible_;
    }
    previousFpsToggleDown_ = fpsToggleDown;

    fpsAccumulator_ += dt;
    fpsFrameCount_ += 1;
    if (fpsAccumulator_ >= 0.5f) {
        fps_ = static_cast<float>(fpsFrameCount_) / fpsAccumulator_;
        fpsAccumulator_ = 0.0f;
        fpsFrameCount_ = 0;
    }

    if (activeScene_ == SceneType::SecurityCheck) {
        updateSecurityGameplay(dt);
        return;
    }

    if (activeScene_ == SceneType::Warehouse) {
        updateWarehouseGameplay(dt);
        return;
    }

    const bool crouchDown = glfwGetKey(window_, GLFW_KEY_C) == GLFW_PRESS;
    const bool proneDown = glfwGetKey(window_, GLFW_KEY_Z) == GLFW_PRESS;
    const bool crouchPressed = crouchDown && !previousCrouchDown_;
    const bool pronePressed = proneDown && !previousProneDown_;
    if (parkingLotScene_.levelComplete()) {
        startLoading(SceneType::SecurityCheck, selectedDifficulty_);
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
    } else if (equippedWeapon_ == WeaponType::Pistol) {
        if (weaponSwitching_) {
            previousFireDown_ = true;
        }
        playerAttacked =
            pistol_.update(window_, camera_, bullets_, soundManager_,
                           previousFireDown_, dt);
    } else {
        if (weaponSwitching_) {
            previousFireDown_ = true;
        }
        playerAttacked =
            sniperRifle_.update(window_, camera_, bullets_, soundManager_,
                                previousFireDown_, dt);
    }
    if (playerAttacked && equippedWeapon_ == WeaponType::Knife) {
        performKnifeAttack();
    }
    const bool playerFired =
        playerAttacked && (equippedWeapon_ == WeaponType::Pistol ||
                           equippedWeapon_ == WeaponType::Sniper);
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

    if (parkingLotScene_.sniperFired()) {
        soundManager_.play3D("sniper.wav",
                             parkingLotScene_.sniperFiredPosition(), 1.0f);
    }

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

void MainScene::updateSecurityGameplay(float dt) {
    const bool alarmNow = securityCheckScene_.alarmTriggered();
    if (alarmNow && !securityAlarmWasTriggered_) {
        soundManager_.play2D("alarm.wav", 1.0f);
    }
    securityAlarmWasTriggered_ = alarmNow;

    const bool crouchDown = glfwGetKey(window_, GLFW_KEY_C) == GLFW_PRESS;
    const bool proneDown = glfwGetKey(window_, GLFW_KEY_Z) == GLFW_PRESS;
    const bool crouchPressed = crouchDown && !previousCrouchDown_;
    const bool pronePressed = proneDown && !previousProneDown_;
    const bool restartDown = glfwGetKey(window_, GLFW_KEY_R) == GLFW_PRESS;

    if (playerDown_) {
        if (restartDown && !previousRestartDown_) {
            resetGame();
        }
        previousRestartDown_ = restartDown;
        previousCrouchDown_ = crouchDown;
        previousProneDown_ = proneDown;
        return;
    }

    if (securityCheckScene_.alarmTriggered() && restartDown &&
        !previousRestartDown_) {
        resetGame();
        previousRestartDown_ = restartDown;
        previousCrouchDown_ = crouchDown;
        previousProneDown_ = proneDown;
        return;
    }

    if (securityCheckScene_.levelComplete()) {
        startLoading(SceneType::Warehouse, selectedDifficulty_);
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

    updateWeaponSelection(dt);
    bool playerAttacked = false;
    if (equippedWeapon_ == WeaponType::Knife) {
        if (weaponSwitching_) {
            previousFireDown_ = true;
        }
        playerAttacked =
            knife_.update(window_, camera_, bullets_, soundManager_,
                          previousFireDown_, dt);
    } else if (equippedWeapon_ == WeaponType::Pistol) {
        if (weaponSwitching_) {
            previousFireDown_ = true;
        }
        playerAttacked =
            pistol_.update(window_, camera_, bullets_, soundManager_,
                           previousFireDown_, dt);
    } else {
        if (weaponSwitching_) {
            previousFireDown_ = true;
        }
        playerAttacked =
            sniperRifle_.update(window_, camera_, bullets_, soundManager_,
                                previousFireDown_, dt);
    }
    if (playerAttacked && equippedWeapon_ == WeaponType::Knife) {
        performKnifeAttack();
    }

    const bool interactDown =
        glfwGetKey(window_, GLFW_KEY_E) == GLFW_PRESS;
    if (interactDown && !previousInteractDown_) {
        if (securityCheckScene_.interactWithSwitch(camera_.position())) {
            soundManager_.play2D("btnclick.wav", 1.0f);
        }
    }
    previousInteractDown_ = interactDown;
    previousRestartDown_ = restartDown;
    previousCrouchDown_ = crouchDown;
    previousProneDown_ = proneDown;

    const int playerDamage =
        securityCheckScene_.update(dt, camera_.position());
    if (playerDamage > 0) {
        playerHealth_ = std::max(0, playerHealth_ - playerDamage);
        playerDamageFlashTimer_ = 0.32f;
        if (playerHealth_ <= 0) {
            playerDown_ = true;
        }
    }
    playerDamageFlashTimer_ =
        std::max(0.0f, playerDamageFlashTimer_ - dt);

    if (securityCheckScene_.sniperFired()) {
        soundManager_.play3D("sniper.wav",
                             securityCheckScene_.sniperFiredPosition(), 1.0f);
    }

    updateImpactEffects(dt);
    updateBullets(dt);
}

void MainScene::updateWarehouseGameplay(float dt) {
    const bool crouchDown = glfwGetKey(window_, GLFW_KEY_C) == GLFW_PRESS;
    const bool proneDown = glfwGetKey(window_, GLFW_KEY_Z) == GLFW_PRESS;
    const bool crouchPressed = crouchDown && !previousCrouchDown_;
    const bool pronePressed = proneDown && !previousProneDown_;
    const bool restartDown = glfwGetKey(window_, GLFW_KEY_R) == GLFW_PRESS;
    const bool interactDown =
        glfwGetKey(window_, GLFW_KEY_E) == GLFW_PRESS;
    const bool jumpDown =
        glfwGetKey(window_, GLFW_KEY_SPACE) == GLFW_PRESS;

    if (warehouseScene_.drivingForklift()) {
        const bool accelerate =
            glfwGetKey(window_, GLFW_KEY_W) == GLFW_PRESS;
        const bool reverse =
            glfwGetKey(window_, GLFW_KEY_S) == GLFW_PRESS;
        const bool steerLeft =
            glfwGetKey(window_, GLFW_KEY_A) == GLFW_PRESS;
        const bool steerRight =
            glfwGetKey(window_, GLFW_KEY_D) == GLFW_PRESS;
        warehouseScene_.updateForklift(dt, accelerate, reverse, steerLeft,
                                       steerRight);
        if (warehouseScene_.doorCrashed()) {
            soundManager_.play3D("crash.wav",
                                 warehouseScene_.doorCrashPosition(), 1.0f);
            spawnImpactEffect(warehouseScene_.doorCrashPosition(),
                              ImpactType::Geometry);
        }
        camera_.setPosition(warehouseScene_.forkliftSeatPosition());
        camera_.setYawDegrees(warehouseScene_.forkliftHeadingDegrees());
        camera_.setPitchDegrees(0.0f);
        camera_.setPosture(Camera::Posture::Standing);
        camera_.updateAim(window_, dt);
        soundManager_.setListener(camera_.position(), camera_.forward(),
                                  {0.0f, 1.0f, 0.0f});
        soundManager_.update();

        if (interactDown && !previousInteractDown_) {
            warehouseScene_.exitForklift();
            camera_.setPosition(warehouseScene_.forkliftExitPosition());
            camera_.setYawDegrees(warehouseScene_.forkliftHeadingDegrees());
            camera_.setPitchDegrees(0.0f);
            camera_.resetLookTracking(window_);
            soundManager_.play2D("btnclick.wav", 1.0f);
        }
        previousInteractDown_ = interactDown;
        previousRestartDown_ = restartDown;
        previousCrouchDown_ = crouchDown;
        previousProneDown_ = proneDown;
        previousJumpDown_ = jumpDown;
        updateImpactEffects(dt);
        updateBullets(dt);
        return;
    }

    if (playerDown_) {
        if (restartDown && !previousRestartDown_) {
            resetGame();
        }
        previousRestartDown_ = restartDown;
        previousCrouchDown_ = crouchDown;
        previousProneDown_ = proneDown;
        previousJumpDown_ = jumpDown;
        return;
    }

    if (warehouseScene_.levelComplete()) {
        previousRestartDown_ = restartDown;
        previousCrouchDown_ = crouchDown;
        previousProneDown_ = proneDown;
        previousJumpDown_ = jumpDown;
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

    camera_.updateLook(window_);
    camera_.update(
        window_, dt, previousJumpDown_,
        [this](const Vec3& position) {
            return cameraPositionBlocked(position, camera_.position());
        },
        [this](const Vec3& position) {
            return warehouseScene_.groundHeightAt(position);
        });

    camera_.updateAim(window_, dt);
    soundManager_.setListener(camera_.position(), camera_.forward(),
                              {0.0f, 1.0f, 0.0f});
    soundManager_.update();

    updateWeaponSelection(dt);
    bool playerAttacked = false;
    if (equippedWeapon_ == WeaponType::Knife) {
        if (weaponSwitching_) {
            previousFireDown_ = true;
        }
        playerAttacked =
            knife_.update(window_, camera_, bullets_, soundManager_,
                          previousFireDown_, dt);
    } else if (equippedWeapon_ == WeaponType::Pistol) {
        if (weaponSwitching_) {
            previousFireDown_ = true;
        }
        playerAttacked =
            pistol_.update(window_, camera_, bullets_, soundManager_,
                           previousFireDown_, dt);
    } else {
        if (weaponSwitching_) {
            previousFireDown_ = true;
        }
        playerAttacked =
            sniperRifle_.update(window_, camera_, bullets_, soundManager_,
                                previousFireDown_, dt);
    }
    if (playerAttacked && equippedWeapon_ == WeaponType::Knife) {
        performKnifeAttack();
    }

    if (interactDown && !previousInteractDown_) {
        if (warehouseScene_.nearSniperCrate(camera_.position()) &&
            !warehouseScene_.sniperRifleTaken()) {
            if (warehouseScene_.interactWithSniperCrate(
                    camera_.position())) {
                inventoryWeapons_.push_back(WeaponType::Sniper);
                requestWeapon(WeaponType::Sniper);
                soundManager_.play2D("btnclick.wav", 1.0f);
            }
        } else if (warehouseScene_.nearForklift(camera_.position()) &&
                   warehouseScene_.enterForklift(camera_.position())) {
            camera_.resetLookTracking(window_);
            camera_.setPosition(warehouseScene_.forkliftSeatPosition());
            camera_.setYawDegrees(warehouseScene_.forkliftHeadingDegrees());
            camera_.setPitchDegrees(0.0f);
            soundManager_.play2D("btnclick.wav", 1.0f);
        }
    }
    previousInteractDown_ = interactDown;
    previousRestartDown_ = restartDown;
    previousCrouchDown_ = crouchDown;
    previousProneDown_ = proneDown;

    const int playerDamage =
        warehouseScene_.update(dt, camera_.position());
    if (playerDamage > 0) {
        playerHealth_ = std::max(0, playerHealth_ - playerDamage);
        playerDamageFlashTimer_ = 0.32f;
        if (playerHealth_ <= 0) {
            playerDown_ = true;
        }
    }
    playerDamageFlashTimer_ =
        std::max(0.0f, playerDamageFlashTimer_ - dt);

    if (warehouseScene_.sniperFired()) {
        soundManager_.play3D("sniper.wav",
                             warehouseScene_.sniperFiredPosition(), 1.0f);
    }

    updateImpactEffects(dt);
    updateBullets(dt);
}

bool MainScene::sceneSegmentHitsGuard(const Vec3& start, const Vec3& end,
                                      float& hitT,
                                      std::size_t& guardIndex) const {
    if (activeScene_ == SceneType::SecurityCheck) {
        return securityCheckScene_.segmentHitsGuard(start, end, hitT,
                                                    guardIndex);
    }
    if (activeScene_ == SceneType::Warehouse) {
        return warehouseScene_.segmentHitsGuard(start, end, hitT, guardIndex);
    }
    return parkingLotScene_.segmentHitsGuard(start, end, hitT, guardIndex);
}

bool MainScene::sceneSegmentHitsGeometry(const Vec3& start, const Vec3& end,
                                         float& hitT) const {
    if (activeScene_ == SceneType::SecurityCheck) {
        return securityCheckScene_.segmentHitsGeometry(start, end, hitT);
    }
    if (activeScene_ == SceneType::Warehouse) {
        return warehouseScene_.segmentHitsGeometry(start, end, hitT);
    }
    return parkingLotScene_.segmentHitsGeometry(start, end, hitT);
}

void MainScene::sceneApplyGuardDamage(std::size_t guardIndex,
                                      const Vec3& hitPosition) {
    if (activeScene_ == SceneType::SecurityCheck) {
        securityCheckScene_.applyGuardDamage(guardIndex, hitPosition);
    } else if (activeScene_ == SceneType::Warehouse) {
        warehouseScene_.applyGuardDamage(guardIndex, hitPosition);
    } else {
        parkingLotScene_.applyGuardDamage(guardIndex, hitPosition);
    }
}

void MainScene::sceneApplyGuardSniperDamage(std::size_t guardIndex,
                                            const Vec3& hitPosition) {
    if (activeScene_ == SceneType::SecurityCheck) {
        securityCheckScene_.applyGuardSniperDamage(guardIndex, hitPosition);
    } else if (activeScene_ == SceneType::Warehouse) {
        warehouseScene_.applyGuardSniperDamage(guardIndex, hitPosition);
    } else {
        parkingLotScene_.applyGuardSniperDamage(guardIndex, hitPosition);
    }
}

void MainScene::sceneApplyGuardKnifeDamage(std::size_t guardIndex,
                                           const Vec3& hitPosition) {
    if (activeScene_ == SceneType::SecurityCheck) {
        securityCheckScene_.applyGuardKnifeDamage(guardIndex, hitPosition);
    } else if (activeScene_ == SceneType::Warehouse) {
        warehouseScene_.applyGuardKnifeDamage(guardIndex, hitPosition);
    } else {
        parkingLotScene_.applyGuardKnifeDamage(guardIndex, hitPosition);
    }
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
        if (sceneSegmentHitsGuard(previousPosition, nextPosition,
                                      guardHitT, guardIndex) &&
            guardHitT < closestHitT) {
            hit = true;
            hitType = ImpactType::Character;
            closestHitT = guardHitT;
        }

        float geometryHitT = 0.0f;
        if (sceneSegmentHitsGeometry(previousPosition, nextPosition,
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
                if (bullet->type() == BulletType::Sniper) {
                    sceneApplyGuardSniperDamage(guardIndex, hitPosition);
                } else {
                    sceneApplyGuardDamage(guardIndex, hitPosition);
                }
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
    if (activeScene_ == SceneType::SecurityCheck) {
        return securityCheckScene_.cameraPositionBlocked(position, currentPosition);
    }
    if (activeScene_ == SceneType::Warehouse) {
        return warehouseScene_.cameraPositionBlocked(position, currentPosition);
    }
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
        const bool sniperScoped =
            equippedWeapon_ == WeaponType::Sniper && !weaponSwitching_ &&
            camera.aimAmount() > kSniperScopeOverlayStart;
        if (sniperScoped) {
            renderSniperScopeOverlay(camera);
        } else {
            renderEquippedWeapon(camera);
            renderCrosshair();
        }
    }

    if (state_ == AppState::Playing) {
        if (activeScene_ == SceneType::SecurityCheck) {
            renderSecurityHud();
        } else if (activeScene_ == SceneType::Warehouse) {
            renderWarehouseHud();
        } else {
            renderParkingHud();
        }
        renderWeaponInventory();
        renderFpsOverlay();
    }
}

void MainScene::renderWeapon(const Camera& camera, WeaponType weapon,
                             const WeaponRenderMotion& motion) const {
    if (weapon == WeaponType::Knife) {
        knife_.render(camera, motion);
    } else if (weapon == WeaponType::Pistol) {
        pistol_.render(camera, motion);
    } else {
        sniperRifle_.render(camera, motion);
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
    if (activeScene_ == SceneType::SecurityCheck) {
        securityCheckScene_.render();
    } else if (activeScene_ == SceneType::Warehouse) {
        warehouseScene_.render();
    } else {
        parkingLotScene_.render();
    }
}

void MainScene::startLoading(SceneType destination, Difficulty difficulty) {
    loadingDestination_ = destination;
    selectedDifficulty_ = difficulty;
    saveCheckpoint(destination);
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
    const bool weapon3Down =
        numberKeyDown(window_, GLFW_KEY_3, GLFW_KEY_KP_3);

    const bool weaponDowns[] = {weapon1Down, weapon2Down, weapon3Down};
    const bool previousWeaponDowns[] = {
        previousWeapon1Down_, previousWeapon2Down_, previousWeapon3Down_};
    for (std::size_t index = 0; index < 3; ++index) {
        if (weaponDowns[index] && !previousWeaponDowns[index] &&
            index < inventoryWeapons_.size()) {
            requestWeapon(inventoryWeapons_[index]);
        }
    }

    previousWeapon1Down_ = weapon1Down;
    previousWeapon2Down_ = weapon2Down;
    previousWeapon3Down_ = weapon3Down;

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

void MainScene::renderDeathPrompt() const {
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
        text.deathPromptTitle, layout.panel, std::max(1.0f, 4.0f * uiScale),
        kInkColor, 42.0f * uiScale);
    ThreeDUtils::drawCenteredText(
        text.deathPromptSubtitle, layout.panel,
        std::max(1.0f, 2.4f * uiScale),
        ThreeDUtils::shade(kInkColor, 1.2f), 104.0f * uiScale);

    drawPromptButton(layout.yesButton, text.retryButton, kButtonStart,
                     contains(layout.yesButton, promptMouse_.x, promptMouse_.y),
                     uiScale);
    drawPromptButton(layout.noButton, text.mainMenuButton, kButtonExit,
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

void MainScene::renderSniperScopeOverlay(const Camera& camera) const {
    if (framebufferWidth_ <= 0 || framebufferHeight_ <= 0) {
        return;
    }

    const float aimAmount = camera.aimAmount();
    if (aimAmount <= kSniperScopeOverlayStart) {
        return;
    }

    const float scopeAmount =
        ThreeDUtils::smoothStep((aimAmount - kSniperScopeOverlayStart) /
                                kSniperScopeOverlayFade);
    if (scopeAmount <= 0.0f) {
        return;
    }

    const float width = static_cast<float>(framebufferWidth_);
    const float height = static_cast<float>(framebufferHeight_);
    const float uiScale =
        std::min(width / static_cast<float>(kWindowWidth),
                 height / static_cast<float>(kWindowHeight));
    const float centerX = width * 0.5f;
    const float centerY = height * 0.5f;
    const float lensRadius = std::min(width, height) * 0.42f;

    const float texScale = 1024.0f / std::max(width, height);
    const int texWidth = std::max(
        1, static_cast<int>(std::round(width * texScale)));
    const int texHeight = std::max(
        1, static_cast<int>(std::round(height * texScale)));

    static GLuint scopeTexture = 0;
    static int cachedTexWidth = 0;
    static int cachedTexHeight = 0;
    if (scopeTexture == 0 || cachedTexWidth != texWidth ||
        cachedTexHeight != texHeight) {
        if (scopeTexture != 0) {
            glDeleteTextures(1, &scopeTexture);
        }

        const float fadeWidth =
            std::max(18.0f * uiScale, lensRadius * 0.14f);
        std::vector<unsigned char> pixels(
            static_cast<std::size_t>(texWidth) * texHeight * 4u);
        for (int y = 0; y < texHeight; ++y) {
            for (int x = 0; x < texWidth; ++x) {
                const float screenX =
                    (static_cast<float>(x) + 0.5f) /
                    static_cast<float>(texWidth) * width;
                const float screenY =
                    (static_cast<float>(y) + 0.5f) /
                    static_cast<float>(texHeight) * height;
                const float dx = screenX - centerX;
                const float dy = screenY - centerY;
                const float radius = std::sqrt(dx * dx + dy * dy);
                const float t = std::clamp((radius - lensRadius) / fadeWidth,
                                           0.0f, 1.0f);
                const unsigned char alpha = static_cast<unsigned char>(
                    std::round(ThreeDUtils::smoothStep(t) * 255.0f));
                const std::size_t index =
                    (static_cast<std::size_t>(y) * texWidth + x) * 4u;
                pixels[index] = 0;
                pixels[index + 1] = 0;
                pixels[index + 2] = 0;
                pixels[index + 3] = alpha;
            }
        }

        glGenTextures(1, &scopeTexture);
        glBindTexture(GL_TEXTURE_2D, scopeTexture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glBindTexture(GL_TEXTURE_2D, 0);

        cachedTexWidth = texWidth;
        cachedTexHeight = texHeight;
    }

    ThreeDUtils::setUiProjection(framebufferWidth_, framebufferHeight_);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, scopeTexture);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glColor4f(0.0f, 0.0f, 0.0f, scopeAmount);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(0.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(width, 0.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(width, height);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(0.0f, height);
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);

    const float pixel = std::max(2.0f, std::round(2.5f * uiScale));
    const float thickness = pixel;
    const float armLength = lensRadius * 0.32f;
    const float gap = lensRadius * 0.055f;
    const float halfThickness = thickness * 0.5f;

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
            kCrosshairShadow, 0.55f * scopeAmount);
    }
    for (const Rect& piece : pieces) {
        ThreeDUtils::drawRect2D(piece, kCrosshairCore,
                                0.92f * scopeAmount);
    }

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    ThreeDUtils::setProjection(
        framebufferWidth_, framebufferHeight_,
        ThreeDUtils::currentFieldOfView(camera_.aimAmount()));
}

void MainScene::renderLowHealthOverlay() const {
    const float healthRatio =
        static_cast<float>(playerHealth_) /
        static_cast<float>(std::max(1, playerMaxHealth_));
    if (healthRatio >= 0.20f) {
        return;
    }

    const float intensity = 1.0f - healthRatio / 0.20f;
    const float cornerAlpha = 0.62f * intensity;
    if (cornerAlpha <= 0.0f) {
        return;
    }

    const float width = static_cast<float>(framebufferWidth_);
    const float height = static_cast<float>(framebufferHeight_);
    const float centerX = width * 0.5f;
    const float centerY = height * 0.5f;

    glShadeModel(GL_SMOOTH);
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(0.82f, 0.04f, 0.02f, 0.0f);
    glVertex2f(centerX, centerY);
    glColor4f(0.82f, 0.04f, 0.02f, cornerAlpha);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(width, 0.0f);
    glVertex2f(width, height);
    glVertex2f(0.0f, height);
    glEnd();
    glShadeModel(GL_FLAT);
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

    renderLowHealthOverlay();

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

void MainScene::renderSecurityHud() const {
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
    const float panelHeight = 164.0f * uiScale;
    const Rect panel{margin, margin, panelWidth, panelHeight};
    const Color panelColor{0.03f, 0.07f, 0.11f};
    const Color panelAccent{0.16f, 0.64f, 0.72f};
    const Color textColor{0.95f, 0.98f, 0.92f};
    const Color mutedColor{0.65f, 0.78f, 0.78f};
    const Color dangerColor{1.0f, 0.42f, 0.28f};
    const Color successColor{0.42f, 1.0f, 0.58f};
    const SecurityText& labels = securityText(language_);

    ThreeDUtils::setUiProjection(framebufferWidth_, framebufferHeight_);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (securityCheckScene_.alarmTriggered()) {
        const float flashAlpha =
            0.10f + 0.18f * securityCheckScene_.alarmFlashAmount();
        ThreeDUtils::drawRect2D(
            {0.0f, 0.0f, static_cast<float>(framebufferWidth_),
             static_cast<float>(framebufferHeight_)},
            dangerColor, flashAlpha);
    }

    if (playerDamageFlashTimer_ > 0.0f) {
        const float flashAlpha =
            std::min(0.28f, playerDamageFlashTimer_ * 0.95f);
        ThreeDUtils::drawRect2D(
            {0.0f, 0.0f, static_cast<float>(framebufferWidth_),
             static_cast<float>(framebufferHeight_)},
            dangerColor, flashAlpha);
    }

    renderLowHealthOverlay();

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
    ThreeDUtils::drawText(labels.title, panel.x + 14.0f * uiScale,
                          panel.y + 17.0f * uiScale, titleScale, textColor);

    const std::string powerStatus =
        std::string(labels.powerLabel) +
        (securityCheckScene_.powerOn() ? labels.powerOn : labels.powerOff);
    const Color powerColor =
        securityCheckScene_.powerOn() ? dangerColor : successColor;
    ThreeDUtils::drawText(powerStatus, panel.x + 14.0f * uiScale,
                          panel.y + 47.0f * uiScale, textScale, powerColor);

    const std::string healthStatus =
        std::string(labels.healthLabel) + std::to_string(playerHealth_) +
        " / " + std::to_string(playerMaxHealth_);
    const float healthRatio =
        static_cast<float>(playerHealth_) /
        static_cast<float>(std::max(1, playerMaxHealth_));
    const Color healthColor =
        healthRatio > 0.55f
            ? successColor
            : (healthRatio > 0.25f ? Color{1.0f, 0.78f, 0.20f}
                                   : dangerColor);
    ThreeDUtils::drawText(healthStatus, panel.x + 14.0f * uiScale,
                          panel.y + 73.0f * uiScale, textScale,
                          healthColor);

    const Rect healthBar{panel.x + 14.0f * uiScale,
                         panel.y + 94.0f * uiScale,
                         panel.width - 28.0f * uiScale,
                         13.0f * uiScale};
    ThreeDUtils::drawRect2D(
        {healthBar.x + 2.0f * uiScale, healthBar.y + 3.0f * uiScale,
         healthBar.width, healthBar.height},
        kInkColor, 0.42f);
    ThreeDUtils::drawRect2D(healthBar, kHealthBarBack, 0.95f);
    if (healthRatio > 0.0f) {
        ThreeDUtils::drawRect2D(
            {healthBar.x, healthBar.y, healthBar.width * healthRatio,
             healthBar.height},
            healthColor, 0.98f);
        ThreeDUtils::drawRect2D(
            {healthBar.x, healthBar.y, healthBar.width * healthRatio,
             std::max(1.0f, 2.0f * uiScale)},
            ThreeDUtils::shade(healthColor, 1.22f), 0.95f);
    }
    ThreeDUtils::drawFrame2D(healthBar, kInkColor,
                             std::max(1.0f, 2.0f * uiScale));

    std::string prompt;
    Color promptColor = mutedColor;
    if (playerDown_) {
        prompt = labels.playerDownPrompt;
        promptColor = dangerColor;
    } else if (securityCheckScene_.alarmTriggered()) {
        prompt = labels.alarmPrompt;
        promptColor = dangerColor;
    } else if (securityCheckScene_.levelComplete()) {
        prompt = labels.completePrompt;
        promptColor = successColor;
    } else if (securityCheckScene_.nearSwitch(camera_.position())) {
        prompt = labels.switchPrompt;
        promptColor = successColor;
    } else {
        prompt = labels.objectivePrompt;
    }
    ThreeDUtils::drawText(prompt, panel.x + 14.0f * uiScale,
                          panel.y + 117.0f * uiScale, textScale, promptColor);

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    ThreeDUtils::setProjection(
        framebufferWidth_, framebufferHeight_,
        ThreeDUtils::currentFieldOfView(camera_.aimAmount()));
}
void MainScene::renderWarehouseHud() const {
    if (framebufferWidth_ <= 0 || framebufferHeight_ <= 0) {
        return;
    }

    const float uiScale =
        std::min(static_cast<float>(framebufferWidth_) /
                     static_cast<float>(kWindowWidth),
                 static_cast<float>(framebufferHeight_) /
                     static_cast<float>(kWindowHeight));
    const float margin = 22.0f * uiScale;
    const float panelWidth = std::min(390.0f * uiScale,
                                      static_cast<float>(framebufferWidth_) -
                                          margin * 2.0f);
    const float panelHeight = 164.0f * uiScale;
    const Rect panel{margin, margin, panelWidth, panelHeight};
    const Color panelColor{0.03f, 0.07f, 0.11f};
    const Color panelAccent{0.94f, 0.54f, 0.10f};
    const Color textColor{0.95f, 0.98f, 0.92f};
    const Color mutedColor{0.65f, 0.78f, 0.78f};
    const Color dangerColor{1.0f, 0.42f, 0.28f};
    const Color successColor{0.42f, 1.0f, 0.58f};
    const WarehouseText& labels = warehouseText(language_);

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

    renderLowHealthOverlay();

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
    ThreeDUtils::drawText(labels.title, panel.x + 14.0f * uiScale,
                          panel.y + 17.0f * uiScale, titleScale, textColor);

    const std::string healthStatus =
        std::string(labels.healthLabel) + std::to_string(playerHealth_) +
        " / " + std::to_string(playerMaxHealth_);
    const float healthRatio =
        static_cast<float>(playerHealth_) /
        static_cast<float>(std::max(1, playerMaxHealth_));
    const Color healthColor =
        healthRatio > 0.55f
            ? successColor
            : (healthRatio > 0.25f ? Color{1.0f, 0.78f, 0.20f}
                                   : dangerColor);
    ThreeDUtils::drawText(healthStatus, panel.x + 14.0f * uiScale,
                          panel.y + 47.0f * uiScale, textScale,
                          healthColor);

    const Rect healthBar{panel.x + 14.0f * uiScale,
                         panel.y + 68.0f * uiScale,
                         panel.width - 28.0f * uiScale,
                         13.0f * uiScale};
    ThreeDUtils::drawRect2D(
        {healthBar.x + 2.0f * uiScale, healthBar.y + 3.0f * uiScale,
         healthBar.width, healthBar.height},
        kInkColor, 0.42f);
    ThreeDUtils::drawRect2D(healthBar, kHealthBarBack, 0.95f);
    if (healthRatio > 0.0f) {
        ThreeDUtils::drawRect2D(
            {healthBar.x, healthBar.y, healthBar.width * healthRatio,
             healthBar.height},
            healthColor, 0.98f);
        ThreeDUtils::drawRect2D(
            {healthBar.x, healthBar.y, healthBar.width * healthRatio,
             std::max(1.0f, 2.0f * uiScale)},
            ThreeDUtils::shade(healthColor, 1.22f), 0.95f);
    }
    ThreeDUtils::drawFrame2D(healthBar, kInkColor,
                             std::max(1.0f, 2.0f * uiScale));

    std::string prompt;
    Color promptColor = mutedColor;
    if (playerDown_) {
        prompt = labels.playerDownPrompt;
        promptColor = dangerColor;
    } else if (warehouseScene_.levelComplete()) {
        prompt = labels.completePrompt;
        promptColor = successColor;
    } else if (warehouseScene_.drivingForklift()) {
        prompt = labels.forkliftDrivingPrompt;
        promptColor = successColor;
    } else if (warehouseScene_.nearExit(camera_.position())) {
        prompt = labels.exitPrompt;
        promptColor = successColor;
    } else if (warehouseScene_.nearSniperCrate(camera_.position()) &&
               !warehouseScene_.sniperRifleTaken()) {
        prompt = labels.sniperCratePrompt;
        promptColor = successColor;
    } else if (warehouseScene_.nearPlatformClimb(camera_.position())) {
        prompt = labels.climbPrompt;
        promptColor = successColor;
    } else if (warehouseScene_.onPlatform(camera_.position())) {
        prompt = labels.descendPrompt;
        promptColor = successColor;
    } else if (warehouseScene_.nearForklift(camera_.position())) {
        prompt = labels.forkliftPrompt;
        promptColor = successColor;
    } else if (!warehouseScene_.ironDoorOpen()) {
        prompt = labels.doorBlockedPrompt;
        promptColor = dangerColor;
    } else {
        prompt = labels.objectivePrompt;
    }
    ThreeDUtils::drawText(prompt, panel.x + 14.0f * uiScale,
                          panel.y + 95.0f * uiScale, textScale, promptColor);

    if (warehouseScene_.sniperRifleTaken()) {
        ThreeDUtils::drawText(labels.sniperAcquiredLabel, panel.x + 14.0f * uiScale,
                              panel.y + 120.0f * uiScale, textScale,
                              successColor);
    }

    if (warehouseScene_.drivingForklift()) {
        const float wheelX =
            static_cast<float>(framebufferWidth_) * 0.5f;
        const float wheelRadius = std::max(24.0f, 34.0f * uiScale);
        const float wheelY = margin + wheelRadius + 8.0f * uiScale;
        const float spokeThickness =
            std::max(2.0f, 4.0f * uiScale);
        const float hubRadius = std::max(5.0f, 7.0f * uiScale);
        const float headingDegrees = warehouseScene_.forkliftHeadingDegrees();

        drawRing(wheelX, wheelY, wheelRadius * 0.72f, wheelRadius,
                 kSignColor, 0.95f);

        glPushMatrix();
        glTranslatef(wheelX, wheelY, 0.0f);
        glRotatef(-headingDegrees, 0.0f, 0.0f, 1.0f);
        ThreeDUtils::drawRect2D(
            {-wheelRadius, -spokeThickness * 0.5f, wheelRadius * 2.0f,
             spokeThickness},
            kSignColor, 0.85f);
        ThreeDUtils::drawRect2D(
            {-spokeThickness * 0.5f, -wheelRadius, spokeThickness,
             wheelRadius * 2.0f},
            kSignColor, 0.85f);
        drawFilledCircle(0.0f, wheelRadius * 0.84f,
                         std::max(3.0f, 4.5f * uiScale), kSunCore, 1.0f);
        glPopMatrix();

        drawFilledCircle(wheelX, wheelY, hubRadius, kInkColor, 0.95f);
        drawFilledCircle(wheelX, wheelY, hubRadius * 0.55f,
                         successColor, 0.98f);
    }

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
    const float margin = std::max(18.0f, 26.0f * uiScale);
    const float startX = margin;
    const float startY =
        static_cast<float>(framebufferHeight_) - margin - slotSize;
    const float slotGap = std::max(10.0f, 14.0f * uiScale);

    ThreeDUtils::setUiProjection(framebufferWidth_, framebufferHeight_);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const auto drawSlot = [&](const Rect& slot, bool selected) {
        const Color fill = selected ? Color{0.10f, 0.30f, 0.34f}
                                    : Color{0.12f, 0.36f, 0.40f};
        const Color outline = selected ? kSunCore : kInkColor;
        ThreeDUtils::drawRect2D(
            {slot.x + 5.0f * uiScale, slot.y + 6.0f * uiScale,
             slot.width, slot.height},
            kInkColor, 0.45f);
        ThreeDUtils::drawRect2D(slot, fill, selected ? 0.98f : 0.90f);
        ThreeDUtils::drawRect2D(
            {slot.x, slot.y, slot.width, 5.0f * uiScale},
            selected ? kSunCore : kPanelBottom, 0.95f);
        ThreeDUtils::drawFrame2D(
            slot, outline, std::max(2.0f, (selected ? 4.0f : 2.0f) * uiScale));
    };

    const float keyScale = std::max(1.0f, 2.0f * uiScale);
    const Color keyColor{0.95f, 0.98f, 0.92f};

    for (std::size_t index = 0; index < inventoryWeapons_.size(); ++index) {
        const Rect slot{
            startX + static_cast<float>(index) * (slotSize + slotGap),
            startY,
            slotSize,
            slotSize,
        };
        const WeaponType weapon = inventoryWeapons_[index];
        const bool selected = equippedWeapon_ == weapon;

        drawSlot(slot, selected);
        if (weapon == WeaponType::Knife) {
            drawInventoryKnifeIcon(slot, uiScale, selected ? 1.0f : 0.76f);
        } else if (weapon == WeaponType::Sniper) {
            drawInventorySniperIcon(slot, uiScale, selected ? 1.0f : 0.76f);
        }

        ThreeDUtils::drawText(
            std::to_string(index + 1),
            slot.x + 8.0f * uiScale, slot.y + 9.0f * uiScale,
            keyScale, keyColor);
    }

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    ThreeDUtils::setProjection(
        framebufferWidth_, framebufferHeight_,
        ThreeDUtils::currentFieldOfView(camera_.aimAmount()));
}

void MainScene::renderFpsOverlay() const {
    if (!fpsDisplayVisible_ || framebufferWidth_ <= 0 ||
        framebufferHeight_ <= 0) {
        return;
    }

    const float uiScale =
        std::min(static_cast<float>(framebufferWidth_) /
                     static_cast<float>(kWindowWidth),
                 static_cast<float>(framebufferHeight_) /
                     static_cast<float>(kWindowHeight));
    const float margin = 20.0f * uiScale;
    const float textScale = std::max(1.0f, 2.2f * uiScale);
    const std::string fpsText =
        "FPS " + std::to_string(static_cast<int>(std::round(fps_)));
    const float textWidth =
        ThreeDUtils::textWidth(fpsText, textScale);
    const float panelWidth = textWidth + 20.0f * uiScale;
    const float panelHeight = 34.0f * uiScale;
    const Rect panel{
        static_cast<float>(framebufferWidth_) - margin - panelWidth,
        margin,
        panelWidth,
        panelHeight,
    };

    ThreeDUtils::setUiProjection(framebufferWidth_, framebufferHeight_);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ThreeDUtils::drawRect2D(
        {panel.x + 5.0f * uiScale, panel.y + 6.0f * uiScale,
         panel.width, panel.height},
        kInkColor, 0.55f);
    ThreeDUtils::drawRect2D(panel, Color{0.03f, 0.07f, 0.11f}, 0.88f);
    ThreeDUtils::drawFrame2D(
        panel, kPanelBottom, std::max(2.0f, 2.5f * uiScale));
    ThreeDUtils::drawText(
        fpsText, panel.x + 10.0f * uiScale,
        panel.y + (panel.height - 7.0f * textScale) * 0.5f,
        textScale, Color{0.95f, 0.98f, 0.92f});

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
    if (sceneSegmentHitsGuard(start, end, guardHitT, guardIndex) &&
        guardHitT < closestHitT) {
        hit = true;
        hitType = ImpactType::Character;
        closestHitT = guardHitT;
    }

    float geometryHitT = 0.0f;
    if (sceneSegmentHitsGeometry(start, end, geometryHitT) &&
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
        sceneApplyGuardKnifeDamage(guardIndex, hitPosition);
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
                                  Language language,
                                  SceneType activeScene) {
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
            if (activeScene == SceneType::SecurityCheck) {
                title = chinese
                            ? "Pixel World 3D | \xE5\xAE\x89\xE6\xA3\x80\xE5\x8C\xBA"
                            : "Pixel World 3D | Security Check | E toggle power | "
                              "LMB slash | RMB aim | Shift run | Space jump/stand | "
                              "C crouch | Z prone | WASD move | Esc exit";
            } else if (activeScene == SceneType::Warehouse) {
                title = chinese
                            ? "Pixel World 3D | \xE4\xBB\x93\xE5\xBA\x93"
                            : "Pixel World 3D | Warehouse | E interact | "
                              "LMB fire | RMB aim | Shift run | Space jump/stand | "
                              "C crouch | Z prone | WASD move | Esc exit";
            } else {
                title = chinese
                            ? "Pixel World 3D | \xE5\x81\x9C\xE8\xBD\xA6\xE5\x9C\xBA"
                            : "Pixel World 3D | Mouse look | LMB slash | RMB aim | "
                              "E elevator | Shift run | Space jump/stand | C crouch | "
                              "Z prone | Arrows walk | WASD move | Esc exit";
            }
            break;
    }
    if (title == lastTitle) {
        return;
    }

    glfwSetWindowTitle(window, title.c_str());
    lastTitle = title;
}

}  // namespace pixel_world
