#pragma once

#include "SoundBase.h"

#include <memory>

namespace pixel_world {

class ZombieAttackSound final : public SoundBase {
public:
    ZombieAttackSound();
    ~ZombieAttackSound() override;

    bool initialize() override;
    void play() override;
    void stop() override;
    bool ready() const override;
    void setPosition(const Vec3& position) override;
    void setListener(const Vec3& position, const Vec3& forward,
                     const Vec3& up) override;

private:
    struct Impl;

    std::unique_ptr<Impl> impl_;
};

}  // namespace pixel_world
