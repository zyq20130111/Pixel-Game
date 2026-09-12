#include "Pistol.h"

#include "PistolBullet.h"
#include "SoundManager.h"
#include "Camera.h"
#include "game_constants.h"
#include "ThreeDUtils.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace pixel_world {
namespace {

struct Vertex {
    GLfloat position[3];
    GLfloat color[3];
};

class VertexBuffer {
public:
    void addTriangle(const Vec3& a, const Color& colorA, const Vec3& b,
                     const Color& colorB, const Vec3& c,
                     const Color& colorC) {
        triangles_.push_back(makeVertex(a, colorA));
        triangles_.push_back(makeVertex(b, colorB));
        triangles_.push_back(makeVertex(c, colorC));
    }

    void addQuad(const Vec3& a, const Color& colorA, const Vec3& b,
                 const Color& colorB, const Vec3& c, const Color& colorC,
                 const Vec3& d, const Color& colorD) {
        addTriangle(a, colorA, b, colorB, c, colorC);
        addTriangle(a, colorA, c, colorC, d, colorD);
    }

    void addLine(const Vec3& a, const Vec3& b, const Color& color) {
        lines_.push_back(makeVertex(a, color));
        lines_.push_back(makeVertex(b, color));
    }

    void draw() const {
        if (!triangles_.empty()) {
            drawArray(triangles_, GL_TRIANGLES);
        }
        if (!lines_.empty()) {
            glLineWidth(constants::kCartoonOutlineWidth);
            drawArray(lines_, GL_LINES);
        }
    }

private:
    static Vertex makeVertex(const Vec3& position, const Color& color) {
        return Vertex{{position.x, position.y, position.z},
                      {color.red, color.green, color.blue}};
    }

    static void drawArray(const std::vector<Vertex>& vertices, GLenum mode) {
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        glVertexPointer(3, GL_FLOAT, sizeof(Vertex), vertices.data()->position);
        glColorPointer(3, GL_FLOAT, sizeof(Vertex), vertices.data()->color);
        glDrawArrays(mode, 0, static_cast<GLsizei>(vertices.size()));
        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
    }

