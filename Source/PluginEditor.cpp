/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

void LookAndFeel::drawRotarySlider (juce::Graphics &g,
                                    int x, int y,
                                    int width, int height,
                                    float sliderPosProportional,
                                    float rotaryStartAngle, float rotaryEndAngle,
                                    juce::Slider &slider)
{
    using namespace juce;
    
    auto bounds = Rectangle<float> (x, y, width, height);
    
    g.setColour (Colour (97u, 20u, 160u));
    g.fillEllipse (bounds);
    
    g.setColour (Colour (255u, 160u, 1u));
    g.drawEllipse (bounds, 1.f);
    
    if (auto* rswl = dynamic_cast<RotarySliderWithLabels*> (&slider))
    {
        auto center = bounds.getCentre();
        Path p;
        
        Rectangle<float> r;
        r.setLeft (center.getX() - 2);
        r.setRight (center.getX() + 2);
        r.setTop (bounds.getY());
        r.setBottom (center.getY() - rswl -> getTextHeight() * 2);
        
        p.addRoundedRectangle (r, 2.f);
        
        jassert (rotaryStartAngle < rotaryEndAngle);
        
        auto sliderAngRad = jmap (sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);
        p.applyTransform (AffineTransform().rotated (sliderAngRad, center.getX(), center.getY()));
        
        g.fillPath (p);
        
        g.setFont (rswl -> getTextHeight());
        auto text = rswl -> getDisplayString();
        auto strWidth = g.getCurrentFont().getStringWidth (text);
        
        r.setSize (strWidth + 4, rswl -> getTextHeight() + 2);
        r.setCentre (bounds.getCentre());
        
        g.setColour (Colours::black);
        g.fillRect (r);
        
        g.setColour (Colours::white);
        g.drawFittedText (text, r.toNearestInt(), juce::Justification::centred, 1);
    }
}

void RotarySliderWithLabels::paint (juce::Graphics &g)
{
    using namespace juce;
    
    auto startAng = degreesToRadians (180.f + 45.f);
    auto endAng = degreesToRadians (180.f - 45.f) + MathConstants<float>::twoPi;
    
    auto range = getRange();
    
    auto sliderBounds = getSliderBounds();
    
    g.setColour (Colours::red);
    g.drawRect (getLocalBounds());
    g.setColour (Colours::yellow);
    g.drawRect (sliderBounds);
    
    getLookAndFeel().drawRotarySlider (g,
                                     sliderBounds.getX(), sliderBounds.getY(),
                                     sliderBounds.getWidth(), sliderBounds.getHeight(),
                                     jmap (getValue(), range.getStart(), range.getEnd(), 0.0, 1.0),
                                     startAng, endAng,
                                     *this);
}

juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const
{
    auto bounds = getLocalBounds();
    
    auto size = juce::jmin (bounds.getWidth(), bounds.getHeight());
    size -= getTextHeight() * 2;
    
    juce::Rectangle<int> r;
    r.setSize (size, size);
    r.setCentre (bounds.getCentreX(), 0);
    r.setY (2);
    
    return r;
}

juce::String RotarySliderWithLabels::getDisplayString() const
{
    return juce::String (getValue());
}

//==============================================================================
ResponseCurveComponent::ResponseCurveComponent (SimpleEqualizerAudioProcessor& p) : audioProcessor (p)
{
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param -> addListener (this);
    }
    
    startTimerHz (60);
}

ResponseCurveComponent::~ResponseCurveComponent()
{
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param -> removeListener (this);
    }
}

void ResponseCurveComponent::parameterValueChanged (int parameterIndex, float newValue)
{
    parametersChanged.set (true);
}

void ResponseCurveComponent::timerCallback()
{
    if (parametersChanged.compareAndSetBool (false, true))
    {
        auto chainSettings = getChainSettings (audioProcessor.apvts);
        auto peakCoefficients = makePeakFilter (chainSettings, audioProcessor.getSampleRate());
        updateCoefficients (monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
        
        auto highPassCoefficients = makeHighPassFilter (chainSettings, audioProcessor.getSampleRate());
        auto lowPassCoefficients = makeLowPassFilter (chainSettings, audioProcessor.getSampleRate());
        
        updatePassFilter (monoChain.get<ChainPositions::HighPass>(), highPassCoefficients, chainSettings.highPassSlope);
        updatePassFilter (monoChain.get<ChainPositions::LowPass>(), lowPassCoefficients, chainSettings.lowPassSlope);
        
        repaint();
    }
}

void ResponseCurveComponent::paint (juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (Colours::black);
    
    auto responseArea = getLocalBounds();
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

//==============================================================================
SimpleEqualizerAudioProcessorEditor::SimpleEqualizerAudioProcessorEditor (SimpleEqualizerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
highPassFreqSlider (*audioProcessor.apvts.getParameter ("HighPass Freq"), "Hz"),
highPassSlopeSlider (*audioProcessor.apvts.getParameter ("HighPass Slope"), "dB/Oct"),
lowPassFreqSlider (*audioProcessor.apvts.getParameter ("LowPass Freq"), "Hz"),
lowPassSlopeSlider (*audioProcessor.apvts.getParameter ("LowPass Slope"), "dB/Oct"),
peakFreqSlider (*audioProcessor.apvts.getParameter ("Peak Freq"), "Hz"),
peakGainSlider (*audioProcessor.apvts.getParameter ("Peak Gain"), "dB"),
peakQualitySlider (*audioProcessor.apvts.getParameter ("Peak Quality"), ""),
responseCurveComponent(audioProcessor),
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
    
    setSize (800, 600);
}

SimpleEqualizerAudioProcessorEditor::~SimpleEqualizerAudioProcessorEditor()
{
}

//==============================================================================
void SimpleEqualizerAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (Colours::black);
}

void SimpleEqualizerAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    auto bounds = getLocalBounds();
    
    auto responseArea = bounds.removeFromTop (bounds.getHeight() * 0.33);
    responseCurveComponent.setBounds (responseArea);
    
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
        &lowPassSlopeSlider,
        &responseCurveComponent
    };
}
