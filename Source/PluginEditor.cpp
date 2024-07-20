/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimpleEqualizerAudioProcessorEditor::SimpleEqualizerAudioProcessorEditor (SimpleEqualizerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    for (auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }
    setSize (800, 600);
}

SimpleEqualizerAudioProcessorEditor::~SimpleEqualizerAudioProcessorEditor()
{
}

//==============================================================================
void SimpleEqualizerAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (15.0f));
    g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void SimpleEqualizerAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    auto bounds = getLocalBounds();
    
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.33);
    
    auto highPassArea = bounds.removeFromLeft(bounds.getWidth() * 0.33);
    auto lowPassArea = bounds.removeFromRight(bounds.getWidth() * 0.5);
    
    highPassFreqSlider.setBounds(highPassArea.removeFromTop(highPassArea.getHeight() * 0.5));
    highPassSlopeSlider.setBounds(highPassArea);
    lowPassFreqSlider.setBounds(lowPassArea.removeFromTop(lowPassArea.getHeight() * 0.5));
    lowPassSlopeSlider.setBounds(lowPassArea);
    
    peakFreqSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.33));
    peakGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.5));
    peakQualitySlider.setBounds(bounds);
}

std::vector<juce::Component*> SimpleEqualizerAudioProcessorEditor::getComps()
{
    return
    {
        &highPassFreqSlider,
        &lowPassFreqSlider,
        &peakFreqSlider,
        &peakGainSlider,
        &peakQualitySlider,
        &highPassSlopeSlider,
        &lowPassSlopeSlider
    };
}
