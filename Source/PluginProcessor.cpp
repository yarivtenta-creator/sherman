#include "PluginProcessor.h"
#include "PluginEditor.h"

VintageDualFilterAudioProcessor::VintageDualFilterAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "STATE", Params::createLayout()) {}

bool VintageDualFilterAudioProcessor::isBusesLayoutSupported(const BusesLayout& l) const
{
    return l.getMainInputChannelSet() == l.getMainOutputChannelSet()
        && (l.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
         || l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo());
}

const juce::String VintageDualFilterAudioProcessor::getProgramName(int index)
{
    const std::array<juce::String, 5> names{"INIT", "WARM LADDER", "ACID CUT", "DUB MOTION", "PARALLEL AIR"};
    return names[(size_t)juce::jlimit(0, (int)names.size() - 1, index)];
}

void VintageDualFilterAudioProcessor::setCurrentProgram(int index)
{
    currentProgram = juce::jlimit(0, getNumPrograms() - 1, index);
    auto set = [this](const juce::String& id, float plainValue)
    {
        if (auto* parameter = parameters.getParameter(id))
            parameter->setValueNotifyingHost(parameter->convertTo0to1(plainValue));
    };
    set("inputGain", 0.f); set("outputGain", 0.f); set("routing", 0.f);
    set("kill.low", 0.f); set("kill.mid", 0.f); set("kill.high", 0.f);
    for (int filter = 1; filter <= 2; ++filter)
    {
        set(Params::id(filter, "enabled"), 1.f);
        set(Params::id(filter, "model"), filter == 1 ? 1.f : 3.f);
        set(Params::id(filter, "mode"), 0.f);
        set(Params::id(filter, "slope"), 2.f);
        set(Params::id(filter, "cutoff"), 1000.f);
        set(Params::id(filter, "resonance"), 0.707f);
        set(Params::id(filter, "thd"), 0.f);
        set(Params::id(filter, "mix"), 100.f);
        set(Params::id(filter, "delay.enabled"), 0.f);
        set(Params::id(filter, "delay.time"), 320.f);
        set(Params::id(filter, "delay.feedback"), 35.f);
        set(Params::id(filter, "delay.tone"), 6500.f);
        set(Params::id(filter, "delay.mix"), 25.f);
        set(Params::id(filter, "reverb.enabled"), 0.f);
        set(Params::id(filter, "reverb.size"), 55.f);
        set(Params::id(filter, "reverb.damping"), 45.f);
        set(Params::id(filter, "reverb.predelay"), 18.f);
        set(Params::id(filter, "reverb.width"), 100.f);
        set(Params::id(filter, "reverb.mix"), 25.f);
        set(Params::id(filter, "distortion.enabled"), 0.f);
        set(Params::id(filter, "distortion.type"), 0.f);
        set(Params::id(filter, "distortion.drive"), 12.f);
        set(Params::id(filter, "distortion.tone"), 8000.f);
        set(Params::id(filter, "distortion.mix"), 100.f);
        for (int lfo = 1; lfo <= 2; ++lfo)
        {
            const auto prefix = "lfo" + juce::String(lfo) + ".";
            set(Params::id(filter, prefix + "shape"), 0.f);
            set(Params::id(filter, prefix + "rate"), 1.f);
            set(Params::id(filter, prefix + "depth"), 0.f);
            set(Params::id(filter, prefix + "target"), 0.f);
        }
    }
    set("routing", currentProgram == 4 ? 1.f : 0.f);
    set(Params::id(1, "model"), currentProgram == 2 ? 2.f : 1.f);
    set(Params::id(2, "model"), currentProgram == 4 ? 0.f : 3.f);
    set(Params::id(1, "cutoff"), std::array<float, 5>{1000.f, 420.f, 1450.f, 520.f, 6800.f}[(size_t)currentProgram]);
    set(Params::id(2, "cutoff"), std::array<float, 5>{1000.f, 6200.f, 2800.f, 3200.f, 11000.f}[(size_t)currentProgram]);
    set(Params::id(1, "resonance"), std::array<float, 5>{0.707f, 1.1f, 5.5f, 1.8f, 0.6f}[(size_t)currentProgram]);
    set(Params::id(1, "thd"), std::array<float, 5>{0.f, 22.f, 48.f, 30.f, 8.f}[(size_t)currentProgram]);
    if (currentProgram == 3)
    {
        set(Params::id(1, "lfo1.rate"), 0.28f);
        set(Params::id(1, "lfo1.depth"), 0.35f);
        set(Params::id(1, "lfo1.shape"), 1.f);
    }
    if (currentProgram == 2)
    {
        set(Params::id(1, "distortion.enabled"), 1.f);
        set(Params::id(1, "distortion.type"), 2.f);
        set(Params::id(1, "distortion.drive"), 18.f);
        set(Params::id(1, "distortion.tone"), 10500.f);
    }
    if (currentProgram == 3)
    {
        set(Params::id(1, "delay.enabled"), 1.f);
        set(Params::id(1, "delay.time"), 420.f);
        set(Params::id(1, "delay.feedback"), 58.f);
        set(Params::id(1, "reverb.enabled"), 1.f);
        set(Params::id(1, "reverb.size"), 68.f);
    }
    parameters.state.setProperty("currentProgram", currentProgram, nullptr);
}

