#include "ThreeDUtils.h"

#include "game_constants.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>

namespace pixel_world {
namespace {

std::array<const char*, 7> glyphFor(char character) {
    switch (static_cast<char>(std::toupper(static_cast<unsigned char>(character)))) {
        case 'A':
            return {"01110", "10001", "10001", "11111", "10001", "10001", "10001"};
        case 'D':
            return {"11110", "10001", "10001", "10001", "10001", "10001", "11110"};
        case 'E':
            return {"11111", "10000", "10000", "11110", "10000", "10000", "11111"};
        case 'G':
            return {"01110", "10001", "10000", "10111", "10001", "10001", "01110"};
        case 'I':
            return {"11111", "00100", "00100", "00100", "00100", "00100", "11111"};
        case 'L':
            return {"10000", "10000", "10000", "10000", "10000", "10000", "11111"};
        case 'M':
            return {"10001", "11011", "10101", "10101", "10001", "10001", "10001"};
        case 'N':
            return {"10001", "11001", "10101", "10011", "10001", "10001", "10001"};
        case 'O':
            return {"01110", "10001", "10001", "10001", "10001", "10001", "01110"};
        case 'P':
            return {"11110", "10001", "10001", "11110", "10000", "10000", "10000"};
        case 'Q':
            return {"01110", "10001", "10001", "10001", "10101", "10010", "01101"};
        case 'R':
            return {"11110", "10001", "10001", "11110", "10100", "10010", "10001"};
        case 'S':
            return {"01111", "10000", "10000", "01110", "00001", "00001", "11110"};
        case 'T':
            return {"11111", "00100", "00100", "00100", "00100", "00100", "00100"};
        case 'U':
            return {"10001", "10001", "10001", "10001", "10001", "10001", "01110"};
        case 'V':
            return {"10001", "10001", "10001", "10001", "10001", "01010", "00100"};
        case 'W':
            return {"10001", "10001", "10001", "10101", "10101", "11011", "10001"};
        case 'X':
            return {"10001", "10001", "01010", "00100", "01010", "10001", "10001"};
        case 'Y':
            return {"10001", "10001", "01010", "00100", "00100", "00100", "00100"};
        case '3':
            return {"11110", "00001", "00001", "01110", "00001", "00001", "11110"};
        default:
            return {"00000", "00000", "00000", "00000", "00000", "00000", "00000"};
    }
}

}  // namespace

float ThreeDUtils::dot(const Vec3& a, const Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 ThreeDUtils::cross(const Vec3& a, const Vec3& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x};
}

float ThreeDUtils::length(const Vec3& value) {
    return std::sqrt(dot(value, value));
}

Vec3 ThreeDUtils::normalize(const Vec3& value) {
    const float valueLength = length(value);
    if (valueLength <= 0.0001f) {
        return {};
    }
    return {value.x / valueLength, value.y / valueLength, value.z / valueLength};
}

Color ThreeDUtils::shade(const Color& color, float factor) {
    return {std::clamp(color.red * factor, 0.0f, 1.0f),
            std::clamp(color.green * factor, 0.0f, 1.0f),
            std::clamp(color.blue * factor, 0.0f, 1.0f)};
}

float ThreeDUtils::smoothStep(float value) {
    const float t = std::clamp(value, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

Vec3 ThreeDUtils::lerp(const Vec3& a, const Vec3& b, float t) {
    return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t,
            a.z + (b.z - a.z) * t};
}

Vec3 ThreeDUtils::cameraForward() {
    return normalize({0.0f, -0.22f, -1.0f});
}

Vec3 ThreeDUtils::cameraRight() {
    return normalize(cross(cameraForward(), {0.0f, 1.0f, 0.0f}));
}

Vec3 ThreeDUtils::cameraUp() {
    return cross(cameraRight(), cameraForward());
}

Vec3 ThreeDUtils::cameraToWorld(const Vec3& cameraPosition,
                                const Vec3& viewPosition) {
    return cameraPosition + cameraRight() * viewPosition.x +
           cameraUp() * viewPosition.y + cameraForward() * (-viewPosition.z);
}

float ThreeDUtils::currentFieldOfView(float aimAmount) {
    const float easedAim = smoothStep(aimAmount);
    return constants::kFieldOfViewDegrees +
           (constants::kAimFieldOfViewDegrees - constants::kFieldOfViewDegrees) *
               easedAim;
}

void ThreeDUtils::setProjection(int width, int height,
                                float fieldOfViewDegrees) {
    if (height <= 0) {
        height = 1;
    }

    const float aspect = static_cast<float>(width) / static_cast<float>(height);
    const float top =
        constants::kNearPlane *
        std::tan(fieldOfViewDegrees * constants::kPi / 360.0f);
    const float right = top * aspect;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-right, right, -top, top, constants::kNearPlane,
              constants::kFarPlane);
    glMatrixMode(GL_MODELVIEW);
}

void ThreeDUtils::framebufferSizeCallback(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
    setProjection(width, height);
}

void ThreeDUtils::configureRendering() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DITHER);
    glShadeModel(GL_FLAT);
}

