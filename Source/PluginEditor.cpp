#include "PluginEditor.h"

namespace Palette
{
const auto burgundy = juce::Colour(0xff4d1428);
const auto burgundyDark = juce::Colour(0xff260914);
const auto cream = juce::Colour(0xffeadfca);
const auto brass = juce::Colour(0xffc89a4b);
const auto charcoal = juce::Colour(0xff171719);
const auto turquoise = juce::Colour(0xff4ec8bd);
}

VintageLookAndFeel::VintageLookAndFeel()
{
    setColour(juce::Label::textColourId, Palette::cream);
    setColour(juce::Slider::textBoxTextColourId, Palette::cream);
    setColour(juce::Slider::textBoxBackgroundColourId, Palette::burgundyDark);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::backgroundColourId, Palette::burgundyDark);
    setColour(juce::ComboBox::textColourId, Palette::cream);
    setColour(juce::ComboBox::outlineColourId, Palette::brass.withAlpha(0.7f));
    setColour(juce::PopupMenu::backgroundColourId, Palette::burgundyDark);
    setColour(juce::PopupMenu::textColourId, Palette::cream);
}

void VintageLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
    float position, float start, float end, juce::Slider& slider)
{
    auto r = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h).reduced(9.f);
    const auto centre = r.getCentre();
    const auto angle = start + position * (end - start);
    const auto accent = slider.getProperties().getWithDefault("accent", false) ? Palette::turquoise : Palette::brass;

    g.setColour(juce::Colours::black.withAlpha(0.35f));
    g.fillEllipse(r.translated(2.f, 3.f));
    g.setGradientFill({juce::Colour(0xff343238), centre.x, r.getY(), Palette::charcoal, centre.x, r.getBottom(), false});
    g.fillEllipse(r);
    g.setColour(accent); g.drawEllipse(r, 2.2f);
    g.setColour(Palette::cream.withAlpha(0.22f)); g.drawEllipse(r.reduced(5.f), 1.f);
    g.setColour(accent);
    g.drawLine(centre.x, centre.y, centre.x + std::sin(angle) * r.getWidth() * 0.38f,
               centre.y - std::cos(angle) * r.getHeight() * 0.38f, 3.f);
}

void VintageLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button, bool, bool)
{
    auto r = button.getLocalBounds().toFloat().reduced(2.f);
    g.setColour(button.getToggleState() ? Palette::turquoise : Palette::burgundyDark); g.fillRoundedRectangle(r, 4.f);
    g.setColour(button.getToggleState() ? Palette::cream : Palette::brass); g.drawRoundedRectangle(r, 4.f, 1.5f);
    g.setColour(button.getToggleState() ? Palette::charcoal : Palette::cream);
    g.setFont(juce::FontOptions(11.f).withStyle("Bold")); g.drawText(button.getButtonText(), r, juce::Justification::centred);
}

VintageDualFilterAudioProcessorEditor::FilterPanel::FilterPanel(juce::AudioProcessorValueTreeState& state, int filter)
{
    heading.setText("FILTER  " + juce::String(filter), juce::dontSendNotification);
    heading.setJustificationType(juce::Justification::centred);
    heading.setFont(juce::FontOptions(19.f).withStyle("Bold"));

    const std::array<juce::String, 7> selectorNames{"MODEL", "TYPE", "SLOPE", "LFO1 SHAPE", "LFO1 TARGET", "LFO2 SHAPE", "LFO2 TARGET"};
    const std::array<juce::StringArray, 7> choices{
        juce::StringArray{"Clean", "Ladder Colour", "MS-20 Colour", "OTA Colour"},
        juce::StringArray{"Low-pass", "High-pass", "Band-pass"},
        juce::StringArray{"6 dB", "12 dB", "24 dB", "48 dB"},
        juce::StringArray{"Sine", "Triangle", "Saw", "Square", "S&H"},
        juce::StringArray{"Cutoff", "Resonance", "Drive", "Mix"},
        juce::StringArray{"Sine", "Triangle", "Saw", "Square", "S&H"},
        juce::StringArray{"Cutoff", "Resonance", "Drive", "Mix"}};
    const std::array<juce::String, 7> selectorIds{"model", "mode", "slope", "lfo1.shape", "lfo1.target", "lfo2.shape", "lfo2.target"};
    for (size_t i = 0; i < selectors.size(); ++i)
    {
        selectors[i].addItemList(choices[i], 1);
        selectorLabels[i].setText(selectorNames[i], juce::dontSendNotification);
        selectorLabels[i].setJustificationType(juce::Justification::centred);
        selectorAttachments[i] = std::make_unique<ComboAttachment>(state, Params::id(filter, selectorIds[i]), selectors[i]);
    }

    const std::array<juce::String, 8> names{"CUTOFF", "RESONANCE", "THD", "MIX", "LFO 1 RATE", "LFO 1 DEPTH", "LFO 2 RATE", "LFO 2 DEPTH"};
    const std::array<juce::String, 8> ids{"cutoff", "resonance", "thd", "mix", "lfo1.rate", "lfo1.depth", "lfo2.rate", "lfo2.depth"};
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        knobs[i].setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        knobs[i].setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 18);
        if (i >= 4) knobs[i].getProperties().set("accent", true);
        knobLabels[i].setText(names[i], juce::dontSendNotification);
        knobLabels[i].setJustificationType(juce::Justification::centred);
        knobLabels[i].setFont(juce::FontOptions(10.f).withStyle("Bold"));
        knobAttachments[i] = std::make_unique<SliderAttachment>(state, Params::id(filter, ids[i]), knobs[i]);
    }
    enabledAttachment = std::make_unique<ButtonAttachment>(state, Params::id(filter, "enabled"), enabled);
}

