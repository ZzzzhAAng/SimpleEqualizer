/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

struct CustomRotarySlider : juce::Slider
{
    CustomRotarySlider() : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
                                        juce::Slider::TextEntryBoxPosition::NoTextBox)
    {
        
    }
};

//==============================================================================
/**
*/
class SimpleEqualizerAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    SimpleEqualizerAudioProcessorEditor (SimpleEqualizerAudioProcessor&);
    ~SimpleEqualizerAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SimpleEqualizerAudioProcessor& audioProcessor;
    
    CustomRotarySlider highPassFreqSlider, lowPassFreqSlider, peakFreqSlider, peakGainSlider, peakQualitySlider,
    highPassSlopeSlider, lowPassSlopeSlider;
    
    std::vector<juce::Component*> getComps();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEqualizerAudioProcessorEditor)
};
