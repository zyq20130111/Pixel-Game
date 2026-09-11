#include "ZombieModel.h"

#include "ThreeDUtils.h"
#include "game_constants.h"
#include "platform.h"

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

Color shade(const Color& color, float factor) {
    return ThreeDUtils::shade(color, factor);
}

constexpr float kTau = constants::kPi * 2.0f;
constexpr float kAttackCycleDuration = 4.8f;
constexpr float kAttackStartTime = 2.15f;
constexpr float kAttackDuration = 1.60f;
constexpr float kAttackSoundHeight = 1.55f;

float attackAmount(float animationTime) {
    const float attackTime = animationTime - kAttackStartTime;
    if (attackTime <= 0.0f || attackTime >= kAttackDuration) {
        return 0.0f;
    }

    const float progress = attackTime / kAttackDuration;
    // Short anticipation, a sharp lunge, then a slower recovery.
    if (progress < 0.24f) {
        return 0.18f * ThreeDUtils::smoothStep(progress / 0.24f);
    }
    if (progress < 0.52f) {
        return ThreeDUtils::smoothStep((progress - 0.24f) / 0.28f);
    }
    return 1.0f -
           ThreeDUtils::smoothStep((progress - 0.52f) / 0.48f);
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
            shade(sideColor, 0.78f + 0.22f * std::cos(middleAngle));

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
                std::clamp(0.82f + 0.15f * std::cos(sliceAngleA) +
                               0.12f * std::cos(stackAngleA),
                           0.56f, 1.12f);
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
    const Color highlight = shade(color, 1.06f);
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
                       dark, {0.42f, 0.18f, 0.0f}, light);
    buffer.addTriangle({-0.5f, -0.5f, 0.0f}, dark, {0.42f, 0.18f, 0.0f},
                       light, {0.16f, 0.50f, 0.0f}, light);
    buffer.addTriangle({-0.5f, -0.5f, 0.0f}, dark, {0.16f, 0.50f, 0.0f},
                       light, {-0.28f, 0.28f, 0.0f}, light);
    return buffer;
}

