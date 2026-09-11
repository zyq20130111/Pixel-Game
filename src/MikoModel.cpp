#include "MikoModel.h"

#include "ThreeDUtils.h"
#include "game_constants.h"
#include "platform.h"

#include <algorithm>
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
        vertices_.push_back(makeVertex(a, colorA));
        vertices_.push_back(makeVertex(b, colorB));
        vertices_.push_back(makeVertex(c, colorC));
    }

    void addQuad(const Vec3& a, const Color& colorA, const Vec3& b,
                 const Color& colorB, const Vec3& c, const Color& colorC,
                 const Vec3& d, const Color& colorD) {
        addTriangle(a, colorA, b, colorB, c, colorC);
        addTriangle(a, colorA, c, colorC, d, colorD);
    }

    void draw() const {
        if (vertices_.empty()) {
            return;
        }

        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        glVertexPointer(3, GL_FLOAT, sizeof(Vertex),
                        vertices_.data()->position);
        glColorPointer(3, GL_FLOAT, sizeof(Vertex),
                       vertices_.data()->color);
        glDrawArrays(GL_TRIANGLES, 0,
                     static_cast<GLsizei>(vertices_.size()));
        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
    }

private:
    static Vertex makeVertex(const Vec3& position, const Color& color) {
        return Vertex{{position.x, position.y, position.z},
                      {color.red, color.green, color.blue}};
    }

    std::vector<Vertex> vertices_;
};

constexpr float kTau = constants::kPi * 2.0f;

Color shade(const Color& color, float factor) {
    return ThreeDUtils::shade(color, factor);
}

VertexBuffer makeFrustum(int sides, float topRadiusX, float topRadiusZ,
                         float bottomRadiusX, float bottomRadiusZ,
                         float halfHeight, const Color& topColor,
                         const Color& sideColor, const Color& bottomColor) {
    VertexBuffer buffer;
    const Vec3 topCenter{0.0f, halfHeight, 0.0f};
    const Vec3 bottomCenter{0.0f, -halfHeight, 0.0f};

    const auto ringPoint = [](float angle, float radiusX, float radiusZ,
                              float y) {
        return Vec3{std::cos(angle) * radiusX, y,
                    std::sin(angle) * radiusZ};
    };

    for (int side = 0; side < sides; ++side) {
        const int nextSide = (side + 1) % sides;
        const float angleA =
            kTau * static_cast<float>(side) / static_cast<float>(sides);
        const float angleB =
            kTau * static_cast<float>(nextSide) / static_cast<float>(sides);
        const Vec3 topA =
            ringPoint(angleA, topRadiusX, topRadiusZ, halfHeight);
        const Vec3 topB =
            ringPoint(angleB, topRadiusX, topRadiusZ, halfHeight);
        const Vec3 bottomA =
            ringPoint(angleA, bottomRadiusX, bottomRadiusZ, -halfHeight);
        const Vec3 bottomB =
            ringPoint(angleB, bottomRadiusX, bottomRadiusZ, -halfHeight);
        const float middleAngle = (angleA + angleB) * 0.5f;
        const Color faceColor =
            shade(sideColor, 0.80f + 0.20f * std::cos(middleAngle));

        buffer.addTriangle(topCenter, topColor, topA, topColor, topB,
                           topColor);
        buffer.addTriangle(bottomCenter, bottomColor, bottomB, bottomColor,
                           bottomA, bottomColor);
        buffer.addQuad(topA, faceColor, topB, faceColor, bottomB, faceColor,
                       bottomA, faceColor);
    }

    return buffer;
}

