#include "LowPolyCharacterModel.h"

#include "ThreeDUtils.h"
#include "game_constants.h"

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

struct Matrix4;

class VertexBuffer {
public:
    void clear() {
        vertices_.clear();
    }

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
        drawVertices(vertices_);
    }

    void drawTransformed(const Matrix4& transform) const;

private:
    static void drawVertices(const std::vector<Vertex>& vertices) {
        if (vertices.empty()) {
            return;
        }

        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        glVertexPointer(3, GL_FLOAT, sizeof(Vertex), vertices.data()->position);
        glColorPointer(3, GL_FLOAT, sizeof(Vertex), vertices.data()->color);
        glDrawArrays(GL_TRIANGLES, 0,
                     static_cast<GLsizei>(vertices.size()));
        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
    }

    static Vertex makeVertex(const Vec3& position, const Color& color) {
        return Vertex{{position.x, position.y, position.z},
                      {color.red, color.green, color.blue}};
    }

    std::vector<Vertex> vertices_;
};

constexpr float kTau = constants::kPi * 2.0f;

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

Matrix4 scaleMatrix(const Vec3& scale) {
    Matrix4 result = identityMatrix();
    result.values[0] = scale.x;
    result.values[5] = scale.y;
    result.values[10] = scale.z;
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

void VertexBuffer::drawTransformed(const Matrix4& transform) const {
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
    drawVertices(transformed);
}

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
    const char* name;
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
    Death,
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
    setPoseBone(pose, kTorsoBone, {0.0f, 1.65f, 0.0f}, {});
    setPoseBone(pose, kHeadBone, {0.0f, 1.40f, 0.0f}, {});
    setPoseBone(pose, kLeftUpperArmBone, {-0.72f, 0.45f, 0.12f}, {});
    setPoseBone(pose, kLeftLowerArmBone, {0.0f, -0.80f, 0.0f}, {});
    setPoseBone(pose, kRightUpperArmBone, {0.72f, 0.45f, -0.12f}, {});
    setPoseBone(pose, kRightLowerArmBone, {0.0f, -0.80f, 0.0f}, {});
    setPoseBone(pose, kLeftUpperLegBone, {-0.20f, -0.10f, 0.05f}, {});
    setPoseBone(pose, kLeftLowerLegBone, {0.0f, -0.73f, 0.0f}, {});
    setPoseBone(pose, kRightUpperLegBone, {0.20f, -0.10f, -0.05f}, {});
    setPoseBone(pose, kRightLowerLegBone, {0.0f, -0.73f, 0.0f}, {});
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
    setPoseBone(idle1, kRootBone, {0.0f, 0.035f, 0.0f}, {});
    setPoseBone(idle1, kTorsoBone, {0.0f, 1.66f, 0.0f}, {});
    setPoseBone(idle1, kHeadBone, {0.0f, 1.39f, 0.0f},
                {0.0f, 0.0f, -1.5f});
    setPoseBone(idle1, kLeftUpperArmBone, {-0.72f, 0.46f, 0.12f},
                {-3.0f, 0.0f, 1.5f});
    setPoseBone(idle1, kRightUpperArmBone, {0.72f, 0.46f, -0.12f},
                {3.0f, 0.0f, -1.5f});
    const Pose idle2 = idle0;

    Pose walk0 = base;
    Pose walk1 = base;
    setPoseBone(walk1, kRootBone, {0.0f, 0.045f, 0.0f}, {});
    setPoseBone(walk1, kTorsoBone, {0.0f, 1.67f, 0.0f}, {});
    setPoseBone(walk1, kLeftUpperArmBone, {-0.72f, 0.47f, 0.12f},
                {-30.0f, 0.0f, 0.0f});
    setPoseBone(walk1, kLeftLowerArmBone, {0.0f, -0.79f, 0.0f},
                {6.0f, 0.0f, 0.0f});
    setPoseBone(walk1, kRightUpperArmBone, {0.72f, 0.47f, -0.12f},
                {30.0f, 0.0f, 0.0f});
    setPoseBone(walk1, kRightLowerArmBone, {0.0f, -0.79f, 0.0f},
                {-6.0f, 0.0f, 0.0f});
    setPoseBone(walk1, kLeftUpperLegBone, {-0.20f, -0.12f, 0.05f},
                {28.0f, 0.0f, 0.0f});
    setPoseBone(walk1, kLeftLowerLegBone, {0.0f, -0.72f, 0.0f},
                {-10.0f, 0.0f, 0.0f});
    setPoseBone(walk1, kRightUpperLegBone, {0.20f, -0.08f, -0.05f},
                {-28.0f, 0.0f, 0.0f});
    setPoseBone(walk1, kRightLowerLegBone, {0.0f, -0.74f, 0.0f},
                {10.0f, 0.0f, 0.0f});
    Pose walk2 = base;
    setPoseBone(walk2, kRootBone, {0.0f, 0.015f, 0.0f}, {});
    setPoseBone(walk2, kHeadBone, {0.0f, 1.40f, 0.0f},
                {0.0f, 0.0f, -1.0f});
    Pose walk3 = base;
    setPoseBone(walk3, kRootBone, {0.0f, 0.045f, 0.0f}, {});
    setPoseBone(walk3, kTorsoBone, {0.0f, 1.67f, 0.0f}, {});
    setPoseBone(walk3, kLeftUpperArmBone, {-0.72f, 0.47f, 0.12f},
                {30.0f, 0.0f, 0.0f});
    setPoseBone(walk3, kLeftLowerArmBone, {0.0f, -0.79f, 0.0f},
                {-6.0f, 0.0f, 0.0f});
    setPoseBone(walk3, kRightUpperArmBone, {0.72f, 0.47f, -0.12f},
                {-30.0f, 0.0f, 0.0f});
    setPoseBone(walk3, kRightLowerArmBone, {0.0f, -0.79f, 0.0f},
                {6.0f, 0.0f, 0.0f});
    setPoseBone(walk3, kLeftUpperLegBone, {-0.20f, -0.08f, 0.05f},
                {-28.0f, 0.0f, 0.0f});
    setPoseBone(walk3, kLeftLowerLegBone, {0.0f, -0.74f, 0.0f},
                {10.0f, 0.0f, 0.0f});
    setPoseBone(walk3, kRightUpperLegBone, {0.20f, -0.12f, -0.05f},
                {28.0f, 0.0f, 0.0f});
    setPoseBone(walk3, kRightLowerLegBone, {0.0f, -0.72f, 0.0f},
                {-10.0f, 0.0f, 0.0f});
    const Pose walk4 = walk0;

    Pose run0 = base;
    Pose run1 = walk1;
    setPoseBone(run1, kRootBone, {0.0f, 0.075f, -0.015f}, {});
    setPoseBone(run1, kTorsoBone, {0.0f, 1.69f, 0.0f},
                {-4.0f, 0.0f, 0.0f});
    setPoseBone(run1, kLeftUpperArmBone, {-0.72f, 0.50f, 0.12f},
                {-48.0f, 0.0f, 0.0f});
    setPoseBone(run1, kLeftLowerArmBone, {0.0f, -0.78f, 0.0f},
                {12.0f, 0.0f, 0.0f});
    setPoseBone(run1, kRightUpperArmBone, {0.72f, 0.50f, -0.12f},
                {48.0f, 0.0f, 0.0f});
    setPoseBone(run1, kRightLowerArmBone, {0.0f, -0.78f, 0.0f},
                {-12.0f, 0.0f, 0.0f});
    setPoseBone(run1, kLeftUpperLegBone, {-0.20f, -0.15f, 0.05f},
                {42.0f, 0.0f, 0.0f});
    setPoseBone(run1, kLeftLowerLegBone, {0.0f, -0.70f, 0.0f},
                {-18.0f, 0.0f, 0.0f});
    setPoseBone(run1, kRightUpperLegBone, {0.20f, -0.05f, -0.05f},
                {-42.0f, 0.0f, 0.0f});
    setPoseBone(run1, kRightLowerLegBone, {0.0f, -0.76f, 0.0f},
                {18.0f, 0.0f, 0.0f});
    Pose run2 = base;
    setPoseBone(run2, kRootBone, {0.0f, 0.02f, -0.005f}, {});
    setPoseBone(run2, kTorsoBone, {0.0f, 1.65f, 0.0f},
                {-8.0f, 0.0f, 0.0f});
    Pose run3 = run1;
    setPoseBone(run3, kRootBone, {0.0f, 0.075f, -0.015f}, {});
    setPoseBone(run3, kLeftUpperArmBone, {-0.72f, 0.50f, 0.12f},
                {48.0f, 0.0f, 0.0f});
    setPoseBone(run3, kLeftLowerArmBone, {0.0f, -0.78f, 0.0f},
                {-12.0f, 0.0f, 0.0f});
    setPoseBone(run3, kRightUpperArmBone, {0.72f, 0.50f, -0.12f},
                {-48.0f, 0.0f, 0.0f});
    setPoseBone(run3, kRightLowerArmBone, {0.0f, -0.78f, 0.0f},
                {12.0f, 0.0f, 0.0f});
    setPoseBone(run3, kLeftUpperLegBone, {-0.20f, -0.05f, 0.05f},
                {-42.0f, 0.0f, 0.0f});
    setPoseBone(run3, kLeftLowerLegBone, {0.0f, -0.76f, 0.0f},
                {18.0f, 0.0f, 0.0f});
    setPoseBone(run3, kRightUpperLegBone, {0.20f, -0.15f, -0.05f},
                {42.0f, 0.0f, 0.0f});
    setPoseBone(run3, kRightLowerLegBone, {0.0f, -0.70f, 0.0f},
                {-18.0f, 0.0f, 0.0f});
    const Pose run4 = run0;

    Pose attack0 = base;
    Pose attack1 = base;
    setPoseBone(attack1, kRootBone, {0.0f, 0.02f, -0.03f},
                {0.0f, 0.0f, -4.0f});
    setPoseBone(attack1, kTorsoBone, {0.0f, 1.66f, 0.0f},
                {-4.0f, 0.0f, 0.0f});
    setPoseBone(attack1, kHeadBone, {0.0f, 1.40f, 0.0f},
                {8.0f, 0.0f, 0.0f});
    setPoseBone(attack1, kLeftUpperArmBone, {-0.72f, 0.50f, 0.14f},
                {-24.0f, 0.0f, 0.0f});
    setPoseBone(attack1, kRightUpperArmBone, {0.74f, 0.54f, -0.18f},
                {78.0f, 0.0f, 0.0f});
    setPoseBone(attack1, kRightLowerArmBone, {0.0f, -0.73f, -0.08f},
                {18.0f, 0.0f, 0.0f});
    Pose attack2 = attack1;
    setPoseBone(attack2, kRootBone, {0.0f, 0.035f, -0.05f},
                {0.0f, 0.0f, -8.0f});
    setPoseBone(attack2, kTorsoBone, {0.0f, 1.68f, 0.0f},
                {-8.0f, 0.0f, 0.0f});
    setPoseBone(attack2, kHeadBone, {0.0f, 1.38f, 0.0f},
                {16.0f, 0.0f, 0.0f});
    setPoseBone(attack2, kLeftUpperArmBone, {-0.72f, 0.52f, 0.14f},
                {-36.0f, 0.0f, 0.0f});
    setPoseBone(attack2, kRightUpperArmBone, {0.76f, 0.58f, -0.22f},
                {132.0f, 0.0f, 0.0f});
    setPoseBone(attack2, kRightLowerArmBone, {0.0f, -0.66f, -0.12f},
                {48.0f, 0.0f, 0.0f});
    Pose attack3 = attack2;
    setPoseBone(attack3, kRootBone, {0.0f, 0.015f, -0.02f},
                {0.0f, 0.0f, 4.0f});
    setPoseBone(attack3, kTorsoBone, {0.0f, 1.65f, 0.0f},
                {4.0f, 0.0f, 0.0f});
    setPoseBone(attack3, kHeadBone, {0.0f, 1.40f, 0.0f},
                {5.0f, 0.0f, 0.0f});
    setPoseBone(attack3, kRightUpperArmBone, {0.74f, 0.50f, -0.15f},
                {96.0f, 0.0f, 0.0f});
    setPoseBone(attack3, kRightLowerArmBone, {0.0f, -0.75f, -0.06f},
                {30.0f, 0.0f, 0.0f});
    const Pose attack4 = base;

    Pose death0 = base;
    Pose death1 = base;
    setPoseBone(death1, kRootBone, {0.0f, -0.02f, -0.03f},
                {-22.0f, 0.0f, 0.0f});
    setPoseBone(death1, kTorsoBone, {0.0f, 1.62f, 0.0f}, {});
    setPoseBone(death1, kHeadBone, {0.0f, 1.36f, 0.0f},
                {-15.0f, 0.0f, 0.0f});
    setPoseBone(death1, kLeftUpperArmBone, {-0.72f, 0.42f, 0.14f},
                {-35.0f, 0.0f, -12.0f});
    setPoseBone(death1, kRightUpperArmBone, {0.72f, 0.42f, -0.14f},
                {-28.0f, 0.0f, 12.0f});
    setPoseBone(death1, kLeftUpperLegBone, {-0.20f, -0.10f, 0.05f},
                {-8.0f, 0.0f, 0.0f});
    setPoseBone(death1, kRightUpperLegBone, {0.20f, -0.10f, -0.05f},
                {8.0f, 0.0f, 0.0f});
    Pose death2 = death1;
    setPoseBone(death2, kRootBone, {0.0f, -0.10f, -0.10f},
                {-58.0f, 0.0f, 0.0f});
    setPoseBone(death2, kTorsoBone, {0.0f, 1.54f, 0.0f}, {});
    setPoseBone(death2, kHeadBone, {0.0f, 1.25f, 0.0f},
                {-32.0f, 0.0f, 0.0f});
    setPoseBone(death2, kLeftUpperArmBone, {-0.76f, 0.30f, 0.16f},
                {-75.0f, 0.0f, -24.0f});
    setPoseBone(death2, kRightUpperArmBone, {0.76f, 0.30f, -0.16f},
                {-65.0f, 0.0f, 24.0f});
    setPoseBone(death2, kLeftUpperLegBone, {-0.20f, -0.08f, 0.05f},
                {-14.0f, 0.0f, 0.0f});
    setPoseBone(death2, kRightUpperLegBone, {0.20f, -0.08f, -0.05f},
                {14.0f, 0.0f, 0.0f});
    Pose death3 = death2;
    setPoseBone(death3, kRootBone, {0.0f, -0.16f, -0.16f},
                {-86.0f, 0.0f, 0.0f});
    setPoseBone(death3, kTorsoBone, {0.0f, 1.48f, 0.0f}, {});
    setPoseBone(death3, kHeadBone, {0.0f, 1.16f, 0.0f},
                {-48.0f, 0.0f, 0.0f});
    setPoseBone(death3, kLeftUpperArmBone, {-0.80f, 0.18f, 0.18f},
                {-104.0f, 0.0f, -32.0f});
    setPoseBone(death3, kRightUpperArmBone, {0.80f, 0.18f, -0.18f},
                {-94.0f, 0.0f, 32.0f});
    setPoseBone(death3, kLeftUpperLegBone, {-0.20f, -0.08f, 0.05f},
                {-18.0f, 0.0f, 0.0f});
    setPoseBone(death3, kRightUpperLegBone, {0.20f, -0.08f, -0.05f},
                {18.0f, 0.0f, 0.0f});

    const auto makeCrouchPose =
        [&](float bob, float torsoRoll, float armSwing, float legSwing) {
            Pose pose = base;
            setPoseBone(pose, kRootBone, {0.0f, -0.20f + bob, 0.0f}, {});
            setPoseBone(pose, kTorsoBone, {0.0f, 1.03f, 0.0f},
                        {-15.0f, 0.0f, torsoRoll});
            setPoseBone(pose, kHeadBone, {0.0f, 0.88f, 0.0f},
                        {11.0f, 0.0f, -torsoRoll * 0.5f});
            setPoseBone(pose, kLeftUpperArmBone, {-0.64f, 0.34f, 0.13f},
                        {-20.0f - armSwing, 0.0f, -3.0f});
            setPoseBone(pose, kLeftLowerArmBone, {0.0f, -0.55f, 0.02f},
                        {30.0f + armSwing * 0.25f, 0.0f, 0.0f});
            setPoseBone(pose, kRightUpperArmBone, {0.64f, 0.34f, -0.13f},
                        {-20.0f + armSwing, 0.0f, 3.0f});
            setPoseBone(pose, kRightLowerArmBone, {0.0f, -0.55f, 0.02f},
                        {30.0f - armSwing * 0.25f, 0.0f, 0.0f});
            setPoseBone(pose, kLeftUpperLegBone, {-0.20f, -0.02f, 0.04f},
                        {-64.0f + legSwing, 0.0f, 0.0f});
            setPoseBone(pose, kLeftLowerLegBone, {0.0f, -0.54f, 0.02f},
                        {76.0f - legSwing * 0.5f, 0.0f, 0.0f});
            setPoseBone(pose, kRightUpperLegBone, {0.20f, -0.02f, -0.04f},
                        {-64.0f - legSwing, 0.0f, 0.0f});
            setPoseBone(pose, kRightLowerLegBone, {0.0f, -0.54f, -0.02f},
                        {76.0f + legSwing * 0.5f, 0.0f, 0.0f});
            return pose;
        };
    const auto makePronePose =
        [&](float bob, float torsoRoll, float armSwing, float legSwing) {
            Pose pose = base;
            setPoseBone(pose, kRootBone, {0.0f, -0.38f + bob, 0.18f}, {});
            setPoseBone(pose, kTorsoBone, {0.0f, 0.68f, 0.0f},
                        {80.0f, 0.0f, torsoRoll});
            setPoseBone(pose, kHeadBone, {0.0f, 0.78f, -0.02f},
                        {-64.0f, 0.0f, -torsoRoll * 0.4f});
            setPoseBone(pose, kLeftUpperArmBone, {-0.52f, 0.27f, 0.20f},
                        {68.0f - armSwing, 0.0f, -7.0f});
            setPoseBone(pose, kLeftLowerArmBone, {0.0f, -0.47f, 0.03f},
                        {-20.0f + armSwing * 0.45f, 0.0f, 0.0f});
            setPoseBone(pose, kRightUpperArmBone, {0.52f, 0.27f, 0.20f},
                        {68.0f + armSwing, 0.0f, 7.0f});
            setPoseBone(pose, kRightLowerArmBone, {0.0f, -0.47f, 0.03f},
                        {-20.0f - armSwing * 0.45f, 0.0f, 0.0f});
            setPoseBone(pose, kLeftUpperLegBone, {-0.20f, -0.10f, -0.14f},
                        {84.0f + legSwing, 0.0f, 0.0f});
            setPoseBone(pose, kLeftLowerLegBone, {0.0f, -0.46f, 0.0f},
                        {-14.0f - legSwing * 0.5f, 0.0f, 0.0f});
            setPoseBone(pose, kRightUpperLegBone, {0.20f, -0.10f, -0.14f},
                        {84.0f - legSwing, 0.0f, 0.0f});
            setPoseBone(pose, kRightLowerLegBone, {0.0f, -0.46f, 0.0f},
                        {-14.0f + legSwing * 0.5f, 0.0f, 0.0f});
            return pose;
        };

    const Pose crouchIdle0 = makeCrouchPose(0.0f, 0.0f, 0.0f, 0.0f);
    const Pose crouchIdle1 = makeCrouchPose(0.025f, 0.0f, 3.0f, 0.0f);
    const Pose crouchIdle2 = crouchIdle0;
    const Pose crouchWalk0 = makeCrouchPose(0.0f, -2.0f, -28.0f, -20.0f);
    const Pose crouchWalk1 = makeCrouchPose(0.025f, -2.0f, 28.0f, 20.0f);
    const Pose crouchWalk2 = makeCrouchPose(0.0f, 0.0f, 0.0f, 0.0f);
    const Pose crouchWalk3 = makeCrouchPose(0.025f, 2.0f, -28.0f, -20.0f);
    const Pose crouchWalk4 = crouchWalk0;

    const Pose proneIdle0 = makePronePose(0.0f, 0.0f, 0.0f, 0.0f);
    const Pose proneIdle1 = makePronePose(0.02f, 0.0f, 4.0f, 2.0f);
    const Pose proneIdle2 = proneIdle0;
    const Pose proneWalk0 = makePronePose(0.0f, -2.0f, -26.0f, -12.0f);
    const Pose proneWalk1 = makePronePose(0.025f, -2.0f, 26.0f, 12.0f);
    const Pose proneWalk2 = makePronePose(0.0f, 0.0f, 0.0f, 0.0f);
    const Pose proneWalk3 = makePronePose(0.025f, 2.0f, -26.0f, -12.0f);
    const Pose proneWalk4 = proneWalk0;

    return {
        makeClip(2.0f, true,
                 {makeKeyframe(0.0f, idle0), makeKeyframe(1.0f, idle1),
                  makeKeyframe(2.0f, idle2)}),
        makeClip(0.80f, true,
                 {makeKeyframe(0.0f, walk0), makeKeyframe(0.20f, walk1),
                  makeKeyframe(0.40f, walk2), makeKeyframe(0.60f, walk3),
                  makeKeyframe(0.80f, walk4)}),
        makeClip(0.56f, true,
                 {makeKeyframe(0.0f, run0), makeKeyframe(0.14f, run1),
                  makeKeyframe(0.28f, run2), makeKeyframe(0.42f, run3),
                  makeKeyframe(0.56f, run4)}),
        makeClip(0.72f, false,
                 {makeKeyframe(0.0f, attack0), makeKeyframe(0.12f, attack1),
                  makeKeyframe(0.30f, attack2), makeKeyframe(0.46f, attack3),
                  makeKeyframe(0.72f, attack4)}),
        makeClip(1.10f, false,
                 {makeKeyframe(0.0f, death0), makeKeyframe(0.18f, death1),
                  makeKeyframe(0.45f, death2), makeKeyframe(1.10f, death3)}),
        makeClip(1.60f, true,
                 {makeKeyframe(0.0f, crouchIdle0),
                  makeKeyframe(0.80f, crouchIdle1),
                  makeKeyframe(1.60f, crouchIdle2)}),
        makeClip(0.94f, true,
                 {makeKeyframe(0.0f, crouchWalk0),
                  makeKeyframe(0.235f, crouchWalk1),
                  makeKeyframe(0.47f, crouchWalk2),
                  makeKeyframe(0.705f, crouchWalk3),
                  makeKeyframe(0.94f, crouchWalk4)}),
        makeClip(1.70f, true,
                 {makeKeyframe(0.0f, proneIdle0),
                  makeKeyframe(0.85f, proneIdle1),
                  makeKeyframe(1.70f, proneIdle2)}),
        makeClip(1.12f, true,
                 {makeKeyframe(0.0f, proneWalk0),
                  makeKeyframe(0.28f, proneWalk1),
                  makeKeyframe(0.56f, proneWalk2),
                  makeKeyframe(0.84f, proneWalk3),
                  makeKeyframe(1.12f, proneWalk4)}),
    };
}

