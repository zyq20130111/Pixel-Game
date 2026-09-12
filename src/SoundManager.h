#pragma once

#include "types.h"

#include <cstdint>
#include <memory>
#include <string>

namespace pixel_world {

class SoundManager final {
public:
    using SoundId = std::uint32_t;
    static constexpr SoundId kInvalidSoundId = 0;

    SoundManager();
    ~SoundManager();

    SoundManager(const SoundManager&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;
    SoundManager(SoundManager&&) = delete;
    SoundManager& operator=(SoundManager&&) = delete;

    bool initialize();
    bool ready() const;
    void update();

    SoundId play2D(const std::string& fileName, float volume = 1.0f,
                   bool looping = false);
    SoundId play3D(const std::string& fileName, const Vec3& position,
                   float volume = 1.0f, bool looping = false);

    void stop(SoundId soundId);
    void stopAll();
    bool setPosition(SoundId soundId, const Vec3& position);
    void setListener(const Vec3& position, const Vec3& forward,
                     const Vec3& up);

private:
    struct Impl;

    SoundId playInternal(const std::string& fileName, bool spatialized,
                         const Vec3& position, float volume, bool looping);

    std::unique_ptr<Impl> impl_;
};

}  // namespace pixel_world
