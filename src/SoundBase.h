#pragma once

#include "types.h"

namespace pixel_world {

class SoundBase {
public:
    virtual ~SoundBase();

    SoundBase(const SoundBase&) = delete;
    SoundBase& operator=(const SoundBase&) = delete;
    SoundBase(SoundBase&&) = delete;
    SoundBase& operator=(SoundBase&&) = delete;

    virtual bool initialize() = 0;
    virtual void play() = 0;
    virtual void stop() = 0;
    virtual bool ready() const = 0;
    virtual void setPosition(const Vec3& position) = 0;
    virtual void setListener(const Vec3& position, const Vec3& forward,
                             const Vec3& up) = 0;

protected:
    SoundBase() = default;
};

}  // namespace pixel_world
