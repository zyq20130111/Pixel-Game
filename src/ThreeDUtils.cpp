#include "ThreeDUtils.h"

#include "game_constants.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <unordered_map>
#include <vector>

namespace pixel_world {
namespace {

std::array<const char*, 7> glyphFor(std::uint32_t character) {
    if (character <= 0x7Fu) {
        switch (static_cast<char>(
            std::toupper(static_cast<unsigned char>(character)))) {
        case 'B':
            return {"11110", "10001", "10001", "11110", "10001", "10001", "11110"};
        case 'C':
            return {"01111", "10000", "10000", "10000", "10000", "10000", "01111"};
        case 'A':
            return {"01110", "10001", "10001", "11111", "10001", "10001", "10001"};
        case 'D':
            return {"11110", "10001", "10001", "10001", "10001", "10001", "11110"};
        case 'E':
            return {"11111", "10000", "10000", "11110", "10000", "10000", "11111"};
        case 'F':
            return {"11111", "10000", "10000", "11110", "10000", "10000", "10000"};
        case 'G':
            return {"01110", "10001", "10000", "10111", "10001", "10001", "01110"};
        case 'H':
            return {"10001", "10001", "10001", "11111", "10001", "10001", "10001"};
        case 'I':
            return {"11111", "00100", "00100", "00100", "00100", "00100", "11111"};
        case 'K':
            return {"10001", "10010", "10100", "11000", "10100", "10010", "10001"};
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
        case 'Z':
            return {"11111", "00001", "00010", "00100", "01000", "10000", "11111"};
        case '3':
            return {"11110", "00001", "00001", "01110", "00001", "00001", "11110"};
        default:
            break;
        }
    }

    // Chinese glyphs are handled before the legacy switch below. Keeping the
    // mappings in an ASCII-only block avoids source-code page corruption.
    switch (character) {
        case 0x50CFu:  // U+50CF
            return {"10001", "11011", "10101", "11111", "00100", "01010", "10001"};
        case 0x7D20u:  // U+7D20
            return {"11111", "00100", "11111", "10101", "01110", "10101", "00100"};
        case 0x4E16u:  // U+4E16
            return {"00100", "11111", "00100", "11111", "00100", "00100", "11111"};
        case 0x754Cu:  // U+754C
            return {"11111", "10001", "10101", "10001", "11111", "00100", "11111"};
        case 0x5192u:  // U+5192
            return {"01110", "10001", "11111", "10001", "11111", "10001", "11111"};
        case 0x9669u:  // U+9669
            return {"00100", "01110", "10101", "00100", "11111", "00100", "00100"};
        case 0x5F00u:  // U+5F00
            return {"10001", "10001", "11111", "10001", "11111", "10001", "10001"};
        case 0x59CBu:  // U+59CB
            return {"00100", "11111", "00100", "01110", "10101", "10101", "01110"};
        case 0x6E38u:  // U+6E38
            return {"10101", "11111", "00100", "10111", "10101", "10101", "10101"};
        case 0x620Fu:  // U+620F
            return {"10101", "00100", "11111", "00100", "10101", "10101", "10001"};
        case 0x79FBu:  // U+79FB
            return {"10101", "11111", "00100", "10101", "11111", "00100", "00100"};
        case 0x52A8u:  // U+52A8
            return {"00100", "01110", "00100", "11111", "00100", "01010", "10001"};
        case 0x9000u:  // U+9000
            return {"11111", "00001", "01111", "00101", "01001", "10001", "11111"};
        case 0x51FAu:  // U+51FA
            return {"00100", "10101", "10101", "10101", "10101", "10101", "11111"};
        case 0x786Eu:  // U+786E
            return {"11111", "10000", "10111", "10101", "11111", "00100", "00100"};
        case 0x5B9Au:  // U+5B9A
            return {"00100", "11111", "10101", "00100", "01110", "10001", "10001"};
        case 0x5417u:  // U+5417
            return {"10111", "10101", "11111", "00100", "00100", "00100", "00100"};
        case 0x662Fu:  // U+662F
            return {"11111", "00100", "01110", "00100", "11111", "10001", "11111"};
        case 0x5426u:  // U+5426
            return {"11111", "00100", "01110", "00100", "00100", "10101", "11111"};
        default:
            break;
    }

#if 0
    switch (character) {
        case 0x50CFu:  // 像
            return {"10001", "11011", "10101", "11111", "00100", "01010", "10001"};
        case 0x7D20u:  // 素
            return {"11111", "00100", "11111", "10101", "01110", "10101", "00100"};
        case 0x4E16u:  // 世
            return {"00100", "11111", "00100", "11111", "00100", "00100", "11111"};
        case 0x754Cu:  // 界
            return {"11111", "10001", "10101", "10001", "11111", "00100", "11111"};
        case 0x5192u:  // 冒
            return {"01110", "10001", "11111", "10001", "11111", "10001", "11111"};
        case 0x9669u:  // 险
            return {"00100", "01110", "10101", "00100", "11111", "00100", "00100"};
        case 0x5F00u:  // 开
            return {"10001", "10001", "11111", "10001", "11111", "10001", "10001"};
        case 0x59CBu:  // 始
            return {"00100", "11111", "00100", "01110", "10101", "10101", "01110"};
        case 0x6E38u:  // 游
            return {"10101", "11111", "00100", "10111", "10101", "10101", "10101"};
        case 0x620Fu:  // 戏
            return {"10101", "00100", "11111", "00100", "10101", "10101", "10001"};
        case 0x79FBu:  // 移
            return {"10101", "11111", "00100", "10101", "11111", "00100", "00100"};
        case 0x52A8u:  // 动
            return {"00100", "01110", "00100", "11111", "00100", "01010", "10001"};
        case 0x9000u:  // 退
            return {"11111", "00001", "01111", "00101", "01001", "10001", "11111"};
        case 0x51FAu:  // 出
            return {"00100", "10101", "10101", "10101", "10101", "10101", "11111"};
        case 0x786Eu:  // 确
            return {"11111", "10000", "10111", "10101", "11111", "00100", "00100"};
        case 0x5B9Au:  // 定
            return {"00100", "11111", "10101", "00100", "01110", "10001", "10001"};
        case 0x5417u:  // 吗
            return {"10111", "10101", "11111", "00100", "00100", "00100", "00100"};
        case 0x662Fu:  // 是
            return {"11111", "00100", "01110", "00100", "11111", "10001", "11111"};
        case 0x5426u:  // 否
            return {"11111", "00100", "01110", "00100", "00100", "10101", "11111"};
        default:
            return {"00000", "00000", "00000", "00000", "00000", "00000", "00000"};
    }
#endif
    return {"00000", "00000", "00000", "00000", "00000", "00000", "00000"};
}

std::vector<std::uint32_t> decodeUtf8(const std::string& text) {
    std::vector<std::uint32_t> codepoints;
    for (std::size_t index = 0; index < text.size();) {
        const auto byte = static_cast<unsigned char>(text[index]);
        if (byte <= 0x7Fu) {
            codepoints.push_back(byte);
            ++index;
            continue;
        }

        std::size_t length = 0;
        std::uint32_t codepoint = 0;
        std::uint32_t minimum = 0;
        if ((byte & 0xE0u) == 0xC0u) {
            length = 2;
            codepoint = byte & 0x1Fu;
            minimum = 0x80u;
        } else if ((byte & 0xF0u) == 0xE0u) {
            length = 3;
            codepoint = byte & 0x0Fu;
            minimum = 0x800u;
        } else if ((byte & 0xF8u) == 0xF0u) {
            length = 4;
            codepoint = byte & 0x07u;
            minimum = 0x10000u;
        } else {
            codepoints.push_back('?');
            ++index;
            continue;
        }

        if (index + length > text.size()) {
            codepoints.push_back('?');
            ++index;
            continue;
        }

        bool valid = true;
        for (std::size_t offset = 1; offset < length; ++offset) {
            const auto continuation =
                static_cast<unsigned char>(text[index + offset]);
            if ((continuation & 0xC0u) != 0x80u) {
                valid = false;
                break;
            }
            codepoint = (codepoint << 6u) | (continuation & 0x3Fu);
        }

        if (!valid || codepoint < minimum || codepoint > 0x10FFFFu ||
            (codepoint >= 0xD800u && codepoint <= 0xDFFFu)) {
            codepoints.push_back('?');
            ++index;
            continue;
        }

        codepoints.push_back(codepoint);
        index += length;
    }
    return codepoints;
}

#ifdef _WIN32
constexpr int kSystemPixelFontHeight = 12;
constexpr float kSystemPixelFontScale = 8.0f / 12.0f;

struct SystemTextTexture {
    GLuint texture{0};
    int width{0};
    int height{0};
};

std::unordered_map<std::string, SystemTextTexture>& systemTextTextures() {
    static auto* textures =
        new std::unordered_map<std::string, SystemTextTexture>();
    return *textures;
}

bool utf8ToWide(const std::string& text, std::wstring& wideText) {
    if (text.empty()) {
        wideText.clear();
        return true;
    }

    const int sourceLength = static_cast<int>(text.size());
    int wideLength = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                         text.data(), sourceLength, nullptr, 0);
    if (wideLength <= 0) {
        wideLength = MultiByteToWideChar(CP_UTF8, 0, text.data(), sourceLength,
                                         nullptr, 0);
    }
    if (wideLength <= 0) {
        return false;
    }