void ThreeDUtils::setUiProjection(int width, int height) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, static_cast<GLdouble>(width), static_cast<GLdouble>(height),
            0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void ThreeDUtils::applyCamera(const Vec3& cameraPosition) {
    const Vec3 forward = cameraForward();
    const Vec3 up{0.0f, 1.0f, 0.0f};
    const Vec3 center = cameraPosition + forward;
    const Vec3 f = normalize(center - cameraPosition);
    const Vec3 s = normalize(cross(f, up));
    const Vec3 u = cross(s, f);

    const GLfloat matrix[16] = {
        s.x, u.x, -f.x, 0.0f,
        s.y, u.y, -f.y, 0.0f,
        s.z, u.z, -f.z, 0.0f,
        -dot(s, cameraPosition), -dot(u, cameraPosition),
        dot(f, cameraPosition), 1.0f,
    };

    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(matrix);
}

void ThreeDUtils::drawRect2D(const Rect& rect, const Color& color, float alpha) {
    glColor4f(color.red, color.green, color.blue, alpha);
    glBegin(GL_QUADS);
    glVertex2f(rect.x, rect.y);
    glVertex2f(rect.x + rect.width, rect.y);
    glVertex2f(rect.x + rect.width, rect.y + rect.height);
    glVertex2f(rect.x, rect.y + rect.height);
    glEnd();
}

void ThreeDUtils::drawFrame2D(const Rect& rect, const Color& color,
                              float thickness) {
    drawRect2D({rect.x, rect.y, rect.width, thickness}, color);
    drawRect2D({rect.x, rect.y + rect.height - thickness, rect.width, thickness},
               color);
    drawRect2D({rect.x, rect.y, thickness, rect.height}, color);
    drawRect2D({rect.x + rect.width - thickness, rect.y, thickness, rect.height},
               color);
}

float ThreeDUtils::textWidth(const std::string& text, float pixelScale) {
    if (text.empty()) {
        return 0.0f;
    }
    return static_cast<float>(text.size() * 6) * pixelScale - pixelScale;
}

void ThreeDUtils::drawText(const std::string& text, float x, float y,
                           float pixelScale, const Color& color) {
    for (const char character : text) {
        const auto glyph = glyphFor(character);
        for (int row = 0; row < 7; ++row) {
            for (int column = 0; column < 5; ++column) {
                if (glyph[row][column] == '1') {
                    drawRect2D(
                        {x + static_cast<float>(column) * pixelScale,
                         y + static_cast<float>(row) * pixelScale, pixelScale,
                         pixelScale},
                        color);
                }
            }
        }
        x += 6.0f * pixelScale;
    }
}

void ThreeDUtils::drawCenteredText(const std::string& text, const Rect& area,
                                   float pixelScale, const Color& color,
                                   float yOffset) {
    const float x = area.x + (area.width - textWidth(text, pixelScale)) * 0.5f;
    drawText(text, x, area.y + yOffset, pixelScale, color);
}

