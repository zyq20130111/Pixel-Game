#pragma once

#include "platform.h"
#include "types.h"

#include <string>

namespace pixel_world {

class ThreeDUtils {
public:
    static float dot(const Vec3& a, const Vec3& b);
    static Vec3 cross(const Vec3& a, const Vec3& b);
    static float length(const Vec3& value);
    static Vec3 normalize(const Vec3& value);
    static Color shade(const Color& color, float factor);
    static float smoothStep(float value);
    static Vec3 lerp(const Vec3& a, const Vec3& b, float t);

    static Vec3 cameraForward();
    static Vec3 cameraForward(float yawDegrees, float pitchDegrees);
    static Vec3 cameraRight();
    static Vec3 cameraRight(float yawDegrees, float pitchDegrees);
    static Vec3 cameraUp();
    static Vec3 cameraUp(float yawDegrees, float pitchDegrees);
    static Vec3 cameraToWorld(const Vec3& cameraPosition,
                              const Vec3& viewPosition);
    static Vec3 cameraToWorld(const Vec3& cameraPosition,
                              const Vec3& viewPosition, float yawDegrees,
                              float pitchDegrees);
    static float currentFieldOfView(float aimAmount);

    static void setProjection(int width, int height,
                              float fieldOfViewDegrees = 60.0f);
    static void framebufferSizeCallback(GLFWwindow* window, int width,
                                        int height);
    static void configureRendering();
    static void setUiProjection(int width, int height);
    static void applyCamera(const Vec3& cameraPosition);
    static void applyCamera(const Vec3& cameraPosition, float yawDegrees,
                            float pitchDegrees);

    static void drawRect2D(const Rect& rect, const Color& color,
                           float alpha = 1.0f);
    static void drawFrame2D(const Rect& rect, const Color& color,
                            float thickness);
    static float textWidth(const std::string& text, float pixelScale);
    static void drawText(const std::string& text, float x, float y,
                         float pixelScale, const Color& color);
    static void drawCenteredText(const std::string& text, const Rect& area,
                                 float pixelScale, const Color& color,
                                 float yOffset = 0.0f);

    static void drawFace(const Vec3& a, const Vec3& b, const Vec3& c,
                         const Vec3& d, const Color& color);
    static void drawCube(const Vec3& center, const Vec3& size,
                         const Color& baseColor);
    static void drawPivotedCube(const Vec3& pivot, const Vec3& offset,
                                const Vec3& size, float angleDegrees,
                                const Color& color);
    static void drawViewPivotedCube(const Vec3& pivot, const Vec3& offset,
                                    const Vec3& size, float angleDegrees,
                                    const Color& color);
    static void drawViewOrientedCube(const Vec3& center, const Vec3& size,
                                     const Vec3& rotationDegrees,
                                     const Color& color);
};

}  // namespace pixel_world
