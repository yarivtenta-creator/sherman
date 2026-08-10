#include "PluginEditor.h"

namespace Palette
{
const auto silver = juce::Colour(0xffb9bcc0);
const auto silverDark = juce::Colour(0xff6f7479);
const auto acid = juce::Colour(0xffffd900);
const auto cream = juce::Colour(0xfff4f1e8);
const auto brass = juce::Colour(0xffd1a33d);
const auto charcoal = juce::Colour(0xff111214);
const auto turquoise = juce::Colour(0xff39d6c5);
}

VintageLookAndFeel::VintageLookAndFeel()
{
    setColour(juce::Label::textColourId, Palette::charcoal);
    setColour(juce::Slider::textBoxTextColourId, Palette::cream);
    setColour(juce::Slider::textBoxBackgroundColourId, Palette::charcoal);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::backgroundColourId, Palette::charcoal);
    setColour(juce::ComboBox::textColourId, Palette::cream);
    setColour(juce::ComboBox::outlineColourId, Palette::brass.withAlpha(0.7f));
    setColour(juce::PopupMenu::backgroundColourId, Palette::charcoal);
    setColour(juce::PopupMenu::textColourId, Palette::cream);
}

void VintageLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
    float position, float start, float end, juce::Slider& slider)
{
    const auto diameter = (float) juce::jmin(w, h);
    auto bounds = juce::Rectangle<float>(diameter, diameter)
                 .withCentre(juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h).getCentre())
                 .reduced(3.f);
    auto r = bounds.reduced(diameter * 0.15f);
    const auto centre = r.getCentre();
    const auto angle = start + position * (end - start);
    const auto accent = slider.getProperties().getWithDefault("accent", false) ? Palette::turquoise : Palette::cream;

    for (int tick = 0; tick < 11; ++tick)
    {
        const auto tickAngle = start + (end - start) * (float) tick / 10.f;
        const auto innerRadius = bounds.getWidth() * 0.39f;
        const auto outerRadius = bounds.getWidth() * 0.46f;
        const auto inner = centre + juce::Point<float>(std::sin(tickAngle), -std::cos(tickAngle)) * innerRadius;
        const auto outer = centre + juce::Point<float>(std::sin(tickAngle), -std::cos(tickAngle)) * outerRadius;
        g.setColour(Palette::cream.withAlpha(tick == 0 || tick == 10 ? 0.9f : 0.58f));
        g.drawLine({inner, outer}, tick == 0 || tick == 10 ? 1.7f : 1.1f);
    }

    g.setColour(juce::Colours::black.withAlpha(0.35f));
    g.fillEllipse(r.translated(2.f, 3.f));
    g.setColour(Palette::charcoal);
    g.fillEllipse(r.expanded(3.f));
    g.setColour(juce::Colour(0xff565a5e));
    g.drawEllipse(r.expanded(2.f), 2.f);
    g.setGradientFill({juce::Colour(0xff34363a), centre.x, r.getY(), juce::Colour(0xff08090a), centre.x, r.getBottom(), false});
    g.fillEllipse(r);
    auto cap = r.reduced(r.getWidth() * 0.22f);
    g.setGradientFill({juce::Colour(0xff777b80), cap.getTopLeft(), juce::Colour(0xff25272a), cap.getBottomRight(), false});
    g.fillEllipse(cap);
    g.setColour(juce::Colours::black.withAlpha(0.65f)); g.drawEllipse(cap, 1.4f);
    g.setColour(accent);
    g.drawLine(centre.x, centre.y, centre.x + std::sin(angle) * r.getWidth() * 0.34f,
               centre.y - std::cos(angle) * r.getHeight() * 0.34f, 2.8f);
}

void VintageLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button, bool, bool)
{
    auto r = button.getLocalBounds().toFloat().reduced(2.f);
    const auto selected = (bool) button.getProperties().getWithDefault("selected", false);
    g.setColour(button.getToggleState() ? Palette::acid : Palette::charcoal); g.fillRoundedRectangle(r, 4.f);
    g.setColour(button.getToggleState() ? Palette::charcoal : (selected ? Palette::turquoise : Palette::silver));
    g.drawRoundedRectangle(r, 4.f, selected ? 2.4f : 1.5f);
    g.setColour(button.getToggleState() ? Palette::charcoal : Palette::cream);
    g.setFont(juce::FontOptions(11.f).withStyle("Bold")); g.drawText(button.getButtonText(), r, juce::Justification::centred);
}

VintageDualFilterAudioProcessorEditor::ModularPanel::ModularPanel(juce::AudioProcessorValueTreeState& state)
{
    setOpaque(true);
    const std::array<juce::String, 21> names{
        "INPUT DRIVE", "HIGH BOOST/CUT", "NOISE", "PITCH TRACK",
        "FILTER 1 CHARACTER", "FILTER 2 CHARACTER", "ATTACK", "DECAY", "SUSTAIN", "RELEASE", "FILTER 1 ENV", "FILTER 2 ENV",
        "FM DEPTH", "AM DEPTH", "LFO RATE", "LFO DEPTH", "VCA DRIVE",
        "VCA ATTACK", "VCA RELEASE", "SERIES/PARALLEL", "WET/DRY"};
    const std::array<juce::String, 21> ids{
        "input.drive", "input.highShelf", "input.noise", "input.pitchTrack",
        "filter1.character", "filter2.character", "env.attack", "env.decay", "env.sustain", "env.release", "filter1.envAmount", "filter2.envAmount",
        "fm.depth", "am.depth", "modLfo.rate", "modLfo.depth", "vca.drive",
        "vca.attack", "vca.release", "routingBlend", "globalMix"};
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        knobs[i].setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        knobs[i].setTextBoxStyle(juce::Slider::TextBoxBelow, false, 62, 16);
        knobLabels[i].setText(names[i], juce::dontSendNotification);
        knobLabels[i].setJustificationType(juce::Justification::centred);
        knobLabels[i].setFont(juce::FontOptions(9.f).withStyle("Bold"));
        addAndMakeVisible(knobs[i]); addAndMakeVisible(knobLabels[i]);
        knobAttachments[i] = std::make_unique<SliderAttachment>(state, ids[i], knobs[i]);
    }
    const std::array<juce::String, 4> selectorNames{"HARMONIC", "FM SOURCE", "AM SOURCE", "LFO SHAPE"};
    const std::array<juce::String, 4> selectorIds{"filter2.harmonic", "fm.source", "am.source", "modLfo.shape"};
    const std::array<juce::StringArray, 4> choices{
        juce::StringArray{"-3 OCT", "-2 OCT", "-1 OCT", "UNISON", "FIFTH", "+1 OCT", "+2 OCT", "+3 OCT"},
        juce::StringArray{"OFF", "INPUT", "LFO SINE", "LFO SAW", "SIDECHAIN/CV"},
        juce::StringArray{"OFF", "INPUT", "LFO SINE", "LFO SAW", "SIDECHAIN/CV"},
        juce::StringArray{"SINE", "SAW"}};
    for (size_t i = 0; i < selectors.size(); ++i)
    {
        selectors[i].addItemList(choices[i], 1);
        selectorLabels[i].setText(selectorNames[i], juce::dontSendNotification);
        selectorLabels[i].setJustificationType(juce::Justification::centred);
        addAndMakeVisible(selectors[i]); addAndMakeVisible(selectorLabels[i]);
        selectorAttachments[i] = std::make_unique<ComboAttachment>(state, selectorIds[i], selectors[i]);
    }
    const std::array<juce::String, 3> switchNames{"ADSR ON", "HARMONIC LINK", "VCA ON"};
    const std::array<juce::String, 3> switchIds{"env.enabled", "filter2.sync", "vca.enabled"};
    for (size_t i = 0; i < switches.size(); ++i)
    {
        switches[i].setButtonText(switchNames[i]); addAndMakeVisible(switches[i]);
        switchAttachments[i] = std::make_unique<ButtonAttachment>(state, switchIds[i], switches[i]);
    }
}

