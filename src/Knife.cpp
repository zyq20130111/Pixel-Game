#include "Knife.h"

#include "Camera.h"
#include "FirstPersonHands.h"
#include "ThreeDUtils.h"
#include "game_constants.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace pixel_world {
namespace {

using namespace constants;

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
            glLineWidth(kCartoonOutlineWidth);
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
                           kInkColor);
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
                           kInkColor);
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
        rings.push_back(
            {-0.5f + t,
             profile *
                 (distalHalfX + (proximalHalfX - distalHalfX) * t) *
                 jointBulge,
             profile *
                 (distalHalfZ + (proximalHalfZ - distalHalfZ) * t) *
                 jointBulge,
             0.0f,
             0.0f,
             shade(baseColor, 0.90f + 0.14f * t)});
    }
    addOrganicRingMesh(buffer, rings, 16, true);
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

    for (int side = 0; side < kSides; ++side) {
        const int nextSide = (side + 1) % kSides;
        const Vec3 backA{profile[side].x, profile[side].y, -0.5f};
        const Vec3 backB{profile[nextSide].x, profile[nextSide].y, -0.5f};
        const Vec3 frontA{profile[side].x, profile[side].y, 0.5f};
        const Vec3 frontB{profile[nextSide].x, profile[nextSide].y, 0.5f};
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
        buffer.addLine(backA, backB, kInkColor);
        buffer.addLine(frontA, frontB, kInkColor);
        if (side % 2 == 0) {
            buffer.addLine(backA, frontA, kInkColor);
        }
    }
    return buffer;
}

struct BladeProfilePoint {
    float y;
    float edgeX;
    float spineX;
    Color faceColor;
};

float bladeWidth(const BladeProfilePoint& point) {
    return std::max(0.0f, point.spineX - point.edgeX);
}

float bladeBevelX(const BladeProfilePoint& point) {
    return point.edgeX + std::min(0.075f, bladeWidth(point) * 0.30f);
}

