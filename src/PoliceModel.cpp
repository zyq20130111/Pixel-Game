#include "PoliceModel.h"

#include "ThreeDUtils.h"
#include "game_constants.h"
#include "platform.h"
#include "types.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <initializer_list>
#include <vector>

namespace pixel_world {
namespace {

struct Vertex {
    GLfloat position[3];
    GLfloat color[3];
};

struct Matrix4 {
    GLfloat values[16];
};

Matrix4 identityMatrix() {
    return Matrix4{{1.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 1.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 1.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f}};
}

Matrix4 multiply(const Matrix4& left, const Matrix4& right) {
    Matrix4 result{};
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            float value = 0.0f;
            for (int inner = 0; inner < 4; ++inner) {
                value += left.values[inner * 4 + row] *
                         right.values[column * 4 + inner];
            }
            result.values[column * 4 + row] = value;
        }
    }
    return result;
}

Matrix4 translationMatrix(const Vec3& position) {
    Matrix4 result = identityMatrix();
    result.values[12] = position.x;
    result.values[13] = position.y;
    result.values[14] = position.z;
    return result;
}

Matrix4 scaleMatrix(const Vec3& scale) {
    Matrix4 result = identityMatrix();
    result.values[0] = scale.x;
    result.values[5] = scale.y;
    result.values[10] = scale.z;
    return result;
}

Matrix4 rotationXMatrix(float angleDegrees) {
    const float angleRadians = angleDegrees * constants::kPi / 180.0f;
    const float sine = std::sin(angleRadians);
    const float cosine = std::cos(angleRadians);
    Matrix4 result = identityMatrix();
    result.values[5] = cosine;
    result.values[6] = sine;
    result.values[9] = -sine;
    result.values[10] = cosine;
    return result;
}

Matrix4 rotationYMatrix(float angleDegrees) {
    const float angleRadians = angleDegrees * constants::kPi / 180.0f;
    const float sine = std::sin(angleRadians);
    const float cosine = std::cos(angleRadians);
    Matrix4 result = identityMatrix();
    result.values[0] = cosine;
    result.values[2] = -sine;
    result.values[8] = sine;
    result.values[10] = cosine;
    return result;
}

Matrix4 rotationZMatrix(float angleDegrees) {
    const float angleRadians = angleDegrees * constants::kPi / 180.0f;
    const float sine = std::sin(angleRadians);
    const float cosine = std::cos(angleRadians);
    Matrix4 result = identityMatrix();
    result.values[0] = cosine;
    result.values[1] = sine;
    result.values[4] = -sine;
    result.values[5] = cosine;
    return result;
}

Matrix4 boneLocalMatrix(const Vec3& position, const Vec3& rotationDegrees) {
    Matrix4 result = translationMatrix(position);
    result = multiply(result, rotationZMatrix(rotationDegrees.z));
    result = multiply(result, rotationYMatrix(rotationDegrees.y));
    result = multiply(result, rotationXMatrix(rotationDegrees.x));
    return result;
}

