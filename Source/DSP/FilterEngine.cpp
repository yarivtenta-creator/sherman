#include "FilterEngine.h"

FilterEngine::FilterEngine()
    : oversampling(2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true)
{
}

void FilterEngine::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    maximumBlockSize = (int)spec.maximumBlockSize;
    for (auto& stage : stages) stage.prepare(spec);
    for (auto& bank : morphStages) for (auto& stage : bank) stage.prepare(spec);
    oversampling.initProcessing(spec.maximumBlockSize);
    dryDelay.prepare(spec);
    dryDelay.setDelay(oversampling.getLatencyInSamples());
    dryBuffer.setSize((int)spec.numChannels, maximumBlockSize, false, false, true);
    for (auto& scratch : morphBuffers)
        scratch.setSize((int) spec.numChannels, maximumBlockSize, false, false, true);
    reset();
}

void FilterEngine::reset()
{
    for (auto& stage : stages) stage.reset();
    for (auto& bank : morphStages) for (auto& stage : bank) stage.reset();
    oversampling.reset();
    dryDelay.reset();
    phases.fill(0.f);
    heldRandom.fill(0.f);
    onePoleLowState.fill(0.f);
    onePoleBandState.fill(0.f);
    heldModulation.fill(0.f);
    samplesUntilControlUpdate = 0;
}

void FilterEngine::setSettings(const Settings& newSettings) { settings = newSettings; }

float FilterEngine::nextLfoValue(size_t index, int numSamples)
{
    const auto oldPhase = phases[index];
    auto& phase = phases[index];
    const auto shape = settings.lfo[index].shape;
    float value = 0.f;

    switch (shape)
    {
        case LfoShape::sine: value = std::sin(phase * juce::MathConstants<float>::twoPi); break;
        case LfoShape::triangle: value = 1.f - 4.f * std::abs(phase - 0.5f); break;
        case LfoShape::saw: value = phase * 2.f - 1.f; break;
        case LfoShape::square: value = phase < 0.5f ? 1.f : -1.f; break;
        case LfoShape::sampleAndHold: value = heldRandom[index]; break;
    }

    phase += settings.lfo[index].rate * (float)numSamples / (float)sampleRate;
    phase -= std::floor(phase);
    if (shape == LfoShape::sampleAndHold && phase < oldPhase)
        heldRandom[index] = random.nextFloat() * 2.f - 1.f;
    return value * settings.lfo[index].depth;
}

void FilterEngine::updateFilters(float cutoff, float resonance)
{
    using Type = juce::dsp::StateVariableTPTFilterType;
    const std::array<Type, 3> types{Type::lowpass, Type::highpass, Type::bandpass};
    cutoff = juce::jlimit(20.f, (float)sampleRate * 0.45f, cutoff);
    resonance = juce::jlimit(0.1f, 12.f, resonance);

    for (auto& stage : stages)
    {
        stage.setType(types[(size_t)settings.mode]);
        stage.setCutoffFrequency(cutoff);
        stage.setResonance(resonance);
    }
    const std::array<Type, 3> morphTypes{Type::lowpass, Type::bandpass, Type::highpass};
    for (size_t bank = 0; bank < morphStages.size(); ++bank)
        for (auto& stage : morphStages[bank])
        {
            stage.setType(morphTypes[bank]);
            stage.setCutoffFrequency(cutoff);
            stage.setResonance(resonance);
        }

    onePoleAlpha = 1.f - std::exp(-juce::MathConstants<float>::twoPi * cutoff / (float)sampleRate);
    const auto bandwidth = juce::jmax(0.25f, resonance);
    const auto lower = juce::jlimit(20.f, (float)sampleRate * 0.4f, cutoff / std::sqrt(bandwidth + 1.f));
    const auto upper = juce::jlimit(lower + 1.f, (float)sampleRate * 0.45f, cutoff * std::sqrt(bandwidth + 1.f));
    bandLowAlpha = 1.f - std::exp(-juce::MathConstants<float>::twoPi * lower / (float)sampleRate);
    bandHighAlpha = 1.f - std::exp(-juce::MathConstants<float>::twoPi * upper / (float)sampleRate);
}

void FilterEngine::applyModelSaturation(juce::dsp::AudioBlock<float> block, float driveAmount)
{
    if (driveAmount <= 0.001f) return;
    const auto drive = juce::Decibels::decibelsToGain(driveAmount * 0.24f);
    const auto normaliser = 1.f / juce::jmax(0.001f, std::tanh(drive));

    for (size_t channel = 0; channel < block.getNumChannels(); ++channel)
    {
        auto* samples = block.getChannelPointer(channel);
        for (size_t i = 0; i < block.getNumSamples(); ++i)
        {
            const auto x = samples[i] * drive;
            switch (settings.model)
            {
                case Model::clean:  samples[i] = std::tanh(x) * normaliser; break;
                case Model::ladder: samples[i] = std::tanh(x + 0.08f * x * x) * normaliser; break;
                case Model::ms20:   samples[i] = std::atan(x * 1.6f) * (2.f / juce::MathConstants<float>::pi); break;
                case Model::ota:    samples[i] = x / (1.f + std::abs(x)); break;
            }
        }
    }
}

