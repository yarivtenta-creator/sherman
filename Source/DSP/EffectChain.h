#pragma once

#include <juce_dsp/juce_dsp.h>

class EffectChain
{
public:
    enum class DistortionType { softClip, hardClip, diode };

    struct Settings
    {
        bool delayEnabled = false;
        float delayTimeMs = 320.f;
        float delayFeedback = 0.35f;
        float delayTone = 6500.f;
        float delayMix = 0.25f;

        bool reverbEnabled = false;
        float reverbSize = 0.55f;
        float reverbDamping = 0.45f;
        float reverbPreDelayMs = 18.f;
        float reverbWidth = 1.f;
        float reverbMix = 0.25f;

        bool distortionEnabled = false;
        DistortionType distortionType = DistortionType::softClip;
        float distortionDrive = 12.f;
        float distortionTone = 8000.f;
        float distortionMix = 1.f;
    };

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        maxBlockSize = (int) spec.maximumBlockSize;
        numChannels = (int) spec.numChannels;
        delayBuffer.setSize(numChannels, (int) std::ceil(sampleRate * 2.1), false, true, true);
        preDelayBuffer.setSize(numChannels, (int) std::ceil(sampleRate * 0.25), false, true, true);
        dryBuffer.setSize(numChannels, maxBlockSize, false, false, true);
        reverbBuffer.setSize(numChannels, maxBlockSize, false, false, true);
        for (auto& filter : delayTone) filter.prepare(spec);
        for (auto& filter : distortionTone) filter.prepare(spec);
        reset();
    }

    void reset()
    {
        delayBuffer.clear(); preDelayBuffer.clear(); dryBuffer.clear(); reverbBuffer.clear();
        delayWrite = 0; preDelayWrite = 0; reverb.reset();
        for (auto& filter : delayTone) filter.reset();
        for (auto& filter : distortionTone) filter.reset();
    }

    void setSettings(const Settings& value) { settings = value; }

    void process(juce::AudioBuffer<float>& buffer)
    {
        if (buffer.getNumSamples() == 0) return;
        jassert(buffer.getNumSamples() <= maxBlockSize);
        if (settings.delayEnabled) processDelay(buffer);
        if (settings.reverbEnabled) processReverb(buffer);
        if (settings.distortionEnabled) processDistortion(buffer);
    }

private:
    void copyDry(const juce::AudioBuffer<float>& source)
    {
        for (int ch = 0; ch < source.getNumChannels(); ++ch)
            dryBuffer.copyFrom(ch, 0, source, ch, 0, source.getNumSamples());
    }

    void processDelay(juce::AudioBuffer<float>& buffer)
    {
        copyDry(buffer);
        const auto delaySamples = juce::jlimit(1, delayBuffer.getNumSamples() - 2,
            (int) std::round(settings.delayTimeMs * 0.001 * sampleRate));
        const auto feedback = juce::jlimit(0.f, 0.95f, settings.delayFeedback);
        const auto wet = juce::jlimit(0.f, 1.f, settings.delayMix);
        const auto coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate,
            juce::jlimit(250.f, (float) sampleRate * 0.45f, settings.delayTone));
        for (auto& filter : delayTone) filter.state = coefficients;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* output = buffer.getWritePointer(ch);
            auto* line = delayBuffer.getWritePointer(ch);
            auto read = (delayWrite - delaySamples + delayBuffer.getNumSamples()) % delayBuffer.getNumSamples();
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                const auto delayed = delayTone[(size_t) ch].processor.processSample(line[read]);
                line[(delayWrite + i) % delayBuffer.getNumSamples()] = output[i] + delayed * feedback;
                output[i] = output[i] * (1.f - wet) + delayed * wet;
                read = (read + 1) % delayBuffer.getNumSamples();
            }
        }
        delayWrite = (delayWrite + buffer.getNumSamples()) % delayBuffer.getNumSamples();
    }

    void processReverb(juce::AudioBuffer<float>& buffer)
    {
        copyDry(buffer);
        const auto preSamples = juce::jlimit(0, preDelayBuffer.getNumSamples() - 1,
            (int) std::round(settings.reverbPreDelayMs * 0.001 * sampleRate));
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* source = buffer.getReadPointer(ch);
            auto* line = preDelayBuffer.getWritePointer(ch);
            auto* target = reverbBuffer.getWritePointer(ch);
            auto read = (preDelayWrite - preSamples + preDelayBuffer.getNumSamples()) % preDelayBuffer.getNumSamples();
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                line[(preDelayWrite + i) % preDelayBuffer.getNumSamples()] = source[i];
                target[i] = line[read];
                read = (read + 1) % preDelayBuffer.getNumSamples();
            }
        }
        preDelayWrite = (preDelayWrite + buffer.getNumSamples()) % preDelayBuffer.getNumSamples();
        juce::Reverb::Parameters p;
        p.roomSize = juce::jlimit(0.f, 1.f, settings.reverbSize);
        p.damping = juce::jlimit(0.f, 1.f, settings.reverbDamping);
        p.width = juce::jlimit(0.f, 1.f, settings.reverbWidth);
        p.wetLevel = 1.f; p.dryLevel = 0.f; p.freezeMode = 0.f;
        reverb.setParameters(p);
        if (buffer.getNumChannels() >= 2)
            reverb.processStereo(reverbBuffer.getWritePointer(0), reverbBuffer.getWritePointer(1), buffer.getNumSamples());
        else
            reverb.processMono(reverbBuffer.getWritePointer(0), buffer.getNumSamples());
        const auto wet = juce::jlimit(0.f, 1.f, settings.reverbMix);
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            buffer.copyFrom(ch, 0, dryBuffer, ch, 0, buffer.getNumSamples());
            buffer.applyGain(ch, 0, buffer.getNumSamples(), 1.f - wet);
            buffer.addFrom(ch, 0, reverbBuffer, ch, 0, buffer.getNumSamples(), wet);
        }
    }

    void processDistortion(juce::AudioBuffer<float>& buffer)
    {
        copyDry(buffer);
        const auto drive = juce::Decibels::decibelsToGain(juce::jlimit(0.f, 36.f, settings.distortionDrive));
        const auto wet = juce::jlimit(0.f, 1.f, settings.distortionMix);
        const auto coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate,
            juce::jlimit(400.f, (float) sampleRate * 0.45f, settings.distortionTone));
        for (auto& filter : distortionTone) filter.state = coefficients;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* samples = buffer.getWritePointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                const auto x = samples[i] * drive;
                float shaped = 0.f;
                switch (settings.distortionType)
                {
                    case DistortionType::softClip: shaped = std::tanh(x); break;
                    case DistortionType::hardClip: shaped = juce::jlimit(-1.f, 1.f, x); break;
                    case DistortionType::diode: shaped = x >= 0.f ? 1.f - std::exp(-x) : -0.55f * (1.f - std::exp(x)); break;
                }
                const auto filtered = distortionTone[(size_t) ch].processor.processSample(shaped);
                samples[i] = dryBuffer.getSample(ch, i) * (1.f - wet) + filtered * wet;
            }
        }
    }

    Settings settings;
    double sampleRate = 44100.0;
    int maxBlockSize = 512, numChannels = 2, delayWrite = 0, preDelayWrite = 0;
    juce::AudioBuffer<float> delayBuffer, preDelayBuffer, dryBuffer, reverbBuffer;
    using ToneFilter = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>;
    std::array<ToneFilter, 2> delayTone, distortionTone;
    juce::Reverb reverb;
};
