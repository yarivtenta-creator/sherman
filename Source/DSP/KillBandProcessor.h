#pragma once
#include <juce_dsp/juce_dsp.h>

class KillBandProcessor
{
public:
    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        maximumBlockSize = (int)spec.maximumBlockSize;
        lowBuffer.setSize((int)spec.numChannels, maximumBlockSize, false, false, true);
        midBuffer.setSize((int)spec.numChannels, maximumBlockSize, false, false, true);
        highBuffer.setSize((int)spec.numChannels, maximumBlockSize, false, false, true);
        for (auto* filter : {&lowPass, &highPass}) filter->prepare(spec);
        lowPass.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
        highPass.setType(juce::dsp::LinkwitzRileyFilterType::highpass);
        lowPass.setCutoffFrequency(200.f);
        highPass.setCutoffFrequency(2500.f);
        for (auto* gain : {&lowGain, &midGain, &highGain}) { gain->reset(spec.sampleRate, 0.012); gain->setCurrentAndTargetValue(1.f); }
    }

    void reset()
    {
        for (auto* filter : {&lowPass, &highPass}) filter->reset();
    }

    void process(juce::AudioBuffer<float>& source, bool killLow, bool killMid, bool killHigh)
    {
        jassert(source.getNumSamples() <= maximumBlockSize);
        copy(source, lowBuffer); copy(source, midBuffer); copy(source, highBuffer);
        processFilter(lowBuffer, source.getNumSamples(), lowPass);
        processFilter(highBuffer, source.getNumSamples(), highPass);
        for (int ch = 0; ch < source.getNumChannels(); ++ch)
        {
            midBuffer.copyFrom(ch, 0, source, ch, 0, source.getNumSamples());
            midBuffer.addFrom(ch, 0, lowBuffer, ch, 0, source.getNumSamples(), -1.f);
            midBuffer.addFrom(ch, 0, highBuffer, ch, 0, source.getNumSamples(), -1.f);
        }
        lowGain.setTargetValue(killLow ? 0.f : 1.f);
        midGain.setTargetValue(killMid ? 0.f : 1.f);
        highGain.setTargetValue(killHigh ? 0.f : 1.f);
        source.clear();
        for (int sample = 0; sample < source.getNumSamples(); ++sample)
        {
            const auto low = lowGain.getNextValue(), mid = midGain.getNextValue(), high = highGain.getNextValue();
            for (int ch = 0; ch < source.getNumChannels(); ++ch)
                source.setSample(ch, sample, lowBuffer.getSample(ch, sample) * low
                                              + midBuffer.getSample(ch, sample) * mid
                                              + highBuffer.getSample(ch, sample) * high);
        }
    }

private:
    static void copy(const juce::AudioBuffer<float>& source, juce::AudioBuffer<float>& target)
    {
        for (int ch = 0; ch < source.getNumChannels(); ++ch)
            target.copyFrom(ch, 0, source, ch, 0, source.getNumSamples());
    }

    static void processFilter(juce::AudioBuffer<float>& buffer, int numSamples, juce::dsp::LinkwitzRileyFilter<float>& filter)
    {
        auto block = juce::dsp::AudioBlock<float>(buffer).getSubBlock(0, (size_t)numSamples);
        juce::dsp::ProcessContextReplacing<float> context(block);
        filter.process(context);
    }

    juce::dsp::LinkwitzRileyFilter<float> lowPass, highPass;
    juce::AudioBuffer<float> lowBuffer, midBuffer, highBuffer;
    juce::SmoothedValue<float> lowGain, midGain, highGain;
    int maximumBlockSize = 512;
};