    wideText.resize(static_cast<std::size_t>(wideLength));
    return MultiByteToWideChar(CP_UTF8, 0, text.data(), sourceLength,
                               wideText.data(), wideLength) > 0;
}

HFONT createSystemFont(int fontHeight) {
    return CreateFontW(
        -fontHeight, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");
}

bool measureSystemText(const std::wstring& text, int fontHeight, int& width,
                       int& height) {
    HDC screenDc = GetDC(nullptr);
    if (screenDc == nullptr) {
        return false;
    }

    HDC dc = CreateCompatibleDC(screenDc);
    HFONT font = createSystemFont(fontHeight);
    if (dc == nullptr || font == nullptr) {
        if (font != nullptr) {
            DeleteObject(font);
        }
        if (dc != nullptr) {
            DeleteDC(dc);
        }
        ReleaseDC(nullptr, screenDc);
        return false;
    }

    const HGDIOBJ previousFont = SelectObject(dc, font);
    SIZE extent{};
    TEXTMETRICW metrics{};
    const bool measured =
        GetTextExtentPoint32W(dc, text.data(), static_cast<int>(text.size()),
                              &extent) != FALSE &&
        GetTextMetricsW(dc, &metrics) != FALSE;
    const int padding = std::max(2, fontHeight / 8);
    if (measured) {
        width = std::max(1, static_cast<int>(extent.cx) + padding * 2);
        height =
            std::max(1, static_cast<int>(metrics.tmHeight) + padding * 2);
    }

    SelectObject(dc, previousFont);
    DeleteObject(font);
    DeleteDC(dc);
    ReleaseDC(nullptr, screenDc);
    return measured;
}

SystemTextTexture* getSystemTextTexture(const std::string& text) {
    // Render once at a tiny fixed resolution, then enlarge with nearest-neighbor
    // sampling so every source pixel becomes a crisp pixel-art block.
    const int fontHeight = kSystemPixelFontHeight;
    std::wstring wideText;
    if (!utf8ToWide(text, wideText)) {
        return nullptr;
    }

    std::string cacheKey = std::to_string(fontHeight);
    cacheKey.push_back('\n');
    cacheKey += text;
    auto& cache = systemTextTextures();
    const auto existing = cache.find(cacheKey);
    if (existing != cache.end()) {
        return &existing->second;
    }

    int width = 0;
    int height = 0;
    if (!measureSystemText(wideText, fontHeight, width, height)) {
        return nullptr;
    }

    HDC screenDc = GetDC(nullptr);
    HDC dc = screenDc != nullptr ? CreateCompatibleDC(screenDc) : nullptr;
    HFONT font = createSystemFont(fontHeight);
    if (screenDc == nullptr || dc == nullptr || font == nullptr) {
        if (font != nullptr) {
            DeleteObject(font);
        }
        if (dc != nullptr) {
            DeleteDC(dc);
        }
        if (screenDc != nullptr) {
            ReleaseDC(nullptr, screenDc);
        }
        return nullptr;
    }

    BITMAPINFO bitmapInfo{};
    bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
    bitmapInfo.bmiHeader.biWidth = width;
    bitmapInfo.bmiHeader.biHeight = -height;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    void* bitmapBits = nullptr;
    HBITMAP bitmap =
        CreateDIBSection(dc, &bitmapInfo, DIB_RGB_COLORS, &bitmapBits, nullptr,
                         0);
    if (bitmap == nullptr || bitmapBits == nullptr) {
        if (bitmap != nullptr) {
            DeleteObject(bitmap);
        }
        DeleteObject(font);
        DeleteDC(dc);
        ReleaseDC(nullptr, screenDc);
        return nullptr;
    }

    const HGDIOBJ previousBitmap = SelectObject(dc, bitmap);
    const HGDIOBJ previousFont = SelectObject(dc, font);
    std::memset(bitmapBits, 0,
                static_cast<std::size_t>(width) *
                    static_cast<std::size_t>(height) * 4u);

    const int padding = std::max(2, fontHeight / 8);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(255, 255, 255));
    TextOutW(dc, padding, padding, wideText.data(),
             static_cast<int>(wideText.size()));

    std::vector<unsigned char> rgba(
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height) *
        4u);
    const auto* bgra = static_cast<const unsigned char*>(bitmapBits);
    for (std::size_t pixel = 0; pixel < rgba.size(); pixel += 4) {
        const unsigned char coverage =
            std::max({bgra[pixel], bgra[pixel + 1], bgra[pixel + 2]});
        const unsigned char alpha = coverage >= 128 ? 255 : 0;
        rgba[pixel] = 255;
        rgba[pixel + 1] = 255;
        rgba[pixel + 2] = 255;
        rgba[pixel + 3] = alpha;
    }

    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, rgba.data());
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    SelectObject(dc, previousFont);
    SelectObject(dc, previousBitmap);
    DeleteObject(font);
    DeleteObject(bitmap);
    DeleteDC(dc);
    ReleaseDC(nullptr, screenDc);

    const auto [iterator, inserted] =
        cache.emplace(std::move(cacheKey),
                      SystemTextTexture{texture, width, height});
    return inserted ? &iterator->second : &iterator->second;
}