void sampleAnimation(const AnimationClip& clip, float time, Pose& pose) {
    if (clip.keyframes.empty()) {
        pose = {};
        return;
    }

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

struct LowPolyCharacterModel::Impl {
    Impl()
        : torso(makePrismBuffer(
              6, 0.50f, 0.40f, 0.58f, 0.48f, 0.50f,
              tint(constants::kCharacterShirt, 1.10f),
              constants::kCharacterShirt,
              tint(constants::kCharacterShirtDark, 0.92f))),
          upperArm(makePrismBuffer(
              6, 0.18f, 0.16f, 0.24f, 0.22f, 0.50f,
              tint(constants::kCharacterShirtDark, 1.05f),
              constants::kCharacterShirtDark,
              tint(constants::kCharacterShirtDark, 0.85f))),
          lowerArm(makePrismBuffer(
              6, 0.16f, 0.15f, 0.21f, 0.20f, 0.50f,
              tint(constants::kCharacterSkin, 1.06f), constants::kCharacterSkin,
              tint(constants::kCharacterSkin, 0.84f))),
          upperLeg(makePrismBuffer(
              6, 0.18f, 0.16f, 0.23f, 0.21f, 0.50f,
              tint(constants::kCharacterPants, 1.05f), constants::kCharacterPants,
              tint(constants::kCharacterPantsDark, 0.90f))),
          lowerLeg(makePrismBuffer(
              6, 0.16f, 0.14f, 0.21f, 0.19f, 0.50f,
              tint(constants::kCharacterBoot, 1.08f), constants::kCharacterBoot,
              tint(constants::kCharacterBoot, 0.82f))),
          head(makePrismBuffer(
              8, 0.46f, 0.44f, 0.52f, 0.50f, 0.50f,
              tint(constants::kCharacterSkin, 1.08f), constants::kCharacterSkin,
              tint(constants::kCharacterSkin, 0.90f))),
          hair(makePrismBuffer(
              8, 0.54f, 0.50f, 0.61f, 0.58f, 0.50f,
              tint(constants::kCharacterHairLight, 1.02f),
              constants::kCharacterHair,
              tint(constants::kCharacterHair, 0.78f))),
          eyeWhite(makePlateBuffer(constants::kCharacterEyeWhite)),
          pupil(makePlateBuffer(constants::kCharacterEye)),
          mouth(makePlateBuffer(constants::kCharacterMouth)),
          animations(makeAnimationClips()),
          bones{} {
        bones[kRootBone] = {"root", -1, {}, {}, identityMatrix()};
        bones[kTorsoBone] = {"torso", kRootBone, {}, {}, identityMatrix()};
        bones[kHeadBone] = {"head", kTorsoBone, {}, {}, identityMatrix()};
        bones[kLeftUpperArmBone] = {
            "left_upper_arm", kTorsoBone, {}, {}, identityMatrix()};
        bones[kLeftLowerArmBone] = {
            "left_lower_arm", kLeftUpperArmBone, {}, {}, identityMatrix()};
        bones[kRightUpperArmBone] = {
            "right_upper_arm", kTorsoBone, {}, {}, identityMatrix()};
        bones[kRightLowerArmBone] = {
            "right_lower_arm", kRightUpperArmBone, {}, {}, identityMatrix()};
        bones[kLeftUpperLegBone] = {
            "left_upper_leg", kTorsoBone, {}, {}, identityMatrix()};
        bones[kLeftLowerLegBone] = {
            "left_lower_leg", kLeftUpperLegBone, {}, {}, identityMatrix()};
        bones[kRightUpperLegBone] = {
            "right_upper_leg", kTorsoBone, {}, {}, identityMatrix()};
        bones[kRightLowerLegBone] = {
            "right_lower_leg", kRightUpperLegBone, {}, {}, identityMatrix()};
    }

    float animationDuration(AnimationId animation) const {
        return animations[static_cast<std::size_t>(animation)].duration;
    }

    void updateSkeleton(AnimationId animation, float time) {
        Pose pose{};
        sampleAnimation(animations[static_cast<std::size_t>(animation)], time,
                        pose);
        for (std::size_t i = 0; i < kBoneCount; ++i) {
            bones[i].localPosition = pose[i].position;
            bones[i].localRotationDegrees = pose[i].rotationDegrees;
        }
        for (std::size_t i = 0; i < kBoneCount; ++i) {
            const Matrix4 local = boneLocalMatrix(
                bones[i].localPosition, bones[i].localRotationDegrees);
            if (bones[i].parent < 0) {
                bones[i].worldMatrix = local;
            } else {
                bones[i].worldMatrix =
                    multiply(bones[static_cast<std::size_t>(bones[i].parent)]
                                 .worldMatrix,
                             local);
            }
        }
    }

    VertexBuffer torso;
    VertexBuffer upperArm;
    VertexBuffer lowerArm;
    VertexBuffer upperLeg;
    VertexBuffer lowerLeg;
    VertexBuffer head;
    VertexBuffer hair;
    VertexBuffer eyeWhite;
    VertexBuffer pupil;
    VertexBuffer mouth;
    std::array<AnimationClip, static_cast<std::size_t>(AnimationId::Count)>
        animations;
    std::array<Bone, kBoneCount> bones;
};

LowPolyCharacterModel::LowPolyCharacterModel() {
    buildMeshes();
    reset();
}

LowPolyCharacterModel::~LowPolyCharacterModel() = default;

void LowPolyCharacterModel::buildMeshes() {
    impl_ = std::make_unique<Impl>();
}

void LowPolyCharacterModel::reset() {
    position_ = {0.0f, 0.0f, 4.2f};
    yawDegrees_ = 0.0f;
    animationPhase_ = 0.0f;
    patrolDirection_ = 1.0f;
    hp_ = constants::kCharacterMaxHp;
    alive_ = true;
    deathTimer_ = 0.0f;
    attackTimer_ = -1.0f;
    previousAttackDown_ = false;
    crouched_ = false;
    prone_ = false;
    impl_->updateSkeleton(AnimationId::Idle, 0.0f);
}

void LowPolyCharacterModel::update(GLFWwindow* window, float dt) {
    const bool attackDown = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;
    if (alive_ && attackDown && !previousAttackDown_ && attackTimer_ < 0.0f) {
        attackTimer_ = 0.0f;
    }
    previousAttackDown_ = attackDown;

    if (!alive_) {
        deathTimer_ =
            std::min(constants::kCharacterDeathDuration, deathTimer_ + dt);
        animationPhase_ = 0.0f;
        impl_->updateSkeleton(AnimationId::Death, deathTimer_);
        return;
    }

    prone_ = glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS;
    crouched_ = !prone_ && glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS;

    Vec3 direction{};
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        direction.z -= 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        direction.z += 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        direction.x -= 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        direction.x += 1.0f;
    }

    const bool manuallyControlled = ThreeDUtils::length(direction) > 0.0f;
    const bool running =
        manuallyControlled &&
        !crouched_ &&
        !prone_ &&
        (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
         glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);
    if (manuallyControlled) {
        direction = ThreeDUtils::normalize(direction);
    } else if (crouched_ || prone_) {
        direction = {};
    } else {
        direction = {0.0f, 0.0f, patrolDirection_};
    }

    const float stanceSpeedMultiplier =
        prone_ ? constants::kCharacterProneSpeedMultiplier
                : (crouched_ ? constants::kCharacterCrouchSpeedMultiplier
                             : 1.0f);
    const float moveSpeed =
        constants::kCharacterMoveSpeed *
        (running ? constants::kCharacterRunSpeedMultiplier
                 : stanceSpeedMultiplier);
    position_ = position_ + direction * (moveSpeed * dt);
    position_.x = std::clamp(position_.x, constants::kCharacterPathMinX,
                             constants::kCharacterPathMaxX);
    position_.z = std::clamp(position_.z, constants::kCharacterPathMinZ,
                             constants::kCharacterPathMaxZ);

    if (manuallyControlled) {
        yawDegrees_ =
            std::atan2(direction.x, direction.z) * 180.0f / constants::kPi;
        if (std::abs(direction.z) > 0.1f) {
            patrolDirection_ = direction.z > 0.0f ? 1.0f : -1.0f;
        }
    } else if (position_.z >= constants::kCharacterPathMaxZ) {
        position_.z = constants::kCharacterPathMaxZ;
        patrolDirection_ = -1.0f;
        yawDegrees_ = 180.0f;
    } else if (position_.z <= constants::kCharacterPathMinZ) {
        position_.z = constants::kCharacterPathMinZ;
        patrolDirection_ = 1.0f;
        yawDegrees_ = 0.0f;
    }

    const bool animatingMove =
        manuallyControlled || (!crouched_ && !prone_ && attackTimer_ < 0.0f);
    const float stanceCycleMultiplier =
        prone_ ? constants::kCharacterProneCycleMultiplier
                : (crouched_ ? constants::kCharacterCrouchCycleMultiplier
                             : 1.0f);
    const float cycleSpeed =
        running
            ? constants::kCharacterWalkCycleSpeed *
                  constants::kCharacterRunCycleMultiplier
            : (animatingMove ? constants::kCharacterWalkCycleSpeed *
                                   stanceCycleMultiplier
                             : 1.5f * stanceCycleMultiplier);
    animationPhase_ =
        std::fmod(animationPhase_ + dt * cycleSpeed / (2.0f * constants::kPi),
                  1.0f);

    if (attackTimer_ >= 0.0f) {
        attackTimer_ += dt;
        const float attackDuration =
            impl_->animationDuration(AnimationId::Attack);
        if (attackTimer_ < attackDuration) {
            impl_->updateSkeleton(AnimationId::Attack, attackTimer_);
            return;
        }
        attackTimer_ = -1.0f;
    }

    const AnimationId animation =
        prone_ ? (animatingMove ? AnimationId::ProneWalk
                                : AnimationId::ProneIdle)
               : (crouched_
                      ? (animatingMove ? AnimationId::CrouchWalk
                                       : AnimationId::CrouchIdle)
                      : (running
                             ? AnimationId::Run
                             : (animatingMove ? AnimationId::Walk
                                               : AnimationId::Idle)));
    impl_->updateSkeleton(animation,
                          animationPhase_ * impl_->animationDuration(animation));
}