Vec3 transformPoint(const Matrix4& matrix, const Vec3& point) {
    return {
        matrix.values[0] * point.x + matrix.values[4] * point.y +
            matrix.values[8] * point.z + matrix.values[12],
        matrix.values[1] * point.x + matrix.values[5] * point.y +
            matrix.values[9] * point.z + matrix.values[13],
        matrix.values[2] * point.x + matrix.values[6] * point.y +
            matrix.values[10] * point.z + matrix.values[14],
    };
}

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

    void drawTransformed(const Matrix4& transform) const {
        std::vector<Vertex> transformed;
        transformed.reserve(vertices_.size());
        for (const Vertex& vertex : vertices_) {
            const Vec3 position{vertex.position[0], vertex.position[1],
                                vertex.position[2]};
            const Vec3 transformedPosition = transformPoint(transform, position);
            transformed.push_back(
                Vertex{{transformedPosition.x, transformedPosition.y,
                        transformedPosition.z},
                       {vertex.color[0], vertex.color[1], vertex.color[2]}});
        }

        if (transformed.empty()) {
            return;
        }
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        glVertexPointer(3, GL_FLOAT, sizeof(Vertex),
                        transformed.data()->position);
        glColorPointer(3, GL_FLOAT, sizeof(Vertex),
                       transformed.data()->color);
        glDrawArrays(GL_TRIANGLES, 0,
                     static_cast<GLsizei>(transformed.size()));
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

enum BoneId : std::size_t {
    kRootBone = 0,
    kTorsoBone,
    kHeadBone,
    kLeftUpperArmBone,
    kLeftLowerArmBone,
    kRightUpperArmBone,
    kRightLowerArmBone,
    kLeftUpperLegBone,
    kLeftLowerLegBone,
    kRightUpperLegBone,
    kRightLowerLegBone,
    kBoneCount,
};

struct Bone {
    int parent;
    Vec3 localPosition;
    Vec3 localRotationDegrees;
    Matrix4 worldMatrix;
};

enum class AnimationId : std::size_t {
    Idle = 0,
    Walk,
    Run,
    Attack,
    CrouchIdle,
    CrouchWalk,
    ProneIdle,
    ProneWalk,
    Count,
};

struct BonePose {
    Vec3 position;
    Vec3 rotationDegrees;
};

using Pose = std::array<BonePose, kBoneCount>;

struct AnimationKeyframe {
    float time;
    Pose bones;
};

struct AnimationClip {
    float duration;
    bool looping;
    std::vector<AnimationKeyframe> keyframes;
};

void setPoseBone(Pose& pose, BoneId bone, const Vec3& position,
                 const Vec3& rotationDegrees) {
    pose[bone] = {position, rotationDegrees};
}

Pose makeBasePose() {
    Pose pose{};
    setPoseBone(pose, kRootBone, {}, {});
    setPoseBone(pose, kTorsoBone, {0.0f, 1.42f, 0.0f}, {});
    setPoseBone(pose, kHeadBone, {0.0f, 1.22f, 0.0f}, {});
    setPoseBone(pose, kLeftUpperArmBone, {-0.66f, 0.40f, 0.02f}, {});
    setPoseBone(pose, kLeftLowerArmBone, {0.0f, -0.72f, 0.0f}, {});
    setPoseBone(pose, kRightUpperArmBone, {0.66f, 0.40f, 0.02f}, {});
    setPoseBone(pose, kRightLowerArmBone, {0.0f, -0.72f, 0.0f}, {});
    setPoseBone(pose, kLeftUpperLegBone, {-0.22f, -0.12f, 0.0f}, {});
    setPoseBone(pose, kLeftLowerLegBone, {0.0f, -0.70f, 0.0f}, {});
    setPoseBone(pose, kRightUpperLegBone, {0.22f, -0.12f, 0.0f}, {});
    setPoseBone(pose, kRightLowerLegBone, {0.0f, -0.70f, 0.0f}, {});
    return pose;
}

AnimationKeyframe makeKeyframe(float time, const Pose& pose) {
    return {time, pose};
}

AnimationClip makeClip(float duration, bool looping,
                       std::initializer_list<AnimationKeyframe> keyframes) {
    return {duration, looping, std::vector<AnimationKeyframe>(keyframes)};
}

std::array<AnimationClip, static_cast<std::size_t>(AnimationId::Count)>
makeAnimationClips() {
    const Pose base = makeBasePose();

    Pose idle0 = base;
    Pose idle1 = base;
    setPoseBone(idle1, kRootBone, {0.0f, 0.025f, 0.0f}, {});
    setPoseBone(idle1, kTorsoBone, {0.0f, 1.43f, 0.0f},
                {0.0f, 0.0f, 1.2f});
    setPoseBone(idle1, kHeadBone, {0.0f, 1.23f, 0.0f},
                {0.0f, 0.0f, -1.8f});
    setPoseBone(idle1, kLeftUpperArmBone, {-0.66f, 0.41f, 0.02f},
                {-2.0f, 0.0f, -1.5f});
    setPoseBone(idle1, kRightUpperArmBone, {0.66f, 0.41f, 0.02f},
                {2.0f, 0.0f, 1.5f});
    const Pose idle2 = idle0;

    Pose walk0 = base;
    Pose walk1 = base;
    setPoseBone(walk1, kRootBone, {0.0f, 0.04f, 0.0f}, {});
    setPoseBone(walk1, kTorsoBone, {0.0f, 1.43f, 0.0f},
                {-1.5f, 0.0f, -1.0f});
    setPoseBone(walk1, kLeftUpperArmBone, {-0.66f, 0.42f, 0.02f},
                {-30.0f, 0.0f, 0.0f});
    setPoseBone(walk1, kLeftLowerArmBone, {0.0f, -0.70f, 0.0f},
                {8.0f, 0.0f, 0.0f});
    setPoseBone(walk1, kRightUpperArmBone, {0.66f, 0.42f, 0.02f},
                {30.0f, 0.0f, 0.0f});
    setPoseBone(walk1, kRightLowerArmBone, {0.0f, -0.70f, 0.0f},
                {-8.0f, 0.0f, 0.0f});
    setPoseBone(walk1, kLeftUpperLegBone, {-0.22f, -0.14f, 0.0f},
                {27.0f, 0.0f, 0.0f});
    setPoseBone(walk1, kLeftLowerLegBone, {0.0f, -0.68f, 0.0f},
                {-10.0f, 0.0f, 0.0f});
    setPoseBone(walk1, kRightUpperLegBone, {0.22f, -0.10f, 0.0f},
                {-27.0f, 0.0f, 0.0f});
    setPoseBone(walk1, kRightLowerLegBone, {0.0f, -0.72f, 0.0f},
                {10.0f, 0.0f, 0.0f});
    Pose walk2 = base;
    setPoseBone(walk2, kRootBone, {0.0f, 0.012f, 0.0f}, {});
    Pose walk3 = walk1;
    setPoseBone(walk3, kTorsoBone, {0.0f, 1.43f, 0.0f},
                {-1.5f, 0.0f, 1.0f});
    setPoseBone(walk3, kLeftUpperArmBone, {-0.66f, 0.42f, 0.02f},
                {30.0f, 0.0f, 0.0f});
    setPoseBone(walk3, kLeftLowerArmBone, {0.0f, -0.70f, 0.0f},
                {-8.0f, 0.0f, 0.0f});
    setPoseBone(walk3, kRightUpperArmBone, {0.66f, 0.42f, 0.02f},
                {-30.0f, 0.0f, 0.0f});
    setPoseBone(walk3, kRightLowerArmBone, {0.0f, -0.70f, 0.0f},
                {8.0f, 0.0f, 0.0f});
    setPoseBone(walk3, kLeftUpperLegBone, {-0.22f, -0.10f, 0.0f},
                {-27.0f, 0.0f, 0.0f});
    setPoseBone(walk3, kLeftLowerLegBone, {0.0f, -0.72f, 0.0f},
                {10.0f, 0.0f, 0.0f});
    setPoseBone(walk3, kRightUpperLegBone, {0.22f, -0.14f, 0.0f},
                {27.0f, 0.0f, 0.0f});
    setPoseBone(walk3, kRightLowerLegBone, {0.0f, -0.68f, 0.0f},
                {-10.0f, 0.0f, 0.0f});
    const Pose walk4 = walk0;

    Pose run0 = base;
    Pose run1 = walk1;
    setPoseBone(run1, kRootBone, {0.0f, 0.07f, -0.018f}, {});
    setPoseBone(run1, kTorsoBone, {0.0f, 1.45f, 0.0f},
                {-7.0f, 0.0f, -1.8f});
    setPoseBone(run1, kLeftUpperArmBone, {-0.66f, 0.45f, 0.02f},
                {-48.0f, 0.0f, 0.0f});
    setPoseBone(run1, kRightUpperArmBone, {0.66f, 0.45f, 0.02f},
                {48.0f, 0.0f, 0.0f});
    setPoseBone(run1, kLeftUpperLegBone, {-0.22f, -0.16f, 0.0f},
                {42.0f, 0.0f, 0.0f});
    setPoseBone(run1, kRightUpperLegBone, {0.22f, -0.06f, 0.0f},
                {-42.0f, 0.0f, 0.0f});
    Pose run2 = base;
    setPoseBone(run2, kRootBone, {0.0f, 0.015f, -0.008f}, {});
    setPoseBone(run2, kTorsoBone, {0.0f, 1.42f, 0.0f},
                {-10.0f, 0.0f, 0.0f});
    Pose run3 = run1;
    setPoseBone(run3, kTorsoBone, {0.0f, 1.45f, 0.0f},
                {-7.0f, 0.0f, 1.8f});
    setPoseBone(run3, kLeftUpperArmBone, {-0.66f, 0.45f, 0.02f},
                {48.0f, 0.0f, 0.0f});
    setPoseBone(run3, kRightUpperArmBone, {0.66f, 0.45f, 0.02f},
                {-48.0f, 0.0f, 0.0f});
    setPoseBone(run3, kLeftUpperLegBone, {-0.22f, -0.06f, 0.0f},
                {-42.0f, 0.0f, 0.0f});
    setPoseBone(run3, kRightUpperLegBone, {0.22f, -0.16f, 0.0f},
                {42.0f, 0.0f, 0.0f});
    const Pose run4 = run0;

    Pose attack0 = base;
    Pose attack1 = base;
    setPoseBone(attack1, kRootBone, {}, {});
    setPoseBone(attack1, kTorsoBone, {0.0f, 1.43f, 0.0f},
                {-4.0f, 0.0f, -3.0f});
    setPoseBone(attack1, kHeadBone, {0.0f, 1.22f, 0.0f},
                {8.0f, 0.0f, 0.0f});
    setPoseBone(attack1, kLeftUpperArmBone, {-0.66f, 0.42f, 0.02f},
                {-20.0f, 0.0f, 0.0f});
    setPoseBone(attack1, kRightUpperArmBone, {0.70f, 0.45f, -0.03f},
                {72.0f, 0.0f, 0.0f});
    setPoseBone(attack1, kRightLowerArmBone, {0.0f, -0.66f, -0.08f},
                {22.0f, 0.0f, 0.0f});
    Pose attack2 = attack1;
    setPoseBone(attack2, kRootBone, {}, {});
    setPoseBone(attack2, kTorsoBone, {0.0f, 1.45f, 0.0f},
                {-9.0f, 0.0f, -6.0f});
    setPoseBone(attack2, kHeadBone, {0.0f, 1.20f, 0.0f},
                {16.0f, 0.0f, 0.0f});
    setPoseBone(attack2, kLeftUpperArmBone, {-0.68f, 0.45f, 0.02f},
                {-32.0f, 0.0f, 0.0f});
    setPoseBone(attack2, kRightUpperArmBone, {0.72f, 0.50f, -0.08f},
                {122.0f, 0.0f, 0.0f});
    setPoseBone(attack2, kRightLowerArmBone, {0.0f, -0.60f, -0.12f},
                {45.0f, 0.0f, 0.0f});
    Pose attack3 = attack2;
    setPoseBone(attack3, kRootBone, {}, {});
    setPoseBone(attack3, kTorsoBone, {0.0f, 1.42f, 0.0f},
                {5.0f, 0.0f, 4.0f});
    setPoseBone(attack3, kHeadBone, {0.0f, 1.22f, 0.0f},
                {6.0f, 0.0f, 0.0f});
    setPoseBone(attack3, kRightUpperArmBone, {0.69f, 0.42f, -0.02f},
                {92.0f, 0.0f, 0.0f});
    setPoseBone(attack3, kRightLowerArmBone, {0.0f, -0.68f, -0.06f},
                {28.0f, 0.0f, 0.0f});
    const Pose attack4 = base;

    const auto makeCrouchPose =
        [&](float bob, float torsoRoll, float armSwing, float legSwing) {
            Pose pose = base;
            setPoseBone(pose, kRootBone, {0.0f, -0.18f + bob, 0.0f}, {});
            setPoseBone(pose, kTorsoBone,
                        {0.0f, base[kTorsoBone].position.y * 0.62f, 0.0f},
                        {-14.0f, 0.0f, torsoRoll});
            setPoseBone(pose, kHeadBone,
                        {0.0f, base[kHeadBone].position.y * 0.72f, 0.0f},
                        {10.0f, 0.0f, -torsoRoll * 0.5f});
            setPoseBone(
                pose, kLeftUpperArmBone,
                {base[kLeftUpperArmBone].position.x * 0.92f,
                 base[kLeftUpperArmBone].position.y * 0.85f,
                 base[kLeftUpperArmBone].position.z},
                {-18.0f - armSwing, 0.0f, -4.0f});
            setPoseBone(pose, kLeftLowerArmBone,
                        {0.0f, base[kLeftLowerArmBone].position.y * 0.68f,
                         0.02f},
                        {28.0f + armSwing * 0.3f, 0.0f, 0.0f});
            setPoseBone(
                pose, kRightUpperArmBone,
                {base[kRightUpperArmBone].position.x * 0.92f,
                 base[kRightUpperArmBone].position.y * 0.85f,
                 base[kRightUpperArmBone].position.z},
                {-18.0f + armSwing, 0.0f, 4.0f});
            setPoseBone(pose, kRightLowerArmBone,
                        {0.0f, base[kRightLowerArmBone].position.y * 0.68f,
                         0.02f},
                        {28.0f - armSwing * 0.3f, 0.0f, 0.0f});
            setPoseBone(
                pose, kLeftUpperLegBone,
                {base[kLeftUpperLegBone].position.x,
                 base[kLeftUpperLegBone].position.y + 0.08f, 0.0f},
                {-62.0f + legSwing, 0.0f, 0.0f});
            setPoseBone(pose, kLeftLowerLegBone,
                        {0.0f, base[kLeftLowerLegBone].position.y * 0.70f,
                         0.02f},
                        {72.0f - legSwing * 0.5f, 0.0f, 0.0f});
            setPoseBone(
                pose, kRightUpperLegBone,
                {base[kRightUpperLegBone].position.x,
                 base[kRightUpperLegBone].position.y + 0.08f, 0.0f},
                {-62.0f - legSwing, 0.0f, 0.0f});
            setPoseBone(pose, kRightLowerLegBone,
                        {0.0f, base[kRightLowerLegBone].position.y * 0.70f,
                         0.02f},
                        {72.0f + legSwing * 0.5f, 0.0f, 0.0f});
            return pose;
        };
    const auto makePronePose =
        [&](float bob, float torsoRoll, float armSwing, float legSwing) {
            Pose pose = base;
            setPoseBone(pose, kRootBone, {0.0f, -0.34f + bob, 0.16f}, {});
            setPoseBone(pose, kTorsoBone,
                        {0.0f, base[kTorsoBone].position.y * 0.40f, 0.0f},
                        {78.0f, 0.0f, torsoRoll});
            setPoseBone(pose, kHeadBone,
                        {0.0f, base[kHeadBone].position.y * 0.55f, -0.02f},
                        {-62.0f, 0.0f, -torsoRoll * 0.4f});
            setPoseBone(
                pose, kLeftUpperArmBone,
                {base[kLeftUpperArmBone].position.x * 0.72f,
                 base[kLeftUpperArmBone].position.y * 0.60f, 0.18f},
                {66.0f - armSwing, 0.0f, -8.0f});
            setPoseBone(pose, kLeftLowerArmBone,
                        {0.0f, base[kLeftLowerArmBone].position.y * 0.58f,
                         0.02f},
                        {-18.0f + armSwing * 0.45f, 0.0f, 0.0f});
            setPoseBone(
                pose, kRightUpperArmBone,
                {base[kRightUpperArmBone].position.x * 0.72f,
                 base[kRightUpperArmBone].position.y * 0.60f, 0.18f},
                {66.0f + armSwing, 0.0f, 8.0f});
            setPoseBone(pose, kRightLowerArmBone,
                        {0.0f, base[kRightLowerArmBone].position.y * 0.58f,
                         0.02f},
                        {-18.0f - armSwing * 0.45f, 0.0f, 0.0f});
            setPoseBone(
                pose, kLeftUpperLegBone,
                {base[kLeftUpperLegBone].position.x,
                 base[kLeftUpperLegBone].position.y, -0.12f},
                {82.0f + legSwing, 0.0f, 0.0f});
            setPoseBone(pose, kLeftLowerLegBone,
                        {0.0f, base[kLeftLowerLegBone].position.y * 0.62f,
                         0.0f},
                        {-12.0f - legSwing * 0.5f, 0.0f, 0.0f});
            setPoseBone(
                pose, kRightUpperLegBone,
                {base[kRightUpperLegBone].position.x,
                 base[kRightUpperLegBone].position.y, -0.12f},
                {82.0f - legSwing, 0.0f, 0.0f});
            setPoseBone(pose, kRightLowerLegBone,
                        {0.0f, base[kRightLowerLegBone].position.y * 0.62f,
                         0.0f},
                        {-12.0f + legSwing * 0.5f, 0.0f, 0.0f});
            return pose;
        };

    const Pose crouchIdle0 = makeCrouchPose(0.0f, 0.0f, 0.0f, 0.0f);
    const Pose crouchIdle1 = makeCrouchPose(0.025f, 0.0f, 3.0f, 0.0f);
    const Pose crouchIdle2 = crouchIdle0;
    const Pose crouchWalk0 = makeCrouchPose(0.0f, -2.0f, -26.0f, -18.0f);
    const Pose crouchWalk1 = makeCrouchPose(0.025f, -2.0f, 26.0f, 18.0f);
    const Pose crouchWalk2 = makeCrouchPose(0.0f, 0.0f, 0.0f, 0.0f);
    const Pose crouchWalk3 = makeCrouchPose(0.025f, 2.0f, -26.0f, -18.0f);
    const Pose crouchWalk4 = crouchWalk0;

    const Pose proneIdle0 = makePronePose(0.0f, 0.0f, 0.0f, 0.0f);
    const Pose proneIdle1 = makePronePose(0.02f, 0.0f, 4.0f, 2.0f);
    const Pose proneIdle2 = proneIdle0;
    const Pose proneWalk0 = makePronePose(0.0f, -2.0f, -24.0f, -10.0f);
    const Pose proneWalk1 = makePronePose(0.025f, -2.0f, 24.0f, 10.0f);
    const Pose proneWalk2 = makePronePose(0.0f, 0.0f, 0.0f, 0.0f);
    const Pose proneWalk3 = makePronePose(0.025f, 2.0f, -24.0f, -10.0f);
    const Pose proneWalk4 = proneWalk0;

    return {
        makeClip(1.9f, true,
                 {makeKeyframe(0.0f, idle0), makeKeyframe(0.95f, idle1),
                  makeKeyframe(1.9f, idle2)}),
        makeClip(0.78f, true,
                 {makeKeyframe(0.0f, walk0), makeKeyframe(0.195f, walk1),
                  makeKeyframe(0.39f, walk2), makeKeyframe(0.585f, walk3),
                  makeKeyframe(0.78f, walk4)}),
        makeClip(0.54f, true,
                 {makeKeyframe(0.0f, run0), makeKeyframe(0.135f, run1),
                  makeKeyframe(0.27f, run2), makeKeyframe(0.405f, run3),
                  makeKeyframe(0.54f, run4)}),
        makeClip(0.70f, false,
                 {makeKeyframe(0.0f, attack0), makeKeyframe(0.12f, attack1),
                  makeKeyframe(0.29f, attack2), makeKeyframe(0.45f, attack3),
                  makeKeyframe(0.70f, attack4)}),
        makeClip(1.55f, true,
                 {makeKeyframe(0.0f, crouchIdle0),
                  makeKeyframe(0.78f, crouchIdle1),
                  makeKeyframe(1.55f, crouchIdle2)}),
        makeClip(0.92f, true,
                 {makeKeyframe(0.0f, crouchWalk0),
                  makeKeyframe(0.23f, crouchWalk1),
                  makeKeyframe(0.46f, crouchWalk2),
                  makeKeyframe(0.69f, crouchWalk3),
                  makeKeyframe(0.92f, crouchWalk4)}),
        makeClip(1.65f, true,
                 {makeKeyframe(0.0f, proneIdle0),
                  makeKeyframe(0.83f, proneIdle1),
                  makeKeyframe(1.65f, proneIdle2)}),
        makeClip(1.08f, true,
                 {makeKeyframe(0.0f, proneWalk0),
                  makeKeyframe(0.27f, proneWalk1),
                  makeKeyframe(0.54f, proneWalk2),
                  makeKeyframe(0.81f, proneWalk3),
                  makeKeyframe(1.08f, proneWalk4)}),
    };
}

void sampleAnimation(const AnimationClip& clip, float time, Pose& pose) {
    const float sampleTime =
        clip.looping
            ? std::fmod(std::max(0.0f, time), clip.duration)
            : std::clamp(time, 0.0f, clip.duration);
    const AnimationKeyframe& first = clip.keyframes.front();
    const AnimationKeyframe& last = clip.keyframes.back();
    if (sampleTime <= first.time) {
        pose = first.bones;
        return;
    }
    if (sampleTime >= last.time) {
        pose = last.bones;
        return;
    }

    const auto next = std::upper_bound(
        clip.keyframes.begin(), clip.keyframes.end(), sampleTime,
        [](float value, const AnimationKeyframe& keyframe) {
            return value < keyframe.time;
        });
    const std::size_t nextIndex =
        static_cast<std::size_t>(next - clip.keyframes.begin());
    const AnimationKeyframe& previous = clip.keyframes[nextIndex - 1];
    const AnimationKeyframe& following = clip.keyframes[nextIndex];
    const float span = following.time - previous.time;
    const float blend =
        span > 0.0001f ? (sampleTime - previous.time) / span : 0.0f;

    for (std::size_t i = 0; i < kBoneCount; ++i) {
        pose[i].position =
            ThreeDUtils::lerp(previous.bones[i].position,
                              following.bones[i].position, blend);
        pose[i].rotationDegrees =
            ThreeDUtils::lerp(previous.bones[i].rotationDegrees,
                              following.bones[i].rotationDegrees, blend);
    }
}

Color tint(const Color& color, float factor) {
    return ThreeDUtils::shade(color, factor);
}

Vec3 ringPoint(float angle, float radiusX, float radiusZ, float y) {
    return {std::sin(angle) * radiusX, y, std::cos(angle) * radiusZ};
}

void addFrustum(VertexBuffer& buffer, int sides, float topRadiusX,
                float topRadiusZ, float bottomRadiusX, float bottomRadiusZ,
                float halfHeight, const Color& topColor,
                const Color& sideColor, const Color& bottomColor) {
    const Vec3 topCenter{0.0f, halfHeight, 0.0f};
    const Vec3 bottomCenter{0.0f, -halfHeight, 0.0f};
    for (int i = 0; i < sides; ++i) {
        const int next = (i + 1) % sides;
        const float angleA = kTau * static_cast<float>(i) /
                             static_cast<float>(sides);
        const float angleB = kTau * static_cast<float>(next) /
                             static_cast<float>(sides);
        const Vec3 topA = ringPoint(angleA, topRadiusX, topRadiusZ, halfHeight);
        const Vec3 topB = ringPoint(angleB, topRadiusX, topRadiusZ, halfHeight);
        const Vec3 bottomA =
            ringPoint(angleA, bottomRadiusX, bottomRadiusZ, -halfHeight);
        const Vec3 bottomB =
            ringPoint(angleB, bottomRadiusX, bottomRadiusZ, -halfHeight);
        const float midAngle = (angleA + angleB) * 0.5f;
        const Color faceColor =
            tint(sideColor, 0.82f + 0.18f * std::cos(midAngle));
        buffer.addTriangle(topCenter, topColor, topA, topColor, topB, topColor);
        buffer.addTriangle(bottomCenter, bottomColor, bottomB, bottomColor,
                           bottomA, bottomColor);
        buffer.addQuad(topA, faceColor, topB, faceColor, bottomB, faceColor,
                       bottomA, faceColor);
    }
}

VertexBuffer makePrismBuffer(int sides, float topRadiusX, float topRadiusZ,
                             float bottomRadiusX, float bottomRadiusZ,
                             float halfHeight, const Color& topColor,
                             const Color& sideColor,
                             const Color& bottomColor) {
    VertexBuffer buffer;
    addFrustum(buffer, sides, topRadiusX, topRadiusZ, bottomRadiusX,
               bottomRadiusZ, halfHeight, topColor, sideColor, bottomColor);
    return buffer;
}

VertexBuffer makePlateBuffer(const Color& color) {
    VertexBuffer buffer;
    const Color shadow = tint(color, 0.90f);
    const Color highlight = tint(color, 1.05f);
    buffer.addQuad({-0.5f, -0.5f, 0.0f}, shadow, {0.5f, -0.5f, 0.0f},
                   shadow, {0.5f, 0.5f, 0.0f}, highlight,
                   {-0.5f, 0.5f, 0.0f}, highlight);
    return buffer;
}

void drawBoundPart(const VertexBuffer& buffer, const Bone& bone,
                   const Vec3& offset, const Vec3& size) {
    const Matrix4 transform =
        multiply(bone.worldMatrix,
                 multiply(translationMatrix(offset), scaleMatrix(size)));
    buffer.drawTransformed(transform);
}

}  // namespace

