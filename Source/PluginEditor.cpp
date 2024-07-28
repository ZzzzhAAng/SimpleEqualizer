/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimpleEqualizerAudioProcessorEditor::SimpleEqualizerAudioProcessorEditor (SimpleEqualizerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
highPassFreqSliderAttachment (audioProcessor.apvts, "HighPass Freq", highPassFreqSlider),
highPassSlopeSliderAttachment (audioProcessor.apvts, "HighPass Slope", highPassSlopeSlider),
lowPassFreqSliderAttachment (audioProcessor.apvts, "LowPass Freq", lowPassFreqSlider),
lowPassSlopeSliderAttachment (audioProcessor.apvts, "LowPass Slope", lowPassSlopeSlider),
peakFreqSliderAttachment (audioProcessor.apvts, "Peak Freq", peakFreqSlider),
peakGainSliderAttachment (audioProcessor.apvts, "Peak Gain", peakGainSlider),
peakQualitySliderAttachment (audioProcessor.apvts, "Peak Quality", peakQualitySlider)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    for (auto* comp : getComps())
    {
        addAndMakeVisible (comp);
    }
    
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param -> addListener(this);
    }
    
    startTimerHz(60);
    
    setSize (800, 600);
}

SimpleEqualizerAudioProcessorEditor::~SimpleEqualizerAudioProcessorEditor()
{
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param -> removeListener(this);
    }
}

//==============================================================================
void SimpleEqualizerAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (Colours::black);
    
    auto bounds = getLocalBounds();
    
    auto responseArea = bounds.removeFromTop (bounds.getHeight() * 0.33);
    auto w = responseArea.getWidth();
    
    auto& highpass = monoChain.get<ChainPositions::HighPass>();
    auto& lowpass = monoChain.get<ChainPositions::LowPass>();
    auto& peak = monoChain.get<ChainPositions::Peak>();
    
    auto sampleRate = audioProcessor.getSampleRate();
    
    std::vector<double> mags;
    mags.resize(w);
    
    for (int i = 0; i < w; ++i)
    {
        double mag = 1.f;
        auto freq = mapToLog10 (double (i) / double (w), 20.0, 20000.0);
        
        if (! monoChain.isBypassed<ChainPositions::Peak>())
        {
            mag *= peak.coefficients->getMagnitudeForFrequency (freq, sampleRate);
        }
        if (!highpass.isBypassed<0>())
            mag *= highpass.get<0>().coefficients->getMagnitudeForFrequency (freq, sampleRate);
        if (!highpass.isBypassed<1>())
            mag *= highpass.get<1>().coefficients->getMagnitudeForFrequency (freq, sampleRate);
        if (!highpass.isBypassed<2>())
            mag *= highpass.get<2>().coefficients->getMagnitudeForFrequency (freq, sampleRate);
        if (!highpass.isBypassed<3>())
            mag *= highpass.get<3>().coefficients->getMagnitudeForFrequency (freq, sampleRate);
        if (!highpass.isBypassed<4>())
            mag *= highpass.get<4>().coefficients->getMagnitudeForFrequency (freq, sampleRate);
        if (! highpass.isBypassed<5>())
            mag *= highpass.get<5>().coefficients->getMagnitudeForFrequency (freq, sampleRate);
        
        if (! lowpass.isBypassed<0>())
            mag *= lowpass.get<0>().coefficients->getMagnitudeForFrequency (freq, sampleRate);
        if (! lowpass.isBypassed<0>())
            mag *= lowpass.get<0>().coefficients->getMagnitudeForFrequency (freq, sampleRate);
        if (! lowpass.isBypassed<0>())
            mag *= lowpass.get<0>().coefficients->getMagnitudeForFrequency (freq, sampleRate);
        if (! lowpass.isBypassed<0>())
            mag *= lowpass.get<0>().coefficients->getMagnitudeForFrequency (freq, sampleRate);
        if (! lowpass.isBypassed<0>())
            mag *= lowpass.get<0>().coefficients->getMagnitudeForFrequency (freq, sampleRate);
        if (! lowpass.isBypassed<0>())
            mag *= lowpass.get<0>().coefficients->getMagnitudeForFrequency (freq, sampleRate);
        
        mags[i] = Decibels::gainToDecibels  (mag);
    }
    
    Path responseCurve;
    
    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();
    auto map = [outputMin, outputMax] (double input)
    {
        return jmap (input, -24.0, 24.0, outputMin, outputMax);
    };
    
    responseCurve.startNewSubPath (responseArea.getX(), map(mags.front()));
    
    for (size_t i = 1; i < mags.size(); ++i)
    {
        responseCurve.lineTo (responseArea.getX() + i, map (mags[i]));
    }
    
    g.setColour (Colours::orange);
    g.drawRoundedRectangle (responseArea.toFloat(), 4.f, 1.f);
    
    g.setColour (Colours::white);
    g.strokePath (responseCurve, PathStrokeType(2.f));
}

void SimpleEqualizerAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    auto bounds = getLocalBounds();
    
    auto responseArea = bounds.removeFromTop (bounds.getHeight() * 0.33);
    
    auto highPassArea = bounds.removeFromLeft (bounds.getWidth() * 0.33);
    auto lowPassArea = bounds.removeFromRight (bounds.getWidth() * 0.5);
    
    highPassFreqSlider.setBounds (highPassArea.removeFromTop (highPassArea.getHeight() * 0.5));
    highPassSlopeSlider.setBounds (highPassArea);
    lowPassFreqSlider.setBounds (lowPassArea.removeFromTop (lowPassArea.getHeight() * 0.5));
    lowPassSlopeSlider.setBounds (lowPassArea);
    
    peakFreqSlider.setBounds (bounds.removeFromTop (bounds.getHeight() * 0.33));
    peakGainSlider.setBounds (bounds.removeFromTop (bounds.getHeight() * 0.5));
    peakQualitySlider.setBounds (bounds);
}

void SimpleEqualizerAudioProcessorEditor::parameterValueChanged (int parameterIndex, float newValue)
{
    parametersChanged.set (true);
}

void SimpleEqualizerAudioProcessorEditor::timerCallback()
{
    if (parametersChanged.compareAndSetBool (false, true))
    {
        auto chainSettings = getChainSettings (audioProcessor.apvts);
        auto peakCoefficients = makePeakFilter (chainSettings, audioProcessor.getSampleRate());
        updateCoefficients (monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
        
        repaint();
    }
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