void LowPolyCharacterModel::render() const {
    glPushMatrix();
    glTranslatef(position_.x, position_.y, position_.z);
    glRotatef(yawDegrees_, 0.0f, 1.0f, 0.0f);
    glScalef(constants::kCharacterScale, constants::kCharacterScale,
             constants::kCharacterScale);

    drawBoundPart(impl_->torso, impl_->bones[kTorsoBone], {},
                  {1.05f, 1.35f, 0.65f});

    drawBoundPart(impl_->upperLeg, impl_->bones[kLeftUpperLegBone],
                  {0.0f, -0.40f, 0.0f}, {0.38f, 0.95f, 0.38f});
    drawBoundPart(impl_->lowerLeg, impl_->bones[kLeftLowerLegBone],
                  {0.0f, -0.30f, 0.03f}, {0.34f, 0.70f, 0.34f});
    drawBoundPart(impl_->upperLeg, impl_->bones[kRightUpperLegBone],
                  {0.0f, -0.40f, 0.0f}, {0.38f, 0.95f, 0.38f});
    drawBoundPart(impl_->lowerLeg, impl_->bones[kRightLowerLegBone],
                  {0.0f, -0.30f, -0.03f}, {0.34f, 0.70f, 0.34f});

    drawBoundPart(impl_->upperArm, impl_->bones[kLeftUpperArmBone],
                  {0.0f, -0.40f, 0.0f}, {0.28f, 1.00f, 0.28f});
    drawBoundPart(impl_->lowerArm, impl_->bones[kLeftLowerArmBone],
                  {0.0f, -0.30f, 0.02f}, {0.24f, 0.92f, 0.24f});
    drawBoundPart(impl_->upperArm, impl_->bones[kRightUpperArmBone],
                  {0.0f, -0.40f, 0.0f}, {0.28f, 1.00f, 0.28f});
    drawBoundPart(impl_->lowerArm, impl_->bones[kRightLowerArmBone],
                  {0.0f, -0.30f, -0.02f}, {0.24f, 0.92f, 0.24f});

    drawBoundPart(impl_->head, impl_->bones[kHeadBone], {},
                  {1.0f, 1.0f, 1.0f});
    drawBoundPart(impl_->hair, impl_->bones[kHeadBone],
                  {0.0f, 0.18f, -0.08f}, {1.08f, 0.48f, 1.08f});

    drawBoundPart(impl_->eyeWhite, impl_->bones[kHeadBone],
                  {-0.15f, -0.04f, 0.52f}, {0.24f, 0.20f, 1.0f});
    drawBoundPart(impl_->eyeWhite, impl_->bones[kHeadBone],
                  {0.15f, -0.04f, 0.52f}, {0.24f, 0.20f, 1.0f});
    drawBoundPart(impl_->pupil, impl_->bones[kHeadBone],
                  {-0.15f, -0.04f, 0.57f}, {0.10f, 0.12f, 1.0f});
    drawBoundPart(impl_->pupil, impl_->bones[kHeadBone],
                  {0.15f, -0.04f, 0.57f}, {0.10f, 0.12f, 1.0f});
    drawBoundPart(impl_->mouth, impl_->bones[kHeadBone],
                  {0.0f, -0.18f, 0.54f}, {0.20f, 0.07f, 1.0f});

    glPopMatrix();
}

