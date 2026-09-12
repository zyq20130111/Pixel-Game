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
constexpr float kSoundDurationSeconds = 0.88f;
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

    // Keep the low growl dominant. The rasp and breath layers are accents,
    // otherwise the sound turns into a bright filtered-noise burst.
    Biquad chestFormant;
    Biquad throatFormant;
    Biquad mouthFormant;
    Biquad raspFormant;
    Biquad hissFormant;
    Biquad attackFormant;
    chestFormant.configureBandPass(145.0f, 0.9f);
    throatFormant.configureBandPass(325.0f, 1.05f);
    mouthFormant.configureBandPass(720.0f, 1.15f);
    raspFormant.configureBandPass(1450.0f, 0.9f);
    hissFormant.configureBandPass(2800.0f, 0.95f);
    attackFormant.configureBandPass(1900.0f, 0.8f);

    OnePoleLowPass lowNoiseFilter;
    OnePoleLowPass midNoiseFilter;
    OnePoleLowPass raspNoiseFilter;
    OnePoleLowPass hissNoiseFilter;
    lowNoiseFilter.configure(70.0f);
    midNoiseFilter.configure(360.0f);
    raspNoiseFilter.configure(900.0f);
    hissNoiseFilter.configure(2200.0f);

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
    float pulseWidth = 0.32f;
    float pulseAccent = 0.92f;

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

        const float onset = smoothStep(time / 0.018f);
        const float release = 1.0f - smoothStep((time - 0.57f) / 0.31f);
        const float roarEnvelope = onset * release;
        const float inhaleEnvelope =
            smoothStep(time / 0.003f) *
            (1.0f - smoothStep((time - 0.045f) / 0.11f));
        const float raspEnvelope =
            smoothStep(time / 0.012f) *
            (1.0f - smoothStep((time - 0.36f) / 0.27f));
        const float attackEnvelope =
            smoothStep(time / 0.0008f) *
            (1.0f - smoothStep((time - 0.018f) / 0.05f));

        // A falling pitch gives the attack a physical downward pull instead
        // of making it sound like a sustained synth note.
        const float frequency =
            158.0f - 82.0f * smoothStep(progress) +
            4.0f * std::sin(kTau * 3.8f * time) +
            8.0f * pitchJitter;
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
            pulseWidth = std::clamp(0.32f + 0.09f * randomPulse,
                                    0.20f, 0.43f);
            pulseAccent = std::clamp(
                0.90f + 0.12f * nextNoise(pulseRandomState), 0.74f, 1.08f);
        }

        // A rounded glottal pulse supplies the vocal character. The small
        // negative closed-phase tail adds roughness without harsh alias-like
        // edges.
        const float openPhase = vocalPhase / pulseWidth;
        const float glottalPulse =
            vocalPhase < pulseWidth
                ? std::pow(std::max(0.0f, std::sin(kPi * openPhase)), 0.62f)
                : -0.055f *
                      std::sin(kPi * (vocalPhase - pulseWidth) /
                               (1.0f - pulseWidth));
        const float fryPulse = glottalPulse * pulseAccent;

        const float subharmonic =
            std::sin(kTau * subharmonicPhase) +
            0.24f * std::sin(kTau * subharmonicPhase * 2.0f);
        const float detunedVoice =
            std::sin(kTau * detunedPhase) +
            0.18f * std::sin(kTau * detunedPhase * 2.0f);
        const float larynxNoise =
            0.42f * lowNoise + 0.14f * midNoise +
            0.08f * std::sin(kTau * 47.0f * time);
        const float throatDrive =
            1.42f * fryPulse + 0.34f * subharmonic +
            0.12f * detunedVoice + 0.18f * larynxNoise;

        const float chest = chestFormant.process(throatDrive);
        const float throat = throatFormant.process(throatDrive);
        const float mouth = mouthFormant.process(throatDrive);
        const float formantRasp = raspFormant.process(
            throatDrive + 0.42f * raspNoise);
        const float hiss = hissFormant.process(hissNoise);

        const float cleanVoice =
            3.8f * chest + 2.2f * throat + 0.95f * mouth;
        const float distortedVoice = normalizedSaturation(
            2.6f * cleanVoice + 1.1f * formantRasp, 1.8f);
        const float body =
            0.78f * normalizedSaturation(cleanVoice, 1.45f) +
            0.34f * distortedVoice;

        const float breath =
            raspEnvelope *
            (0.24f * raspNoise + 0.48f * formantRasp +
             0.16f * hiss);
        const float inhale =
            inhaleEnvelope *
            (0.26f * raspNoise + 0.46f * hiss +
             0.16f * std::sin(kTau * 92.0f * time));
        const float attackNoise =
            attackEnvelope *
            (0.68f * attackFormant.process(whiteNoise) +
             0.22f * raspNoise);
        const float growlMod =
            0.91f + 0.09f * std::sin(kTau * 4.7f * time + 0.7f) +
            0.05f * amplitudeJitter;
        const float sample = std::clamp(
            1.12f *
                    (roarEnvelope * growlMod * (0.88f * body + 0.20f * breath) +
                     0.22f * inhale + 0.24f * attackNoise),
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