struct PoliceModel::Impl {
    Impl()
        : torso(makePrismBuffer(
              6, 0.46f, 0.36f, 0.54f, 0.44f, 0.52f,
              tint(constants::kPoliceUniform, 1.08f),
              constants::kPoliceUniform,
              tint(constants::kPoliceUniformDark, 0.85f))),
          shirtPanel(makePlateBuffer(constants::kPoliceUniformLight)),
          badge(makePlateBuffer(constants::kPoliceBadge)),
          belt(makePlateBuffer(constants::kPoliceBelt)),
          upperArm(makePrismBuffer(
              6, 0.16f, 0.15f, 0.22f, 0.20f, 0.45f,
              tint(constants::kPoliceUniformDark, 1.04f),
              constants::kPoliceUniformDark,
              tint(constants::kPoliceUniformDark, 0.82f))),
          lowerArm(makePrismBuffer(
              6, 0.14f, 0.13f, 0.19f, 0.18f, 0.42f,
              tint(constants::kPoliceSkin, 1.04f),
              constants::kPoliceSkin,
              tint(constants::kPoliceSkin, 0.82f))),
          upperLeg(makePrismBuffer(
              6, 0.18f, 0.16f, 0.23f, 0.21f, 0.45f,
              tint(constants::kPoliceUniformDark, 1.02f),
              constants::kPoliceUniformDark,
              tint(constants::kPoliceUniformDark, 0.76f))),
          lowerLeg(makePrismBuffer(
              6, 0.16f, 0.14f, 0.21f, 0.19f, 0.42f,
              tint(constants::kPoliceShoe, 1.08f),
              constants::kPoliceShoe,
              tint(constants::kPoliceShoe, 0.76f))),
          head(makePrismBuffer(
              8, 0.44f, 0.40f, 0.50f, 0.46f, 0.46f,
              tint(constants::kPoliceSkin, 1.08f),
              constants::kPoliceSkin,
              tint(constants::kPoliceSkin, 0.88f))),
          cap(makePrismBuffer(
              8, 0.46f, 0.42f, 0.56f, 0.50f, 0.22f,
              tint(constants::kPoliceCap, 1.05f),
              constants::kPoliceCap,
              tint(constants::kPoliceCapDark, 0.82f))),
          capBadge(makePlateBuffer(constants::kPoliceBadge)),
          capBrim(makePlateBuffer(constants::kPoliceCapDark)),
          eyeWhite(makePlateBuffer(constants::kPoliceEyeWhite)),
          pupil(makePlateBuffer(constants::kPoliceEye)),
          mouth(makePlateBuffer(constants::kPoliceMouth)),
          animations(makeAnimationClips()),
          bones{} {
        bones[kRootBone] = {-1, {}, {}, identityMatrix()};
        bones[kTorsoBone] = {kRootBone, {}, {}, identityMatrix()};
        bones[kHeadBone] = {kTorsoBone, {}, {}, identityMatrix()};
        bones[kLeftUpperArmBone] = {kTorsoBone, {}, {}, identityMatrix()};
        bones[kLeftLowerArmBone] = {
            kLeftUpperArmBone, {}, {}, identityMatrix()};
        bones[kRightUpperArmBone] = {kTorsoBone, {}, {}, identityMatrix()};
        bones[kRightLowerArmBone] = {
            kRightUpperArmBone, {}, {}, identityMatrix()};
        bones[kLeftUpperLegBone] = {kTorsoBone, {}, {}, identityMatrix()};
        bones[kLeftLowerLegBone] = {
            kLeftUpperLegBone, {}, {}, identityMatrix()};
        bones[kRightUpperLegBone] = {kTorsoBone, {}, {}, identityMatrix()};
        bones[kRightLowerLegBone] = {
            kRightUpperLegBone, {}, {}, identityMatrix()};
    }