void VintageDualFilterAudioProcessorEditor::ModularPanel::paint(juce::Graphics& g)
{
    g.fillAll(Palette::charcoal);
    auto rack = getLocalBounds().toFloat().reduced(4.f);
    g.setGradientFill({Palette::silver.brighter(0.15f), rack.getTopLeft(), Palette::silverDark, rack.getBottomRight(), false});
    g.fillRoundedRectangle(rack, 8.f);
    g.setColour(juce::Colours::white.withAlpha(0.72f)); g.drawRoundedRectangle(rack.reduced(2.f), 8.f, 1.3f);
    g.setColour(Palette::charcoal); g.setFont(juce::FontOptions(16.f).withStyle("Bold"));
    g.drawText("MODULAR CONTROL", getLocalBounds().removeFromTop(32), juce::Justification::centred);
}

void VintageDualFilterAudioProcessorEditor::ModularPanel::resized()
{
    auto area = getLocalBounds().reduced(16); area.removeFromTop(30);
    auto top = area.removeFromTop(62);
    for (size_t i = 0; i < switches.size(); ++i)
        switches[i].setBounds(top.removeFromLeft(top.getWidth() / (int)(switches.size() - i)).reduced(8, 12));
    auto selectorsRow = area.removeFromTop(62);
    for (size_t i = 0; i < selectors.size(); ++i)
    {
        auto cell = selectorsRow.removeFromLeft(selectorsRow.getWidth() / (int)(selectors.size() - i));
        selectorLabels[i].setBounds(cell.removeFromTop(18)); selectors[i].setBounds(cell.reduced(6, 2));
    }
    for (int row = 0; row < 5; ++row)
    {
        auto rowArea = area.removeFromTop(area.getHeight() / (5 - row));
        const auto columns = row == 4 ? 1 : 5;
        for (int col = 0; col < columns; ++col)
        {
            const auto index = row * 5 + col;
            if (index >= (int) knobs.size()) break;
            auto cell = rowArea.removeFromLeft(rowArea.getWidth() / (columns - col));
            knobLabels[(size_t) index].setBounds(cell.removeFromTop(16));
            knobs[(size_t) index].setBounds(cell.reduced(2));
        }
    }
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

    const std::array<juce::String, 3> effectNames{"DELAY", "REVERB", "DISTORTION"};
    const std::array<juce::String, 3> effectIds{"delay.enabled", "reverb.enabled", "distortion.enabled"};
    for (size_t i = 0; i < effectButtons.size(); ++i)
    {
        effectButtons[i].setButtonText(effectNames[i]);
        effectButtons[i].onClick = [this, i] { selectEffect((int) i); };
        effectButtonAttachments[i] = std::make_unique<ButtonAttachment>(state, Params::id(filter, effectIds[i]), effectButtons[i]);
    }
    const std::array<juce::String, 12> effectNamesFull{
        "TIME", "FEEDBACK", "TONE", "MIX",
        "SIZE", "TONE", "PRE-DELAY", "WIDTH", "MIX",
        "DRIVE", "TONE", "MIX"};
    const std::array<juce::String, 12> effectIdsFull{
        "delay.time", "delay.feedback", "delay.tone", "delay.mix",
        "reverb.size", "reverb.damping", "reverb.predelay", "reverb.width", "reverb.mix",
        "distortion.drive", "distortion.tone", "distortion.mix"};
    for (size_t i = 0; i < effectKnobs.size(); ++i)
    {
        effectKnobs[i].setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        effectKnobs[i].setTextBoxStyle(juce::Slider::TextBoxBelow, false, 58, 16);
        effectLabels[i].setText(effectNamesFull[i], juce::dontSendNotification);
        effectLabels[i].setJustificationType(juce::Justification::centred);
        effectLabels[i].setFont(juce::FontOptions(9.f).withStyle("Bold"));
        effectKnobAttachments[i] = std::make_unique<SliderAttachment>(state, Params::id(filter, effectIdsFull[i]), effectKnobs[i]);
    }
    distortionType.addItemList({"SOFT", "HARD", "DIODE"}, 1);
    distortionTypeLabel.setText("TYPE", juce::dontSendNotification);
    distortionTypeLabel.setJustificationType(juce::Justification::centred);
    distortionTypeAttachment = std::make_unique<ComboAttachment>(state, Params::id(filter, "distortion.type"), distortionType);
    selectEffect(0);
}