    std::vector<Vertex> triangles_;
    std::vector<Vertex> lines_;
};

Color shade(const Color& color, float factor) {
    return ThreeDUtils::shade(color, factor);
}

constexpr float kTau = constants::kPi * 2.0f;

Color blendColor(const Color& a, const Color& b, float t) {
    return {a.red + (b.red - a.red) * t,
            a.green + (b.green - a.green) * t,
            a.blue + (b.blue - a.blue) * t};
}

struct MeshRing {
    float y;
    float radiusX;
    float radiusZ;
    float offsetX;
    float offsetZ;
    Color color;
};

Vec3 ringPoint(const MeshRing& ring, int index, int sides) {
    const float angle = kTau * static_cast<float>(index) /
                        static_cast<float>(sides);
    return {ring.offsetX + std::sin(angle) * ring.radiusX, ring.y,
            ring.offsetZ + std::cos(angle) * ring.radiusZ};
}

float roundedProfile(float t) {
    return std::clamp(std::sin(constants::kPi * std::clamp(t, 0.0f, 1.0f)) *
                          1.45f,
                      0.0f, 1.0f);
}

void addOrganicRingMesh(VertexBuffer& buffer, const std::vector<MeshRing>& rings,
                        int sides, bool addContours) {
    if (rings.size() < 2 || sides < 3) {
        return;
    }

    for (std::size_t ringIndex = 0; ringIndex + 1 < rings.size();
         ++ringIndex) {
        const MeshRing& lower = rings[ringIndex];
        const MeshRing& upper = rings[ringIndex + 1];
        for (int side = 0; side < sides; ++side) {
            const int nextSide = (side + 1) % sides;
            const float midAngle =
                kTau * (static_cast<float>(side) + 0.5f) /
                static_cast<float>(sides);
            const float light =
                std::clamp(0.86f + 0.14f * std::cos(midAngle) +
                               0.10f * std::sin(midAngle),
                           0.66f, 1.12f);
            const Color faceColor =
                shade(blendColor(lower.color, upper.color, 0.5f), light);

            const Vec3 lowerA = ringPoint(lower, side, sides);
            const Vec3 lowerB = ringPoint(lower, nextSide, sides);
            const Vec3 upperA = ringPoint(upper, side, sides);
            const Vec3 upperB = ringPoint(upper, nextSide, sides);

            const bool lowerPoint =
                lower.radiusX <= 0.001f || lower.radiusZ <= 0.001f;
            const bool upperPoint =
                upper.radiusX <= 0.001f || upper.radiusZ <= 0.001f;
            if (lowerPoint) {
                buffer.addTriangle(lowerA, faceColor, upperB, faceColor,
                                   upperA, faceColor);
            } else if (upperPoint) {
                buffer.addTriangle(lowerA, faceColor, lowerB, faceColor,
                                   upperA, faceColor);
            } else {
                buffer.addQuad(lowerA, faceColor, lowerB, faceColor, upperB,
                               faceColor, upperA, faceColor);
            }
        }
    }

    if (!addContours) {
        return;
    }

    for (std::size_t ringIndex = 1; ringIndex + 1 < rings.size();
         ++ringIndex) {
        if (ringIndex != 1 && ringIndex + 2 != rings.size() &&
            ringIndex != rings.size() / 2) {
            continue;
        }
        for (int side = 0; side < sides; ++side) {
            buffer.addLine(ringPoint(rings[ringIndex], side, sides),
                           ringPoint(rings[ringIndex], (side + 1) % sides,
                                     sides),
                           constants::kInkColor);
        }
    }

    constexpr std::array<int, 4> contourSides{0, 4, 8, 12};
    for (const int side : contourSides) {
        const int wrappedSide = side % sides;
        for (std::size_t ringIndex = 1; ringIndex + 2 < rings.size();
             ++ringIndex) {
            buffer.addLine(ringPoint(rings[ringIndex], wrappedSide, sides),
                           ringPoint(rings[ringIndex + 1], wrappedSide,
                                     sides),
                           constants::kInkColor);
        }
    }
}

VertexBuffer makeRoundedTaperY(float proximalHalfX, float proximalHalfZ,
                               float distalHalfX, float distalHalfZ,
                               const Color& baseColor) {
    VertexBuffer buffer;
    std::vector<MeshRing> rings;
    constexpr int kRingCount = 10;
    rings.reserve(kRingCount + 1);
    for (int ring = 0; ring <= kRingCount; ++ring) {
        const float t = static_cast<float>(ring) /
                        static_cast<float>(kRingCount);
        const float profile = roundedProfile(t);
        const float jointBulge = 1.0f + 0.06f * std::sin(constants::kPi * t);
        const float radiusX =
            profile *
            (distalHalfX + (proximalHalfX - distalHalfX) * t) * jointBulge;
        const float radiusZ =
            profile *
            (distalHalfZ + (proximalHalfZ - distalHalfZ) * t) * jointBulge;
        rings.push_back({-0.5f + t, radiusX, radiusZ, 0.0f, 0.0f,
                         shade(baseColor, 0.90f + 0.14f * t)});
    }
    addOrganicRingMesh(buffer, rings, 20, true);
    return buffer;
}

VertexBuffer makePalmModelY(const Color& baseColor) {
    VertexBuffer buffer;
    const std::vector<MeshRing> rings{
        {-0.57f, 0.00f, 0.00f, -0.06f, 0.02f, shade(baseColor, 0.86f)},
        {-0.52f, 0.28f, 0.20f, -0.05f, 0.04f, shade(baseColor, 0.98f)},
        {-0.40f, 0.47f, 0.32f, -0.03f, 0.07f, shade(baseColor, 1.06f)},
        {-0.21f, 0.57f, 0.42f, 0.00f, 0.08f, shade(baseColor, 1.08f)},
        {0.02f, 0.62f, 0.47f, 0.04f, 0.04f, baseColor},
        {0.24f, 0.57f, 0.45f, 0.06f, -0.01f, shade(baseColor, 0.98f)},
        {0.43f, 0.42f, 0.39f, 0.04f, -0.05f, shade(baseColor, 0.88f)},
        {0.55f, 0.20f, 0.25f, 0.00f, -0.07f, shade(baseColor, 0.80f)},
        {0.59f, 0.00f, 0.00f, -0.02f, -0.08f, shade(baseColor, 0.74f)},
    };
    addOrganicRingMesh(buffer, rings, 22, true);
    return buffer;
}

VertexBuffer makeCurvedFingerModel(const Color& baseColor) {
    VertexBuffer buffer;
    const Color jointColor = shade(baseColor, 0.96f);
    const Color tipColor = shade(baseColor, 1.04f);
    const std::vector<MeshRing> rings{
        {0.03f, 0.00f, 0.00f, 0.00f, 0.00f, shade(baseColor, 0.88f)},
        {-0.01f, 0.42f, 0.39f, 0.00f, 0.00f, baseColor},
        {-0.09f, 0.53f, 0.46f, 0.00f, 0.01f, shade(baseColor, 1.02f)},
        {-0.21f, 0.50f, 0.43f, 0.00f, 0.03f, baseColor},
        {-0.34f, 0.44f, 0.38f, 0.00f, 0.07f, jointColor},
        {-0.43f, 0.47f, 0.39f, 0.00f, 0.09f, shade(baseColor, 1.01f)},
        {-0.56f, 0.40f, 0.35f, 0.00f, 0.13f, baseColor},
        {-0.65f, 0.42f, 0.35f, 0.00f, 0.15f, shade(baseColor, 1.02f)},
        {-0.77f, 0.35f, 0.30f, 0.00f, 0.18f, jointColor},
        {-0.86f, 0.34f, 0.28f, 0.00f, 0.19f, tipColor},
        {-0.96f, 0.27f, 0.23f, 0.00f, 0.20f, tipColor},
        {-1.01f, 0.00f, 0.00f, 0.00f, 0.20f, shade(baseColor, 0.96f)},
    };
    addOrganicRingMesh(buffer, rings, 22, true);
    return buffer;
}

VertexBuffer makeCurvedThumbModel(const Color& baseColor) {
    VertexBuffer buffer;
    const Color jointColor = shade(baseColor, 0.95f);
    const Color tipColor = shade(baseColor, 1.03f);
    const std::vector<MeshRing> rings{
        {0.06f, 0.00f, 0.00f, 0.00f, 0.00f, shade(baseColor, 0.88f)},
        {0.00f, 0.42f, 0.38f, 0.00f, 0.00f, baseColor},
        {-0.10f, 0.51f, 0.43f, 0.03f, 0.02f,
         shade(baseColor, 1.02f)},
        {-0.23f, 0.49f, 0.41f, 0.08f, 0.05f, baseColor},
        {-0.36f, 0.44f, 0.37f, 0.14f, 0.08f, jointColor},
        {-0.49f, 0.43f, 0.35f, 0.19f, 0.11f,
         shade(baseColor, 1.01f)},
        {-0.62f, 0.38f, 0.32f, 0.23f, 0.14f, baseColor},
        {-0.74f, 0.34f, 0.29f, 0.25f, 0.17f, jointColor},
        {-0.85f, 0.29f, 0.25f, 0.25f, 0.19f, tipColor},
        {-0.94f, 0.00f, 0.00f, 0.25f, 0.20f,
         shade(baseColor, 0.96f)},
    };
    addOrganicRingMesh(buffer, rings, 20, true);
    return buffer;
}

VertexBuffer makeNailModel(const Color& baseColor) {
    VertexBuffer buffer;
    constexpr int kSides = 16;
    const Color topColor = shade(baseColor, 1.05f);
    const Color edgeColor = shade(baseColor, 0.82f);
    const Vec3 topCenter{0.0f, 0.0f, 0.045f};
    const Vec3 bottomCenter{0.0f, 0.0f, 0.0f};

    for (int side = 0; side < kSides; ++side) {
        const int nextSide = (side + 1) % kSides;
        const float angleA =
            kTau * static_cast<float>(side) / static_cast<float>(kSides);
        const float angleB =
            kTau * static_cast<float>(nextSide) / static_cast<float>(kSides);
        const Vec3 topA{std::sin(angleA) * 0.47f,
                        std::cos(angleA) * 0.50f, 0.04f};
        const Vec3 topB{std::sin(angleB) * 0.47f,
                        std::cos(angleB) * 0.50f, 0.04f};
        const Vec3 bottomA{topA.x, topA.y, 0.0f};
        const Vec3 bottomB{topB.x, topB.y, 0.0f};
        buffer.addTriangle(topCenter, topColor, topA, topColor, topB,
                           topColor);
        buffer.addQuad(bottomA, edgeColor, bottomB, edgeColor, topB, edgeColor,
                       topA, edgeColor);
    }
    return buffer;
}

VertexBuffer makePalmDetailModel() {
    VertexBuffer buffer;
    const Color crease = shade(constants::kPistolHandShadow, 0.82f);
    const Color highlight = shade(constants::kPistolHandLight, 0.90f);

    const auto addCurve = [&buffer](const std::array<Vec3, 4>& points,
                                    const Color& color) {
        for (std::size_t index = 0; index + 1 < points.size(); ++index) {
            buffer.addLine(points[index], points[index + 1], color);
        }
    };

    addCurve({{{-0.46f, 0.18f, 0.49f},
               {-0.37f, 0.03f, 0.51f},
               {-0.27f, -0.13f, 0.51f},
               {-0.24f, -0.29f, 0.49f}}},
             crease);
    addCurve({{{-0.34f, 0.08f, 0.50f},
               {-0.10f, 0.00f, 0.51f},
               {0.15f, 0.02f, 0.50f},
               {0.34f, 0.10f, 0.47f}}},
             crease);
    addCurve({{{-0.22f, 0.28f, 0.48f},
               {0.00f, 0.23f, 0.50f},
               {0.21f, 0.22f, 0.48f},
               {0.36f, 0.17f, 0.45f}}},
             highlight);
    return buffer;
}

VertexBuffer makeOvalPlateXY(const Color& baseColor) {
    VertexBuffer buffer;
    constexpr int kSides = 14;
    const Vec3 center{0.0f, 0.0f, 0.0f};
    const Color centerColor = shade(baseColor, 1.10f);
    for (int side = 0; side < kSides; ++side) {
        const int nextSide = (side + 1) % kSides;
        const float angleA =
            kTau * static_cast<float>(side) / static_cast<float>(kSides);
        const float angleB =
            kTau * static_cast<float>(nextSide) / static_cast<float>(kSides);
        const Vec3 pointA{std::sin(angleA) * 0.45f,
                          std::cos(angleA) * 0.50f, 0.0f};
        const Vec3 pointB{std::sin(angleB) * 0.45f,
                          std::cos(angleB) * 0.50f, 0.0f};
        const Color edgeColor = shade(baseColor, 0.96f);
        buffer.addTriangle(center, centerColor, pointA, edgeColor, pointB,
                           edgeColor);
        buffer.addLine(pointA, pointB, shade(constants::kInkColor, 1.15f));
    }
    return buffer;
}

VertexBuffer makeBeveledBoxZ(const Color& baseColor) {
    VertexBuffer buffer;
    constexpr int kSides = 8;
    constexpr std::array<Vec3, kSides> profile{{
        {-0.40f, -0.50f, 0.0f},
        {0.40f, -0.50f, 0.0f},
        {0.50f, -0.38f, 0.0f},
        {0.50f, 0.34f, 0.0f},
        {0.36f, 0.50f, 0.0f},
        {-0.36f, 0.50f, 0.0f},
        {-0.50f, 0.34f, 0.0f},
        {-0.50f, -0.38f, 0.0f},
    }};
    const Vec3 backCenter{0.0f, 0.0f, -0.5f};
    const Vec3 frontCenter{0.0f, 0.0f, 0.5f};
    const Color backColor = shade(baseColor, 0.72f);
    const Color frontColor = shade(baseColor, 1.06f);

    const auto pointAt = [](const Vec3& point, float z) {
        return Vec3{point.x, point.y, z};
    };

    for (int side = 0; side < kSides; ++side) {
        const int nextSide = (side + 1) % kSides;
        const Vec3 backA = pointAt(profile[side], -0.5f);
        const Vec3 backB = pointAt(profile[nextSide], -0.5f);
        const Vec3 frontA = pointAt(profile[side], 0.5f);
        const Vec3 frontB = pointAt(profile[nextSide], 0.5f);
        const float sideAngle =
            kTau * (static_cast<float>(side) + 0.5f) /
            static_cast<float>(kSides);
        const Color sideColor =
            shade(baseColor, 0.78f + 0.24f * std::cos(sideAngle));

        buffer.addTriangle(backCenter, backColor, backB, backColor, backA,
                           backColor);
        buffer.addTriangle(frontCenter, frontColor, frontA, frontColor,
                           frontB, frontColor);
        buffer.addQuad(backA, sideColor, backB, sideColor, frontB, sideColor,
                       frontA, sideColor);

        buffer.addLine(backA, backB, constants::kInkColor);
        buffer.addLine(frontA, frontB, constants::kInkColor);
        if (side % 2 == 0) {
            buffer.addLine(backA, frontA, constants::kInkColor);
        }
    }
    return buffer;
}

const VertexBuffer& sleeveMesh() {
    static const VertexBuffer mesh =
        makeRoundedTaperY(0.48f, 0.48f, 0.38f, 0.36f,
                          constants::kPistolSleeve);
    return mesh;
}

const VertexBuffer& sleeveDarkMesh() {
    static const VertexBuffer mesh = makeRoundedTaperY(
        0.50f, 0.48f, 0.38f, 0.36f, constants::kPistolSleeveDark);
    return mesh;
}

const VertexBuffer& palmMesh() {
    static const VertexBuffer mesh = makePalmModelY(constants::kPistolHand);
    return mesh;
}

const VertexBuffer& palmShadowMesh() {
    static const VertexBuffer mesh =
        makePalmModelY(constants::kPistolHandShadow);
    return mesh;
}

const VertexBuffer& thenarPadMesh() {
    static const VertexBuffer mesh = makeRoundedTaperY(
        0.58f, 0.50f, 0.40f, 0.36f, constants::kPistolHandLight);
    return mesh;
}

const VertexBuffer& fingerMesh() {
    static const VertexBuffer mesh =
        makeCurvedFingerModel(constants::kPistolHand);
    return mesh;
}

const VertexBuffer& fingerShadowMesh() {
    static const VertexBuffer mesh =
        makeCurvedFingerModel(constants::kPistolHandShadow);
    return mesh;
}

const VertexBuffer& fingerTipMesh() {
    static const VertexBuffer mesh = makeRoundedTaperY(
        0.40f, 0.36f, 0.27f, 0.28f, constants::kPistolHandLight);
    return mesh;
}

const VertexBuffer& thumbMesh() {
    static const VertexBuffer mesh =
        makeCurvedThumbModel(constants::kPistolHand);
    return mesh;
}

const VertexBuffer& knuckleMesh() {
    static const VertexBuffer mesh = makeRoundedTaperY(
        0.55f, 0.35f, 0.46f, 0.28f, constants::kPistolHandLight);
    return mesh;
}

const VertexBuffer& nailMesh() {
    static const VertexBuffer mesh =
        makeNailModel(constants::kPistolHandLight);
    return mesh;
}

const VertexBuffer& palmDetailMesh() {
    static const VertexBuffer mesh = makePalmDetailModel();
    return mesh;
}

const VertexBuffer& pistolFrameMesh() {
    static const VertexBuffer mesh = makeBeveledBoxZ(constants::kPistolMetalDark);
    return mesh;
}

const VertexBuffer& pistolSlideMesh() {
    static const VertexBuffer mesh = makeBeveledBoxZ(constants::kPistolMetal);
    return mesh;
}

const VertexBuffer& pistolHighlightMesh() {
    static const VertexBuffer mesh =
        makeBeveledBoxZ(constants::kPistolHighlight);
    return mesh;
}

const VertexBuffer& pistolBarrelMesh() {
    static const VertexBuffer mesh = makeRoundedTaperY(
        0.48f, 0.48f, 0.48f, 0.48f, constants::kPistolMetalDark);
    return mesh;
}

const VertexBuffer& pistolGripMesh() {
    static const VertexBuffer mesh = makeBeveledBoxZ(constants::kPistolGrip);
    return mesh;
}

const VertexBuffer& pistolGripDarkMesh() {
    static const VertexBuffer mesh =
        makeBeveledBoxZ(constants::kPistolGripDark);
    return mesh;
}

const VertexBuffer& pistolSmallMetalMesh() {
    static const VertexBuffer mesh = makeRoundedTaperY(
        0.45f, 0.45f, 0.45f, 0.45f, constants::kPistolMetalDark);
    return mesh;
}

const VertexBuffer& pistolSightMesh() {
    static const VertexBuffer mesh = makeBeveledBoxZ(constants::kPistolMetalDark);
    return mesh;
}

const VertexBuffer& muzzleFlashOuterMesh() {
    static const VertexBuffer mesh =
        makeOvalPlateXY(constants::kMuzzleFlashOuter);
    return mesh;
}

const VertexBuffer& muzzleFlashCoreMesh() {
    static const VertexBuffer mesh =
        makeOvalPlateXY(constants::kMuzzleFlashCore);
    return mesh;
}

void drawHandBuffer(const VertexBuffer& buffer, const Vec3& position,
                    const Vec3& rotationDegrees, const Vec3& scale) {
    glPushMatrix();
    glTranslatef(position.x, position.y, position.z);
    glRotatef(rotationDegrees.z, 0.0f, 0.0f, 1.0f);
    glRotatef(rotationDegrees.y, 0.0f, 1.0f, 0.0f);
    glRotatef(rotationDegrees.x, 1.0f, 0.0f, 0.0f);
    glScalef(scale.x, scale.y, scale.z);
    buffer.draw();
    glPopMatrix();
}

void drawHandPivotedBuffer(const VertexBuffer& buffer, const Vec3& pivot,
                           const Vec3& offset,
                           const Vec3& rotationDegrees,
                           const Vec3& scale) {
    glPushMatrix();
    glTranslatef(pivot.x, pivot.y, pivot.z);
    glRotatef(rotationDegrees.z, 0.0f, 0.0f, 1.0f);
    glRotatef(rotationDegrees.y, 0.0f, 1.0f, 0.0f);
    glRotatef(rotationDegrees.x, 1.0f, 0.0f, 0.0f);
    glTranslatef(offset.x, offset.y, offset.z);
    glScalef(scale.x, scale.y, scale.z);
    buffer.draw();
    glPopMatrix();
}

void drawGunBuffer(const VertexBuffer& buffer, const Vec3& position,
                   const Vec3& rotationDegrees, const Vec3& scale) {
    drawHandBuffer(buffer, position, rotationDegrees, scale);
}

void drawGunPivotedBuffer(const VertexBuffer& buffer, const Vec3& pivot,
                          const Vec3& offset, const Vec3& rotationDegrees,
                          const Vec3& scale) {
    drawHandPivotedBuffer(buffer, pivot, offset, rotationDegrees, scale);
}

}  // namespace