    void updateSkeleton(AnimationId animation, float time) {
        Pose pose{};
        sampleAnimation(animations[static_cast<std::size_t>(animation)], time,
                        pose);
        const bool attackUsesFixedLowerBody = animation == AnimationId::Attack;
        const Pose basePose =
            attackUsesFixedLowerBody ? makeBasePose() : Pose{};
        for (std::size_t i = 0; i < kBoneCount; ++i) {
            bones[i].localPosition = pose[i].position;
            bones[i].localRotationDegrees = pose[i].rotationDegrees;
        }
        for (std::size_t i = 0; i < kBoneCount; ++i) {
            const bool isUpperLegBone =
                i == kLeftUpperLegBone || i == kRightUpperLegBone;
            const bool isFixedLowerBodyBone =
                attackUsesFixedLowerBody &&
                (isUpperLegBone || i == kLeftLowerLegBone ||
                 i == kRightLowerLegBone);
            int parent = bones[i].parent;
            Vec3 localPosition = bones[i].localPosition;
            if (isFixedLowerBodyBone) {
                parent = i == kLeftUpperLegBone || i == kRightUpperLegBone
                             ? kRootBone
                             : bones[i].parent;
            }
            if (attackUsesFixedLowerBody && isUpperLegBone) {
                localPosition = localPosition + basePose[kTorsoBone].position;
            }
            const Matrix4 local = boneLocalMatrix(
                localPosition, bones[i].localRotationDegrees);
            bones[i].worldMatrix =
                parent < 0
                    ? local
                    : multiply(
                          bones[static_cast<std::size_t>(parent)].worldMatrix,
                          local);
        }
    }