void VintageDualFilterAudioProcessorEditor::FilterPanel::selectEffect(int effect)
{
    selectedEffect = juce::jlimit(0, 2, effect);
    for (size_t i = 0; i < effectButtons.size(); ++i)
    {
        effectButtons[i].getProperties().set("selected", (int) i == selectedEffect);
        effectButtons[i].repaint();
    }
    const std::array<std::array<int, 5>, 3> indices{{{{0, 1, 2, 3, -1}}, {{4, 5, 8, -1, -1}}, {{9, 10, 11, -1, -1}}}};
    const std::array<int, 3> counts{4, 3, 3};
    for (size_t i = 0; i < effectKnobs.size(); ++i)
    {
        effectKnobs[i].setVisible(false);
        effectLabels[i].setVisible(false);
    }
    for (int i = 0; i < counts[(size_t) selectedEffect]; ++i)
    {
        const auto index = (size_t) indices[(size_t) selectedEffect][(size_t) i];
        effectKnobs[index].setVisible(true);
        effectLabels[index].setVisible(true);
    }
    distortionType.setVisible(selectedEffect == 2);
    distortionTypeLabel.setVisible(selectedEffect == 2);
    if (!lastLayoutArea.isEmpty()) layout(lastLayoutArea);
}

void VintageDualFilterAudioProcessorEditor::FilterPanel::addTo(juce::Component& parent)
{
    parent.addAndMakeVisible(heading); parent.addAndMakeVisible(enabled);
    for (size_t i = 0; i < selectors.size(); ++i) { parent.addAndMakeVisible(selectorLabels[i]); parent.addAndMakeVisible(selectors[i]); }
    for (size_t i = 0; i < knobs.size(); ++i) { parent.addAndMakeVisible(knobLabels[i]); parent.addAndMakeVisible(knobs[i]); }
    for (auto& button : effectButtons) parent.addAndMakeVisible(button);
    for (size_t i = 0; i < effectKnobs.size(); ++i) { parent.addAndMakeVisible(effectLabels[i]); parent.addAndMakeVisible(effectKnobs[i]); }
    parent.addAndMakeVisible(distortionTypeLabel); parent.addAndMakeVisible(distortionType);
    selectEffect(selectedEffect);
}

void VintageDualFilterAudioProcessorEditor::FilterPanel::layout(juce::Rectangle<int> area)
{
    lastLayoutArea = area;
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
    auto filterKnobs = area.removeFromTop(190);
    for (int row = 0; row < 2; ++row)
    {
        auto rowArea = filterKnobs.removeFromTop(filterKnobs.getHeight() / (2 - row));
        for (int col = 0; col < 4; ++col)
        {
            const auto index = (size_t)(row * 4 + col);
            auto cell = rowArea.removeFromLeft(rowArea.getWidth() / (4 - col));
            knobLabels[index].setBounds(cell.removeFromTop(18)); knobs[index].setBounds(cell.reduced(2));
        }
    }
    auto tabs = area.removeFromTop(38);
    for (int tab = 0; tab < 3; ++tab)
    {
        auto cell = tabs.removeFromLeft(tabs.getWidth() / (3 - tab));
        effectButtons[(size_t) tab].setBounds(cell.reduced(3, 4));
    }
    const std::array<std::array<int, 5>, 3> indices{{{{0, 1, 2, 3, -1}}, {{4, 5, 8, -1, -1}}, {{9, 10, 11, -1, -1}}}};
    const std::array<int, 3> counts{4, 3, 3};
    auto controls = area.reduced(2, 3);
    if (selectedEffect == 2)
    {
        auto typeArea = controls.removeFromLeft(76);
        distortionTypeLabel.setBounds(typeArea.removeFromTop(18));
        distortionType.setBounds(typeArea.removeFromTop(26));
    }
    for (int col = 0; col < counts[(size_t) selectedEffect]; ++col)
    {
        const auto index = (size_t) indices[(size_t) selectedEffect][(size_t) col];
        auto cell = controls.removeFromLeft(controls.getWidth() / (counts[(size_t) selectedEffect] - col));
        effectLabels[index].setBounds(cell.removeFromTop(16));
        effectKnobs[index].setBounds(cell.reduced(1));
    }
}