VertexBuffer makeKnifeBladeMesh() {
    VertexBuffer buffer;
    constexpr float kFrontZ = 0.072f;
    constexpr float kBackZ = -0.072f;
    const std::array<BladeProfilePoint, 19> profile{{
        {0.00f, -0.16f, 0.15f, kKnifeBladeDark},
        {0.10f, -0.23f, 0.23f, kKnifeBlade},
        {0.21f, -0.26f, 0.21f, kKnifeBlade},
        {0.31f, -0.29f, 0.27f, kKnifeBlade},
        {0.41f, -0.32f, 0.23f, kKnifeBlade},
        {0.51f, -0.35f, 0.29f, kKnifeBlade},
        {0.61f, -0.38f, 0.25f, kKnifeBlade},
        {0.71f, -0.40f, 0.30f, kKnifeBlade},
        {0.81f, -0.42f, 0.26f, kKnifeBlade},
        {0.91f, -0.43f, 0.31f, shade(kKnifeBlade, 0.98f)},
        {1.01f, -0.43f, 0.27f, shade(kKnifeBlade, 0.98f)},
        {1.11f, -0.42f, 0.30f, shade(kKnifeBlade, 0.98f)},
        {1.21f, -0.40f, 0.26f, shade(kKnifeBlade, 0.98f)},
        {1.31f, -0.36f, 0.25f, shade(kKnifeBlade, 0.98f)},
        {1.40f, -0.31f, 0.21f, shade(kKnifeBlade, 0.99f)},
        {1.49f, -0.24f, 0.15f, kKnifeBladeEdge},
        {1.57f, -0.16f, 0.09f, kKnifeBladeEdge},
        {1.64f, -0.07f, 0.035f, kKnifeBladeEdge},
        {1.69f, 0.00f, 0.00f, kKnifeBladeEdge},
    }};

    for (std::size_t index = 0; index + 1 < profile.size(); ++index) {
        const BladeProfilePoint& lower = profile[index];
        const BladeProfilePoint& upper = profile[index + 1];
        const Color faceColor =
            shade(blendColor(lower.faceColor, upper.faceColor, 0.5f),
                  0.96f + 0.035f * static_cast<float>(index % 2));
        const float lowerWidth = bladeWidth(lower);
        const float upperWidth = bladeWidth(upper);

        const Vec3 lowerEdgeFront{lower.edgeX, lower.y, kFrontZ};
        const Vec3 lowerSpineFront{lower.spineX, lower.y, kFrontZ};
        const Vec3 upperEdgeFront{upper.edgeX, upper.y, kFrontZ};
        const Vec3 upperSpineFront{upper.spineX, upper.y, kFrontZ};
        const Vec3 lowerEdgeBack{lower.edgeX, lower.y, kBackZ};
        const Vec3 lowerSpineBack{lower.spineX, lower.y, kBackZ};
        const Vec3 upperEdgeBack{upper.edgeX, upper.y, kBackZ};
        const Vec3 upperSpineBack{upper.spineX, upper.y, kBackZ};

        if (upperWidth <= 0.001f) {
            buffer.addTriangle(lowerEdgeFront, faceColor, lowerSpineFront,
                               faceColor, upperEdgeFront, faceColor);
            buffer.addTriangle(lowerEdgeBack, shade(faceColor, 0.74f),
                               upperEdgeBack, shade(faceColor, 0.74f),
                               lowerSpineBack, shade(faceColor, 0.74f));
        } else {
            buffer.addQuad(lowerEdgeFront, faceColor, lowerSpineFront,
                           faceColor, upperSpineFront, faceColor,
                           upperEdgeFront, faceColor);
            buffer.addQuad(lowerEdgeBack, shade(faceColor, 0.74f),
                           upperEdgeBack, shade(faceColor, 0.74f),
                           upperSpineBack, shade(faceColor, 0.74f),
                           lowerSpineBack, shade(faceColor, 0.74f));
        }

        const Color edgeColor =
            shade(blendColor(kKnifeBladeEdge, faceColor, 0.28f), 1.02f);
        const Color spineColor = shade(kKnifeBladeDark, 0.80f);
        buffer.addQuad(lowerEdgeFront, edgeColor, upperEdgeFront, edgeColor,
                       upperEdgeBack, shade(faceColor, 0.72f),
                       lowerEdgeBack, shade(faceColor, 0.72f));
        buffer.addQuad(lowerSpineFront, spineColor, lowerSpineBack,
                       spineColor, upperSpineBack, spineColor, upperSpineFront,
                       spineColor);

        if (lowerWidth > 0.001f && upperWidth > 0.001f) {
            const Vec3 lowerBevel{bladeBevelX(lower), lower.y, kFrontZ + 0.002f};
            const Vec3 upperBevel{bladeBevelX(upper), upper.y, kFrontZ + 0.002f};
            buffer.addQuad(lowerEdgeFront, edgeColor, lowerBevel, edgeColor,
                           upperBevel, edgeColor, upperEdgeFront, edgeColor);
            buffer.addLine(lowerBevel, upperBevel, kInkColor);
        }

        buffer.addLine(lowerEdgeFront, upperEdgeFront, kInkColor);
        buffer.addLine(lowerSpineFront, upperSpineFront, kInkColor);
    }

    const BladeProfilePoint& base = profile.front();
    buffer.addQuad({base.edgeX, base.y, kFrontZ}, kKnifeBladeDark,
                   {base.edgeX, base.y, kBackZ}, shade(kKnifeBladeDark, 0.72f),
                   {base.spineX, base.y, kBackZ},
                   shade(kKnifeBladeDark, 0.72f), {base.spineX, base.y, kFrontZ},
                   kKnifeBladeDark);
    return buffer;
}