    VertexBuffer torso;
    VertexBuffer shirtPanel;
    VertexBuffer badge;
    VertexBuffer belt;
    VertexBuffer upperArm;
    VertexBuffer lowerArm;
    VertexBuffer upperLeg;
    VertexBuffer lowerLeg;
    VertexBuffer head;
    VertexBuffer cap;
    VertexBuffer capBadge;
    VertexBuffer capBrim;
    VertexBuffer eyeWhite;
    VertexBuffer pupil;
    VertexBuffer mouth;
    std::array<AnimationClip, static_cast<std::size_t>(AnimationId::Count)>
        animations;
    std::array<Bone, kBoneCount> bones;
};

PoliceModel::PoliceModel()
    : animationPhase_(0.0f),
      crouched_(false),
      prone_(false),
      moving_(false),
      position_({3.0f, 0.0f, 0.6f}),
      impl_(nullptr) {
    buildMeshes();
    reset();
}

PoliceModel::~PoliceModel() = default;

void PoliceModel::buildMeshes() {
    impl_ = std::make_unique<Impl>();
}

void PoliceModel::reset() {
    animationPhase_ = 0.0f;
    crouched_ = false;
    prone_ = false;
    moving_ = false;
    position_ = {3.0f, 0.0f, 0.6f};
    impl_->updateSkeleton(AnimationId::Attack, 0.0f);
}