void LowPolyCharacterModel::renderHealthBar() const {
    if (!alive_ && deathTimer_ >= constants::kCharacterDeathDuration) {
        return;
    }

    const float healthRatio = std::clamp(
        static_cast<float>(hp_) / static_cast<float>(constants::kCharacterMaxHp),
        0.0f, 1.0f);
    const Color fillColor =
        healthRatio > 0.55f
            ? constants::kHealthBarFill
            : (healthRatio > 0.25f ? constants::kHealthBarMid
                                   : constants::kHealthBarLow);
    const float barScale = 0.5f;
    const float fullWidth = 1.14f * barScale;
    const float fillWidth = fullWidth * healthRatio;
    const float x = position_.x;
    const float y = position_.y + constants::kCharacterHealthBarHeight;
    const float z = position_.z + 0.06f;

    ThreeDUtils::drawCube({x, y, z},
                          {1.32f * barScale, 0.22f * barScale,
                           0.08f * barScale},
                          constants::kInkColor);
    ThreeDUtils::drawCube({x, y, z + 0.01f},
                          {fullWidth, 0.11f * barScale, 0.09f * barScale},
                          constants::kHealthBarBack);
    if (fillWidth > 0.01f) {
        ThreeDUtils::drawCube(
            {x - fullWidth * 0.5f + fillWidth * 0.5f, y, z + 0.02f},
            {fillWidth, 0.11f * barScale, 0.10f * barScale}, fillColor);
    }

    for (int i = 1; i < 5; ++i) {
        ThreeDUtils::drawCube(
            {x - fullWidth * 0.5f +
                 fullWidth * 0.2f * static_cast<float>(i),
             y, z + 0.03f},
            {0.025f * barScale, 0.13f * barScale, 0.11f * barScale},
            constants::kInkColor);
    }
}

