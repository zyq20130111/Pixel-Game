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
constexpr float kSoundDurationSeconds = 0.96f;
constexpr float kPi = 3.14159265358979323846f;
constexpr float kTau = kPi * 2.0f;
constexpr ma_uint32 kListenerIndex = 0;

float smoothStep(float value) {
    const float t = std::clamp(value, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

float nextNoise(std::uint32_t& state) {
    state = state * 1664525u + 1013904223u;
    const float normalized =
        static_cast<float>((state >> 8u) & 0x00ffffffu) /
        static_cast<float>(0x01000000u);
    return normalized * 2.0f - 1.0f;
}

struct Biquad {
    float b0{0.0f};
    float b1{0.0f};
    float b2{0.0f};
    float a1{0.0f};
    float a2{0.0f};
    float x1{0.0f};
    float x2{0.0f};
    float y1{0.0f};
    float y2{0.0f};

    void configureBandPass(float frequency, float quality) {
        const float angularFrequency =
            kTau * frequency / static_cast<float>(kSampleRate);
        const float sine = std::sin(angularFrequency);
        const float alpha = sine / (2.0f * quality);
        const float a0 = 1.0f + alpha;

        b0 = alpha / a0;
        b1 = 0.0f;
        b2 = -alpha / a0;
        a1 = -2.0f * std::cos(angularFrequency) / a0;
        a2 = (1.0f - alpha) / a0;
    }

    float process(float input) {
        const float output =
            b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        x2 = x1;
        x1 = input;
        y2 = y1;
        y1 = output;
        return output;
    }
};

struct OnePoleLowPass {
    float state{0.0f};
    float coefficient{0.0f};

    void configure(float cutoff) {
        coefficient = std::exp(
            -kTau * cutoff / static_cast<float>(kSampleRate));
    }

    float process(float input) {
        state = (1.0f - coefficient) * input + coefficient * state;
        return state;
    }
};

float normalizedSaturation(float input, float drive) {
    return std::tanh(input * drive) / std::tanh(drive);
}

std::vector<float> makeAttackSamples() {
    const ma_uint64 frameCount = static_cast<ma_uint64>(
        static_cast<float>(kSampleRate) * kSoundDurationSeconds);
    std::vector<float> samples(static_cast<std::size_t>(
        frameCount * static_cast<ma_uint64>(kChannelCount)));

    // The body, rasp and hiss are deliberately separate.  A believable
    // creature voice is usually a small stack of unlike layers, rather than
    // one oscillator with a low-pass filter.
    Biquad chestFormant;
    Biquad throatFormant;
    Biquad mouthFormant;
    Biquad raspFormant;
    Biquad hissFormant;
    Biquad attackFormant;
    chestFormant.configureBandPass(185.0f, 1.25f);
    throatFormant.configureBandPass(390.0f, 1.55f);
    mouthFormant.configureBandPass(860.0f, 2.0f);
    raspFormant.configureBandPass(1720.0f, 1.25f);
    hissFormant.configureBandPass(3300.0f, 1.45f);
    attackFormant.configureBandPass(2100.0f, 1.15f);

    OnePoleLowPass lowNoiseFilter;
    OnePoleLowPass midNoiseFilter;
    OnePoleLowPass raspNoiseFilter;
    OnePoleLowPass hissNoiseFilter;
    lowNoiseFilter.configure(95.0f);
    midNoiseFilter.configure(480.0f);
    raspNoiseFilter.configure(1050.0f);
    hissNoiseFilter.configure(2650.0f);

    std::uint32_t randomState = 0x6d2b79f5u;
    std::uint32_t pulseRandomState = 0x1f123bb5u;
    float lowNoise = 0.0f;
    float midNoise = 0.0f;
    float raspNoise = 0.0f;
    float hissNoise = 0.0f;
    float pitchJitter = 0.0f;
    float amplitudeJitter = 0.0f;
    float vocalPhase = 0.0f;
    float subharmonicPhase = 0.0f;
    float detunedPhase = 0.0f;
    std::uint32_t pulseIndex = 0;
    float pulseWidth = 0.21f;

    for (ma_uint64 frame = 0; frame < frameCount; ++frame) {
        const float time =
            static_cast<float>(frame) / static_cast<float>(kSampleRate);
        const float progress = time / kSoundDurationSeconds;

        const float whiteNoise = nextNoise(randomState);
        lowNoise = lowNoiseFilter.process(whiteNoise);
        midNoise = midNoiseFilter.process(whiteNoise);
        raspNoise = whiteNoise - raspNoiseFilter.process(whiteNoise);
        hissNoise = whiteNoise - hissNoiseFilter.process(whiteNoise);
        pitchJitter += (lowNoise - pitchJitter) * 0.025f;
        amplitudeJitter += (midNoise - amplitudeJitter) * 0.06f;

        const float onset = smoothStep(time / 0.012f);
        const float release =
            1.0f - smoothStep((time - 0.68f) / 0.28f);
        const float roarEnvelope = onset * release;
        const float inhaleEnvelope =
            smoothStep(time / 0.004f) *
            (1.0f - smoothStep((time - 0.085f) / 0.105f));
        const float raspEnvelope =
            smoothStep(time / 0.006f) *
            (1.0f - smoothStep((time - 0.31f) / 0.30f));
        const float attackEnvelope =
            std::exp(-time * 105.0f) * smoothStep(time / 0.0015f);

        // A falling pitch and a little cycle-to-cycle instability are more
        // characteristic of a strained growl than a stable musical note.
        const float frequency =
            143.0f - 70.0f * smoothStep(progress) +
            5.5f * std::sin(kTau * 4.2f * time) +
            10.0f * pitchJitter;
        const float phaseIncrement =
            std::max(38.0f, frequency) /
            static_cast<float>(kSampleRate);
        const float previousPhase = vocalPhase;
        vocalPhase += phaseIncrement;
        subharmonicPhase +=
            (frequency * 0.5f) / static_cast<float>(kSampleRate);
        detunedPhase +=
            (frequency * 0.985f) / static_cast<float>(kSampleRate);
        vocalPhase -= std::floor(vocalPhase);
        subharmonicPhase -= std::floor(subharmonicPhase);
        detunedPhase -= std::floor(detunedPhase);

        if (vocalPhase < previousPhase) {
            ++pulseIndex;
            const float randomPulse = nextNoise(pulseRandomState);
            pulseWidth = std::clamp(0.18f + 0.075f * randomPulse,
                                    0.105f, 0.275f);
        }

        // Long closed phases and alternating pulse strength approximate
        // vocal fry / creaky phonation.  The alternating cycles keep the
        // voice from collapsing into a perfectly periodic synth tone.
        const float openPhase = vocalPhase / pulseWidth;
        const float glottalPulse =
            vocalPhase < pulseWidth
                ? std::pow(std::max(0.0f, std::sin(kPi * openPhase)), 0.42f)
                : -0.08f *
                      std::sin(kPi * (vocalPhase - pulseWidth) /
                               (1.0f - pulseWidth));
        const float pulseAccent =
            pulseIndex % 3u == 0u ? 0.68f
                                  : (pulseIndex % 5u == 2u ? 1.16f : 0.94f);
        const float fryPulse = glottalPulse * pulseAccent;

        const float subharmonic =
            std::sin(kTau * subharmonicPhase) +
            0.38f * std::sin(kTau * subharmonicPhase * 2.0f);
        const float detunedVoice =
            std::sin(kTau * detunedPhase) +
            0.28f * std::sin(kTau * detunedPhase * 2.0f);
        const float larynxNoise =
            0.32f * lowNoise + 0.18f * midNoise +
            0.12f * std::sin(kTau * 47.0f * time);
        const float throatDrive =
            1.18f * fryPulse + 0.28f * subharmonic +
            0.14f * detunedVoice + 0.20f * larynxNoise;

        const float chest = chestFormant.process(throatDrive);
        const float throat = throatFormant.process(throatDrive);
        const float mouth = mouthFormant.process(throatDrive);
        const float formantRasp = raspFormant.process(
            throatDrive + 0.42f * raspNoise);
        const float hiss = hissFormant.process(hissNoise);

        const float cleanVoice =
            2.8f * chest + 2.0f * throat + 1.24f * mouth;
        const float distortedVoice = normalizedSaturation(
            2.25f * cleanVoice + 1.8f * formantRasp, 2.6f);
        const float body =
            0.62f * normalizedSaturation(cleanVoice, 1.75f) +
            0.52f * distortedVoice;

        const float breath =
            raspEnvelope *
            (0.40f * raspNoise + 0.78f * formantRasp +
             0.32f * hiss);
        const float inhale =
            inhaleEnvelope *
            (0.34f * raspNoise + 0.76f * hiss +
             0.22f * std::sin(kTau * 92.0f * time));
        const float attackNoise =
            attackEnvelope *
            (1.15f * attackFormant.process(whiteNoise) +
             0.38f * raspNoise);
        const float growlMod =
            0.84f + 0.16f * std::sin(kTau * 5.4f * time + 0.7f) +
            0.08f * amplitudeJitter;
        const float sample = std::clamp(
            roarEnvelope * growlMod * (0.72f * body + 0.32f * breath) +
                0.38f * inhale + 0.42f * attackNoise,
            -0.95f, 0.95f);

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
