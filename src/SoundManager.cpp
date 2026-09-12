#include "SoundManager.h"

#include <miniaudio.h>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <memory>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#elif defined(__linux__)
#include <limits.h>
#include <unistd.h>
#endif

namespace pixel_world {
namespace {

constexpr ma_uint32 kListenerIndex = 0;
constexpr float kDefaultMinDistance = 4.5f;
constexpr float kDefaultMaxDistance = 32.0f;

std::filesystem::path executableDirectory() {
#ifdef _WIN32
    std::wstring modulePath(32768, L'\0');
    const DWORD length = GetModuleFileNameW(
        nullptr, modulePath.data(), static_cast<DWORD>(modulePath.size()));
    if (length > 0 && length < modulePath.size()) {
        modulePath.resize(length);
        return std::filesystem::path(modulePath).parent_path();
    }
#elif defined(__linux__)
    char modulePath[PATH_MAX] = {};
    const ssize_t length =
        readlink("/proc/self/exe", modulePath, sizeof(modulePath) - 1);
    if (length > 0) {
        modulePath[length] = '\0';
        return std::filesystem::path(modulePath).parent_path();
    }
#endif
    return {};
}

std::filesystem::path findSoundFile(const std::string& fileName) {
    const std::filesystem::path relativePath(fileName);
    if (relativePath.empty() || relativePath.is_absolute() ||
        relativePath.has_root_name() || relativePath.has_root_directory()) {
        return {};
    }
    for (const std::filesystem::path& component : relativePath) {
        if (component == "..") {
            return {};
        }
    }

    std::vector<std::filesystem::path> roots;
    const std::filesystem::path currentDirectory =
        std::filesystem::current_path();
    const std::filesystem::path moduleDirectory = executableDirectory();
    roots.push_back(currentDirectory / "res" / "sound");
    if (!moduleDirectory.empty()) {
        roots.push_back(moduleDirectory / "res" / "sound");
        roots.push_back(moduleDirectory / ".." / "res" / "sound");
        roots.push_back(moduleDirectory / ".." / ".." / "res" / "sound");
    }

    for (const std::filesystem::path& root : roots) {
        const std::filesystem::path candidate =
            (root / relativePath).lexically_normal();
        std::error_code error;
        if (std::filesystem::is_regular_file(candidate, error)) {
            return candidate;
        }
    }
    return {};
}

}  // namespace

struct SoundManager::Impl {
    struct ActiveSound {
        SoundId id{kInvalidSoundId};
        ma_sound sound{};
        bool initialized{false};
        bool spatialized{false};
    };