bool LowPolyCharacterModel::segmentHit(const Vec3& start, const Vec3& end,
                                       float& hitT) const {
    if (!alive_) {
        return false;
    }

    const Vec3 boundsMin{
        position_.x - constants::kCharacterHitHalfWidth,
        position_.y + constants::kCharacterHitBottom,
        position_.z - constants::kCharacterHitHalfDepth};
    const Vec3 boundsMax{
        position_.x + constants::kCharacterHitHalfWidth,
        position_.y + constants::kCharacterHitTop,
        position_.z + constants::kCharacterHitHalfDepth};
    return segmentIntersectsAabb(start, end, boundsMin, boundsMax, hitT);
}

void LowPolyCharacterModel::applyPistolDamage(const Vec3& hitPosition) {
    if (!alive_) {
        return;
    }

    hp_ = std::max(0, hp_ - pistolDamageForZone(hitZoneForPoint(hitPosition)));
    if (hp_ <= 0) {
        kill();
    }
}

const Vec3& LowPolyCharacterModel::position() const {
    return position_;
}

bool LowPolyCharacterModel::alive() const {
    return alive_;
}

bool LowPolyCharacterModel::segmentIntersectsAabb(const Vec3& start,
                                                  const Vec3& end,
                                                  const Vec3& boundsMin,
                                                  const Vec3& boundsMax,
                                                  float& hitT) {
    const Vec3 delta = end - start;
    float tMin = 0.0f;
    float tMax = 1.0f;

    const auto clipAxis = [&](float startCoord, float deltaCoord, float minCoord,
                              float maxCoord) {
        if (std::abs(deltaCoord) <= 0.0001f) {
            return startCoord >= minCoord && startCoord <= maxCoord;
        }

        float enter = (minCoord - startCoord) / deltaCoord;
        float exit = (maxCoord - startCoord) / deltaCoord;
        if (enter > exit) {
            std::swap(enter, exit);
        }

        tMin = std::max(tMin, enter);
        tMax = std::min(tMax, exit);
        return tMin <= tMax;
    };

    if (!clipAxis(start.x, delta.x, boundsMin.x, boundsMax.x) ||
        !clipAxis(start.y, delta.y, boundsMin.y, boundsMax.y) ||
        !clipAxis(start.z, delta.z, boundsMin.z, boundsMax.z)) {
        return false;
    }

    hitT = tMin;
    return true;
}

void LowPolyCharacterModel::kill() {
    if (!alive_) {
        return;
    }

    hp_ = 0;
    alive_ = false;
    deathTimer_ = 0.0f;
    animationPhase_ = 0.0f;
    attackTimer_ = -1.0f;
    impl_->updateSkeleton(AnimationId::Death, 0.0f);
}

CharacterHitZone LowPolyCharacterModel::hitZoneForPoint(
    const Vec3& hitPosition) const {
    const float localHitY = hitPosition.y - position_.y;
    if (localHitY >= constants::kCharacterWaistHitTop) {
        return CharacterHitZone::Head;
    }
    if (localHitY >= constants::kCharacterLegHitTop) {
        return CharacterHitZone::Waist;
    }
    return CharacterHitZone::Legs;
}

int LowPolyCharacterModel::pistolDamageForZone(CharacterHitZone zone) {
    switch (zone) {
        case CharacterHitZone::Head:
            return constants::kPistolHeadDamage;
        case CharacterHitZone::Waist:
            return constants::kPistolWaistDamage;
        case CharacterHitZone::Legs:
            return constants::kPistolLegDamage;
    }
    return 0;
}

}  // namespace pixel_world
