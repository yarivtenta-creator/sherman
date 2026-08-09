#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace Params
{
inline juce::String id(int filter, const juce::String& name)
{
    return "filter" + juce::String(filter) + "." + name;
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using APF = juce::AudioParameterFloat;
    using APC = juce::AudioParameterChoice;
    using APB = juce::AudioParameterBool;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<APF>("inputGain", "Input", juce::NormalisableRange<float>{-24.f, 24.f}, 0.f));
    layout.add(std::make_unique<APF>("outputGain", "Output", juce::NormalisableRange<float>{-24.f, 24.f}, 0.f));
    layout.add(std::make_unique<APC>("routing", "Routing", juce::StringArray{"Series", "Parallel"}, 0));
    layout.add(std::make_unique<APB>("kill.low", "Low Kill", false));
    layout.add(std::make_unique<APB>("kill.mid", "Mid Kill", false));
    layout.add(std::make_unique<APB>("kill.high", "High Kill", false));

    for (int f = 1; f <= 2; ++f)
    {
        const auto prefix = "Filter " + juce::String(f) + " ";
        layout.add(std::make_unique<APB>(id(f, "enabled"), prefix + "Enabled", true));
        layout.add(std::make_unique<APC>(id(f, "model"), prefix + "Model",
            juce::StringArray{"Clean", "Ladder Colour", "MS-20 Colour", "OTA Colour"}, 1));
        layout.add(std::make_unique<APC>(id(f, "mode"), prefix + "Mode",
            juce::StringArray{"Low-pass", "High-pass", "Band-pass"}, 0));
        layout.add(std::make_unique<APC>(id(f, "slope"), prefix + "Slope",
            juce::StringArray{"6 dB", "12 dB", "24 dB", "48 dB"}, 2));
        layout.add(std::make_unique<APF>(id(f, "cutoff"), prefix + "Cutoff",
            juce::NormalisableRange<float>{20.f, 20000.f, 0.f, 0.25f}, 1000.f));
        layout.add(std::make_unique<APF>(id(f, "resonance"), prefix + "Resonance",
            juce::NormalisableRange<float>{0.1f, 10.f, 0.01f}, 0.707f));
        layout.add(std::make_unique<APF>(id(f, "thd"), prefix + "THD",
            juce::NormalisableRange<float>{0.f, 100.f, 0.1f}, 0.f));
        layout.add(std::make_unique<APF>(id(f, "mix"), prefix + "Mix",
            juce::NormalisableRange<float>{0.f, 100.f, 0.1f}, 100.f));

        layout.add(std::make_unique<APB>(id(f, "delay.enabled"), prefix + "Delay Enabled", false));
        layout.add(std::make_unique<APF>(id(f, "delay.time"), prefix + "Delay Time",
            juce::NormalisableRange<float>{1.f, 2000.f, 1.f, 0.35f}, 320.f));
        layout.add(std::make_unique<APF>(id(f, "delay.feedback"), prefix + "Delay Feedback",
            juce::NormalisableRange<float>{0.f, 95.f, 0.1f}, 35.f));
        layout.add(std::make_unique<APF>(id(f, "delay.tone"), prefix + "Delay Tone",
            juce::NormalisableRange<float>{250.f, 18000.f, 1.f, 0.3f}, 6500.f));
        layout.add(std::make_unique<APF>(id(f, "delay.mix"), prefix + "Delay Mix",
            juce::NormalisableRange<float>{0.f, 100.f, 0.1f}, 25.f));

        layout.add(std::make_unique<APB>(id(f, "reverb.enabled"), prefix + "Reverb Enabled", false));
        layout.add(std::make_unique<APF>(id(f, "reverb.size"), prefix + "Reverb Size",
            juce::NormalisableRange<float>{0.f, 100.f, 0.1f}, 55.f));
        layout.add(std::make_unique<APF>(id(f, "reverb.damping"), prefix + "Reverb Damping",
            juce::NormalisableRange<float>{0.f, 100.f, 0.1f}, 45.f));
        layout.add(std::make_unique<APF>(id(f, "reverb.predelay"), prefix + "Reverb Pre-delay",
            juce::NormalisableRange<float>{0.f, 200.f, 0.1f}, 18.f));
        layout.add(std::make_unique<APF>(id(f, "reverb.width"), prefix + "Reverb Width",
            juce::NormalisableRange<float>{0.f, 100.f, 0.1f}, 100.f));
        layout.add(std::make_unique<APF>(id(f, "reverb.mix"), prefix + "Reverb Mix",
            juce::NormalisableRange<float>{0.f, 100.f, 0.1f}, 25.f));

        layout.add(std::make_unique<APB>(id(f, "distortion.enabled"), prefix + "Distortion Enabled", false));
        layout.add(std::make_unique<APC>(id(f, "distortion.type"), prefix + "Distortion Type",
            juce::StringArray{"Soft", "Hard", "Diode"}, 0));
        layout.add(std::make_unique<APF>(id(f, "distortion.drive"), prefix + "Distortion Drive",
            juce::NormalisableRange<float>{0.f, 36.f, 0.1f}, 12.f));
        layout.add(std::make_unique<APF>(id(f, "distortion.tone"), prefix + "Distortion Tone",
            juce::NormalisableRange<float>{400.f, 18000.f, 1.f, 0.3f}, 8000.f));
        layout.add(std::make_unique<APF>(id(f, "distortion.mix"), prefix + "Distortion Mix",
            juce::NormalisableRange<float>{0.f, 100.f, 0.1f}, 100.f));

        for (int l = 1; l <= 2; ++l)
        {
            const auto lfo = "lfo" + juce::String(l) + ".";
            layout.add(std::make_unique<APC>(id(f, lfo + "shape"), prefix + "LFO " + juce::String(l) + " Shape",
                juce::StringArray{"Sine", "Triangle", "Saw", "Square", "S&H"}, 0));
            layout.add(std::make_unique<APF>(id(f, lfo + "rate"), prefix + "LFO " + juce::String(l) + " Rate",
                juce::NormalisableRange<float>{0.01f, 20.f, 0.f, 0.3f}, 1.f));
            layout.add(std::make_unique<APF>(id(f, lfo + "depth"), prefix + "LFO " + juce::String(l) + " Depth",
                juce::NormalisableRange<float>{-1.f, 1.f, 0.001f}, 0.f));
            layout.add(std::make_unique<APC>(id(f, lfo + "target"), prefix + "LFO " + juce::String(l) + " Target",
                juce::StringArray{"Cutoff", "Resonance", "Drive", "Mix"}, 0));
        }
    }
    return layout;
}
}
