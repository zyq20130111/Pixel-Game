#include "LoadingUI.h"

#include "ThreeDUtils.h"
#include "UICommon.h"
#include "game_constants.h"

#include <algorithm>
#include <string>

namespace pixel_world {

LoadingUI::LoadingUI(Language language) : language_(language), progress_(0.0f) {}

MenuAction LoadingUI::update(GLFWwindow*, int, int) {
    return MenuAction::None;
}

void LoadingUI::render(int framebufferWidth, int framebufferHeight) const {
    const UiText& text = uiText(language_);
    const float uiScale = ui::scaleFor(framebufferWidth, framebufferHeight);
    const float panelWidth =
        std::min(620.0f * uiScale, static_cast<float>(framebufferWidth) - 32.0f);
    const float panelHeight = std::min(
        300.0f * uiScale, static_cast<float>(framebufferHeight) - 32.0f);
    const Rect panel{
        (static_cast<float>(framebufferWidth) - panelWidth) * 0.5f,
        (static_cast<float>(framebufferHeight) - panelHeight) * 0.5f,
        panelWidth, panelHeight};

    ui::beginOverlay(framebufferWidth, framebufferHeight);
    ui::drawPanel(panel, uiScale);
    ThreeDUtils::drawCenteredText(
        text.loadingTitle, panel, std::max(1.0f, 4.0f * uiScale),
        constants::kInkColor, 42.0f * uiScale);
    ThreeDUtils::drawCenteredText(
        text.loadingSubtitle, panel, std::max(1.0f, 2.0f * uiScale),
        ThreeDUtils::shade(constants::kInkColor, 1.2f), 104.0f * uiScale);

    const Rect bar{panel.x + 56.0f * uiScale,
                   panel.y + panel.height - 92.0f * uiScale,
                   panel.width - 112.0f * uiScale, 28.0f * uiScale};
    ThreeDUtils::drawRect2D(
        {bar.x + 4.0f * uiScale, bar.y + 4.0f * uiScale, bar.width,
         bar.height},
        constants::kInkColor, 0.45f);
    ThreeDUtils::drawRect2D(bar, constants::kInkColor);
    ThreeDUtils::drawRect2D(
        {bar.x + 4.0f * uiScale, bar.y + 4.0f * uiScale,
         (bar.width - 8.0f * uiScale) * progress_,
         bar.height - 8.0f * uiScale},
        constants::kPortalGlow);
    ThreeDUtils::drawCenteredText(
        text.loadingStatus, panel, std::max(1.0f, 1.5f * uiScale),
        ThreeDUtils::shade(constants::kInkColor, 1.1f),
        panel.height - 48.0f * uiScale);

    const std::string percentage =
        std::to_string(static_cast<int>(progress_ * 100.0f)) + "%";
    const float textScale = std::max(1.0f, 1.7f * uiScale);
    ThreeDUtils::drawText(
        percentage,
        bar.x + (bar.width - ThreeDUtils::textWidth(percentage, textScale)) *
                    0.5f,
        bar.y + (bar.height - 7.0f * textScale) * 0.5f, textScale,
        Color{1.0f, 1.0f, 0.92f});
    ui::endOverlay(framebufferWidth, framebufferHeight);
}

void LoadingUI::setProgress(float progress) {
    progress_ = std::clamp(progress, 0.0f, 1.0f);
}

}  // namespace pixel_world
