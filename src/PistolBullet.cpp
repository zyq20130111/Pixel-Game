#include "PistolBullet.h"

#include "game_constants.h"
#include "platform.h"
#include "ThreeDUtils.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace pixel_world {
namespace {

struct BulletVertex {
    GLfloat position[3];
    GLfloat color[3];
};

class BulletVertexBuffer {
public:
    void addTriangle(const Vec3& a, const Color& colorA, const Vec3& b,
                     const Color& colorB, const Vec3& c,
                     const Color& colorC) {
        vertices_.push_back(makeVertex(a, colorA));
        vertices_.push_back(makeVertex(b, colorB));
        vertices_.push_back(makeVertex(c, colorC));
    }

    void draw() const {
        if (vertices_.empty()) {
            return;
        }

        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        glVertexPointer(3, GL_FLOAT, sizeof(BulletVertex),
                        vertices_.data()->position);
        glColorPointer(3, GL_FLOAT, sizeof(BulletVertex),
                       vertices_.data()->color);
        glDrawArrays(GL_TRIANGLES, 0,
                     static_cast<GLsizei>(vertices_.size()));
        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
    }

private:
    static BulletVertex makeVertex(const Vec3& position,
                                   const Color& color) {
        return BulletVertex{{position.x, position.y, position.z},
                            {color.red, color.green, color.blue}};
    }

    std::vector<BulletVertex> vertices_;
};

BulletVertexBuffer makeBulletSphere() {
    BulletVertexBuffer buffer;
    constexpr int kSlices = 16;
    constexpr int kStacks = 8;
    constexpr float kRadius = 0.02f;

    const auto pointAt = [](float stackAngle, float sliceAngle) {
        const float ringRadius = std::sin(stackAngle) * kRadius;
        return Vec3{std::cos(sliceAngle) * ringRadius,
                    std::cos(stackAngle) * kRadius,
                    std::sin(sliceAngle) * ringRadius};
    };

    for (int stack = 0; stack < kStacks; ++stack) {
        const float stackAngleA =
            constants::kPi * static_cast<float>(stack) /
            static_cast<float>(kStacks);
        const float stackAngleB =
            constants::kPi * static_cast<float>(stack + 1) /
            static_cast<float>(kStacks);

        for (int slice = 0; slice < kSlices; ++slice) {
            const float sliceAngleA =
                2.0f * constants::kPi * static_cast<float>(slice) /
                static_cast<float>(kSlices);
            const float sliceAngleB =
                2.0f * constants::kPi * static_cast<float>(slice + 1) /
                static_cast<float>(kSlices);

            const Vec3 a = pointAt(stackAngleA, sliceAngleA);
            const Vec3 b = pointAt(stackAngleA, sliceAngleB);
            const Vec3 c = pointAt(stackAngleB, sliceAngleB);
            const Vec3 d = pointAt(stackAngleB, sliceAngleA);

            const float light =
                std::clamp(0.78f + 0.22f * std::cos(sliceAngleA) +
                               0.16f * std::cos(stackAngleA),
                           0.52f, 1.16f);
            const Color faceColor =
                ThreeDUtils::shade(constants::kBulletCore, light);
            const Color lowerColor =
                ThreeDUtils::shade(constants::kBulletTrail, 0.86f);

            if (stack == 0) {
                buffer.addTriangle(a, faceColor, c, faceColor, d, faceColor);
            } else if (stack == kStacks - 1) {
                buffer.addTriangle(a, faceColor, b, faceColor, c, faceColor);
            } else {
                buffer.addTriangle(a, faceColor, b, faceColor, c, lowerColor);
                buffer.addTriangle(a, faceColor, c, lowerColor, d, lowerColor);
            }
        }
    }

    return buffer;
}

const BulletVertexBuffer& bulletSphereMesh() {
    static const BulletVertexBuffer mesh = makeBulletSphere();
    return mesh;
}

}  // namespace

PistolBullet::PistolBullet(const Vec3& position, const Vec3& velocity,
                           float lifetime)
    : BulletBase(position, velocity, lifetime) {}

void PistolBullet::render() const {
    glPushMatrix();
    glTranslatef(position().x, position().y, position().z);
    bulletSphereMesh().draw();
    glPopMatrix();
}

}  // namespace pixel_world