void ThreeDUtils::drawFace(const Vec3& a, const Vec3& b, const Vec3& c,
                           const Vec3& d, const Color& color) {
    glColor3f(color.red, color.green, color.blue);
    glBegin(GL_QUADS);
    glVertex3f(a.x, a.y, a.z);
    glVertex3f(b.x, b.y, b.z);
    glVertex3f(c.x, c.y, c.z);
    glVertex3f(d.x, d.y, d.z);
    glEnd();
}

void ThreeDUtils::drawCube(const Vec3& center, const Vec3& size,
                           const Color& baseColor) {
    const float hx = size.x * 0.5f;
    const float hy = size.y * 0.5f;
    const float hz = size.z * 0.5f;

    const Vec3 p000{center.x - hx, center.y - hy, center.z - hz};
    const Vec3 p001{center.x - hx, center.y - hy, center.z + hz};
    const Vec3 p010{center.x - hx, center.y + hy, center.z - hz};
    const Vec3 p011{center.x - hx, center.y + hy, center.z + hz};
    const Vec3 p100{center.x + hx, center.y - hy, center.z - hz};
    const Vec3 p101{center.x + hx, center.y - hy, center.z + hz};
    const Vec3 p110{center.x + hx, center.y + hy, center.z - hz};
    const Vec3 p111{center.x + hx, center.y + hy, center.z + hz};

    drawFace(p000, p100, p110, p010, shade(baseColor, 0.78f));
    drawFace(p001, p011, p111, p101, shade(baseColor, 0.92f));
    drawFace(p000, p010, p011, p001, shade(baseColor, 0.68f));
    drawFace(p100, p101, p111, p110, shade(baseColor, 0.85f));
    drawFace(p010, p110, p111, p011, shade(baseColor, 1.10f));
    drawFace(p000, p001, p101, p100, shade(baseColor, 0.58f));

    // A single ink pass gives every low-poly prop a readable cartoon silhouette.
    glColor3f(constants::kInkColor.red, constants::kInkColor.green,
              constants::kInkColor.blue);
    glLineWidth(constants::kCartoonOutlineWidth);
    glBegin(GL_LINES);
    const auto edge = [](const Vec3& from, const Vec3& to) {
        glVertex3f(from.x, from.y, from.z);
        glVertex3f(to.x, to.y, to.z);
    };
    edge(p000, p001);
    edge(p001, p101);
    edge(p101, p100);
    edge(p100, p000);
    edge(p010, p011);
    edge(p011, p111);
    edge(p111, p110);
    edge(p110, p010);
    edge(p000, p010);
    edge(p001, p011);
    edge(p101, p111);
    edge(p100, p110);
    glEnd();
}

void ThreeDUtils::drawPivotedCube(const Vec3& pivot, const Vec3& offset,
                                  const Vec3& size, float angleDegrees,
                                  const Color& color) {
    glPushMatrix();
    glTranslatef(pivot.x, pivot.y, pivot.z);
    glRotatef(angleDegrees, 1.0f, 0.0f, 0.0f);
    glTranslatef(offset.x, offset.y, offset.z);
    drawCube({}, size, color);
    glPopMatrix();
}

void ThreeDUtils::drawViewPivotedCube(const Vec3& pivot, const Vec3& offset,
                                      const Vec3& size, float angleDegrees,
                                      const Color& color) {
    drawPivotedCube(pivot, offset, size, angleDegrees, color);
}

void ThreeDUtils::drawViewOrientedCube(const Vec3& center, const Vec3& size,
                                       const Vec3& rotationDegrees,
                                       const Color& color) {
    glPushMatrix();
    glTranslatef(center.x, center.y, center.z);
    glRotatef(rotationDegrees.z, 0.0f, 0.0f, 1.0f);
    glRotatef(rotationDegrees.y, 0.0f, 1.0f, 0.0f);
    glRotatef(rotationDegrees.x, 1.0f, 0.0f, 0.0f);
    drawCube({}, size, color);
    glPopMatrix();
}

}  // namespace pixel_world