Pistol::Pistol() : muzzleFlashTimer_(0.0f) {}

void Pistol::reset() {
    muzzleFlashTimer_ = 0.0f;
}

void Pistol::update(GLFWwindow* window, const Camera& camera,
                    std::vector<std::unique_ptr<BulletBase>>& bullets,
                    SoundManager& soundManager, bool& previousFireDown,
                    float dt) {
    muzzleFlashTimer_ = std::max(0.0f, muzzleFlashTimer_ - dt);

    const bool fireDown =
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    if (fireDown && !previousFireDown) {
        if (bullets.size() >= static_cast<std::size_t>(constants::kMaxBullets)) {
            bullets.erase(bullets.begin());
        }

        const Vec3 muzzlePosition = muzzleWorldPosition(camera);
        const Vec3 target =
            camera.position() + camera.forward() * constants::kBulletAimDistance;
        const Vec3 direction = ThreeDUtils::normalize(target - muzzlePosition);
        bullets.push_back(std::make_unique<PistolBullet>(
            muzzlePosition, direction * constants::kBulletSpeed,
            constants::kBulletLifetime));
        muzzleFlashTimer_ = constants::kMuzzleFlashDuration;
        soundManager.play2D("pistol.wav", 0.82f);
    }
    previousFireDown = fireDown;
}