void VintageDualFilterAudioProcessorEditor::FilterPanel::addTo(juce::Component& parent)
{
    parent.addAndMakeVisible(heading); parent.addAndMakeVisible(enabled);
    for (size_t i = 0; i < selectors.size(); ++i) { parent.addAndMakeVisible(selectorLabels[i]); parent.addAndMakeVisible(selectors[i]); }
    for (size_t i = 0; i < knobs.size(); ++i) { parent.addAndMakeVisible(knobLabels[i]); parent.addAndMakeVisible(knobs[i]); }
}

void VintageDualFilterAudioProcessorEditor::FilterPanel::layout(juce::Rectangle<int> area)
{
    auto top = area.removeFromTop(34); heading.setBounds(top.removeFromLeft(top.getWidth() - 85)); enabled.setBounds(top.reduced(3));
    auto selectorsArea = area.removeFromTop(112);
    auto primary = selectorsArea.removeFromTop(56);
    for (size_t i = 0; i < 3; ++i)
    {
        auto cell = primary.removeFromLeft(primary.getWidth() / (3 - (int)i));
        selectorLabels[i].setBounds(cell.removeFromTop(18)); selectors[i].setBounds(cell.reduced(5, 1));
    }
    for (size_t i = 3; i < selectors.size(); ++i)
    {
        auto cell = selectorsArea.removeFromLeft(selectorsArea.getWidth() / (int)(selectors.size() - i));
        selectorLabels[i].setBounds(cell.removeFromTop(18)); selectors[i].setBounds(cell.reduced(3, 1));
    }
    for (int row = 0; row < 2; ++row)
    {
        auto rowArea = area.removeFromTop(area.getHeight() / (2 - row));
        for (int col = 0; col < 4; ++col)
        {
            const auto index = (size_t)(row * 4 + col);
            auto cell = rowArea.removeFromLeft(rowArea.getWidth() / (4 - col));
            knobLabels[index].setBounds(cell.removeFromTop(18)); knobs[index].setBounds(cell.reduced(2));
        }
    }
}

VintageDualFilterAudioProcessorEditor::VintageDualFilterAudioProcessorEditor(VintageDualFilterAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setLookAndFeel(&look); setResizable(true, true); setResizeLimits(900, 540, 1500, 900);
    title.setText("J A R I F I L T E R", juce::dontSendNotification);
    title.setJustificationType(juce::Justification::centred); title.setFont(juce::FontOptions(28.f).withStyle("Bold"));
    subtitle.setText("ANALOG CHARACTER PROCESSOR", juce::dontSendNotification); subtitle.setJustificationType(juce::Justification::centred);
    routingLabel.setText("SIGNAL FLOW", juce::dontSendNotification); routingLabel.setJustificationType(juce::Justification::centred);
    presetLabel.setText("FACTORY COLOUR", juce::dontSendNotification); presetLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(title); addAndMakeVisible(subtitle); addAndMakeVisible(routingLabel); addAndMakeVisible(routing); addAndMakeVisible(presetLabel); addAndMakeVisible(presets);
    routing.addItemList({"SERIES", "PARALLEL"}, 1);
    routingAttachment = std::make_unique<ComboAttachment>(processor.parameters, "routing", routing);
    for (int i = 0; i < processor.getNumPrograms(); ++i) presets.addItem(processor.getProgramName(i), i + 1);
    presets.setSelectedId(processor.getCurrentProgram() + 1, juce::dontSendNotification);
    presets.onChange = [this] { processor.setCurrentProgram(presets.getSelectedId() - 1); };
    const std::array<juce::String, 2> masterIds{"inputGain", "outputGain"};
    const std::array<juce::String, 2> masterNames{"INPUT", "OUTPUT"};
    for (size_t i = 0; i < masterKnobs.size(); ++i)
    {
        masterKnobs[i].setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        masterKnobs[i].setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 17);
        masterLabels[i].setText(masterNames[i], juce::dontSendNotification);
        masterLabels[i].setJustificationType(juce::Justification::centred);
        addAndMakeVisible(masterKnobs[i]); addAndMakeVisible(masterLabels[i]);
        masterAttachments[i] = std::make_unique<SliderAttachment>(processor.parameters, masterIds[i], masterKnobs[i]);
    }
    for (int i = 0; i < 2; ++i) { filterPanels[(size_t)i] = std::make_unique<FilterPanel>(processor.parameters, i + 1); filterPanels[(size_t)i]->addTo(*this); }
    const std::array<juce::String, 3> killNames{"LOW KILL", "MID KILL", "HIGH KILL"};
    const std::array<juce::String, 3> killIds{"kill.low", "kill.mid", "kill.high"};
    for (size_t i = 0; i < kills.size(); ++i) { kills[i].setButtonText(killNames[i]); addAndMakeVisible(kills[i]); killAttachments[i] = std::make_unique<ButtonAttachment>(processor.parameters, killIds[i], kills[i]); }
    setSize(1180, 660);
}