VertexBuffer makeSpike(const Color& baseColor) {
    VertexBuffer buffer;
    const Color sideColor = shade(baseColor, 0.86f);
    const Vec3 tip{0.0f, 0.0f, 1.0f};
    const std::array<Vec3, 4> base{{
        {-0.5f, -0.5f, 0.0f},
        {0.5f, -0.5f, 0.0f},
        {0.5f, 0.5f, 0.0f},
        {-0.5f, 0.5f, 0.0f},
    }};

    for (int side = 0; side < 4; ++side) {
        buffer.addTriangle(base[side], sideColor, base[(side + 1) % 4],
                           sideColor, tip, baseColor);
    }
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

struct ZombieModel::Impl {
    Impl()
        : torso(makeFrustum(
              8, 0.48f, 0.39f, 0.62f, 0.48f, 0.50f,
              shade(constants::kZombieCloth, 1.12f),
              constants::kZombieCloth,
              shade(constants::kZombieClothDark, 0.82f))),
          pelvis(makeFrustum(
              8, 0.46f, 0.38f, 0.52f, 0.42f, 0.22f,
              shade(constants::kZombiePants, 1.05f),
              constants::kZombiePants,
              shade(constants::kZombiePantsDark, 0.82f))),
          neck(makeFrustum(
              10, 0.19f, 0.18f, 0.22f, 0.20f, 0.16f,
              shade(constants::kZombieSkinLight, 1.06f),
              constants::kZombieSkin,
              shade(constants::kZombieSkinDark, 0.82f))),
          head(makeEllipsoid(16, 9, constants::kZombieSkin)),
          upperArm(makeFrustum(
              8, 0.15f, 0.14f, 0.20f, 0.18f, 0.50f,
              shade(constants::kZombieCloth, 1.08f),
              constants::kZombieClothDark,
              shade(constants::kZombieClothDark, 0.78f))),
          forearm(makeFrustum(
              10, 0.13f, 0.12f, 0.17f, 0.15f, 0.50f,
              shade(constants::kZombieSkinLight, 1.05f),
              constants::kZombieSkin,
              shade(constants::kZombieSkinDark, 0.80f))),
          hand(makeEllipsoid(12, 6, constants::kZombieSkin)),
          claw(makeSpike(constants::kZombieClaw)),
          upperLeg(makeFrustum(
              8, 0.18f, 0.17f, 0.24f, 0.20f, 0.50f,
              shade(constants::kZombiePants, 1.04f),
              constants::kZombiePants,
              shade(constants::kZombiePantsDark, 0.76f))),
          lowerLeg(makeFrustum(
              8, 0.16f, 0.15f, 0.20f, 0.18f, 0.50f,
              shade(constants::kZombiePantsDark, 1.04f),
              constants::kZombiePantsDark,
              shade(constants::kZombiePantsDark, 0.72f))),
          boot(makeFrustum(
              8, 0.20f, 0.26f, 0.24f, 0.30f, 0.18f,
              shade(constants::kZombieBoot, 1.08f),
              constants::kZombieBoot,
              shade(constants::kZombieBootDark, 0.72f))),
          eyeSocket(makeEllipsoid(12, 6, constants::kZombieEyeSocket)),
          eye(makeEllipsoid(12, 6, constants::kZombieEye)),
          mouth(makePatch(constants::kZombieMouth)),
          tooth(makePatch(constants::kZombieTooth)),
          chestWound(makeTornPatch(constants::kZombieBlood)),
          shirtTear(makeTornPatch(constants::kZombieClothDark)),
          scar(makePatch(constants::kZombieScar)) {}

    VertexBuffer torso;
    VertexBuffer pelvis;
    VertexBuffer neck;
    VertexBuffer head;
    VertexBuffer upperArm;
    VertexBuffer forearm;
    VertexBuffer hand;
    VertexBuffer claw;
    VertexBuffer upperLeg;
    VertexBuffer lowerLeg;
    VertexBuffer boot;
    VertexBuffer eyeSocket;
    VertexBuffer eye;
    VertexBuffer mouth;
    VertexBuffer tooth;
    VertexBuffer chestWound;
    VertexBuffer shirtTear;
    VertexBuffer scar;
};

ZombieModel::ZombieModel()
    : animationPhase_(0.0f), position_(constants::kZombieDefaultPosition),
      impl_(nullptr) {
    attackSound_.initialize();
    buildMeshes();
    reset();
}

ZombieModel::~ZombieModel() {
    delete impl_;
}

void ZombieModel::buildMeshes() {
    impl_ = new Impl();
}

void ZombieModel::reset() {
    animationPhase_ = 0.0f;
    position_ = constants::kZombieDefaultPosition;
    attackSound_.setPosition(
        {position_.x, position_.y + kAttackSoundHeight, position_.z});
    attackSound_.stop();
}

void ZombieModel::update(float dt) {
    attackSound_.setPosition(
        {position_.x, position_.y + kAttackSoundHeight, position_.z});

    if (dt <= 0.0f) {
        return;
    }

    float currentTime = animationPhase_;
    float remainingTime = dt;
    while (remainingTime > 0.0f) {
        const float attackStart =
            currentTime < kAttackStartTime
                ? kAttackStartTime
                : kAttackCycleDuration + kAttackStartTime;
        const float timeToAttack = attackStart - currentTime;
        if (timeToAttack > remainingTime) {
            currentTime += remainingTime;
            remainingTime = 0.0f;
            break;
        }

        attackSound_.play();
        currentTime = std::fmod(attackStart, kAttackCycleDuration);
        remainingTime -= timeToAttack;
    }

    animationPhase_ = std::fmod(currentTime, kAttackCycleDuration);
}

void ZombieModel::setAudioListener(const Vec3& position,
                                   const Vec3& forward) {
    attackSound_.setListener(position, forward, {0.0f, 1.0f, 0.0f});
}

const Vec3& ZombieModel::position() const {
    return position_;
}

void ZombieModel::render() const {
    const float sway = std::sin(animationPhase_ * 1.15f);
    const float attack = attackAmount(animationPhase_);
    const float bob =
        0.025f * std::sin(animationPhase_ * 2.0f) - 0.10f * attack;
    const float headRoll =
        -4.0f + 3.0f * std::sin(animationPhase_ * 0.7f) + 16.0f * attack;
    const float torsoPitch = -4.0f - 28.0f * attack;
    const float leftArmLift = -58.0f - 72.0f * attack + 7.0f * sway;
    const float rightArmLift = -34.0f - 82.0f * attack - 9.0f * sway;
    const float handForward = 0.58f * attack;
    const float handDrop = 0.22f * attack;

    glPushMatrix();
    glTranslatef(position_.x, position_.y + bob,
                 position_.z + 0.26f * attack);
    glRotatef(-5.0f + 2.0f * sway - 10.0f * attack, 0.0f, 1.0f,
              0.0f);

    drawPart(impl_->pelvis, {0.0f, 1.16f, 0.0f}, {0.0f, 0.0f, 0.0f},
             {1.0f, 1.0f, 1.0f});
    drawPart(impl_->torso, {0.0f, 1.72f, 0.0f},
             {torsoPitch, 0.0f, -2.0f * sway - 7.0f * attack},
             {1.0f, 1.28f, 1.0f});
    drawPart(impl_->chestWound, {0.0f, 1.72f, 0.51f},
             {torsoPitch, 0.0f, -2.0f * sway - 7.0f * attack},
             {0.52f, 0.42f, 1.0f});
    drawPart(impl_->shirtTear, {-0.28f, 1.58f, 0.525f},
             {torsoPitch, 0.0f, -2.0f * sway - 7.0f * attack},
             {0.18f, 0.24f, 1.0f});

    drawDownwardLimb(impl_->upperLeg, {-0.25f, 1.20f, 0.0f},
                     {3.0f * sway, 0.0f, -2.0f}, 0.72f, 0.28f, 0.28f);
    drawDownwardLimb(impl_->lowerLeg, {-0.25f, 0.48f, 0.05f},
                     {-4.0f * sway, 0.0f, 0.0f}, 0.46f, 0.25f, 0.25f);
    drawDownwardLimb(impl_->upperLeg, {0.25f, 1.20f, 0.0f},
                     {-3.0f * sway, 0.0f, 2.0f}, 0.72f, 0.28f, 0.28f);
    drawDownwardLimb(impl_->lowerLeg, {0.25f, 0.48f, 0.05f},
                     {4.0f * sway, 0.0f, 0.0f}, 0.46f, 0.25f, 0.25f);

    drawPart(impl_->boot, {-0.25f, 0.12f, 0.16f},
             {0.0f, 0.0f, -2.0f}, {1.0f, 1.0f, 1.0f});
    drawPart(impl_->boot, {0.25f, 0.12f, 0.16f},
             {0.0f, 0.0f, 2.0f}, {1.0f, 1.0f, 1.0f});

    drawPart(impl_->neck, {0.0f, 2.25f, 0.0f}, {}, {1.0f, 1.0f, 1.0f});
    drawPart(impl_->head, {0.0f, 2.72f, 0.05f},
             {3.0f + 20.0f * attack, 0.0f, headRoll},
             {0.56f, 0.62f, 0.52f});

    drawDownwardLimb(impl_->upperArm, {-0.68f, 2.08f, 0.0f},
                     {leftArmLift, 0.0f, -8.0f}, 0.76f, 0.28f, 0.28f);
    drawDownwardLimb(impl_->forearm, {-0.70f, 1.57f, 0.38f},
                     {-24.0f - 56.0f * attack + 4.0f * sway, 0.0f,
                      -2.0f},
                     0.70f, 0.24f, 0.24f);
    drawPart(impl_->hand, {-0.70f, 1.16f - handDrop,
                           0.68f + handForward},
             {-8.0f - 38.0f * attack, 0.0f, -6.0f},
             {0.16f, 0.18f, 0.14f});

    drawDownwardLimb(impl_->upperArm, {0.68f, 2.08f, 0.0f},
                     {rightArmLift, 0.0f, 8.0f}, 0.76f, 0.28f, 0.28f);
    drawDownwardLimb(impl_->forearm, {0.70f, 1.57f, 0.38f},
                     {-18.0f - 66.0f * attack - 4.0f * sway, 0.0f,
                      2.0f},
                     0.70f, 0.24f, 0.24f);
    drawPart(impl_->hand, {0.70f, 1.16f - handDrop,
                           0.68f + handForward},
             {-8.0f - 44.0f * attack, 0.0f, 6.0f},
             {0.16f, 0.18f, 0.14f});

    for (int side = -1; side <= 1; ++side) {
        const float xOffset = 0.055f * static_cast<float>(side);
        drawPart(impl_->claw,
                 {-0.70f + xOffset, 1.05f - handDrop,
                  0.76f + handForward + 0.10f},
                 {-32.0f * attack, 0.0f,
                  static_cast<float>(side) * 10.0f},
                 {0.07f, 0.07f, 0.22f});
        drawPart(impl_->claw,
                 {0.70f + xOffset, 1.05f - handDrop,
                  0.76f + handForward + 0.10f},
                 {-32.0f * attack, 0.0f,
                  static_cast<float>(side) * 10.0f},
                 {0.07f, 0.07f, 0.22f});
    }

    drawPart(impl_->eyeSocket, {-0.20f, 2.77f, 0.52f},
             {0.0f, 0.0f, -3.0f}, {0.15f, 0.14f, 0.06f});
    drawPart(impl_->eyeSocket, {0.20f, 2.77f, 0.52f},
             {0.0f, 0.0f, 3.0f}, {0.15f, 0.14f, 0.06f});
    drawPart(impl_->eye, {-0.20f, 2.77f, 0.575f}, {},
             {0.075f, 0.075f, 0.035f});
    drawPart(impl_->eye, {0.20f, 2.77f, 0.575f}, {},
             {0.075f, 0.075f, 0.035f});
    drawPart(impl_->mouth, {0.0f, 2.49f, 0.555f},
             {headRoll, 0.0f, 0.0f},
             {0.34f, 0.13f + 0.18f * attack, 1.0f});
    drawPart(impl_->tooth, {-0.10f, 2.50f, 0.575f},
             {headRoll, 0.0f, 0.0f}, {0.055f, 0.09f, 1.0f});
    drawPart(impl_->tooth, {0.10f, 2.50f, 0.575f},
             {headRoll, 0.0f, 0.0f}, {0.055f, 0.09f, 1.0f});
    drawPart(impl_->scar, {-0.34f, 2.96f, 0.49f},
             {headRoll, 0.0f, -20.0f}, {0.08f, 0.26f, 1.0f});

    glPopMatrix();
}

}  // namespace pixel_world
