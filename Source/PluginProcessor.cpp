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
    set("routingBlend", 0.f); set("globalMix", 100.f);
    set("input.drive", 0.f); set("input.highShelf", 0.f); set("input.noise", 0.f); set("input.pitchTrack", 0.f);
    set("env.enabled", 0.f); set("env.attack", 20.f); set("env.decay", 180.f); set("env.sustain", 65.f);
    set("env.release", 420.f); set("env.threshold", -30.f);
    set("filter2.sync", 0.f); set("filter2.harmonic", 3.f);
    set("fm.source", 0.f); set("fm.depth", 0.f); set("am.source", 0.f); set("am.depth", 0.f);
    set("modLfo.shape", 0.f); set("modLfo.rate", 1.f); set("modLfo.depth", 0.f);
    set("vca.enabled", 0.f); set("vca.drive", 0.f); set("vca.attack", 10.f); set("vca.release", 250.f); set("vca.depth", 100.f);
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
        set(Params::id(filter, "character"), 0.f);
        set(Params::id(filter, "envAmount"), 0.f);
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
    parameters.state.setProperty("currentProgram", currentProgram, nullptr);
}

void VintageDualFilterAudioProcessor::saveUserPreset(int slot)
{
    slot = juce::jlimit(1, 4, slot);
    juce::ValueTree preset("USER_PRESET");
    preset.setProperty("slot", slot, nullptr);
    for (auto* parameter : getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(parameter))
            preset.setProperty(ranged->getParameterID(), ranged->convertFrom0to1(ranged->getValue()), nullptr);

    for (int i = parameters.state.getNumChildren(); --i >= 0;)
    {
        const auto child = parameters.state.getChild(i);
        if (child.hasType("USER_PRESET") && (int) child.getProperty("slot") == slot)
            parameters.state.removeChild(i, nullptr);
    }
    parameters.state.appendChild(preset, nullptr);
}

void VintageDualFilterAudioProcessor::loadUserPreset(int slot)
{
    slot = juce::jlimit(1, 4, slot);
    for (int i = 0; i < parameters.state.getNumChildren(); ++i)
    {
        const auto preset = parameters.state.getChild(i);
        if (!preset.hasType("USER_PRESET") || (int) preset.getProperty("slot") != slot) continue;
        for (auto* parameter : getParameters())
            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(parameter))
                if (preset.hasProperty(ranged->getParameterID()))
                    ranged->setValueNotifyingHost(ranged->convertTo0to1((float) preset.getProperty(ranged->getParameterID())));
        return;
    }
}

bool VintageDualFilterAudioProcessor::hasUserPreset(int slot) const
{
    for (int i = 0; i < parameters.state.getNumChildren(); ++i)
    {
        const auto child = parameters.state.getChild(i);
        if (child.hasType("USER_PRESET") && (int) child.getProperty("slot") == slot) return true;
    }
    return false;
}