VintageDualFilterAudioProcessorEditor::VintageDualFilterAudioProcessorEditor(VintageDualFilterAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setLookAndFeel(&look); setResizable(true, true); setResizeLimits(920, 640, 1320, 880);
    title.setText("Y A R I F I L T E R", juce::dontSendNotification);
    title.setJustificationType(juce::Justification::centred); title.setFont(juce::FontOptions(28.f).withStyle("Bold"));
    subtitle.setText("ANALOG CHARACTER PROCESSOR", juce::dontSendNotification); subtitle.setJustificationType(juce::Justification::centred);
    routingLabel.setText("SIGNAL FLOW", juce::dontSendNotification); routingLabel.setJustificationType(juce::Justification::centred);
    presetLabel.setText("FACTORY COLOUR", juce::dontSendNotification); presetLabel.setJustificationType(juce::Justification::centred);
    userPresetLabel.setText("USER PRESET", juce::dontSendNotification); userPresetLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(title); addAndMakeVisible(subtitle); addAndMakeVisible(routingLabel); addAndMakeVisible(routing); addAndMakeVisible(presetLabel); addAndMakeVisible(presets);
    addAndMakeVisible(userPresetLabel); addAndMakeVisible(userPresets); addAndMakeVisible(saveUserPreset); addAndMakeVisible(loadUserPreset);
    addAndMakeVisible(modularButton);
    modularPanel = std::make_unique<ModularPanel>(processor.parameters);
    addChildComponent(*modularPanel);
    modularButton.onClick = [this]
    {
        const auto show = !modularPanel->isVisible();
        modularPanel->setVisible(show);
        modularButton.setButtonText(show ? "FILTERS" : "MODULAR");
        if (show) modularPanel->toFront(false);
    };
    routing.addItemList({"SERIES", "PARALLEL"}, 1);
    routingAttachment = std::make_unique<ComboAttachment>(processor.parameters, "routing", routing);
    for (int i = 0; i < processor.getNumPrograms(); ++i) presets.addItem(processor.getProgramName(i), i + 1);
    presets.setSelectedId(processor.getCurrentProgram() + 1, juce::dontSendNotification);
    presets.onChange = [this] { processor.setCurrentProgram(presets.getSelectedId() - 1); repaint(); };
    userPresets.addItemList({"USER 1", "USER 2", "USER 3", "USER 4"}, 1);
    userPresets.setSelectedId(1, juce::dontSendNotification);
    saveUserPreset.onClick = [this]
    {
        processor.saveUserPreset(userPresets.getSelectedId());
        userPresetLabel.setText("USER PRESET  •  SAVED", juce::dontSendNotification);
    };
    loadUserPreset.onClick = [this]
    {
        if (processor.hasUserPreset(userPresets.getSelectedId()))
        {
            processor.loadUserPreset(userPresets.getSelectedId());
            presets.setSelectedId(0, juce::dontSendNotification);
            userPresetLabel.setText("USER PRESET  •  LOADED", juce::dontSendNotification);
            repaint();
        }
        else userPresetLabel.setText("USER PRESET  •  EMPTY", juce::dontSendNotification);
    };
    for (auto* button : {&saveUserPreset, &loadUserPreset})
    {
        button->setColour(juce::TextButton::buttonColourId, Palette::charcoal);
        button->setColour(juce::TextButton::textColourOffId, Palette::cream);
    }
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
    setSize(1040, 720);
}

