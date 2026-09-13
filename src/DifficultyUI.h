#pragma once

#include "BaseUI.h"
#include "Localization.h"

namespace pixel_world {

class DifficultyUI final : public BaseUI {
public:
    explicit DifficultyUI(Language language);

    MenuAction update(GLFWwindow* window, int framebufferWidth,
                      int framebufferHeight) override;
    void render(int framebufferWidth, int framebufferHeight) const override;
    bool buttonClicked() const;

private:
    struct Layout {
        Rect panel;
        Rect easyButton;
        Rect normalButton;
        Rect hardButton;
        Rect backButton;
    };

    static Layout makeLayout(int width, int height);

    Language language_;
    bool previousMouseDown_;
    bool previousEscapeDown_;
    bool buttonClicked_;
    MouseState mouse_;
};

}  // namespace pixel_world
