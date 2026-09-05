#pragma once

#include "platform.h"
#include "types.h"

namespace pixel_world {

class BaseUI {
public:
    virtual ~BaseUI();

    virtual MenuAction update(GLFWwindow* window, int framebufferWidth,
                              int framebufferHeight) = 0;
    virtual void render(int framebufferWidth, int framebufferHeight) const = 0;
};

}  // namespace pixel_world
