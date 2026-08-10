#pragma once

#include <juce_dsp/juce_dsp.h>

class ModulationProcessor
{
public:
    void prepare(double newSampleRate, int channels, int maximumBlockSize)
    {
        sampleRate = newSampleRate;
        dry.setSize(channels, maximumBlockSize, false, false, true);
        for (auto& filter : highShelf) filter.reset();
        reset();
    }

    void reset()
    {
        envelope = 0.f; vcaEnvelope = 0.f; lfoPhase = 0.f; pitchHz = 0.f;
        previousSample = 0.f; samplesSinceCrossing = 0;
    }

    void captureDry(const juce::AudioBuffer<float>& buffer)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            dry.copyFrom(ch, 0, buffer, ch, 0, buffer.getNumSamples());
    }

    void processInput(juce::AudioBuffer<float>& buffer, float driveDb, float shelfDb, float noisePercent)
    {
        const auto drive = juce::Decibels::decibelsToGain(driveDb);
        const auto noiseGain = noisePercent * 0.00035f;
        const auto shelf = juce::IIRCoefficients::makeHighShelf(sampleRate, 4500.0, 0.707,
            juce::Decibels::decibelsToGain(shelfDb));
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            highShelf[(size_t) ch].setCoefficients(shelf);
            auto* samples = buffer.getWritePointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                auto x = samples[i] + (random.nextFloat() * 2.f - 1.f) * noiseGain;
                if (driveDb > 0.001f) x = std::tanh(x * drive) / std::tanh(drive);
                samples[i] = highShelf[(size_t) ch].processSingleSampleRaw(x);
            }
        }
    }

    void analyse(const juce::AudioBuffer<float>& buffer, float attackMs, float decayMs,
                 float sustain, float releaseMs, float thresholdDb, float lfoRate, bool saw)
    {
        float peak = 0.f;
        if (buffer.getNumChannels() > 0)
        {
            const auto* samples = buffer.getReadPointer(0);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                peak = juce::jmax(peak, std::abs(samples[i]));
                ++samplesSinceCrossing;
                if (samples[i] >= 0.f && previousSample < 0.f && samplesSinceCrossing > 8)
                {
                    pitchHz = (float) sampleRate / (float) samplesSinceCrossing;
                    samplesSinceCrossing = 0;
                }
                previousSample = samples[i];
            }
        }
        const auto gate = juce::Decibels::gainToDecibels(peak, -100.f) >= thresholdDb;
        const auto target = gate ? (envelope < sustain ? 1.f : sustain) : 0.f;
        const auto timeMs = gate ? (target > envelope ? attackMs : decayMs) : releaseMs;
        const auto coefficient = std::exp(-(float) buffer.getNumSamples() / (juce::jmax(1.f, timeMs) * 0.001f * (float) sampleRate));
        envelope = target + coefficient * (envelope - target);

        const auto phaseAdvance = lfoRate * (float) buffer.getNumSamples() / (float) sampleRate;
        lfoPhase -= std::floor(lfoPhase);
        lfoValue = saw ? lfoPhase * 2.f - 1.f : std::sin(lfoPhase * juce::MathConstants<float>::twoPi);
        lfoPhase += phaseAdvance;
    }

    void processOutput(juce::AudioBuffer<float>& buffer, bool vcaEnabled, float vcaDriveDb,
                       float arAttackMs, float arReleaseMs, float arDepth, float amDepth,
                       float amValue, float globalMix)
    {
        const auto peak = buffer.getMagnitude(0, buffer.getNumSamples());
        const auto vcaTarget = peak > 0.02f ? 1.f : 0.f;
        const auto timeMs = vcaTarget > vcaEnvelope ? arAttackMs : arReleaseMs;
        const auto coefficient = std::exp(-(float) buffer.getNumSamples() / (juce::jmax(1.f, timeMs) * 0.001f * (float) sampleRate));
        vcaEnvelope = vcaTarget + coefficient * (vcaEnvelope - vcaTarget);
        const auto vcaGain = vcaEnabled ? juce::jmap(arDepth, 1.f, vcaEnvelope) : 1.f;
        const auto amGain = juce::jlimit(0.f, 1.f, 1.f - amDepth + amDepth * (amValue * 0.5f + 0.5f));
        const auto drive = juce::Decibels::decibelsToGain(vcaDriveDb);
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* samples = buffer.getWritePointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                auto wet = samples[i] * vcaGain * amGain;
                if (vcaDriveDb > 0.001f) wet = std::tanh(wet * drive) / std::tanh(drive);
                samples[i] = dry.getSample(ch, i) * (1.f - globalMix) + wet * globalMix;
            }
        }
    }

    float getEnvelope() const { return envelope; }
    float getLfo() const { return lfoValue; }
    float getPitchHz() const { return pitchHz; }

private:
    double sampleRate = 44100.0;
    juce::AudioBuffer<float> dry;
    std::array<juce::IIRFilter, 2> highShelf;
    juce::Random random;
    float envelope = 0.f, vcaEnvelope = 0.f, lfoPhase = 0.f, lfoValue = 0.f;
    float pitchHz = 0.f, previousSample = 0.f;
    int samplesSinceCrossing = 0;
};