void PoliceModel::update(float dt) {
    const AnimationId animation =
        prone_ ? (moving_ ? AnimationId::ProneWalk : AnimationId::ProneIdle)
               : (crouched_
                      ? (moving_ ? AnimationId::CrouchWalk
                                 : AnimationId::CrouchIdle)
                      : AnimationId::Attack);
    const float cycleMultiplier =
        prone_ ? constants::kCharacterProneCycleMultiplier
               : (crouched_ ? constants::kCharacterCrouchCycleMultiplier
                            : 1.0f);
    const float duration =
        impl_->animations[static_cast<std::size_t>(animation)].duration;
    animationPhase_ = std::fmod(
        animationPhase_ + dt * cycleMultiplier, duration);
    impl_->updateSkeleton(animation, animationPhase_);
}

void PoliceModel::setPosition(const Vec3& position) {
    position_ = position;
}

void PoliceModel::setCrouched(bool crouched) {
    crouched_ = crouched;
    if (crouched_) {
        prone_ = false;
    }
    animationPhase_ = 0.0f;
    const AnimationId animation =
        prone_ ? (moving_ ? AnimationId::ProneWalk : AnimationId::ProneIdle)
               : (crouched_
                      ? (moving_ ? AnimationId::CrouchWalk
                                 : AnimationId::CrouchIdle)
                      : AnimationId::Attack);
    impl_->updateSkeleton(animation, 0.0f);
}

