#include "LoginScreen.h"

#include "game_constants.h"
#include "ThreeDUtils.h"

#include <algorithm>
#include <cmath>

namespace pixel_world {

LoginScreen::LoginScreen()
    : previousMouseDown_(false),
      buttonClicked_(false),
      mouse_{0.0f, 0.0f, false} {}

MenuAction LoginScreen::update(GLFWwindow* window, int framebufferWidth,
                               int framebufferHeight) {
    buttonClicked_ = false;
    if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
        return MenuAction::Login;
    }
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        return MenuAction::Exit;
    }

    mouse_ = getMouseState(window, framebufferWidth, framebufferHeight);
    const bool clicked = mouse_.pressed && !previousMouseDown_;
    previousMouseDown_ = mouse_.pressed;
    if (!clicked) {
        return MenuAction::None;
    }

    const MenuLayout layout = makeLayout(framebufferWidth, framebufferHeight);
    if (contains(layout.loginButton, mouse_.x, mouse_.y)) {
        buttonClicked_ = true;
        return MenuAction::Login;
    }
    if (contains(layout.exitButton, mouse_.x, mouse_.y)) {
        buttonClicked_ = true;
        return MenuAction::Exit;
    }
    return MenuAction::None;
}

bool LoginScreen::buttonClicked() const {
    return buttonClicked_;
}

void LoginScreen::render(int framebufferWidth, int framebufferHeight) const {
    const MenuLayout layout = makeLayout(framebufferWidth, framebufferHeight);
    const float uiScale =
        std::min(static_cast<float>(framebufferWidth) /
                     static_cast<float>(constants::kWindowWidth),
                 static_cast<float>(framebufferHeight) /
                     static_cast<float>(constants::kWindowHeight));

    ThreeDUtils::setUiProjection(framebufferWidth, framebufferHeight);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ThreeDUtils::drawRect2D(
        {0.0f, 0.0f, static_cast<float>(framebufferWidth),
         static_cast<float>(framebufferHeight)},
        constants::kInkColor, 0.24f);

    ThreeDUtils::drawRect2D(layout.panel, constants::kPanelBottom, 0.96f);
    ThreeDUtils::drawRect2D(
        {layout.panel.x, layout.panel.y, layout.panel.width,
         layout.panel.height * 0.47f},
        constants::kPanelTop, 0.96f);
    ThreeDUtils::drawFrame2D(
        layout.panel, constants::kInkColor,
        std::max(2.0f, 5.0f * uiScale));

    const float cornerSize = std::max(4.0f, 12.0f * uiScale);
    const Color cornerColor = constants::kHouseRoof;
    ThreeDUtils::drawRect2D(
        {layout.panel.x - cornerSize, layout.panel.y - cornerSize,
         cornerSize * 2.0f, cornerSize},
        cornerColor);
    ThreeDUtils::drawRect2D(
        {layout.panel.x - cornerSize, layout.panel.y - cornerSize, cornerSize,
         cornerSize * 2.0f},
        cornerColor);
    ThreeDUtils::drawRect2D(
        {layout.panel.x + layout.panel.width - cornerSize,
         layout.panel.y - cornerSize, cornerSize * 2.0f, cornerSize},
        cornerColor);
    ThreeDUtils::drawRect2D(
        {layout.panel.x + layout.panel.width, layout.panel.y - cornerSize,
         cornerSize, cornerSize * 2.0f},
        cornerColor);
    ThreeDUtils::drawRect2D(
        {layout.panel.x - cornerSize,
         layout.panel.y + layout.panel.height, cornerSize * 2.0f,
         cornerSize},
        cornerColor);
    ThreeDUtils::drawRect2D(
        {layout.panel.x - cornerSize,
         layout.panel.y + layout.panel.height - cornerSize, cornerSize,
         cornerSize * 2.0f},
        cornerColor);
    ThreeDUtils::drawRect2D(
        {layout.panel.x + layout.panel.width - cornerSize,
         layout.panel.y + layout.panel.height, cornerSize * 2.0f,
         cornerSize},
        cornerColor);
    ThreeDUtils::drawRect2D(
        {layout.panel.x + layout.panel.width,
         layout.panel.y + layout.panel.height - cornerSize, cornerSize,
         cornerSize * 2.0f},
        cornerColor);

    const float titleScale = std::max(1.0f, 4.0f * uiScale);
    ThreeDUtils::drawCenteredText("PIXEL WORLD 3D", layout.panel, titleScale,
                                  constants::kInkColor, 48.0f * uiScale);
    ThreeDUtils::drawCenteredText(
        "ADVENTURE AWAITS", layout.panel, std::max(1.0f, 2.0f * uiScale),
        ThreeDUtils::shade(constants::kInkColor, 1.35f), 104.0f * uiScale);

    const bool loginHovered =
        contains(layout.loginButton, mouse_.x, mouse_.y);
    const bool exitHovered = contains(layout.exitButton, mouse_.x, mouse_.y);
    drawButton(layout.loginButton, "LOGIN GAME", constants::kButtonStart,
               loginHovered, uiScale);
    drawButton(layout.exitButton, "EXIT GAME", constants::kButtonExit,
               exitHovered, uiScale);

    ThreeDUtils::drawCenteredText(
        "WASD TO MOVE  ESC TO QUIT", layout.panel,
        std::max(1.0f, 1.5f * uiScale),
        ThreeDUtils::shade(constants::kInkColor, 1.18f),
        layout.panel.height - 54.0f * uiScale);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    ThreeDUtils::setProjection(framebufferWidth, framebufferHeight);
}

MenuLayout LoginScreen::makeLayout(int width, int height) {
    const float scale =
        std::min(static_cast<float>(width) /
                     static_cast<float>(constants::kWindowWidth),
                 static_cast<float>(height) /
                     static_cast<float>(constants::kWindowHeight));
    const float panelWidth =
        std::min(500.0f * scale, static_cast<float>(width) - 32.0f);
    const float panelHeight =
        std::min(470.0f * scale, static_cast<float>(height) - 32.0f);
    const float panelX = (static_cast<float>(width) - panelWidth) * 0.5f;
    const float panelY = (static_cast<float>(height) - panelHeight) * 0.5f;
    const float buttonWidth =
        std::min(300.0f * scale, panelWidth - 48.0f);
    const float buttonHeight = std::max(34.0f, 58.0f * scale);
    const float buttonX = panelX + (panelWidth - buttonWidth) * 0.5f;

    return {
        {panelX, panelY, panelWidth, panelHeight},
        {buttonX, panelY + panelHeight * 0.57f, buttonWidth, buttonHeight},
        {buttonX, panelY + panelHeight * 0.57f + buttonHeight +
                       18.0f * scale,
         buttonWidth, buttonHeight},
    };
}

bool LoginScreen::contains(const Rect& rect, float x, float y) {
    return x >= rect.x && x <= rect.x + rect.width && y >= rect.y &&
           y <= rect.y + rect.height;
}

MouseState LoginScreen::getMouseState(GLFWwindow* window, int framebufferWidth,
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

void LoginScreen::drawButton(const Rect& button, const char* label,
                             const Color& baseColor, bool hovered,
                             float uiScale) {
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
    const float textY =
        button.y + (button.height - 7.0f * textScale) * 0.5f;
    const std::string text(label);
    ThreeDUtils::drawText(
        text, button.x + (button.width -
                          ThreeDUtils::textWidth(text, textScale)) *
                             0.5f,
        textY, textScale, Color{1.0f, 1.0f, 0.92f});
}

}  // namespace pixel_world
