#include "ZombieAttackSound.h"

#include <miniaudio.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

namespace pixel_world {
namespace {

constexpr ma_uint32 kSampleRate = 48000;
constexpr ma_uint32 kChannelCount = 1;
constexpr float kSoundDurationSeconds = 0.72f;
constexpr float kPi = 3.14159265358979323846f;
constexpr float kTau = kPi * 2.0f;
constexpr ma_uint32 kListenerIndex = 0;

float nextNoise(std::uint32_t& state) {
    state = state * 1664525u + 1013904223u;
    const float normalized =
        static_cast<float>((state >> 8u) & 0x00ffffffu) /
        static_cast<float>(0x01000000u);
    return normalized * 2.0f - 1.0f;
}

std::vector<float> makeAttackSamples() {
    const ma_uint64 frameCount = static_cast<ma_uint64>(
        static_cast<float>(kSampleRate) * kSoundDurationSeconds);
    std::vector<float> samples(static_cast<std::size_t>(
        frameCount * static_cast<ma_uint64>(kChannelCount)));

    std::uint32_t randomState = 0x6d2b79f5u;
    float rumbleNoise = 0.0f;
    for (ma_uint64 frame = 0; frame < frameCount; ++frame) {
        const float time =
            static_cast<float>(frame) / static_cast<float>(kSampleRate);
        const float progress = time / kSoundDurationSeconds;
        const float attackEnvelope =
            std::min(1.0f, time / 0.035f) *
            std::min(1.0f, (kSoundDurationSeconds - time) / 0.16f);
        const float growlEnvelope =
            std::min(1.0f, time / 0.11f) *
            std::min(1.0f, (kSoundDurationSeconds - time) / 0.24f);

        const float frequency =
            185.0f - 96.0f * progress + 11.0f * std::sin(time * 19.0f);
        const float growl =
            std::sin(kTau * frequency * time) +
            0.42f * std::sin(kTau * frequency * 0.51f * time + 0.8f) +
            0.18f * std::sin(kTau * frequency * 1.93f * time);

        const float whiteNoise = nextNoise(randomState);
        rumbleNoise = rumbleNoise * 0.965f + whiteNoise * 0.035f;
        const float hiss = whiteNoise - rumbleNoise;
        const float sample = std::clamp(
            growlEnvelope * (0.31f * growl + 0.34f * rumbleNoise) +
                attackEnvelope * 0.18f * hiss,
            -0.92f, 0.92f);

        samples[static_cast<std::size_t>(frame * kChannelCount)] =
            sample;
    }

    return samples;
}

}  // namespace

struct ZombieAttackSound::Impl {
    ma_engine engine{};
    ma_audio_buffer audioBuffer{};
    ma_sound sound{};
    std::vector<float> samples;
    bool engineInitialized{false};
    bool audioBufferInitialized{false};
    bool soundInitialized{false};
};

ZombieAttackSound::ZombieAttackSound() : impl_(std::make_unique<Impl>()) {}

ZombieAttackSound::~ZombieAttackSound() {
    if (impl_->soundInitialized) {
        ma_sound_uninit(&impl_->sound);
    }
    if (impl_->audioBufferInitialized) {
        ma_audio_buffer_uninit(&impl_->audioBuffer);
    }
    if (impl_->engineInitialized) {
        ma_engine_uninit(&impl_->engine);
    }
}

bool ZombieAttackSound::initialize() {
    if (ready()) {
        return true;
    }

    const ma_result engineResult = ma_engine_init(nullptr, &impl_->engine);
    if (engineResult != MA_SUCCESS) {
        std::cerr << "Failed to initialize audio engine for zombie attack "
                     "sound. Error: "
                  << engineResult << '\n';
        return false;
    }
    impl_->engineInitialized = true;
    ma_engine_listener_set_position(&impl_->engine, kListenerIndex, 0.0f,
                                    0.0f, 0.0f);
    ma_engine_listener_set_direction(&impl_->engine, kListenerIndex, 0.0f,
                                     0.0f, -1.0f);
    ma_engine_listener_set_world_up(&impl_->engine, kListenerIndex, 0.0f,
                                    1.0f, 0.0f);

    impl_->samples = makeAttackSamples();
    const ma_uint64 frameCount =
        static_cast<ma_uint64>(impl_->samples.size() / kChannelCount);
    ma_audio_buffer_config bufferConfig = ma_audio_buffer_config_init(
        ma_format_f32, kChannelCount, frameCount, impl_->samples.data(),
        nullptr);
    bufferConfig.sampleRate = kSampleRate;

    const ma_result bufferResult =
        ma_audio_buffer_init(&bufferConfig, &impl_->audioBuffer);
    if (bufferResult != MA_SUCCESS) {
        std::cerr << "Failed to initialize generated zombie attack sound "
                     "buffer. Error: "
                  << bufferResult << '\n';
        ma_engine_uninit(&impl_->engine);
        impl_->engineInitialized = false;
        return false;
    }
    impl_->audioBufferInitialized = true;

    const ma_result soundResult = ma_sound_init_from_data_source(
        &impl_->engine, &impl_->audioBuffer, 0, nullptr, &impl_->sound);
    if (soundResult != MA_SUCCESS) {
        std::cerr << "Failed to initialize zombie attack sound. Error: "
                  << soundResult << '\n';
        ma_audio_buffer_uninit(&impl_->audioBuffer);
        impl_->audioBufferInitialized = false;
        ma_engine_uninit(&impl_->engine);
        impl_->engineInitialized = false;
        return false;
    }

    impl_->soundInitialized = true;
    ma_sound_set_volume(&impl_->sound, 0.78f);
    ma_sound_set_looping(&impl_->sound, MA_FALSE);
    ma_sound_set_spatialization_enabled(&impl_->sound, MA_TRUE);
    ma_sound_set_positioning(&impl_->sound, ma_positioning_absolute);
    ma_sound_set_attenuation_model(&impl_->sound,
                                   ma_attenuation_model_inverse);
    ma_sound_set_rolloff(&impl_->sound, 1.0f);
    ma_sound_set_min_distance(&impl_->sound, 4.5f);
    ma_sound_set_max_distance(&impl_->sound, 32.0f);
    ma_sound_set_pinned_listener_index(&impl_->sound, kListenerIndex);
    return true;
}

void ZombieAttackSound::play() {
    if (!ready()) {
        return;
    }

    ma_sound_stop(&impl_->sound);
    ma_sound_seek_to_pcm_frame(&impl_->sound, 0);
    ma_sound_start(&impl_->sound);
}

void ZombieAttackSound::stop() {
    if (ready()) {
        ma_sound_stop(&impl_->sound);
        ma_sound_seek_to_pcm_frame(&impl_->sound, 0);
    }
}

bool ZombieAttackSound::ready() const {
    return impl_->engineInitialized && impl_->audioBufferInitialized &&
           impl_->soundInitialized;
}

void ZombieAttackSound::setPosition(const Vec3& position) {
    if (!ready()) {
        return;
    }

    ma_sound_set_position(&impl_->sound, position.x, position.y, position.z);
}

void ZombieAttackSound::setListener(const Vec3& position,
                                    const Vec3& forward, const Vec3& up) {
    if (!impl_->engineInitialized) {
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
