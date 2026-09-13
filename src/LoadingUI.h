#pragma once

#include "BaseUI.h"
#include "Localization.h"

namespace pixel_world {

class LoadingUI final : public BaseUI {
public:
    explicit LoadingUI(Language language);

    MenuAction update(GLFWwindow* window, int framebufferWidth,
                      int framebufferHeight) override;
    void render(int framebufferWidth, int framebufferHeight) const override;
    void setProgress(float progress);

private:
    Language language_;
    float progress_;
};

}  // namespace pixel_world