void Pistol::render(const Camera& camera) const {
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    const Vec3 root = weaponRoot(camera, muzzleFlashTimer_);
    glTranslatef(root.x, root.y, root.z);
    glScalef(constants::kPistolScale, constants::kPistolScale,
             constants::kPistolScale);

    drawFirstPersonHandBack();

    drawGunBuffer(pistolFrameMesh(), {0.0f, -0.02f, 0.0f}, {},
                  {0.50f, 0.32f, 0.72f});
    drawGunBuffer(pistolSlideMesh(), {0.0f, 0.09f, -0.30f}, {},
                  {0.42f, 0.22f, 0.76f});
    drawGunBuffer(pistolHighlightMesh(), {0.0f, 0.205f, -0.31f}, {},
                  {0.28f, 0.06f, 0.62f});

    drawGunBuffer(pistolFrameMesh(), {0.0f, -0.01f, -0.77f}, {},
                  {0.25f, 0.18f, 0.28f});
    drawGunBuffer(pistolBarrelMesh(), {0.0f, 0.12f, -0.81f},
                  {-90.0f, 0.0f, 0.0f}, {0.14f, 0.12f, 0.14f});

    drawGunPivotedBuffer(pistolGripMesh(), {0.0f, -0.18f, 0.10f},
                         {0.0f, -0.38f, 0.05f},
                         {-12.0f, 0.0f, 0.0f},
                         {0.34f, 0.78f, 0.42f});
    drawGunPivotedBuffer(pistolGripDarkMesh(), {0.0f, -0.18f, 0.10f},
                         {0.0f, -0.40f, 0.22f},
                         {-12.0f, 0.0f, 0.0f},
                         {0.22f, 0.62f, 0.12f});

    drawGunBuffer(pistolSmallMetalMesh(), {-0.15f, -0.15f, -0.24f},
                  {0.0f, 0.0f, 0.0f}, {0.08f, 0.12f, 0.08f});
    drawGunBuffer(pistolSmallMetalMesh(), {0.15f, -0.15f, -0.24f},
                  {0.0f, 0.0f, 0.0f}, {0.08f, 0.12f, 0.08f});
    drawGunBuffer(pistolSmallMetalMesh(), {0.0f, -0.29f, -0.18f},
                  {0.0f, 0.0f, -90.0f}, {0.055f, 0.30f, 0.055f});
    drawGunBuffer(pistolSmallMetalMesh(), {0.02f, -0.19f, -0.16f},
                  {0.0f, 0.0f, -18.0f}, {0.050f, 0.18f, 0.045f});

    drawIronSights(camera.aimAmount());
    drawFirstPersonHandGrip();
    drawMuzzleFlash(muzzleFlashTimer_);

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
}