VertexBuffer makeEllipsoid(int slices, int stacks, const Color& baseColor) {
    VertexBuffer buffer;

    const auto pointAt = [](float stackAngle, float sliceAngle) {
        const float ringRadius = std::sin(stackAngle);
        return Vec3{std::cos(sliceAngle) * ringRadius,
                    std::cos(stackAngle),
                    std::sin(sliceAngle) * ringRadius};
    };

    for (int stack = 0; stack < stacks; ++stack) {
        const float stackAngleA =
            constants::kPi * static_cast<float>(stack) /
            static_cast<float>(stacks);
        const float stackAngleB =
            constants::kPi * static_cast<float>(stack + 1) /
            static_cast<float>(stacks);

        for (int slice = 0; slice < slices; ++slice) {
            const float sliceAngleA =
                kTau * static_cast<float>(slice) /
                static_cast<float>(slices);
            const float sliceAngleB =
                kTau * static_cast<float>(slice + 1) /
                static_cast<float>(slices);

            const Vec3 a = pointAt(stackAngleA, sliceAngleA);
            const Vec3 b = pointAt(stackAngleA, sliceAngleB);
            const Vec3 c = pointAt(stackAngleB, sliceAngleB);
            const Vec3 d = pointAt(stackAngleB, sliceAngleA);
            const float light =
                std::clamp(0.84f + 0.14f * std::cos(sliceAngleA) +
                               0.10f * std::cos(stackAngleA),
                           0.58f, 1.10f);
            const Color faceColor = shade(baseColor, light);

            if (stack == 0) {
                buffer.addTriangle(a, faceColor, c, faceColor, d, faceColor);
            } else if (stack == stacks - 1) {
                buffer.addTriangle(a, faceColor, b, faceColor, c, faceColor);
            } else {
                buffer.addTriangle(a, faceColor, b, faceColor, c, faceColor);
                buffer.addTriangle(a, faceColor, c, faceColor, d, faceColor);
            }
        }
    }

    return buffer;
}

VertexBuffer makePatch(const Color& color) {
    VertexBuffer buffer;
    const Color shadow = shade(color, 0.78f);
    const Color highlight = shade(color, 1.08f);
    buffer.addQuad({-0.5f, -0.5f, 0.0f}, shadow, {0.5f, -0.5f, 0.0f},
                   shadow, {0.5f, 0.5f, 0.0f}, highlight,
                   {-0.5f, 0.5f, 0.0f}, highlight);
    return buffer;
}

VertexBuffer makeTornPatch(const Color& color) {
    VertexBuffer buffer;
    const Color dark = shade(color, 0.72f);
    const Color light = shade(color, 1.08f);
    buffer.addTriangle({-0.5f, -0.5f, 0.0f}, dark, {0.5f, -0.5f, 0.0f},
                       dark, {0.34f, 0.18f, 0.0f}, light);
    buffer.addTriangle({-0.5f, -0.5f, 0.0f}, dark, {0.34f, 0.18f, 0.0f},
                       light, {0.12f, 0.50f, 0.0f}, light);
    buffer.addTriangle({-0.5f, -0.5f, 0.0f}, dark, {0.12f, 0.50f, 0.0f},
                       light, {-0.30f, 0.28f, 0.0f}, light);
    return buffer;
}

VertexBuffer makeBow(const Color& color) {
    VertexBuffer buffer;
    const Color dark = shade(color, 0.72f);
    const Color light = shade(color, 1.10f);
    buffer.addTriangle({0.0f, 0.0f, 0.0f}, dark, {-0.50f, 0.28f, 0.0f},
                       light, {-0.50f, -0.20f, 0.0f}, dark);
    buffer.addTriangle({0.0f, 0.0f, 0.0f}, dark, {0.50f, 0.28f, 0.0f},
                       light, {0.50f, -0.20f, 0.0f}, dark);
    return buffer;
}

void drawPart(const VertexBuffer& buffer, const Vec3& position,
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

void drawDownwardLimb(const VertexBuffer& buffer, const Vec3& joint,
                      const Vec3& rotationDegrees, float length, float width,
                      float depth) {
    glPushMatrix();
    glTranslatef(joint.x, joint.y, joint.z);
    glRotatef(rotationDegrees.z, 0.0f, 0.0f, 1.0f);
    glRotatef(rotationDegrees.y, 0.0f, 1.0f, 0.0f);
    glRotatef(rotationDegrees.x, 1.0f, 0.0f, 0.0f);
    glTranslatef(0.0f, -length * 0.5f, 0.0f);
    glScalef(width, length, depth);
    buffer.draw();
    glPopMatrix();
}

}  // namespace

