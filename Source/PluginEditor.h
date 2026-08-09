#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

class VintageLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    VintageLookAndFeel();
    void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    void drawToggleButton(juce::Graphics&, juce::ToggleButton&, bool, bool) override;
};

class VintageDualFilterAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit VintageDualFilterAudioProcessorEditor(VintageDualFilterAudioProcessor&);
    ~VintageDualFilterAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct FilterPanel
    {
        FilterPanel(juce::AudioProcessorValueTreeState&, int);
        void addTo(juce::Component&);
        void layout(juce::Rectangle<int>);

        juce::Label heading;
        std::array<juce::ComboBox, 7> selectors;
        std::array<juce::Label, 7> selectorLabels;
        std::array<juce::Slider, 8> knobs;
        std::array<juce::Label, 8> knobLabels;
        juce::ToggleButton enabled{"ACTIVE"};
        std::array<std::unique_ptr<SliderAttachment>, 8> knobAttachments;
        std::array<std::unique_ptr<ComboAttachment>, 7> selectorAttachments;
        std::unique_ptr<ButtonAttachment> enabledAttachment;
    };

    VintageDualFilterAudioProcessor& processor;
    VintageLookAndFeel look;
    juce::Label title, subtitle, routingLabel, presetLabel;
    std::array<std::unique_ptr<FilterPanel>, 2> filterPanels;
    juce::ComboBox routing;
    juce::ComboBox presets;
    std::array<juce::Slider, 2> masterKnobs;
    std::array<juce::Label, 2> masterLabels;
    std::array<std::unique_ptr<SliderAttachment>, 2> masterAttachments;
    std::unique_ptr<ComboAttachment> routingAttachment;
    std::array<juce::ToggleButton, 3> kills;
    std::array<std::unique_ptr<ButtonAttachment>, 3> killAttachments;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VintageDualFilterAudioProcessorEditor)
};