float Pistol::muzzleFlashTimer() const {
    return muzzleFlashTimer_;
}

float Pistol::bob(bool cameraMoving, bool cameraRunning,
                  float cameraWalkPhase) {
    const float runScale = cameraRunning ? 1.75f : 1.0f;
    return cameraMoving
               ? std::sin(cameraWalkPhase * 2.0f) * 0.035f * runScale
               : 0.0f;
}

float Pistol::sway(bool cameraMoving, bool cameraRunning,
                   float cameraWalkPhase) {
    const float runScale = cameraRunning ? 1.45f : 1.0f;
    return cameraMoving
               ? std::cos(cameraWalkPhase) * 0.025f * runScale
               : 0.0f;
}

Vec3 Pistol::weaponRoot(const Camera& camera, float muzzleFlashTimer) {
    const float recoil =
        muzzleFlashTimer > 0.0f
            ? muzzleFlashTimer / constants::kMuzzleFlashDuration
            : 0.0f;
    const float swayAmount =
        sway(camera.moving(), camera.running(), camera.walkPhase()) *
        (1.0f - 0.7f * camera.aimAmount());
    const float bobAmount =
        bob(camera.moving(), camera.running(), camera.walkPhase()) *
        (1.0f - 0.85f * camera.aimAmount());
    return {
        constants::kPistolWeaponBase.x +
            (constants::kPistolWeaponAimBase.x -
             constants::kPistolWeaponBase.x) *
                camera.aimAmount() +
            swayAmount,
        constants::kPistolWeaponBase.y +
            (constants::kPistolWeaponAimBase.y -
             constants::kPistolWeaponBase.y) *
                camera.aimAmount() +
            bobAmount + 0.025f * recoil,
        constants::kPistolWeaponBase.z +
            (constants::kPistolWeaponAimBase.z -
             constants::kPistolWeaponBase.z) *
                camera.aimAmount() +
            0.08f * recoil,
    };
}