void FilterEngine::process(juce::AudioBuffer<float>& buffer)
{
    if (buffer.getNumSamples() == 0) return;
    if (!settings.enabled)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            dryBuffer.copyFrom(ch, 0, buffer, ch, 0, buffer.getNumSamples());
        juce::dsp::AudioBlock<float> bypassBlock(buffer);
        oversampling.processSamplesUp(bypassBlock);
        oversampling.processSamplesDown(bypassBlock);
        auto disabledDry = juce::dsp::AudioBlock<float>(dryBuffer).getSubBlock(0, (size_t)buffer.getNumSamples());
        juce::dsp::ProcessContextReplacing<float> disabledDryContext(disabledDry);
        dryDelay.process(disabledDryContext);
        for (auto& stage : stages) stage.reset();
        onePoleLowState.fill(0.f); onePoleBandState.fill(0.f);
        return;
    }
    jassert(buffer.getNumSamples() <= maximumBlockSize);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        dryBuffer.copyFrom(ch, 0, buffer, ch, 0, buffer.getNumSamples());

    const auto modelCutoff = std::array<float, 4>{1.f, 0.94f, 1.08f, 1.02f}[(size_t)settings.model];
    const auto modelResonance = std::array<float, 4>{1.f, 1.18f, 1.35f, 0.9f}[(size_t)settings.model];
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::AudioBlock<float> dryBlock(dryBuffer);
    auto drySlice = dryBlock.getSubBlock(0, (size_t)buffer.getNumSamples());
    juce::dsp::ProcessContextReplacing<float> dryContext(drySlice);
    dryDelay.process(dryContext);

    constexpr int controlQuantum = 16;
    for (int offset = 0; offset < buffer.getNumSamples();)
    {
        if (samplesUntilControlUpdate == 0)
        {
            heldModulation.fill(0.f);
            for (size_t i = 0; i < settings.lfo.size(); ++i)
            {
                const auto value = nextLfoValue(i, controlQuantum);
                heldModulation[(size_t)settings.lfo[i].target] += value;
            }
            samplesUntilControlUpdate = controlQuantum;
        }
        const auto count = juce::jmin(samplesUntilControlUpdate, buffer.getNumSamples() - offset);
        const auto cutoffMod = heldModulation[(size_t)LfoTarget::cutoff];
        const auto resonanceMod = heldModulation[(size_t)LfoTarget::resonance];
        const auto driveMod = heldModulation[(size_t)LfoTarget::drive];
        const auto mixMod = heldModulation[(size_t)LfoTarget::mix];
        updateFilters(settings.cutoff * modelCutoff * std::pow(2.f, cutoffMod * 3.f),
                      settings.resonance * modelResonance * (1.f + resonanceMod * 0.75f));

        auto slice = block.getSubBlock((size_t)offset, (size_t)count);
        auto oversampled = oversampling.processSamplesUp(slice);
        applyModelSaturation(oversampled, juce::jlimit(0.f, 100.f, settings.thd + driveMod * 50.f));
        oversampling.processSamplesDown(slice);

        if (settings.slopeIndex == 0 && settings.character < 0.f)
        {
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                auto* samples = buffer.getWritePointer(ch, offset);
                auto& low = onePoleLowState[(size_t)ch];
                auto& bandLow = onePoleBandState[(size_t)ch];
                for (int sample = 0; sample < count; ++sample)
                {
                    const auto input = samples[sample];
                    if (settings.mode == Mode::bandPass)
                    {
                        bandLow += bandLowAlpha * (input - bandLow);
                        const auto highPassed = input - bandLow;
                        low += bandHighAlpha * (highPassed - low);
                        samples[sample] = low;
                    }
                    else
                    {
                        low += onePoleAlpha * (input - low);
                        samples[sample] = settings.mode == Mode::lowPass ? low : input - low;
                    }
                }
            }
        }
        else if (settings.character >= 0.f)
        {
            const auto stageCount = std::array<int, 4>{1, 1, 2, 4}[(size_t)juce::jlimit(0, 3, settings.slopeIndex)];
            for (int bank = 0; bank < 3; ++bank)
            {
                for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                    morphBuffers[(size_t) bank].copyFrom(ch, offset, buffer, ch, offset, count);
                auto morphBlock = juce::dsp::AudioBlock<float>(morphBuffers[(size_t) bank]).getSubBlock((size_t) offset, (size_t) count);
                juce::dsp::ProcessContextReplacing<float> morphContext(morphBlock);
                for (int stage = 0; stage < stageCount; ++stage)
                    morphStages[(size_t) bank][(size_t) stage].process(morphContext);
            }
            const auto position = juce::jlimit(0.f, 1.f, settings.character) * 3.f;
            const auto segment = juce::jmin(2, (int) std::floor(position));
            const auto fraction = position - (float) segment;
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                auto* output = buffer.getWritePointer(ch, offset);
                const auto* low = morphBuffers[0].getReadPointer(ch, offset);
                const auto* band = morphBuffers[1].getReadPointer(ch, offset);
                const auto* high = morphBuffers[2].getReadPointer(ch, offset);
                for (int sample = 0; sample < count; ++sample)
                {
                    const auto notch = (low[sample] + high[sample]) * 0.7071f;
                    const std::array<float, 4> anchors{low[sample], band[sample], notch, high[sample]};
                    output[sample] = anchors[(size_t) segment] * (1.f - fraction)
                                   + anchors[(size_t) segment + 1] * fraction;
                }
            }
        }
        else
        {
            juce::dsp::ProcessContextReplacing<float> context(slice);
            const auto stageCount = std::array<int, 4>{1, 1, 2, 4}[(size_t)juce::jlimit(0, 3, settings.slopeIndex)];
            for (int i = 0; i < stageCount; ++i) stages[(size_t)i].process(context);
        }

        const auto wet = juce::jlimit(0.f, 1.f, settings.mix + mixMod * 0.5f);
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            buffer.applyGain(ch, offset, count, wet);
            buffer.addFrom(ch, offset, dryBuffer, ch, offset, count, 1.f - wet);
        }
        offset += count;
        samplesUntilControlUpdate -= count;
    }
}