void VintageDualFilterAudioProcessor::prepareToPlay(double rate, int block)
{
    const juce::dsp::ProcessSpec spec{rate, (juce::uint32) block, (juce::uint32) getTotalNumOutputChannels()};
    for (auto& f : filters) f.prepare(spec);
    for (auto& effect : effects) effect.prepare(spec);
    filterLatency = filters[0].getLatencySamples();
    parallelLatencyCompensation.prepare(spec);
    parallelLatencyCompensation.setDelay(filterLatency);
    setLatencySamples((int)std::ceil(filterLatency * 2.f));
    killBands.prepare(spec);
    parallelBuffer.setSize((int)spec.numChannels, block, false, false, true);
    inputGain.prepare(spec); outputGain.prepare(spec);
}

FilterEngine::Settings VintageDualFilterAudioProcessor::readSettings(int f) const
{
    FilterEngine::Settings s;
    auto value = [this](const juce::String& key) { return parameters.getRawParameterValue(key)->load(); };
    s.enabled = value(Params::id(f, "enabled")) > 0.5f;
    s.model = static_cast<FilterEngine::Model>((int)value(Params::id(f, "model")));
    s.mode = static_cast<FilterEngine::Mode>((int)value(Params::id(f, "mode")));
    s.slopeIndex = (int)value(Params::id(f, "slope")); s.cutoff = value(Params::id(f, "cutoff"));
    s.resonance = value(Params::id(f, "resonance")); s.thd = value(Params::id(f, "thd"));
    s.mix = value(Params::id(f, "mix")) / 100.f;
    for (int l = 0; l < 2; ++l) {
        const auto p = "lfo" + juce::String(l + 1) + ".";
        s.lfo[(size_t)l].rate = value(Params::id(f, p + "rate"));
        s.lfo[(size_t)l].depth = value(Params::id(f, p + "depth"));
        s.lfo[(size_t)l].shape = static_cast<FilterEngine::LfoShape>((int)value(Params::id(f, p + "shape")));
        s.lfo[(size_t)l].target = static_cast<FilterEngine::LfoTarget>((int)value(Params::id(f, p + "target")));
    }
    return s;
}

EffectChain::Settings VintageDualFilterAudioProcessor::readEffectSettings(int f) const
{
    EffectChain::Settings s;
    auto value = [this, f](const juce::String& key)
    {
        return parameters.getRawParameterValue(Params::id(f, key))->load();
    };
    s.delayEnabled = value("delay.enabled") > 0.5f;
    s.delayTimeMs = value("delay.time");
    s.delayFeedback = value("delay.feedback") / 100.f;
    s.delayTone = value("delay.tone");
    s.delayMix = value("delay.mix") / 100.f;
    s.reverbEnabled = value("reverb.enabled") > 0.5f;
    s.reverbSize = value("reverb.size") / 100.f;
    s.reverbDamping = value("reverb.damping") / 100.f;
    s.reverbPreDelayMs = value("reverb.predelay");
    s.reverbWidth = value("reverb.width") / 100.f;
    s.reverbMix = value("reverb.mix") / 100.f;
    s.distortionEnabled = value("distortion.enabled") > 0.5f;
    s.distortionType = static_cast<EffectChain::DistortionType>((int) value("distortion.type"));
    s.distortionDrive = value("distortion.drive");
    s.distortionTone = value("distortion.tone");
    s.distortionMix = value("distortion.mix") / 100.f;
    return s;
}

void VintageDualFilterAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals guard;
    inputGain.setGainDecibels(parameters.getRawParameterValue("inputGain")->load());
    outputGain.setGainDecibels(parameters.getRawParameterValue("outputGain")->load());
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    inputGain.process(context);
    filters[0].setSettings(readSettings(1)); filters[1].setSettings(readSettings(2));
    effects[0].setSettings(readEffectSettings(1)); effects[1].setSettings(readEffectSettings(2));

    if (parameters.getRawParameterValue("routing")->load() < 0.5f) {
        filters[0].process(buffer); effects[0].process(buffer);
        filters[1].process(buffer); effects[1].process(buffer);
    } else {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            parallelBuffer.copyFrom(ch, 0, buffer, ch, 0, buffer.getNumSamples());
        juce::AudioBuffer<float> parallelView(parallelBuffer.getArrayOfWritePointers(), buffer.getNumChannels(), buffer.getNumSamples());
        filters[0].process(buffer); effects[0].process(buffer);
        filters[1].process(parallelView); effects[1].process(parallelView);
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            buffer.addFrom(ch, 0, parallelBuffer, ch, 0, buffer.getNumSamples());
            buffer.applyGain(ch, 0, buffer.getNumSamples(), 0.5f);
        }
        parallelLatencyCompensation.process(context);
    }
    killBands.process(buffer,
        parameters.getRawParameterValue("kill.low")->load() > 0.5f,
        parameters.getRawParameterValue("kill.mid")->load() > 0.5f,
        parameters.getRawParameterValue("kill.high")->load() > 0.5f);
    outputGain.process(context);
}

void VintageDualFilterAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    if (auto xml = parameters.copyState().createXml()) copyXmlToBinary(*xml, dest);
}

void VintageDualFilterAudioProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size); xml && xml->hasTagName(parameters.state.getType()))
    {
        parameters.replaceState(juce::ValueTree::fromXml(*xml));
        currentProgram = juce::jlimit(0, getNumPrograms() - 1, (int)parameters.state.getProperty("currentProgram", 0));
    }
}

juce::AudioProcessorEditor* VintageDualFilterAudioProcessor::createEditor() { return new VintageDualFilterAudioProcessorEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new VintageDualFilterAudioProcessor(); }