Vec3 Pistol::muzzleFrontViewPosition(const Camera& camera,
                                     float muzzleFlashTimer) {
    Vec3 muzzle = weaponRoot(camera, muzzleFlashTimer);
    muzzle = muzzle + constants::kPistolMuzzleLocal * constants::kPistolScale;
    muzzle.z -= constants::kMuzzleFlashForwardOffset * constants::kPistolScale;
    return muzzle;
}

Vec3 Pistol::muzzleWorldPosition(const Camera& camera) {
    return camera.toWorld(muzzleFrontViewPosition(camera, 0.0f));
}

void Pistol::drawFirstPersonHandBack() {
    drawHandBuffer(sleeveDarkMesh(), {0.48f, -1.08f, 0.70f},
                   {-24.0f, 0.0f, -10.0f}, {0.42f, 0.92f, 0.38f});
    drawHandBuffer(sleeveMesh(), {0.32f, -0.82f, 0.55f},
                   {-18.0f, 0.0f, -8.0f}, {0.48f, 0.38f, 0.42f});
    drawHandBuffer(palmShadowMesh(), {0.16f, -0.66f, 0.41f},
                   {-15.0f, 0.0f, -8.0f}, {0.40f, 0.24f, 0.34f});
    drawHandBuffer(palmMesh(), {0.02f, -0.55f, 0.36f},
                   {-9.0f, 0.0f, -1.5f}, {0.56f, 0.43f, 0.43f});
    drawHandBuffer(thenarPadMesh(), {0.25f, -0.58f, 0.43f},
                   {-20.0f, 6.0f, -18.0f}, {0.18f, 0.30f, 0.20f});

    const std::array<float, 4> knuckleX{-0.22f, -0.08f, 0.07f, 0.21f};
    const std::array<float, 4> knuckleScale{0.92f, 1.08f, 1.04f, 0.84f};
    for (std::size_t i = 0; i < knuckleX.size(); ++i) {
        drawHandBuffer(knuckleMesh(), {knuckleX[i], -0.39f, 0.55f},
                       {-10.0f, 0.0f,
                        -4.0f + 2.5f * static_cast<float>(i)},
                       {0.075f * knuckleScale[i], 0.055f, 0.060f});
    }

    drawHandPivotedBuffer(thumbMesh(), {0.26f, -0.54f, 0.40f},
                          {0.0f, -0.14f, 0.03f},
                          {-16.0f, 10.0f, -30.0f},
                          {0.16f, 0.28f, 0.15f});
    drawHandPivotedBuffer(fingerTipMesh(), {0.34f, -0.72f, 0.52f},
                          {0.0f, -0.10f, 0.02f},
                          {-30.0f, 15.0f, -34.0f},
                          {0.12f, 0.20f, 0.11f});
}

