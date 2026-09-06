#pragma once

#include "PistolBullet.h"
#include "Camera.h"
#include "CharacterModel.h"
#include "LoginScreen.h"
#include "platform.h"
#include "SceneBase.h"
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
    void updateExitPrompt();
    void updateGameplay(float dt);
    void updateBullets(float dt);
    void updateImpactEffects(float dt);
    void renderFrame(const Camera& camera, bool showPistol);
    void renderExitPrompt() const;
    void renderScene() const;
    void renderImpactEffects() const;
    void renderCrosshair() const;

    void drawSkybox(const Vec3& cameraPosition) const;
    void drawSun(const Vec3& cameraPosition) const;
    void drawGround() const;
    void drawTree(const Vec3& base) const;
    void drawRock(const Vec3& base, float scale) const;
    void drawHouse(const Vec3& base) const;

    static bool segmentHitsGround(const Vec3& start, const Vec3& end,
                                  float& hitT);
    static Vec3 pointOnSegment(const Vec3& start, const Vec3& end, float t);
    void spawnImpactEffect(const Vec3& position, ImpactType type);
    void renderGroundImpactEffect(const ImpactEffect& effect) const;
    void renderCharacterImpactEffect(const ImpactEffect& effect) const;

    static void updateWindowTitle(GLFWwindow* window, AppState state);

    GLFWwindow* window_;
    bool glfwInitialized_;
    int framebufferWidth_;
    int framebufferHeight_;
    AppState state_;

    Camera camera_;
    Camera menuCamera_;
    CharacterModel character_;
    Pistol pistol_;
    LoginScreen loginScreen_;
    std::vector<std::unique_ptr<BulletBase>> bullets_;
    std::vector<ImpactEffect> impactEffects_;
    bool previousFireDown_;
    bool previousJumpDown_;
    bool previousEscapeDown_;
    bool exitPromptVisible_;
    bool previousPromptMouseDown_;
    MouseState promptMouse_;
};

}  // namespace pixel_world