VintageDualFilterAudioProcessorEditor::~VintageDualFilterAudioProcessorEditor() { setLookAndFeel(nullptr); }

void VintageDualFilterAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(Palette::burgundyDark);
    auto chassis = getLocalBounds().toFloat().reduced(10.f);
    g.setGradientFill({Palette::burgundy.brighter(0.12f), chassis.getTopLeft(), Palette::burgundyDark, chassis.getBottomRight(), false});
    g.fillRoundedRectangle(chassis, 10.f);
    g.setColour(Palette::brass); g.drawRoundedRectangle(chassis, 10.f, 2.f);
    g.setColour(juce::Colours::black.withAlpha(0.12f));
    for (int y = 18; y < getHeight(); y += 5) g.drawHorizontalLine(y, 15.f, (float)getWidth() - 15.f);
    g.setColour(Palette::cream.withAlpha(0.12f));
    g.fillRoundedRectangle(juce::Rectangle<float>(28.f, 92.f, (float)getWidth() * 0.43f, (float)getHeight() - 185.f), 8.f);
    g.fillRoundedRectangle(juce::Rectangle<float>((float)getWidth() * 0.57f, 92.f, (float)getWidth() * 0.43f - 28.f, (float)getHeight() - 185.f), 8.f);
    for (auto p : {juce::Point<float>{25.f, 25.f}, juce::Point<float>{(float)getWidth()-25.f, 25.f},
                   juce::Point<float>{25.f, (float)getHeight()-25.f}, juce::Point<float>{(float)getWidth()-25.f, (float)getHeight()-25.f}})
    { g.setColour(Palette::charcoal); g.fillEllipse(juce::Rectangle<float>(10.f, 10.f).withCentre(p)); g.setColour(Palette::brass); g.drawEllipse(juce::Rectangle<float>(10.f, 10.f).withCentre(p), 1.f); }
}

void VintageDualFilterAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(28);
    title.setBounds(area.removeFromTop(38)); subtitle.setBounds(area.removeFromTop(22)); area.removeFromTop(12);
    auto bottom = area.removeFromBottom(58); area.removeFromBottom(10);
    const auto centreWidth = juce::jmax(145, area.getWidth() / 8);
    auto left = area.removeFromLeft((area.getWidth() - centreWidth) / 2); auto centre = area.removeFromLeft(centreWidth); auto right = area;
    if (filterPanels[0] != nullptr) filterPanels[0]->layout(left.reduced(8));
    if (filterPanels[1] != nullptr) filterPanels[1]->layout(right.reduced(8));
    presetLabel.setBounds(centre.removeFromTop(22)); presets.setBounds(centre.removeFromTop(32).reduced(5, 2)); centre.removeFromTop(12);
    routingLabel.setBounds(centre.removeFromTop(22)); routing.setBounds(centre.removeFromTop(32).reduced(5, 2));
    centre.removeFromTop(8);
    auto masterArea = centre.removeFromTop(105);
    for (size_t i = 0; i < masterKnobs.size(); ++i)
    {
        auto cell = masterArea.removeFromLeft(masterArea.getWidth() / (int)(masterKnobs.size() - i));
        masterLabels[i].setBounds(cell.removeFromTop(18)); masterKnobs[i].setBounds(cell);
    }
    auto killArea = centre.withSizeKeepingCentre(115, 150);
    for (auto& kill : kills) kill.setBounds(killArea.removeFromTop(50).reduced(7));
    bottom = bottom.withSizeKeepingCentre(420, bottom.getHeight());
    for (size_t i = 0; i < kills.size(); ++i) bottom.removeFromLeft(bottom.getWidth() / (int)(kills.size() - i));
}