void VintageDualFilterAudioProcessor::prepareToPlay(double rate, int block)
{
    const juce::dsp::ProcessSpec spec{rate, (juce::uint32) block, (juce::uint32) getTotalNumOutputChannels()};
    for (auto& f : filters) f.prepare(spec);
    for (auto& effect : effects) effect.prepare(spec);
    for (auto& f : parallelFilters) f.prepare(spec);
    for (auto& effect : parallelEffects) effect.prepare(spec);
    modulation.prepare(rate, (int) spec.numChannels, block);
    filterLatency = filters[0].getLatencySamples();
    parallelLatencyCompensation.prepare(spec);
    parallelLatencyCompensation.setDelay(filterLatency);
    setLatencySamples((int)std::ceil(filterLatency * 2.f));
    killBands.prepare(spec);
    parallelBuffer.setSize((int)spec.numChannels, block, false, false, true);
    parallelSecondBuffer.setSize((int)spec.numChannels, block, false, false, true);
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
    s.character = value(Params::id(f, "character")) / 100.f;
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

void VintageDualFilterAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals guard;
    for (const auto metadata : midi)
    {
        const auto message = metadata.getMessage();
        if (message.isNoteOn()) midiGate = true;
        else if (message.isNoteOff() || message.isAllNotesOff()) midiGate = false;
    }
    auto value = [this](const juce::String& id) { return parameters.getRawParameterValue(id)->load(); };
    modulation.captureDry(buffer);
    modulation.processInput(buffer, value("input.drive"), value("input.highShelf"), value("input.noise"));
    modulation.analyse(buffer, value("env.attack"), value("env.decay"), value("env.sustain") / 100.f,
                       value("env.release"), value("env.threshold"), value("modLfo.rate"), value("modLfo.shape") > 0.5f, midiGate);
    inputGain.setGainDecibels(parameters.getRawParameterValue("inputGain")->load());
    outputGain.setGainDecibels(parameters.getRawParameterValue("outputGain")->load());
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    inputGain.process(context);
    auto firstSettings = readSettings(1);
    auto secondSettings = readSettings(2);
    if (value("env.enabled") > 0.5f)
    {
        firstSettings.cutoff *= std::pow(2.f, modulation.getEnvelope() * value(Params::id(1, "envAmount")) * 0.06f);
        secondSettings.cutoff *= std::pow(2.f, modulation.getEnvelope() * value(Params::id(2, "envAmount")) * 0.06f);
    }
    const auto pitchAmount = value("input.pitchTrack") / 100.f;
    if (pitchAmount > 0.f && modulation.getPitchHz() > 20.f)
    {
        firstSettings.cutoff *= std::pow(modulation.getPitchHz() / juce::jmax(20.f, firstSettings.cutoff), pitchAmount);
        secondSettings.cutoff *= std::pow(modulation.getPitchHz() / juce::jmax(20.f, secondSettings.cutoff), pitchAmount);
    }
    const auto fmSource = (int) value("fm.source");
    const auto fmValue = fmSource == 1 ? modulation.getEnvelope() * 2.f - 1.f : modulation.getLfo();
    if (fmSource != 0)
    {
        const auto ratio = std::pow(2.f, fmValue * value("fm.depth") / 12.f);
        firstSettings.cutoff *= ratio; secondSettings.cutoff *= ratio;
    }
    if (value("filter2.sync") > 0.5f)
    {
        const std::array<float, 8> ratios{0.125f, 0.25f, 0.5f, 1.f, 1.5f, 2.f, 4.f, 8.f};
        secondSettings.cutoff = firstSettings.cutoff * ratios[(size_t) juce::jlimit(0, 7, (int) value("filter2.harmonic"))];
    }
    filters[0].setSettings(firstSettings); filters[1].setSettings(secondSettings);
    parallelFilters[0].setSettings(firstSettings); parallelFilters[1].setSettings(secondSettings);
    effects[0].setSettings(readEffectSettings(1)); effects[1].setSettings(readEffectSettings(2));
    parallelEffects[0].setSettings(readEffectSettings(1)); parallelEffects[1].setSettings(readEffectSettings(2));

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        parallelBuffer.copyFrom(ch, 0, buffer, ch, 0, buffer.getNumSamples());
        parallelSecondBuffer.copyFrom(ch, 0, buffer, ch, 0, buffer.getNumSamples());
    }
    filters[0].process(buffer); effects[0].process(buffer);
    filters[1].process(buffer); effects[1].process(buffer);
    juce::AudioBuffer<float> parallelViewA(parallelBuffer.getArrayOfWritePointers(), buffer.getNumChannels(), buffer.getNumSamples());
    juce::AudioBuffer<float> parallelViewB(parallelSecondBuffer.getArrayOfWritePointers(), buffer.getNumChannels(), buffer.getNumSamples());
    parallelFilters[0].process(parallelViewA); parallelEffects[0].process(parallelViewA);
    parallelFilters[1].process(parallelViewB); parallelEffects[1].process(parallelViewB);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        parallelBuffer.addFrom(ch, 0, parallelSecondBuffer, ch, 0, buffer.getNumSamples());
        parallelBuffer.applyGain(ch, 0, buffer.getNumSamples(), 0.5f);
    }
    auto parallelBlock = juce::dsp::AudioBlock<float>(parallelBuffer).getSubBlock(0, (size_t) buffer.getNumSamples());
    juce::dsp::ProcessContextReplacing<float> parallelContext(parallelBlock);
    parallelLatencyCompensation.process(parallelContext);
    auto routingBlend = value("routingBlend") / 100.f;
    if (value("routing") > 0.5f && routingBlend <= 0.001f) routingBlend = 1.f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        buffer.applyGain(ch, 0, buffer.getNumSamples(), 1.f - routingBlend);
        buffer.addFrom(ch, 0, parallelBuffer, ch, 0, buffer.getNumSamples(), routingBlend);
    }
    killBands.process(buffer,
        parameters.getRawParameterValue("kill.low")->load() > 0.5f,
        parameters.getRawParameterValue("kill.mid")->load() > 0.5f,
        parameters.getRawParameterValue("kill.high")->load() > 0.5f);
    const auto amSource = (int) value("am.source");
    const auto amValue = amSource == 1 ? modulation.getEnvelope() * 2.f - 1.f : modulation.getLfo();
    modulation.processOutput(buffer, value("vca.enabled") > 0.5f, value("vca.drive"), value("vca.attack"),
                             value("vca.release"), value("vca.depth") / 100.f,
                             amSource == 0 ? 0.f : value("am.depth") / 100.f, amValue, value("globalMix") / 100.f);
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