void PoliceModel::setProne(bool prone) {
    prone_ = prone;
    if (prone_) {
        crouched_ = false;
    }
    animationPhase_ = 0.0f;
    const AnimationId animation =
        prone_ ? (moving_ ? AnimationId::ProneWalk : AnimationId::ProneIdle)
               : (crouched_
                      ? (moving_ ? AnimationId::CrouchWalk
                                 : AnimationId::CrouchIdle)
                      : AnimationId::Attack);
    impl_->updateSkeleton(animation, 0.0f);
}

void PoliceModel::setMoving(bool moving) {
    moving_ = moving;
    animationPhase_ = 0.0f;
    const AnimationId animation =
        prone_ ? (moving_ ? AnimationId::ProneWalk : AnimationId::ProneIdle)
               : (crouched_
                      ? (moving_ ? AnimationId::CrouchWalk
                                 : AnimationId::CrouchIdle)
                      : AnimationId::Attack);
    impl_->updateSkeleton(animation, 0.0f);
}

void PoliceModel::render() const {
    glPushMatrix();
    glTranslatef(position_.x, position_.y, position_.z);
    glScalef(0.56f, 0.56f, 0.56f);

    drawBoundPart(impl_->torso, impl_->bones[kTorsoBone], {},
                  {1.02f, 1.20f, 0.72f});
    drawBoundPart(impl_->shirtPanel, impl_->bones[kTorsoBone],
                  {0.0f, 0.13f, 0.39f}, {0.70f, 0.58f, 1.0f});
    drawBoundPart(impl_->badge, impl_->bones[kTorsoBone],
                  {0.24f, 0.28f, 0.44f}, {0.14f, 0.16f, 1.0f});
    drawBoundPart(impl_->belt, impl_->bones[kTorsoBone],
                  {0.0f, -0.44f, 0.43f}, {0.88f, 0.14f, 1.0f});

    drawBoundPart(impl_->upperLeg, impl_->bones[kLeftUpperLegBone],
                  {0.0f, -0.34f, 0.0f}, {0.40f, 0.88f, 0.40f});
    drawBoundPart(impl_->lowerLeg, impl_->bones[kLeftLowerLegBone],
                  {0.0f, -0.28f, 0.04f}, {0.36f, 0.74f, 0.36f});
    drawBoundPart(impl_->upperLeg, impl_->bones[kRightUpperLegBone],
                  {0.0f, -0.34f, 0.0f}, {0.40f, 0.88f, 0.40f});
    drawBoundPart(impl_->lowerLeg, impl_->bones[kRightLowerLegBone],
                  {0.0f, -0.28f, 0.04f}, {0.36f, 0.74f, 0.36f});

    drawBoundPart(impl_->upperArm, impl_->bones[kLeftUpperArmBone],
                  {0.0f, -0.36f, 0.0f}, {0.32f, 0.90f, 0.32f});
    drawBoundPart(impl_->lowerArm, impl_->bones[kLeftLowerArmBone],
                  {0.0f, -0.30f, 0.02f}, {0.28f, 0.78f, 0.28f});
    drawBoundPart(impl_->upperArm, impl_->bones[kRightUpperArmBone],
                  {0.0f, -0.36f, 0.0f}, {0.32f, 0.90f, 0.32f});
    drawBoundPart(impl_->lowerArm, impl_->bones[kRightLowerArmBone],
                  {0.0f, -0.30f, 0.02f}, {0.28f, 0.78f, 0.28f});

    drawBoundPart(impl_->head, impl_->bones[kHeadBone], {},
                  {1.0f, 1.0f, 1.0f});
    drawBoundPart(impl_->cap, impl_->bones[kHeadBone],
                  {0.0f, 0.38f, -0.03f}, {1.0f, 0.72f, 1.0f});
    drawBoundPart(impl_->capBrim, impl_->bones[kHeadBone],
                  {0.0f, 0.24f, 0.48f}, {0.70f, 0.16f, 1.0f});
    drawBoundPart(impl_->capBadge, impl_->bones[kHeadBone],
                  {0.0f, 0.36f, 0.49f}, {0.14f, 0.14f, 1.0f});

    drawBoundPart(impl_->eyeWhite, impl_->bones[kHeadBone],
                  {-0.16f, -0.03f, 0.48f}, {0.22f, 0.18f, 1.0f});
    drawBoundPart(impl_->eyeWhite, impl_->bones[kHeadBone],
                  {0.16f, -0.03f, 0.48f}, {0.22f, 0.18f, 1.0f});
    drawBoundPart(impl_->pupil, impl_->bones[kHeadBone],
                  {-0.16f, -0.03f, 0.54f}, {0.09f, 0.11f, 1.0f});
    drawBoundPart(impl_->pupil, impl_->bones[kHeadBone],
                  {0.16f, -0.03f, 0.54f}, {0.09f, 0.11f, 1.0f});
    drawBoundPart(impl_->mouth, impl_->bones[kHeadBone],
                  {0.0f, -0.18f, 0.50f}, {0.22f, 0.07f, 1.0f});

    glPopMatrix();
}

}  // namespace pixel_world
