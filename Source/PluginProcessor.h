#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/FilterEngine.h"
#include "DSP/KillBandProcessor.h"
#include "DSP/EffectChain.h"
#include "DSP/ModulationProcessor.h"
#include "Parameters.h"

class VintageDualFilterAudioProcessor final : public juce::AudioProcessor
{
public:
    VintageDualFilterAudioProcessor();
    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 5; }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram(int) override;
    const juce::String getProgramName(int) override;
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    void saveUserPreset(int slot);
    void loadUserPreset(int slot);
    bool hasUserPreset(int slot) const;
    juce::AudioProcessorValueTreeState parameters;

private:
    FilterEngine::Settings readSettings(int) const;
    EffectChain::Settings readEffectSettings(int) const;
    std::array<FilterEngine, 2> filters;
    std::array<EffectChain, 2> effects;
    ModulationProcessor modulation;
    KillBandProcessor killBands;
    juce::AudioBuffer<float> parallelBuffer;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> parallelLatencyCompensation{4096};
    juce::dsp::Gain<float> inputGain, outputGain;
    int currentProgram = 0;
    float filterLatency = 0.f;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VintageDualFilterAudioProcessor)
};