bool drawSystemText(const std::string& text, float x, float y,
                    float pixelScale, const Color& color) {
    SystemTextTexture* systemText =
        getSystemTextTexture(text);
    if (systemText == nullptr || systemText->texture == 0) {
        return false;
    }

    const float drawScale = pixelScale * kSystemPixelFontScale;
    const float width = static_cast<float>(systemText->width) * drawScale;
    const float height = static_cast<float>(systemText->height) * drawScale;
    glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_CURRENT_BIT);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, systemText->texture);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glColor4f(color.red, color.green, color.blue, 1.0f);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(x, y);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(x + width, y);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(x + width, y + height);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(x, y + height);
    glEnd();
    glPopAttrib();
    return true;
}
#endif

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

Vec3 ThreeDUtils::cameraForward(float yawDegrees, float pitchDegrees) {
    const float yawRadians = yawDegrees * constants::kPi / 180.0f;
    const float pitchRadians = pitchDegrees * constants::kPi / 180.0f;
    const float horizontalLength = std::cos(pitchRadians);
    return normalize({std::sin(yawRadians) * horizontalLength,
                      std::sin(pitchRadians),
                      -std::cos(yawRadians) * horizontalLength});
}

Vec3 ThreeDUtils::cameraRight() {
    return normalize(cross(cameraForward(), {0.0f, 1.0f, 0.0f}));
}

