#pragma once

#include <string>

namespace pixel_world {

struct Vec3 {
    float x{0.0f};
    float y{0.0f};
    float z{0.0f};
};

inline Vec3 operator+(const Vec3& a, const Vec3& b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

inline Vec3 operator-(const Vec3& a, const Vec3& b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

inline Vec3 operator*(const Vec3& value, float scalar) {
    return {value.x * scalar, value.y * scalar, value.z * scalar};
}

struct Color {
    float red;
    float green;
    float blue;
};

struct Rect {
    float x;
    float y;
    float width;
    float height;
};

struct MouseState {
    float x;
    float y;
    bool pressed;
};

enum class ImpactType {
    Ground,
    Character,
};

struct MenuLayout {
    Rect panel;
    Rect loginButton;
    Rect exitButton;
};

struct ImpactEffect {
    Vec3 position;
    ImpactType type;
    float age;
    float duration;
};

enum class CharacterHitZone {
    Legs,
    Waist,
    Head,
};

enum class AppState {
    MainMenu,
    Playing,
};

enum class MenuAction {
    None,
    Login,
    Exit,
};

}  // namespace pixel_world