VertexBuffer makeKnifeFullerMesh() {
    VertexBuffer buffer;
    constexpr float kGrooveZ = 0.077f;
    const Color fuller = shade(kKnifeBladeDark, 0.64f);
    struct FullerPoint {
        float y;
        float leftX;
        float rightX;
    };
    const std::array<FullerPoint, 8> groove{{
        {0.13f, -0.105f, 0.065f},
        {0.30f, -0.16f, 0.045f},
        {0.50f, -0.20f, 0.030f},
        {0.72f, -0.23f, 0.012f},
        {0.94f, -0.235f, -0.005f},
        {1.15f, -0.22f, -0.020f},
        {1.33f, -0.16f, -0.018f},
        {1.45f, -0.09f, 0.000f},
    }};
    for (std::size_t index = 0; index + 1 < groove.size(); ++index) {
        const FullerPoint& lower = groove[index];
        const FullerPoint& upper = groove[index + 1];
        buffer.addQuad({lower.leftX, lower.y, kGrooveZ}, fuller,
                       {lower.rightX, lower.y, kGrooveZ}, fuller,
                       {upper.rightX, upper.y, kGrooveZ}, fuller,
                       {upper.leftX, upper.y, kGrooveZ}, fuller);
        buffer.addLine({lower.leftX, lower.y, kGrooveZ + 0.001f},
                       {upper.leftX, upper.y, kGrooveZ + 0.001f}, kInkColor);
        buffer.addLine({lower.rightX, lower.y, kGrooveZ + 0.001f},
                       {upper.rightX, upper.y, kGrooveZ + 0.001f},
                       shade(kKnifeBladeEdge, 0.38f));
    }
    return buffer;
}

const VertexBuffer& knifeBladeMesh() {
    static const VertexBuffer mesh = makeKnifeBladeMesh();
    return mesh;
}

const VertexBuffer& knifeFullerMesh() {
    static const VertexBuffer mesh = makeKnifeFullerMesh();
    return mesh;
}

const VertexBuffer& knifeHandleMesh() {
    static const VertexBuffer mesh =
        makeRoundedTaperY(0.25f, 0.19f, 0.21f, 0.16f, kKnifeGrip);
    return mesh;
}

const VertexBuffer& knifeHandleCoreMesh() {
    static const VertexBuffer mesh =
        makeRoundedTaperY(0.17f, 0.14f, 0.14f, 0.11f, kKnifeGripDark);
    return mesh;
}

const VertexBuffer& knifeGuardMesh() {
    static const VertexBuffer mesh = makeBeveledBoxZ(kKnifeGuard);
    return mesh;
}

const VertexBuffer& knifeGuardDarkMesh() {
    static const VertexBuffer mesh = makeBeveledBoxZ(kKnifeGripDark);
    return mesh;
}

const VertexBuffer& knifePommelMesh() {
    static const VertexBuffer mesh = makeBeveledBoxZ(kKnifeGrip);
    return mesh;
}

void drawMesh(const VertexBuffer& mesh, const Vec3& position,
              const Vec3& rotationDegrees, const Vec3& scale) {
    glPushMatrix();
    glTranslatef(position.x, position.y, position.z);
    glRotatef(rotationDegrees.z, 0.0f, 0.0f, 1.0f);
    glRotatef(rotationDegrees.y, 0.0f, 1.0f, 0.0f);
    glRotatef(rotationDegrees.x, 1.0f, 0.0f, 0.0f);
    glScalef(scale.x, scale.y, scale.z);
    mesh.draw();
    glPopMatrix();
}

void applyKnifeWeaponMotion(float slash, float windup) {
    const Vec3 weaponRotation{
        -8.0f - 24.0f * slash + 8.0f * windup,
        -8.0f + 22.0f * slash - 8.0f * windup,
        60.0f - 72.0f * slash + 16.0f * windup,
    };

    glTranslatef(0.06f * windup - 0.17f * slash,
                 0.04f * windup + 0.08f * slash,
                 0.06f * windup - 0.12f * slash);
    glRotatef(weaponRotation.z, 0.0f, 0.0f, 1.0f);
    glRotatef(weaponRotation.y, 0.0f, 1.0f, 0.0f);
    glRotatef(weaponRotation.x, 1.0f, 0.0f, 0.0f);
}