void Pistol::drawFirstPersonHandGrip() {
    const std::array<float, 4> fingerX{-0.20f, -0.06f, 0.08f, 0.21f};
    const std::array<float, 4> fingerLength{0.42f, 0.50f, 0.47f, 0.36f};
    const std::array<float, 4> fingerWidth{0.105f, 0.118f, 0.112f, 0.094f};
    const std::array<float, 4> fingerTilt{-12.0f, -4.0f, 4.0f, 13.0f};

    drawHandBuffer(palmMesh(), {-0.31f, -0.49f, 0.31f},
                   {-18.0f, -3.0f, 18.0f}, {0.22f, 0.34f, 0.48f});
    drawHandBuffer(palmDetailMesh(), {-0.31f, -0.49f, 0.31f},
                   {-18.0f, -3.0f, 18.0f}, {0.22f, 0.34f, 0.48f});
    drawHandBuffer(palmShadowMesh(), {-0.23f, -0.68f, 0.43f},
                   {-10.0f, 0.0f, 12.0f}, {0.16f, 0.20f, 0.20f});
    drawHandBuffer(thenarPadMesh(), {-0.18f, -0.45f, 0.23f},
                   {12.0f, 16.0f, 26.0f}, {0.13f, 0.24f, 0.14f});

    for (std::size_t i = 0; i < fingerX.size(); ++i) {
        const VertexBuffer& mainFinger =
            i == fingerX.size() - 1 ? fingerShadowMesh() : fingerMesh();
        const float length = fingerLength[i];
        const float width = fingerWidth[i];
        const float zTilt = fingerTilt[i];
        const Vec3 fingerBase{fingerX[i], -0.30f, 0.485f};
        const Vec3 fingerRotation{-8.0f, 0.0f, zTilt};

        drawHandBuffer(knuckleMesh(), {fingerX[i], -0.285f, 0.555f},
                       {-5.0f, 0.0f, zTilt * 0.45f},
                       {width * 0.95f, 0.052f, 0.060f});
        drawHandBuffer(mainFinger, fingerBase, fingerRotation,
                       {width, length, 0.15f});
        drawHandPivotedBuffer(
            nailMesh(), fingerBase, {0.0f, -length * 0.95f, 0.045f},
            fingerRotation, {width * 0.60f, length * 0.28f, 0.65f});
    }

    const Vec3 thumbPivot{-0.23f, -0.22f, 0.06f};
    const Vec3 thumbRotation{15.0f, 10.0f, 24.0f};
    drawHandPivotedBuffer(thumbMesh(), thumbPivot, {0.0f, -0.15f, 0.02f},
                          thumbRotation, {0.14f, 0.50f, 0.13f});
    drawHandPivotedBuffer(nailMesh(), thumbPivot, {0.0f, -0.56f, 0.055f},
                          thumbRotation, {0.072f, 0.18f, 0.72f});
}

