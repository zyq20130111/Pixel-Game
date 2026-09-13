#pragma once

#include "BaseUI.h"
#include "Localization.h"

namespace pixel_world {

class MainUI final : public BaseUI {
public:
    explicit MainUI(Language language);

    MenuAction update(GLFWwindow* window, int framebufferWidth,
                      int framebufferHeight) override;
    void render(int framebufferWidth, int framebufferHeight) const override;

    bool buttonClicked() const;
    void showMainMenu();

private:
    enum class Page {
        Main,
        Chapters,
        Settings,
        Credits,
    };

    struct Layout {
        Rect panel;
        Rect continueButton;
        Rect newGameButton;
        Rect chapterButton;
        Rect settingsButton;
        Rect creditsButton;
        Rect exitButton;
        Rect backButton;
        Rect item;
    };

    static Layout makeLayout(int width, int height, Page page);
    void drawPageContent(const UiText& text, const Layout& layout,
                         Page page, float uiScale) const;

    Language language_;
    Page page_;
    bool previousMouseDown_;
    bool previousEscapeDown_;
    bool previousEnterDown_;
    bool buttonClicked_;
    MouseState mouse_;
};

}  // namespace pixel_world
