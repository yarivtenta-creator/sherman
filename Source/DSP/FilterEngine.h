#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>

class FilterEngine
{
public:
    enum class Model { clean, ladder, ms20, ota };
    enum class Mode { lowPass, highPass, bandPass };
    enum class LfoShape { sine, triangle, saw, square, sampleAndHold };
    enum class LfoTarget { cutoff, resonance, drive, mix };

    struct LfoSettings
    {
        float rate = 1.f;
        float depth = 0.f;
        LfoShape shape = LfoShape::sine;
        LfoTarget target = LfoTarget::cutoff;
    };

    struct Settings
    {
        bool enabled = true;
        Model model = Model::ladder;
        Mode mode = Mode::lowPass;
        int slopeIndex = 2;
        float cutoff = 1000.f;
        float resonance = 0.707f;
        float thd = 0.f;
        float mix = 1.f;
        std::array<LfoSettings, 2> lfo;
    };

    FilterEngine();
    void prepare(const juce::dsp::ProcessSpec&);
    void reset();
    void setSettings(const Settings&);
    void process(juce::AudioBuffer<float>&);
    float getLatencySamples() const { return oversampling.getLatencyInSamples(); }

private:
    float nextLfoValue(size_t index, int numSamples);
    void updateFilters(float cutoff, float resonance);
    void applyModelSaturation(juce::dsp::AudioBlock<float>, float driveAmount);

    std::array<juce::dsp::StateVariableTPTFilter<float>, 4> stages;
    juce::dsp::Oversampling<float> oversampling;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> dryDelay{4096};
    juce::AudioBuffer<float> dryBuffer;
    std::array<float, 2> phases{};
    std::array<float, 2> heldRandom{};
    std::array<float, 2> onePoleLowState{};
    std::array<float, 2> onePoleBandState{};
    float onePoleAlpha = 0.f;
    float bandLowAlpha = 0.f;
    float bandHighAlpha = 0.f;
    std::array<float, 4> heldModulation{};
    int samplesUntilControlUpdate = 0;
    juce::Random random;
    Settings settings;
    double sampleRate = 44100.0;
    int maximumBlockSize = 512;
};