void drawKnifeWeapon(float slash, float windup) {
    glPushMatrix();
    applyKnifeWeaponMotion(slash, windup);
    // Scale the weapon around the guard so the shared hand stays aligned
    // with the grip while the whole knife becomes half its previous size.
    glTranslatef(0.0f, -0.10f, 0.08f);
    glScalef(kKnifeVisualScale, kKnifeVisualScale, kKnifeVisualScale);
    glTranslatef(0.0f, 0.10f, -0.08f);

    // The handle sits in the shared hand's palm. The blade rises from the
    // guard along +Y so its broad face remains readable in first person.
    drawMesh(knifeHandleMesh(), {0.0f, -0.52f, 0.16f},
             {-10.0f, 0.0f, -2.0f}, {0.92f, 0.82f, 0.94f});
    drawMesh(knifeHandleCoreMesh(), {0.0f, -0.52f, 0.19f},
             {-10.0f, 0.0f, -2.0f}, {0.82f, 0.76f, 0.78f});

    for (int band = 0; band < 3; ++band) {
        const float y = -0.36f - static_cast<float>(band) * 0.20f;
        drawMesh(knifeGuardDarkMesh(), {0.0f, y, 0.16f}, {},
                 {0.18f, 0.035f, 0.12f});
    }

    drawMesh(knifeGuardMesh(), {0.0f, -0.10f, 0.08f},
             {0.0f, 0.0f, -4.0f}, {0.72f, 0.12f, 0.16f});
    drawMesh(knifeGuardDarkMesh(), {0.0f, -0.105f, 0.01f},
             {0.0f, 0.0f, -4.0f}, {0.54f, 0.045f, 0.18f});
    drawMesh(knifePommelMesh(), {0.0f, -0.96f, 0.18f},
             {-10.0f, 0.0f, -2.0f}, {0.28f, 0.20f, 0.24f});

    drawMesh(knifeBladeMesh(), {0.0f, -0.05f, 0.02f}, {},
             {1.0f, 1.0f, 1.0f});
    drawMesh(knifeFullerMesh(), {0.0f, -0.05f, 0.02f}, {},
             {1.0f, 1.0f, 1.0f});

    glPopMatrix();
}

}  // namespace

Knife::Knife() : attackTimer_(-1.0f), hitDelivered_(false) {}

void Knife::reset() {
    attackTimer_ = -1.0f;
    hitDelivered_ = false;
}

bool Knife::update(GLFWwindow* window, const Camera& camera,
                   std::vector<std::unique_ptr<BulletBase>>& bullets,
                   SoundManager& soundManager, bool& previousFireDown,
                   float dt) {
    static_cast<void>(camera);
    static_cast<void>(bullets);
    static_cast<void>(soundManager);

    const bool fireDown =
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    if (fireDown && !previousFireDown && attackTimer_ < 0.0f) {
        attackTimer_ = 0.0f;
        hitDelivered_ = false;
    }
    previousFireDown = fireDown;

    bool hitFrame = false;
    if (attackTimer_ >= 0.0f) {
        const float previousAttackTimer = attackTimer_;
        attackTimer_ += dt;
        if (!hitDelivered_ && previousAttackTimer < kKnifeHitTime &&
            attackTimer_ >= kKnifeHitTime) {
            hitDelivered_ = true;
            hitFrame = true;
        }
        if (attackTimer_ >= kKnifeAttackDuration) {
            attackTimer_ = -1.0f;
            hitDelivered_ = false;
        }
    }

    return hitFrame;
}

