#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class CustomKnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CustomKnobLookAndFeel()
    {
        setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff00e5ff));
        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff1e222b));
        setColour(juce::Slider::thumbColourId, juce::Colour(0xffffffff));
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider&) override
    {
        auto radius = (float)juce::jmin(width / 2, height / 2) - 8.0f;
        auto centreX = (float)x + (float)width * 0.5f;
        auto centreY = (float)y + (float)height * 0.5f;
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;
        auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        // Knob Body
        g.setColour(juce::Colour(0xff12151a));
        g.fillEllipse(rx, ry, rw, rw);

        g.setColour(juce::Colour(0xff2d333f));
        g.drawEllipse(rx, ry, rw, rw, 2.0f);

        // Neon Blue Arc Track
        juce::Path p;
        p.addCentredArc(centreX, centreY, radius - 4.0f, radius - 4.0f, 0.0f, rotaryStartAngle, angle, true);
        g.setColour(juce::Colour(0xff00e5ff));
        g.strokePath(p, juce::PathStrokeType(3.0f));

        // Pointer
        juce::Path ptr;
        auto pointerLength = radius * 0.6f;
        ptr.addRectangle(-2.0f, -radius + 4.0f, 4.0f, pointerLength);
        ptr.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
        g.setColour(juce::Colour(0xff00e5ff));
        g.fillPath(ptr);
    }
};

class AetheriaAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    AetheriaAudioProcessorEditor(AetheriaAudioProcessor&);
    ~AetheriaAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    AetheriaAudioProcessor& audioProcessor;

    CustomKnobLookAndFeel customKnobLAF;

    juce::Slider decaySlider, mixSlider, shimmerSlider, dampSlider, sizeSlider, swellSlider;
    juce::ToggleButton freezeButton;

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> buttonAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AetheriaAudioProcessorEditor)
};
