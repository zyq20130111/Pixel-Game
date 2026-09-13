#pragma once

#include "PistolBullet.h"
#include "Camera.h"
#include "DifficultyUI.h"
#include "LoadingUI.h"
#include "Localization.h"
#include "MainUI.h"
#include "ParkingLotScene.h"
#include "platform.h"
#include "SceneBase.h"
#include "SoundManager.h"
#include "types.h"
#include "Pistol.h"

#include <memory>
#include <vector>

namespace pixel_world {

class MainScene final : public SceneBase {
public:
    MainScene();
    ~MainScene() override;

    int run() override;

private:
    bool initialize();
    void resetGame();
    void startLoading(Difficulty difficulty);
    void updateLoading(float dt);
    void updateExitPrompt();
    void updateGameplay(float dt);
    void updateBullets(float dt);
    void updateImpactEffects(float dt);
    bool cameraPositionBlocked(const Vec3& position,
                               const Vec3& currentPosition) const;
    void renderFrame(const Camera& camera, bool showPistol);
    void renderExitPrompt() const;
    void renderScene() const;
    void renderImpactEffects() const;
    void renderCrosshair() const;
    void renderParkingHud() const;

    static bool segmentHitsGround(const Vec3& start, const Vec3& end,
                                  float& hitT);
    static Vec3 pointOnSegment(const Vec3& start, const Vec3& end, float t);
    void spawnImpactEffect(const Vec3& position, ImpactType type);
    void renderGroundImpactEffect(const ImpactEffect& effect) const;
    void renderGeometryImpactEffect(const ImpactEffect& effect) const;
    void renderCharacterImpactEffect(const ImpactEffect& effect) const;

    static void updateWindowTitle(GLFWwindow* window, AppState state);

    GLFWwindow* window_;
    bool glfwInitialized_;
    int framebufferWidth_;
    int framebufferHeight_;
    AppState state_;
    Language language_;

    Camera camera_;
    SoundManager soundManager_;
    ParkingLotScene parkingLotScene_;
    Pistol pistol_;
    MainUI mainUI_;
    DifficultyUI difficultyUI_;
    LoadingUI loadingUI_;
    Difficulty selectedDifficulty_;
    float loadingElapsed_;
    std::vector<std::unique_ptr<BulletBase>> bullets_;
    std::vector<ImpactEffect> impactEffects_;
    bool previousFireDown_;
    bool previousJumpDown_;
    bool previousInteractDown_;
    bool previousRestartDown_;
    float parkingHintTimer_;
    int playerHealth_;
    int playerMaxHealth_;
    float playerDamageFlashTimer_;
    bool playerDown_;
    bool previousEscapeDown_;
    bool exitPromptVisible_;
    bool previousPromptMouseDown_;
    MouseState promptMouse_;
};

}  // namespace pixel_world