struct MikoModel::Impl {
    Impl()
        : hakama(makeFrustum(
              10, 0.48f, 0.38f, 0.72f, 0.54f, 0.46f,
              shade(constants::kMikoRed, 1.12f), constants::kMikoRed,
              constants::kMikoRedDark)),
          kimono(makeFrustum(
              10, 0.38f, 0.30f, 0.52f, 0.40f, 0.52f,
              shade(constants::kMikoWhite, 1.08f), constants::kMikoWhite,
              constants::kMikoWhiteShadow)),
          obi(makeFrustum(
              10, 0.47f, 0.35f, 0.48f, 0.36f, 0.10f,
              shade(constants::kMikoRedDark, 1.08f), constants::kMikoRedDark,
              shade(constants::kMikoRedDark, 0.78f))),
          sleeve(makeFrustum(
              8, 0.24f, 0.21f, 0.32f, 0.28f, 0.48f,
              shade(constants::kMikoWhite, 1.10f), constants::kMikoWhite,
              constants::kMikoWhiteShadow)),
          forearm(makeFrustum(
              8, 0.13f, 0.12f, 0.16f, 0.14f, 0.24f,
              shade(constants::kMikoSkin, 1.08f), constants::kMikoSkin,
              constants::kMikoSkinShadow)),
          hand(makeEllipsoid(12, 7, constants::kMikoSkin)),
          leg(makeFrustum(
              8, 0.16f, 0.15f, 0.18f, 0.16f, 0.34f,
              shade(constants::kMikoWhite, 1.02f), constants::kMikoWhiteShadow,
              shade(constants::kMikoWhiteShadow, 0.82f))),
          sandal(makeFrustum(
              8, 0.20f, 0.28f, 0.24f, 0.32f, 0.10f,
              shade(constants::kMikoSandal, 1.10f), constants::kMikoSandal,
              constants::kMikoSandalDark)),
          neck(makeFrustum(
              8, 0.16f, 0.15f, 0.18f, 0.17f, 0.14f,
              shade(constants::kMikoSkin, 1.08f), constants::kMikoSkin,
              constants::kMikoSkinShadow)),
          head(makeEllipsoid(18, 10, constants::kMikoSkin)),
          hairCap(makeEllipsoid(18, 10, constants::kMikoHair)),
          hairLock(makeFrustum(
              10, 0.14f, 0.16f, 0.18f, 0.20f, 0.72f,
              shade(constants::kMikoHair, 1.08f), constants::kMikoHair,
              constants::kMikoHair)),
          bow(makeBow(constants::kMikoRibbon)),
          bowTail(makeTornPatch(constants::kMikoRibbon)),
          eyeWhite(makeEllipsoid(12, 7, constants::kMikoEyeWhite)),
          eye(makeEllipsoid(12, 7, constants::kMikoEye)),
          mouth(makePatch(constants::kMikoMouth)),
          bang(makeTornPatch(constants::kMikoHair)),
          wand(makeFrustum(
              8, 0.045f, 0.045f, 0.055f, 0.055f, 0.78f,
              shade(constants::kMikoWand, 1.12f), constants::kMikoWand,
              constants::kMikoWand)),
          paper(makeTornPatch(constants::kMikoPaper)) {}

    VertexBuffer hakama;
    VertexBuffer kimono;
    VertexBuffer obi;
    VertexBuffer sleeve;
    VertexBuffer forearm;
    VertexBuffer hand;
    VertexBuffer leg;
    VertexBuffer sandal;
    VertexBuffer neck;
    VertexBuffer head;
    VertexBuffer hairCap;
    VertexBuffer hairLock;
    VertexBuffer bow;
    VertexBuffer bowTail;
    VertexBuffer eyeWhite;
    VertexBuffer eye;
    VertexBuffer mouth;
    VertexBuffer bang;
    VertexBuffer wand;
    VertexBuffer paper;
};