void Knife::render(const Camera& camera,
                   const WeaponRenderMotion& motion) const {
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    glTranslatef(motion.translation.x, motion.translation.y,
                 motion.translation.z);
    glRotatef(motion.rotationDegrees.z, 0.0f, 0.0f, 1.0f);
    glRotatef(motion.rotationDegrees.y, 0.0f, 1.0f, 0.0f);
    glRotatef(motion.rotationDegrees.x, 1.0f, 0.0f, 0.0f);

    const float progress = attackProgress(attackTimer_);
    const float slash = slashAmount(progress);
    const float windup = windupAmount(progress);
    const Vec3 root = weaponRoot(camera, attackTimer_);
    glTranslatef(root.x, root.y, root.z);
    glScalef(constants::kPistolScale, constants::kPistolScale,
             constants::kPistolScale);
    FirstPersonHands::drawBack(FirstPersonHandAnimation::KnifeSlash,
                               progress);
    drawKnifeWeapon(slash, windup);
    FirstPersonHands::drawGrip(FirstPersonHandAnimation::KnifeSlash,
                               progress);

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
}

float Knife::muzzleFlashTimer() const {
    return 0.0f;
}

void Knife::attackSegment(const Camera& camera, Vec3& start, Vec3& end) const {
    const float progress = attackProgress(attackTimer_);
    const float slash = slashAmount(progress);
    const float side = -0.10f + 0.24f * slash;
    start = camera.toWorld({side, -0.26f, -0.34f});
    end = camera.toWorld({side * 0.45f, -0.28f, -kKnifeRange});
}

float Knife::bob(bool cameraMoving, bool cameraRunning,
                 float cameraWalkPhase) {
    const float runScale = cameraRunning ? 1.65f : 1.0f;
    return cameraMoving
               ? std::sin(cameraWalkPhase * 2.0f) * 0.032f * runScale
               : 0.0f;
}

float Knife::sway(bool cameraMoving, bool cameraRunning,
                  float cameraWalkPhase) {
    const float runScale = cameraRunning ? 1.45f : 1.0f;
    return cameraMoving
               ? std::cos(cameraWalkPhase) * 0.030f * runScale
               : 0.0f;
}

float Knife::smooth01(float value) {
    const float t = std::clamp(value, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

float Knife::attackProgress(float attackTimer) {
    if (attackTimer < 0.0f) {
        return 0.0f;
    }
    return std::clamp(attackTimer / kKnifeAttackDuration, 0.0f, 1.0f);
}

float Knife::slashAmount(float progress) {
    if (progress < 0.18f) {
        return -0.18f * smooth01(progress / 0.18f);
    }
    if (progress < 0.58f) {
        return smooth01((progress - 0.18f) / 0.40f);
    }
    return 1.0f - smooth01((progress - 0.58f) / 0.42f);
}

float Knife::windupAmount(float progress) {
    if (progress < 0.24f) {
        return smooth01(progress / 0.24f);
    }
    return 1.0f - smooth01((progress - 0.24f) / 0.42f);
}

Vec3 Knife::weaponRoot(const Camera& camera, float attackTimer) {
    const float progress = attackProgress(attackTimer);
    const float slash = slashAmount(progress);
    const float windup = windupAmount(progress);
    const float swayAmount =
        sway(camera.moving(), camera.running(), camera.walkPhase()) *
        (1.0f - 0.45f * camera.aimAmount());
    const float bobAmount =
        bob(camera.moving(), camera.running(), camera.walkPhase()) *
        (1.0f - 0.45f * camera.aimAmount());
    return {
        kKnifeWeaponBase.x +
            (kKnifeWeaponAimBase.x - kKnifeWeaponBase.x) *
                camera.aimAmount() +
            swayAmount - 0.18f * slash + 0.05f * windup,
        kKnifeWeaponBase.y +
            (kKnifeWeaponAimBase.y - kKnifeWeaponBase.y) *
                camera.aimAmount() +
            bobAmount + 0.08f * slash + 0.04f * windup,
        kKnifeWeaponBase.z +
            (kKnifeWeaponAimBase.z - kKnifeWeaponBase.z) *
                camera.aimAmount() -
            0.15f * slash + 0.08f * windup,
    };
}

}  // namespace pixel_world
