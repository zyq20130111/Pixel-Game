#pragma once

#include "BaseUI.h"
#include "Localization.h"

namespace pixel_world {

class LoginScreen final : public BaseUI {
public:
    explicit LoginScreen(Language language);

    MenuAction update(GLFWwindow* window, int framebufferWidth,
                      int framebufferHeight) override;
    void render(int framebufferWidth, int framebufferHeight) const override;
    bool buttonClicked() const;

private:
    static MenuLayout makeLayout(int width, int height);
    static bool contains(const Rect& rect, float x, float y);
    static MouseState getMouseState(GLFWwindow* window, int framebufferWidth,
                                    int framebufferHeight);
    static void drawButton(const Rect& button, const char* label,
                           const Color& baseColor, bool hovered,
                           float uiScale);

    Language language_;
    bool previousMouseDown_;
    bool buttonClicked_;
    MouseState mouse_;
};

}  // namespace pixel_world