    ma_engine engine{};
    std::vector<std::unique_ptr<ActiveSound>> activeSounds;
    SoundId nextSoundId{1};
    bool engineInitialized{false};
};

SoundManager::SoundManager() : impl_(std::make_unique<Impl>()) {}

SoundManager::~SoundManager() {
    stopAll();
    if (impl_->engineInitialized) {
        ma_engine_uninit(&impl_->engine);
    }
}

bool SoundManager::initialize() {
    if (ready()) {
        return true;
    }

    const ma_result result = ma_engine_init(nullptr, &impl_->engine);
    if (result != MA_SUCCESS) {
        std::cerr << "Failed to initialize sound manager. Error: " << result
                  << '\n';
        return false;
    }

    impl_->engineInitialized = true;
    ma_engine_listener_set_position(&impl_->engine, kListenerIndex, 0.0f,
                                    0.0f, 0.0f);
    ma_engine_listener_set_direction(&impl_->engine, kListenerIndex, 0.0f,
                                     0.0f, -1.0f);
    ma_engine_listener_set_world_up(&impl_->engine, kListenerIndex, 0.0f,
                                    1.0f, 0.0f);
    return true;
}

bool SoundManager::ready() const {
    return impl_->engineInitialized;
}

void SoundManager::update() {
    if (!ready()) {
        return;
    }

    for (auto iterator = impl_->activeSounds.begin();
         iterator != impl_->activeSounds.end();) {
        const Impl::ActiveSound& active = **iterator;
        if (!active.initialized ||
            (!ma_sound_is_playing(&active.sound) &&
             ma_sound_at_end(&active.sound))) {
            if (active.initialized) {
                ma_sound_uninit(&(*iterator)->sound);
            }
            iterator = impl_->activeSounds.erase(iterator);
        } else {
            ++iterator;
        }
    }
}

SoundManager::SoundId SoundManager::play2D(const std::string& fileName,
                                           float volume, bool looping) {
    return playInternal(fileName, false, {}, volume, looping);
}

SoundManager::SoundId SoundManager::play3D(const std::string& fileName,
                                           const Vec3& position, float volume,
                                           bool looping) {
    return playInternal(fileName, true, position, volume, looping);
}

SoundManager::SoundId SoundManager::playInternal(
    const std::string& fileName, bool spatialized, const Vec3& position,
    float volume, bool looping) {
    if (!ready()) {
        return kInvalidSoundId;
    }

    update();
    const std::filesystem::path path = findSoundFile(fileName);
    if (path.empty()) {
        std::cerr << "Sound file not found in res/sound: " << fileName << '\n';
        return kInvalidSoundId;
    }

    auto active = std::make_unique<Impl::ActiveSound>();
    active->id = impl_->nextSoundId++;
    if (active->id == kInvalidSoundId) {
        active->id = impl_->nextSoundId++;
    }

    const ma_uint32 flags = MA_SOUND_FLAG_DECODE;
    ma_result result = MA_ERROR;
#ifdef _WIN32
    const std::wstring widePath = path.wstring();
    result = ma_sound_init_from_file_w(
        &impl_->engine, widePath.c_str(), flags, nullptr, nullptr,
        &active->sound);
#else
    const std::string narrowPath = path.string();
    result = ma_sound_init_from_file(&impl_->engine, narrowPath.c_str(), flags,
                                     nullptr, nullptr, &active->sound);
#endif
    if (result != MA_SUCCESS) {
        std::cerr << "Failed to load sound file '" << fileName
                  << "'. Error: " << result << '\n';
        return kInvalidSoundId;
    }
    active->initialized = true;

    active->spatialized = spatialized;
    ma_sound_set_volume(&active->sound, std::max(0.0f, volume));
    ma_sound_set_looping(&active->sound, looping ? MA_TRUE : MA_FALSE);
    ma_sound_set_spatialization_enabled(
        &active->sound, spatialized ? MA_TRUE : MA_FALSE);
    if (spatialized) {
        ma_sound_set_positioning(&active->sound, ma_positioning_absolute);
        ma_sound_set_attenuation_model(&active->sound,
                                       ma_attenuation_model_inverse);
        ma_sound_set_rolloff(&active->sound, 1.0f);
        ma_sound_set_min_distance(&active->sound, kDefaultMinDistance);
        ma_sound_set_max_distance(&active->sound, kDefaultMaxDistance);
        ma_sound_set_pinned_listener_index(&active->sound, kListenerIndex);
        ma_sound_set_position(&active->sound, position.x, position.y,
                              position.z);
    }

    result = ma_sound_start(&active->sound);
    if (result != MA_SUCCESS) {
        std::cerr << "Failed to start sound file '" << fileName
                  << "'. Error: " << result << '\n';
        ma_sound_uninit(&active->sound);
        return kInvalidSoundId;
    }

    const SoundId soundId = active->id;
    impl_->activeSounds.push_back(std::move(active));
    return soundId;
}

void SoundManager::stop(SoundId soundId) {
    for (auto iterator = impl_->activeSounds.begin();
         iterator != impl_->activeSounds.end(); ++iterator) {
        if ((*iterator)->id != soundId) {
            continue;
        }
        ma_sound_stop(&(*iterator)->sound);
        ma_sound_uninit(&(*iterator)->sound);
        impl_->activeSounds.erase(iterator);
        return;
    }
}

void SoundManager::stopAll() {
    for (const std::unique_ptr<Impl::ActiveSound>& active :
         impl_->activeSounds) {
        if (active->initialized) {
            ma_sound_stop(&active->sound);
            ma_sound_uninit(&active->sound);
        }
    }
    impl_->activeSounds.clear();
}

bool SoundManager::setPosition(SoundId soundId, const Vec3& position) {
    for (const std::unique_ptr<Impl::ActiveSound>& active :
         impl_->activeSounds) {
        if (active->id == soundId && active->spatialized) {
            ma_sound_set_position(&active->sound, position.x, position.y,
                                  position.z);
            return true;
        }
    }
    return false;
}

void SoundManager::setListener(const Vec3& position, const Vec3& forward,
                               const Vec3& up) {
    if (!ready()) {
        return;
    }

    ma_engine_listener_set_position(&impl_->engine, kListenerIndex,
                                    position.x, position.y, position.z);
    ma_engine_listener_set_direction(&impl_->engine, kListenerIndex,
                                     forward.x, forward.y, forward.z);
    ma_engine_listener_set_world_up(&impl_->engine, kListenerIndex, up.x, up.y,
                                    up.z);
}

}  // namespace pixel_world
