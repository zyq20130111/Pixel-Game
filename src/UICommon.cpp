#include "UICommon.h"

#include "ThreeDUtils.h"
#include "game_constants.h"

#include <algorithm>
#include <string>

namespace pixel_world::ui {
namespace {

void drawMenuBackdrop(int width, int height) {
    const float screenWidth = static_cast<float>(width);
    const float screenHeight = static_cast<float>(height);

    // Use opaque pixel-art bands so the menu never falls back to a black
    // clear color while the game scene is still unloaded.
    ThreeDUtils::drawRect2D(
        {0.0f, 0.0f, screenWidth, screenHeight * 0.24f},
        constants::kSkyTop);
    ThreeDUtils::drawRect2D(
        {0.0f, screenHeight * 0.24f, screenWidth, screenHeight * 0.25f},
        constants::kSkyUpper);
    ThreeDUtils::drawRect2D(
        {0.0f, screenHeight * 0.49f, screenWidth, screenHeight * 0.22f},
        constants::kSkyMiddle);
    ThreeDUtils::drawRect2D(
        {0.0f, screenHeight * 0.71f, screenWidth, screenHeight * 0.11f},
        constants::kSkyHorizon);
    ThreeDUtils::drawRect2D(
        {0.0f, screenHeight * 0.82f, screenWidth, screenHeight * 0.18f},
        constants::kGrassB);

    const Color cloud = constants::kCloudColor;
    const Color cloudShadow = constants::kCloudShadow;
    const float cloudY = screenHeight * 0.16f;
    ThreeDUtils::drawRect2D(
        {screenWidth * 0.08f, cloudY + screenHeight * 0.018f,
         screenWidth * 0.19f, screenHeight * 0.026f},
        cloudShadow, 0.55f);
    ThreeDUtils::drawRect2D(
        {screenWidth * 0.105f, cloudY, screenWidth * 0.13f,
         screenHeight * 0.042f},
        cloud, 0.75f);
    ThreeDUtils::drawRect2D(
        {screenWidth * 0.68f, screenHeight * 0.28f,
         screenWidth * 0.22f, screenHeight * 0.028f},
        cloudShadow, 0.48f);
    ThreeDUtils::drawRect2D(
        {screenWidth * 0.71f, screenHeight * 0.26f,
         screenWidth * 0.15f, screenHeight * 0.044f},
        cloud, 0.68f);

    const float groundY = screenHeight * 0.82f;
    ThreeDUtils::drawRect2D(
        {0.0f, groundY, screenWidth, std::max(3.0f, screenHeight * 0.008f)},
        constants::kWaterHighlight, 0.75f);
    ThreeDUtils::drawRect2D(
        {0.0f, groundY + screenHeight * 0.035f, screenWidth,
         std::max(2.0f, screenHeight * 0.006f)},
        constants::kGrassA, 0.70f);

    // Small block silhouettes keep the empty background visually grounded.
    const float blockWidth = std::max(10.0f, screenWidth * 0.018f);
    const float blockGap = blockWidth * 1.8f;
    for (float x = blockGap * 0.5f; x < screenWidth;
         x += blockGap) {
        const int blockIndex = static_cast<int>(x / blockGap);
        const float blockHeight =
            screenHeight * (0.035f + 0.025f * (blockIndex % 3));
        ThreeDUtils::drawRect2D(
            {x, groundY - blockHeight, blockWidth, blockHeight},
            blockIndex % 2 == 0 ? constants::kLeafDarkColor
                                : constants::kPathDark,
            0.82f);
    }
}

}  // namespace

float scaleFor(int width, int height) {
    return std::min(
        static_cast<float>(width) /
            static_cast<float>(constants::kWindowWidth),
        static_cast<float>(height) /
            static_cast<float>(constants::kWindowHeight));
}

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

void beginOverlay(int framebufferWidth, int framebufferHeight) {
    ThreeDUtils::setUiProjection(framebufferWidth, framebufferHeight);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawMenuBackdrop(framebufferWidth, framebufferHeight);
    ThreeDUtils::drawRect2D(
        {0.0f, 0.0f, static_cast<float>(framebufferWidth),
         static_cast<float>(framebufferHeight)},
        constants::kInkColor, 0.10f);
}

void endOverlay(int framebufferWidth, int framebufferHeight) {
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    ThreeDUtils::setProjection(framebufferWidth, framebufferHeight);
}

void drawPanel(const Rect& panel, float uiScale) {
    ThreeDUtils::drawRect2D(
        {panel.x + 8.0f * uiScale, panel.y + 8.0f * uiScale, panel.width,
         panel.height},
        constants::kPanelShadow, 0.78f);
    ThreeDUtils::drawRect2D(panel, constants::kPanelBottom, 0.97f);
    ThreeDUtils::drawRect2D(
        {panel.x, panel.y, panel.width, panel.height * 0.34f},
        constants::kPanelTop, 0.97f);
    ThreeDUtils::drawFrame2D(
        panel, constants::kPanelOutline, std::max(2.0f, 4.0f * uiScale));
}

void drawButton(const Rect& button, const char* label,
                const Color& baseColor, bool hovered, float uiScale) {
    const Color shadow = ThreeDUtils::shade(baseColor, 0.55f);
    const Color fill =
        hovered ? ThreeDUtils::shade(baseColor, 1.18f) : baseColor;
    const float pixel = std::max(1.0f, 3.0f * uiScale);

    ThreeDUtils::drawRect2D(
        {button.x, button.y + 5.0f * uiScale, button.width, button.height},
        shadow);
    ThreeDUtils::drawRect2D(button, fill);
    ThreeDUtils::drawFrame2D(button, constants::kInkColor, pixel);
    ThreeDUtils::drawRect2D(
        {button.x + pixel, button.y + pixel,
         button.width - 2.0f * pixel, pixel},
        ThreeDUtils::shade(fill, 1.25f));

    const float textScale = std::max(1.0f, 3.0f * uiScale);
    const std::string text(label);
    const float textY =
        button.y + (button.height - 7.0f * textScale) * 0.5f;
    ThreeDUtils::drawText(
        text, button.x +
                  (button.width - ThreeDUtils::textWidth(text, textScale)) *
                      0.5f,
        textY, textScale, Color{1.0f, 1.0f, 0.92f});
}

}  // namespace pixel_world::ui
