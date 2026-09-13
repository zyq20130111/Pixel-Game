#include "MainUI.h"

#include "ThreeDUtils.h"
#include "UICommon.h"
#include "game_constants.h"

#include <algorithm>

namespace pixel_world {

MainUI::MainUI(Language language)
    : language_(language),
      page_(Page::Main),
      previousMouseDown_(false),
      previousEscapeDown_(false),
      previousEnterDown_(false),
      buttonClicked_(false),
      mouse_{0.0f, 0.0f, false} {}

MenuAction MainUI::update(GLFWwindow* window, int framebufferWidth,
                          int framebufferHeight) {
    buttonClicked_ = false;

    const bool escapeDown = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    const bool enterDown = glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS;
    const bool escapePressed = escapeDown && !previousEscapeDown_;
    const bool enterPressed = enterDown && !previousEnterDown_;
    previousEscapeDown_ = escapeDown;
    previousEnterDown_ = enterDown;

    if (escapePressed) {
        if (page_ != Page::Main) {
            page_ = Page::Main;
            buttonClicked_ = true;
            return MenuAction::None;
        }
        return MenuAction::Exit;
    }
    if (enterPressed && page_ == Page::Main) {
        buttonClicked_ = true;
        return MenuAction::NewGame;
    }

    mouse_ = ui::getMouseState(window, framebufferWidth, framebufferHeight);
    const bool clicked = mouse_.pressed && !previousMouseDown_;
    previousMouseDown_ = mouse_.pressed;
    if (!clicked) {
        return MenuAction::None;
    }

    const Layout layout = makeLayout(framebufferWidth, framebufferHeight, page_);
    if (page_ == Page::Main) {
        if (ui::contains(layout.continueButton, mouse_.x, mouse_.y)) {
            buttonClicked_ = true;
            return MenuAction::ContinueGame;
        }
        if (ui::contains(layout.newGameButton, mouse_.x, mouse_.y)) {
            buttonClicked_ = true;
            return MenuAction::NewGame;
        }
        if (ui::contains(layout.chapterButton, mouse_.x, mouse_.y)) {
            buttonClicked_ = true;
            page_ = Page::Chapters;
            return MenuAction::None;
        }
        if (ui::contains(layout.settingsButton, mouse_.x, mouse_.y)) {
            buttonClicked_ = true;
            page_ = Page::Settings;
            return MenuAction::None;
        }
        if (ui::contains(layout.creditsButton, mouse_.x, mouse_.y)) {
            buttonClicked_ = true;
            page_ = Page::Credits;
            return MenuAction::None;
        }
        if (ui::contains(layout.exitButton, mouse_.x, mouse_.y)) {
            buttonClicked_ = true;
            return MenuAction::Exit;
        }
    } else if (ui::contains(layout.backButton, mouse_.x, mouse_.y)) {
        buttonClicked_ = true;
        page_ = Page::Main;
    }
    return MenuAction::None;
}

void MainUI::render(int framebufferWidth, int framebufferHeight) const {
    const UiText& text = uiText(language_);
    const float uiScale = ui::scaleFor(framebufferWidth, framebufferHeight);
    const Layout layout = makeLayout(framebufferWidth, framebufferHeight, page_);

    ui::beginOverlay(framebufferWidth, framebufferHeight);
    ui::drawPanel(layout.panel, uiScale);

    const char* title = page_ == Page::Main
                            ? text.mainTitle
                            : page_ == Page::Chapters
                                  ? text.chapterTitle
                                  : page_ == Page::Settings
                                        ? text.settingsTitle
                                        : text.creditsTitle;
    ThreeDUtils::drawCenteredText(
        title, layout.panel, std::max(1.0f, 4.0f * uiScale),
        constants::kInkColor, 34.0f * uiScale);

    drawPageContent(text, layout, page_, uiScale);
    ui::endOverlay(framebufferWidth, framebufferHeight);
}

bool MainUI::buttonClicked() const {
    return buttonClicked_;
}

void MainUI::showMainMenu() {
    page_ = Page::Main;
    previousMouseDown_ = false;
    previousEscapeDown_ = false;
    previousEnterDown_ = false;
}

MainUI::Layout MainUI::makeLayout(int width, int height, Page page) {
    const float scale = ui::scaleFor(width, height);
    const float panelWidth =
        page == Page::Main ? std::min(560.0f * scale, width - 32.0f)
                           : std::min(600.0f * scale, width - 32.0f);
    const float panelHeight =
        page == Page::Main ? std::min(620.0f * scale, height - 32.0f)
                           : std::min(430.0f * scale, height - 32.0f);
    const float panelX = (static_cast<float>(width) - panelWidth) * 0.5f;
    const float panelY = (static_cast<float>(height) - panelHeight) * 0.5f;
    const float buttonWidth =
        std::min(360.0f * scale, panelWidth - 64.0f * scale);
    const float buttonHeight = std::max(30.0f, 52.0f * scale);
    const float buttonGap = 10.0f * scale;
    const float buttonX = panelX + (panelWidth - buttonWidth) * 0.5f;
    const float firstButtonY = panelY + 146.0f * scale;

    Layout layout{
        {panelX, panelY, panelWidth, panelHeight},
        {},
        {},
        {},
        {},
        {},
        {},
        {},
        {},
    };
    if (page == Page::Main) {
        layout.continueButton =
            {buttonX, firstButtonY, buttonWidth, buttonHeight};
        layout.newGameButton =
            {buttonX, firstButtonY + (buttonHeight + buttonGap), buttonWidth,
             buttonHeight};
        layout.chapterButton =
            {buttonX, firstButtonY + (buttonHeight + buttonGap) * 2.0f,
             buttonWidth, buttonHeight};
        layout.settingsButton =
            {buttonX, firstButtonY + (buttonHeight + buttonGap) * 3.0f,
             buttonWidth, buttonHeight};
        layout.creditsButton =
            {buttonX, firstButtonY + (buttonHeight + buttonGap) * 4.0f,
             buttonWidth, buttonHeight};
        layout.exitButton =
            {buttonX, firstButtonY + (buttonHeight + buttonGap) * 5.0f,
             buttonWidth, buttonHeight};
    } else {
        const float itemWidth =
            std::min(440.0f * scale, panelWidth - 64.0f * scale);
        const float itemHeight = std::max(48.0f, 72.0f * scale);
        layout.item = {panelX + (panelWidth - itemWidth) * 0.5f,
                       panelY + 158.0f * scale, itemWidth, itemHeight};
        layout.backButton = {buttonX, panelY + panelHeight - 88.0f * scale,
                             buttonWidth, buttonHeight};
    }
    return layout;
}

void MainUI::drawPageContent(const UiText& text, const Layout& layout,
                             Page page, float uiScale) const {
    if (page == Page::Main) {
        const Color buttonColors[] = {
            constants::kButtonStart, constants::kStatueColor,
            constants::kPortalGlow, constants::kHouseWindow,
            constants::kPistolGrip, constants::kButtonExit};
        const Rect buttons[] = {layout.continueButton, layout.newGameButton,
                                layout.chapterButton, layout.settingsButton,
                                layout.creditsButton, layout.exitButton};
        const char* labels[] = {
            text.continueButton, text.newGameButton, text.chapterButton,
            text.settingsButton, text.creditsButton, text.exitButton};
        for (int index = 0; index < 6; ++index) {
            ui::drawButton(buttons[index], labels[index], buttonColors[index],
                           ui::contains(buttons[index], mouse_.x, mouse_.y),
                           uiScale);
        }
        ThreeDUtils::drawCenteredText(
            text.mainSubtitle, layout.panel, std::max(1.0f, 2.2f * uiScale),
            ThreeDUtils::shade(constants::kInkColor, 1.25f),
            94.0f * uiScale);
        ThreeDUtils::drawCenteredText(
            text.mainHint, layout.panel, std::max(1.0f, 1.4f * uiScale),
            ThreeDUtils::shade(constants::kInkColor, 1.15f),
            layout.panel.height - 28.0f * uiScale);
        return;
    }

    const char* subtitle = page == Page::Chapters
                               ? text.chapterSubtitle
                               : page == Page::Settings ? text.settingsSubtitle
                                                        : text.creditsSubtitle;
    const char* item = page == Page::Chapters
                           ? text.chapterItem
                           : page == Page::Settings ? text.settingsItem
                                                    : text.creditsItem;
    ThreeDUtils::drawCenteredText(
        subtitle, layout.panel, std::max(1.0f, 2.0f * uiScale),
        ThreeDUtils::shade(constants::kInkColor, 1.2f), 102.0f * uiScale);
    ui::drawButton(layout.item, item, constants::kPortalGlow,
                   ui::contains(layout.item, mouse_.x, mouse_.y), uiScale);
    ui::drawButton(layout.backButton, text.backButton, constants::kButtonExit,
                   ui::contains(layout.backButton, mouse_.x, mouse_.y),
                   uiScale);
}

}  // namespace pixel_world
