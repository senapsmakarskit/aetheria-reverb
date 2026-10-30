#include "PluginProcessor.h"
#include "PluginEditor.h"

AetheriaAudioProcessorEditor::AetheriaAudioProcessorEditor(AetheriaAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(720, 420);

    auto setupSlider = [this](juce::Slider& slider, const juce::String& paramID) {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
        slider.setLookAndFeel(&customKnobLAF);
        addAndMakeVisible(slider);
        sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                audioProcessor.apvts, paramID, slider));
    };

    setupSlider(decaySlider, "decay");
    setupSlider(mixSlider, "mix");
    setupSlider(shimmerSlider, "shimmer");
    setupSlider(dampSlider, "damp");
    setupSlider(sizeSlider, "size");
    setupSlider(swellSlider, "swell");

    freezeButton.setButtonText("FREEZE");
    addAndMakeVisible(freezeButton);
    buttonAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.apvts, "freeze", freezeButton);
}

AetheriaAudioProcessorEditor::~AetheriaAudioProcessorEditor()
{
    decaySlider.setLookAndFeel(nullptr);
    mixSlider.setLookAndFeel(nullptr);
    shimmerSlider.setLookAndFeel(nullptr);
    dampSlider.setLookAndFeel(nullptr);
    sizeSlider.setLookAndFeel(nullptr);
    swellSlider.setLookAndFeel(nullptr);
}

void AetheriaAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0b0d10));

    // Top Header Panel
    g.setColour(juce::Colour(0xff161a22));
    g.fillRect(0, 0, getWidth(), 65);

    // Title
    g.setColour(juce::Colour(0xff00e5ff));
    g.setFont(juce::FontOptions(26.0f).withStyle("Bold"));
    g.drawText("AETHERIA", 20, 12, 200, 35, juce::Justification::left);

    // Subtitle
    g.setColour(juce::Colour(0xff8a99ad));
    g.setFont(juce::FontOptions(12.0f));
    g.drawText("AMBIENT REVERB ENGINE", 165, 22, 200, 20, juce::Justification::left);

    // Developer Credit in Top Right Header
    g.setColour(juce::Colour(0xff00e5ff));
    g.setFont(juce::FontOptions(13.0f).withStyle("Bold"));
    g.drawText("Developed by Martin Strand", getWidth() - 240, 22, 220, 20, juce::Justification::right);

    // Knob Labels
    g.setColour(juce::Colour(0xffc5d1e0));
    g.setFont(juce::FontOptions(12.0f).withStyle("Bold"));

    g.drawText("DECAY", 30, 90, 90, 20, juce::Justification::centred);
    g.drawText("MIX", 140, 90, 90, 20, juce::Justification::centred);
    g.drawText("SHIMMER", 250, 90, 90, 20, juce::Justification::centred);
    g.drawText("DAMPING", 360, 90, 90, 20, juce::Justification::centred);
    g.drawText("SIZE", 470, 90, 90, 20, juce::Justification::centred);
    g.drawText("SWELL", 580, 90, 90, 20, juce::Justification::centred);

    // Bottom Status Bar
    g.setColour(juce::Colour(0xff12151c));
    g.fillRect(0, getHeight() - 30, getWidth(), 30);

    g.setColour(juce::Colour(0xff526075));
    g.setFont(juce::FontOptions(11.0f));
    g.drawText("MIDI CC1: Shimmer  |  MIDI CC64: Freeze Pedal  |  Sigur Rós Swell DSP Active", 20, getHeight() - 25, 500, 20, juce::Justification::left);

    g.setColour(juce::Colour(0xff00e5ff));
    g.setFont(juce::FontOptions(12.0f).withStyle("Bold"));
    g.drawText("v1.0.0", getWidth() - 80, getHeight() - 25, 60, 20, juce::Justification::right);
}

void AetheriaAudioProcessorEditor::resized()
{
    int knobY = 120;
    int knobW = 90;
    int knobH = 110;

    decaySlider.setBounds(30, knobY, knobW, knobH);
    mixSlider.setBounds(140, knobY, knobW, knobH);
    shimmerSlider.setBounds(250, knobY, knobW, knobH);
    dampSlider.setBounds(360, knobY, knobW, knobH);
    sizeSlider.setBounds(470, knobY, knobW, knobH);
    swellSlider.setBounds(580, knobY, knobW, knobH);

    freezeButton.setBounds(305, 290, 110, 40);
}
