#include "DifficultyUI.h"

#include "ThreeDUtils.h"
#include "UICommon.h"
#include "game_constants.h"

#include <algorithm>

namespace pixel_world {

DifficultyUI::DifficultyUI(Language language)
    : language_(language),
      previousMouseDown_(false),
      previousEscapeDown_(false),
      buttonClicked_(false),
      mouse_{0.0f, 0.0f, false} {}

MenuAction DifficultyUI::update(GLFWwindow* window, int framebufferWidth,
                                int framebufferHeight) {
    buttonClicked_ = false;
    const bool escapeDown = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    if (escapeDown && !previousEscapeDown_) {
        previousEscapeDown_ = escapeDown;
        buttonClicked_ = true;
        return MenuAction::Back;
    }
    previousEscapeDown_ = escapeDown;

    mouse_ = ui::getMouseState(window, framebufferWidth, framebufferHeight);
    const bool clicked = mouse_.pressed && !previousMouseDown_;
    previousMouseDown_ = mouse_.pressed;
    if (!clicked) {
        return MenuAction::None;
    }

    const Layout layout = makeLayout(framebufferWidth, framebufferHeight);
    if (ui::contains(layout.easyButton, mouse_.x, mouse_.y)) {
        buttonClicked_ = true;
        return MenuAction::DifficultyEasy;
    }
    if (ui::contains(layout.normalButton, mouse_.x, mouse_.y)) {
        buttonClicked_ = true;
        return MenuAction::DifficultyNormal;
    }
    if (ui::contains(layout.hardButton, mouse_.x, mouse_.y)) {
        buttonClicked_ = true;
        return MenuAction::DifficultyHard;
    }
    if (ui::contains(layout.backButton, mouse_.x, mouse_.y)) {
        buttonClicked_ = true;
        return MenuAction::Back;
    }
    return MenuAction::None;
}

void DifficultyUI::render(int framebufferWidth, int framebufferHeight) const {
    const UiText& text = uiText(language_);
    const float uiScale = ui::scaleFor(framebufferWidth, framebufferHeight);
    const Layout layout = makeLayout(framebufferWidth, framebufferHeight);

    ui::beginOverlay(framebufferWidth, framebufferHeight);
    ui::drawPanel(layout.panel, uiScale);
    ThreeDUtils::drawCenteredText(
        text.difficultyTitle, layout.panel, std::max(1.0f, 4.0f * uiScale),
        constants::kInkColor, 38.0f * uiScale);
    ThreeDUtils::drawCenteredText(
        text.difficultySubtitle, layout.panel,
        std::max(1.0f, 2.0f * uiScale),
        ThreeDUtils::shade(constants::kInkColor, 1.2f), 104.0f * uiScale);

    ui::drawButton(layout.easyButton, text.easyButton, constants::kButtonStart,
                   ui::contains(layout.easyButton, mouse_.x, mouse_.y),
                   uiScale);
    ui::drawButton(layout.normalButton, text.normalButton,
                   constants::kStatueColor,
                   ui::contains(layout.normalButton, mouse_.x, mouse_.y),
                   uiScale);
    ui::drawButton(layout.hardButton, text.hardButton, constants::kButtonExit,
                   ui::contains(layout.hardButton, mouse_.x, mouse_.y),
                   uiScale);
    ui::drawButton(layout.backButton, text.backButton, constants::kStoneDark,
                   ui::contains(layout.backButton, mouse_.x, mouse_.y),
                   uiScale);
    ui::endOverlay(framebufferWidth, framebufferHeight);
}

bool DifficultyUI::buttonClicked() const {
    return buttonClicked_;
}

DifficultyUI::Layout DifficultyUI::makeLayout(int width, int height) {
    const float scale = ui::scaleFor(width, height);
    const float panelWidth = std::min(580.0f * scale, width - 32.0f);
    const float panelHeight = std::min(480.0f * scale, height - 32.0f);
    const float panelX = (static_cast<float>(width) - panelWidth) * 0.5f;
    const float panelY = (static_cast<float>(height) - panelHeight) * 0.5f;
    const float buttonWidth =
        std::min(350.0f * scale, panelWidth - 64.0f * scale);
    const float buttonHeight = std::max(30.0f, 54.0f * scale);
    const float gap = 12.0f * scale;
    const float buttonX = panelX + (panelWidth - buttonWidth) * 0.5f;
    const float firstY = panelY + 150.0f * scale;

    return {
        {panelX, panelY, panelWidth, panelHeight},
        {buttonX, firstY, buttonWidth, buttonHeight},
        {buttonX, firstY + buttonHeight + gap, buttonWidth, buttonHeight},
        {buttonX, firstY + (buttonHeight + gap) * 2.0f, buttonWidth,
         buttonHeight},
        {buttonX, panelY + panelHeight - 76.0f * scale, buttonWidth,
         buttonHeight},
    };
}

}  // namespace pixel_world