Vec3 ThreeDUtils::cameraRight(float yawDegrees, float pitchDegrees) {
    return normalize(
        cross(cameraForward(yawDegrees, pitchDegrees), {0.0f, 1.0f, 0.0f}));
}

Vec3 ThreeDUtils::cameraUp() {
    return cross(cameraRight(), cameraForward());
}

Vec3 ThreeDUtils::cameraUp(float yawDegrees, float pitchDegrees) {
    return cross(cameraRight(yawDegrees, pitchDegrees),
                 cameraForward(yawDegrees, pitchDegrees));
}

Vec3 ThreeDUtils::cameraToWorld(const Vec3& cameraPosition,
                                const Vec3& viewPosition) {
    return cameraPosition + cameraRight() * viewPosition.x +
           cameraUp() * viewPosition.y + cameraForward() * (-viewPosition.z);
}

Vec3 ThreeDUtils::cameraToWorld(const Vec3& cameraPosition,
                                const Vec3& viewPosition, float yawDegrees,
                                float pitchDegrees) {
    return cameraPosition +
           cameraRight(yawDegrees, pitchDegrees) * viewPosition.x +
           cameraUp(yawDegrees, pitchDegrees) * viewPosition.y +
           cameraForward(yawDegrees, pitchDegrees) * (-viewPosition.z);
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

void ThreeDUtils::applyCamera(const Vec3& cameraPosition, float yawDegrees,
                              float pitchDegrees) {
    const Vec3 forward = cameraForward(yawDegrees, pitchDegrees);
    const Vec3 right = cameraRight(yawDegrees, pitchDegrees);
    const Vec3 up = cameraUp(yawDegrees, pitchDegrees);

    const GLfloat matrix[16] = {
        right.x, up.x, -forward.x, 0.0f,
        right.y, up.y, -forward.y, 0.0f,
        right.z, up.z, -forward.z, 0.0f,
        -dot(right, cameraPosition), -dot(up, cameraPosition),
        dot(forward, cameraPosition), 1.0f,
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
    const std::vector<std::uint32_t> codepoints = decodeUtf8(text);
    if (codepoints.empty()) {
        return 0.0f;
    }
#ifdef _WIN32
    if (std::any_of(codepoints.begin(), codepoints.end(),
                    [](std::uint32_t codepoint) {
                        return codepoint > 0x7Fu;
                    })) {
        std::wstring wideText;
        if (utf8ToWide(text, wideText)) {
            int width = 0;
            int height = 0;
            if (measureSystemText(wideText, kSystemPixelFontHeight, width,
                                  height)) {
                return static_cast<float>(width) * pixelScale *
                       kSystemPixelFontScale;
            }
        }
    }
#endif
    return static_cast<float>(codepoints.size() * 6) * pixelScale -
           pixelScale;
}

void ThreeDUtils::drawText(const std::string& text, float x, float y,
                           float pixelScale, const Color& color) {
#ifdef _WIN32
    const std::vector<std::uint32_t> codepoints = decodeUtf8(text);
    if (std::any_of(codepoints.begin(), codepoints.end(),
                    [](std::uint32_t codepoint) {
                        return codepoint > 0x7Fu;
                    }) &&
        drawSystemText(text, x, y, pixelScale, color)) {
        return;
    }
#endif
    for (const std::uint32_t codepoint : decodeUtf8(text)) {
        const auto glyph = glyphFor(codepoint);
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