VintageDualFilterAudioProcessorEditor::~VintageDualFilterAudioProcessorEditor() { setLookAndFeel(nullptr); }

void VintageDualFilterAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(Palette::charcoal);
    auto chassis = getLocalBounds().toFloat().reduced(10.f);
    g.setGradientFill({Palette::silver.brighter(0.2f), chassis.getTopLeft(), Palette::silverDark, chassis.getBottomRight(), false});
    g.fillRoundedRectangle(chassis, 10.f);
    g.setColour(Palette::brass); g.drawRoundedRectangle(chassis, 10.f, 2.f);
    g.setColour(juce::Colours::black.withAlpha(0.12f));
    for (int y = 18; y < getHeight(); y += 5) g.drawHorizontalLine(y, 15.f, (float)getWidth() - 15.f);
    const auto panelColour = processor.getCurrentProgram() == 2 ? Palette::acid : Palette::silver.brighter(0.12f).withAlpha(0.72f);
    g.setColour(panelColour);
    const auto panelWidth = ((float)getWidth() - 56.f - juce::jmax(145.f, (float)getWidth() / 8.f)) * 0.5f;
    const auto leftPanel = juce::Rectangle<float>(28.f, 92.f, panelWidth, (float)getHeight() - 185.f);
    const auto rightPanel = juce::Rectangle<float>((float)getWidth() - 28.f - panelWidth, 92.f, panelWidth, (float)getHeight() - 185.f);
    g.fillRoundedRectangle(leftPanel, 8.f); g.fillRoundedRectangle(rightPanel, 8.f);
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.drawRoundedRectangle(leftPanel.reduced(1.f), 8.f, 1.2f);
    g.drawRoundedRectangle(rightPanel.reduced(1.f), 8.f, 1.2f);
    for (auto p : {juce::Point<float>{25.f, 25.f}, juce::Point<float>{(float)getWidth()-25.f, 25.f},
                   juce::Point<float>{25.f, (float)getHeight()-25.f}, juce::Point<float>{(float)getWidth()-25.f, (float)getHeight()-25.f}})
    { g.setColour(Palette::charcoal); g.fillEllipse(juce::Rectangle<float>(10.f, 10.f).withCentre(p)); g.setColour(Palette::brass); g.drawEllipse(juce::Rectangle<float>(10.f, 10.f).withCentre(p), 1.f); }
}

void VintageDualFilterAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(28);
    title.setBounds(area.removeFromTop(38)); subtitle.setBounds(area.removeFromTop(22)); area.removeFromTop(12);
    modularButton.setBounds(getWidth() - 120, 28, 86, 28);
    auto bottom = area.removeFromBottom(58); area.removeFromBottom(10);
    const auto centreWidth = juce::jmax(145, area.getWidth() / 8);
    auto left = area.removeFromLeft((area.getWidth() - centreWidth) / 2); auto centre = area.removeFromLeft(centreWidth); auto right = area;
    if (filterPanels[0] != nullptr) filterPanels[0]->layout(left.reduced(8));
    if (filterPanels[1] != nullptr) filterPanels[1]->layout(right.reduced(8));
    presetLabel.setBounds(centre.removeFromTop(22)); presets.setBounds(centre.removeFromTop(32).reduced(5, 2)); centre.removeFromTop(12);
    routingLabel.setBounds(centre.removeFromTop(22)); routing.setBounds(centre.removeFromTop(32).reduced(5, 2));
    centre.removeFromTop(8);
    userPresetLabel.setBounds(centre.removeFromTop(20));
    userPresets.setBounds(centre.removeFromTop(28).reduced(5, 1));
    auto userButtons = centre.removeFromTop(30);
    saveUserPreset.setBounds(userButtons.removeFromLeft(userButtons.getWidth() / 2).reduced(3));
    loadUserPreset.setBounds(userButtons.reduced(3));
    centre.removeFromTop(5);
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
    if (modularPanel != nullptr) modularPanel->setBounds(getLocalBounds().reduced(24).withTrimmedTop(66).withTrimmedBottom(18));
}