MikoModel::MikoModel()
    : animationPhase_(0.0f), position_(constants::kMikoDefaultPosition),
      impl_(nullptr) {
    buildMeshes();
    reset();
}

MikoModel::~MikoModel() {
    delete impl_;
}

void MikoModel::buildMeshes() {
    impl_ = new Impl();
}

void MikoModel::reset() {
    animationPhase_ = 0.0f;
    position_ = constants::kMikoDefaultPosition;
}

void MikoModel::update(float dt) {
    animationPhase_ = std::fmod(animationPhase_ + dt, kTau);
}

const Vec3& MikoModel::position() const {
    return position_;
}

void MikoModel::render() const {
    const float sway = std::sin(animationPhase_ * 1.35f);
    const float slowSway = std::sin(animationPhase_ * 0.68f);
    const float bob = 0.025f * std::sin(animationPhase_ * 2.0f);
    const float robeRoll = 1.5f * sway;
    const float sleeveRoll = 4.0f * sway;
    const float goheiRoll = 7.0f * slowSway;

    glPushMatrix();
    glTranslatef(position_.x, position_.y + bob, position_.z);
    glRotatef(1.8f * slowSway, 0.0f, 1.0f, 0.0f);

    drawPart(impl_->hakama, {0.0f, 1.08f, 0.0f},
             {0.0f, 0.0f, robeRoll}, {1.0f, 1.0f, 1.0f});
    drawPart(impl_->obi, {0.0f, 1.59f, 0.02f},
             {0.0f, 0.0f, robeRoll}, {1.0f, 1.0f, 1.0f});
    drawPart(impl_->kimono, {0.0f, 2.08f, 0.0f},
             {0.0f, 0.0f, robeRoll * 0.65f}, {1.0f, 1.0f, 1.0f});

    drawDownwardLimb(impl_->leg, {-0.24f, 0.78f, 0.0f},
                     {1.8f * sway, 0.0f, -2.0f}, 0.62f, 1.0f, 1.0f);
    drawDownwardLimb(impl_->leg, {0.24f, 0.78f, 0.0f},
                     {-1.8f * sway, 0.0f, 2.0f}, 0.62f, 1.0f, 1.0f);
    drawPart(impl_->sandal, {-0.25f, 0.14f, 0.18f},
             {0.0f, 0.0f, -2.0f}, {1.0f, 1.0f, 1.0f});
    drawPart(impl_->sandal, {0.25f, 0.14f, 0.18f},
             {0.0f, 0.0f, 2.0f}, {1.0f, 1.0f, 1.0f});

    drawDownwardLimb(impl_->sleeve, {-0.58f, 2.30f, 0.0f},
                     {0.0f, 0.0f, -9.0f + sleeveRoll}, 0.84f, 1.0f, 1.0f);
    drawDownwardLimb(impl_->sleeve, {0.58f, 2.30f, 0.0f},
                     {0.0f, 0.0f, 9.0f + sleeveRoll}, 0.84f, 1.0f, 1.0f);

    drawDownwardLimb(impl_->forearm, {-0.70f, 1.66f, 0.06f},
                     {0.0f, 0.0f, -5.0f + sleeveRoll}, 0.42f, 1.0f, 1.0f);
    drawPart(impl_->hand, {-0.73f, 1.40f, 0.14f},
             {0.0f, 0.0f, -5.0f}, {0.15f, 0.16f, 0.13f});

    drawDownwardLimb(impl_->forearm, {0.70f, 1.66f, 0.06f},
                     {0.0f, 0.0f, 7.0f + sleeveRoll}, 0.42f, 1.0f, 1.0f);
    drawPart(impl_->hand, {0.76f, 1.40f, 0.18f},
             {0.0f, 0.0f, 7.0f}, {0.15f, 0.16f, 0.13f});

    drawPart(impl_->wand, {0.88f, 1.78f, 0.22f},
             {0.0f, 0.0f, 8.0f + goheiRoll}, {1.0f, 1.0f, 1.0f});
    drawPart(impl_->paper, {0.86f, 2.48f, 0.25f},
             {0.0f, 0.0f, 8.0f + goheiRoll}, {0.34f, 0.26f, 1.0f});
    drawPart(impl_->paper, {1.03f, 2.36f, 0.27f},
             {0.0f, 0.0f, 8.0f + goheiRoll}, {0.27f, 0.21f, 1.0f});
    drawPart(impl_->paper, {0.68f, 2.36f, 0.27f},
             {0.0f, 0.0f, 8.0f + goheiRoll}, {0.27f, 0.21f, 1.0f});

    drawPart(impl_->hairLock, {-0.52f, 2.30f, -0.10f},
             {0.0f, 0.0f, -2.0f * slowSway}, {1.0f, 1.0f, 1.0f});
    drawPart(impl_->hairLock, {0.52f, 2.30f, -0.10f},
             {0.0f, 0.0f, 2.0f * slowSway}, {1.0f, 1.0f, 1.0f});
    drawPart(impl_->hairCap, {0.0f, 3.02f, -0.06f},
             {0.0f, 0.0f, 0.0f}, {0.66f, 0.78f, 0.58f});
    drawPart(impl_->neck, {0.0f, 2.57f, 0.04f}, {}, {1.0f, 1.0f, 1.0f});
    drawPart(impl_->head, {0.0f, 3.02f, 0.16f},
             {0.0f, 0.0f, 1.5f * slowSway}, {0.50f, 0.57f, 0.45f});

    drawPart(impl_->bow, {-0.52f, 3.42f, 0.23f},
             {0.0f, 0.0f, -8.0f}, {0.50f, 0.40f, 1.0f});
    drawPart(impl_->bow, {0.52f, 3.42f, 0.23f},
             {0.0f, 0.0f, 8.0f}, {0.50f, 0.40f, 1.0f});
    drawPart(impl_->bowTail, {-0.66f, 3.18f, 0.24f},
             {0.0f, 0.0f, -12.0f}, {0.22f, 0.42f, 1.0f});
    drawPart(impl_->bowTail, {0.66f, 3.18f, 0.24f},
             {0.0f, 0.0f, 12.0f}, {0.22f, 0.42f, 1.0f});

    drawPart(impl_->eyeWhite, {-0.18f, 3.08f, 0.60f}, {},
             {0.105f, 0.12f, 0.035f});
    drawPart(impl_->eyeWhite, {0.18f, 3.08f, 0.60f}, {},
             {0.105f, 0.12f, 0.035f});
    drawPart(impl_->eye, {-0.18f, 3.08f, 0.635f},
             {0.0f, 0.0f, -2.0f}, {0.052f, 0.070f, 0.024f});
    drawPart(impl_->eye, {0.18f, 3.08f, 0.635f},
             {0.0f, 0.0f, 2.0f}, {0.052f, 0.070f, 0.024f});
    drawPart(impl_->mouth, {0.0f, 2.83f, 0.605f}, {},
             {0.12f, 0.045f, 1.0f});

    drawPart(impl_->bang, {-0.18f, 3.34f, 0.59f},
             {0.0f, 0.0f, -8.0f}, {0.22f, 0.30f, 1.0f});
    drawPart(impl_->bang, {0.0f, 3.38f, 0.60f}, {},
             {0.22f, 0.32f, 1.0f});
    drawPart(impl_->bang, {0.18f, 3.34f, 0.59f},
             {0.0f, 0.0f, 8.0f}, {0.22f, 0.30f, 1.0f});

    glPopMatrix();
}

}  // namespace pixel_world