void Pistol::drawMuzzleFlash(float muzzleFlashTimer) {
    if (muzzleFlashTimer <= 0.0f) {
        return;
    }

    const float flashScale =
        0.55f + 0.45f * (muzzleFlashTimer / constants::kMuzzleFlashDuration);
    const Vec3 flashCenter{
        constants::kPistolMuzzleLocal.x,
        constants::kPistolMuzzleLocal.y,
        constants::kPistolMuzzleLocal.z -
            constants::kMuzzleFlashForwardOffset};

    drawGunBuffer(muzzleFlashOuterMesh(), flashCenter, {},
                  {0.44f * flashScale, 0.18f * flashScale, 1.0f});
    drawGunBuffer(muzzleFlashOuterMesh(), flashCenter, {0.0f, 0.0f, 90.0f},
                  {0.44f * flashScale, 0.18f * flashScale, 1.0f});
    drawGunBuffer(muzzleFlashCoreMesh(),
                  {flashCenter.x, flashCenter.y, flashCenter.z - 0.05f}, {},
                  {0.24f * flashScale, 0.24f * flashScale, 1.0f});
    drawGunBuffer(muzzleFlashCoreMesh(),
                  {flashCenter.x - 0.22f * flashScale, flashCenter.y,
                   flashCenter.z + 0.03f},
                  {}, {0.10f, 0.10f, 1.0f});
    drawGunBuffer(muzzleFlashOuterMesh(),
                  {flashCenter.x + 0.22f * flashScale, flashCenter.y,
                   flashCenter.z + 0.03f},
                  {}, {0.10f, 0.10f, 1.0f});
}

void Pistol::drawIronSights(float aimAmount) {
    const float sightLift = 0.03f * ThreeDUtils::smoothStep(aimAmount);

    drawGunBuffer(pistolSightMesh(), {-0.13f, 0.27f + sightLift, -0.10f}, {},
                  {0.08f, 0.16f, 0.08f});
    drawGunBuffer(pistolSightMesh(), {0.13f, 0.27f + sightLift, -0.10f}, {},
                  {0.08f, 0.16f, 0.08f});
    drawGunBuffer(pistolSightMesh(), {0.0f, 0.30f + sightLift, -0.86f}, {},
                  {0.07f, 0.18f, 0.07f});
    if (aimAmount > 0.45f) {
        drawGunBuffer(muzzleFlashCoreMesh(), {0.0f, 0.40f + sightLift, -0.87f},
                      {}, {0.04f, 0.04f, 1.0f});
    }
}

}  // namespace pixel_world
