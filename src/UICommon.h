#pragma once

#include "platform.h"
#include "types.h"

namespace pixel_world::ui {

float scaleFor(int width, int height);
bool contains(const Rect& rect, float x, float y);
MouseState getMouseState(GLFWwindow* window, int framebufferWidth,
                         int framebufferHeight);

void beginOverlay(int framebufferWidth, int framebufferHeight);
void endOverlay(int framebufferWidth, int framebufferHeight);
void drawPanel(const Rect& panel, float uiScale);
void drawButton(const Rect& button, const char* label, const Color& baseColor,
                bool hovered, float uiScale);

}  // namespace pixel_world::ui
