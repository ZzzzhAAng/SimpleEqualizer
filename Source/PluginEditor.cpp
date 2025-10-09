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
    
//    g.setColour (Colours::red);
//    g.drawRect (getLocalBounds());
//    g.setColour (Colours::yellow);
//    g.drawRect (sliderBounds);
    
    getLookAndFeel().drawRotarySlider (g,
                                     sliderBounds.getX(), sliderBounds.getY(),
                                     sliderBounds.getWidth(), sliderBounds.getHeight(),
                                     jmap (getValue(), range.getStart(), range.getEnd(), 0.0, 1.0),
                                     startAng, endAng,
                                     *this);
    
    auto center = sliderBounds.toFloat().getCentre();
    auto radius = sliderBounds.getWidth() * 0.5f;
    
    g.setColour (Colour (0u, 127u, 1u));
    g.setFont (getTextHeight());
    
    auto numChoices = labels.size();
    for (int i = 0; i < numChoices; ++i)
    {
        auto pos = labels[i].pos;
        jassert (pos >= 0.f);
        jassert (pos <= 1.f);
        
        auto ang = jmap (pos, 0.f, 1.f, startAng, endAng);
        auto c = center.getPointOnCircumference (radius + getTextHeight() * 0.5 + 1, ang);
        
        Rectangle<float> r;
        auto str = labels[i].label;
        r.setSize (g.getCurrentFont().getStringWidth (str), getTextHeight());
        r.setCentre (c);
        r.setY (r.getY() + getTextHeight());
        
        g.drawFittedText (str, r.toNearestInt(), juce::Justification::centred, 1);
    }
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
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*> (param))
    {
        return choiceParam -> getCurrentChoiceName();
    }
    
    juce::String str;
    bool addK = false;
    
    if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*> (param))
    {
        float val = getValue();
        
        if (val >= 1000.f)
        {
            val /= 1000.f;
            addK = true;
        }
        
        str = juce::String (val, (addK ? 2 : 0));
    }
    else
    {
        // It shouldn't happen.
        jassertfalse;
    }
    
    if (suffix.isNotEmpty())
    {
        str << " ";
        if (addK)
            str << "k";
        
        str << suffix;
    }
    
    return str;
}

//==============================================================================
ResponseCurveComponent::ResponseCurveComponent (SimpleEqualizerAudioProcessor& p) : audioProcessor (p)
{
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param -> addListener (this);
    }
    
    updateChain();
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
        updateChain();
        repaint();
    }
}

void ResponseCurveComponent::updateChain()
{
    auto chainSettings = getChainSettings (audioProcessor.apvts);
    auto peakCoefficients = makePeakFilter (chainSettings, audioProcessor.getSampleRate());
    updateCoefficients (monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
    
    auto highPassCoefficients = makeHighPassFilter (chainSettings, audioProcessor.getSampleRate());
    auto lowPassCoefficients = makeLowPassFilter (chainSettings, audioProcessor.getSampleRate());
    
    updatePassFilter (monoChain.get<ChainPositions::HighPass>(), highPassCoefficients, chainSettings.highPassSlope);
    updatePassFilter (monoChain.get<ChainPositions::LowPass>(), lowPassCoefficients, chainSettings.lowPassSlope);
}

void ResponseCurveComponent::paint (juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (Colours::black);
    
    auto responseArea = getLocalBounds();
    auto w = responseArea.getWidth();
    auto h = responseArea.getHeight();
    
    // 绘制频率网格线
    g.setColour (Colours::darkgrey);
    g.setFont (10.0f);
    
    // 频率标签：20Hz, 100Hz, 1kHz, 10kHz, 20kHz
    std::vector<float> freqLabels = {20.0f, 100.0f, 1000.0f, 10000.0f, 20000.0f};
    for (auto freq : freqLabels)
    {
        auto x = mapToLog10 (freq, 20.0f, 20000.0f) * w;
        g.drawVerticalLine (juce::roundToInt (x), 0, h);
        
        juce::String freqText;
        if (freq >= 1000.0f)
            freqText = juce::String (freq / 1000.0f, 1) + "k";
        else
            freqText = juce::String (freq, 0);
        
        g.drawText (freqText, juce::roundToInt (x) - 20, h - 20, 40, 15, juce::Justification::centred);
    }
    
    // 绘制增益网格线 (-24dB, -12dB, 0dB, +12dB, +24dB)
    std::vector<float> gainLabels = {-24.0f, -12.0f, 0.0f, 12.0f, 24.0f};
    for (auto gain : gainLabels)
    {
        auto y = jmap (gain, -24.0f, 24.0f, (float)h, 0.0f);
        g.drawHorizontalLine (juce::roundToInt (y), 0, w);
        
        juce::String gainText = juce::String (gain, 0) + "dB";
        g.drawText (gainText, 5, juce::roundToInt (y) - 7, 30, 14, juce::Justification::left);
    }
    
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
        if (! lowpass.isBypassed<1>())
            mag *= lowpass.get<1>().coefficients->getMagnitudeForFrequency (freq, sampleRate);
        if (! lowpass.isBypassed<2>())
            mag *= lowpass.get<2>().coefficients->getMagnitudeForFrequency (freq, sampleRate);
        if (! lowpass.isBypassed<3>())
            mag *= lowpass.get<3>().coefficients->getMagnitudeForFrequency (freq, sampleRate);
        if (! lowpass.isBypassed<4>())
            mag *= lowpass.get<4>().coefficients->getMagnitudeForFrequency (freq, sampleRate);
        if (! lowpass.isBypassed<5>())
            mag *= lowpass.get<5>().coefficients->getMagnitudeForFrequency (freq, sampleRate);
        
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
highPassBypassButton ("BYPASS"),
lowPassBypassButton ("BYPASS"),
peakBypassButton ("BYPASS"),
highPassLabel ("High Pass", "High Pass"),
lowPassLabel ("Low Pass", "Low Pass"),
peakLabel ("Peak", "Peak"),
presetComboBox(),
savePresetButton ("Save"),
loadPresetButton ("Load"),
deletePresetButton ("Delete"),
presetNameEditor(),
responseCurveComponent(audioProcessor),
highPassFreqSliderAttachment (audioProcessor.apvts, "HighPass Freq", highPassFreqSlider),
highPassSlopeSliderAttachment (audioProcessor.apvts, "HighPass Slope", highPassSlopeSlider),
lowPassFreqSliderAttachment (audioProcessor.apvts, "LowPass Freq", lowPassFreqSlider),
lowPassSlopeSliderAttachment (audioProcessor.apvts, "LowPass Slope", lowPassSlopeSlider),
peakFreqSliderAttachment (audioProcessor.apvts, "Peak Freq", peakFreqSlider),
peakGainSliderAttachment (audioProcessor.apvts, "Peak Gain", peakGainSlider),
peakQualitySliderAttachment (audioProcessor.apvts, "Peak Quality", peakQualitySlider),
highPassBypassButtonAttachment (audioProcessor.apvts, "HighPass Bypass", highPassBypassButton),
lowPassBypassButtonAttachment (audioProcessor.apvts, "LowPass Bypass", lowPassBypassButton),
peakBypassButtonAttachment (audioProcessor.apvts, "Peak Bypass", peakBypassButton)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    
    highPassFreqSlider.labels.add ({0.f, "20Hz"});
    highPassFreqSlider.labels.add ({1.f, "20kHz"});
    highPassSlopeSlider.labels.add ({0.f, "6"});
    highPassSlopeSlider.labels.add ({1.f, "36"});
    
    lowPassFreqSlider.labels.add ({0.f, "20Hz"});
    lowPassFreqSlider.labels.add ({1.f, "20kHz"});
    lowPassSlopeSlider.labels.add ({0.f, "6"});
    lowPassSlopeSlider.labels.add ({1.f, "36"});
    
    peakFreqSlider.labels.add ({0.f, "20Hz"});
    peakFreqSlider.labels.add ({1.f, "20kHz"});
    peakGainSlider.labels.add ({0.f, "-24dB"});
    peakGainSlider.labels.add ({1.f, "+24dB"});
    peakQualitySlider.labels.add ({0.f, "0.1"});
    peakQualitySlider.labels.add ({1.f, "10.0"});
    
    // 设置标签样式
    highPassLabel.setColour (juce::Label::ColourIds::textColourId, juce::Colours::white);
    highPassLabel.setJustificationType (juce::Justification::centred);
    highPassLabel.setFont (juce::Font (16.0f, juce::Font::bold));
    
    lowPassLabel.setColour (juce::Label::ColourIds::textColourId, juce::Colours::white);
    lowPassLabel.setJustificationType (juce::Justification::centred);
    lowPassLabel.setFont (juce::Font (16.0f, juce::Font::bold));
    
    peakLabel.setColour (juce::Label::ColourIds::textColourId, juce::Colours::white);
    peakLabel.setJustificationType (juce::Justification::centred);
    peakLabel.setFont (juce::Font (16.0f, juce::Font::bold));
    
    // 设置按钮样式
    highPassBypassButton.setColour (juce::ToggleButton::ColourIds::textColourId, juce::Colours::white);
    highPassBypassButton.setColour (juce::ToggleButton::ColourIds::tickColourId, juce::Colours::orange);
    highPassBypassButton.setColour (juce::ToggleButton::ColourIds::tickDisabledColourId, juce::Colours::grey);
    
    lowPassBypassButton.setColour (juce::ToggleButton::ColourIds::textColourId, juce::Colours::white);
    lowPassBypassButton.setColour (juce::ToggleButton::ColourIds::tickColourId, juce::Colours::orange);
    lowPassBypassButton.setColour (juce::ToggleButton::ColourIds::tickDisabledColourId, juce::Colours::grey);
    
    peakBypassButton.setColour (juce::ToggleButton::ColourIds::textColourId, juce::Colours::white);
    peakBypassButton.setColour (juce::ToggleButton::ColourIds::tickColourId, juce::Colours::orange);
    peakBypassButton.setColour (juce::ToggleButton::ColourIds::tickDisabledColourId, juce::Colours::grey);
    
    // 设置预设组件样式
    presetComboBox.setColour (juce::ComboBox::ColourIds::backgroundColourId, juce::Colours::darkgrey);
    presetComboBox.setColour (juce::ComboBox::ColourIds::textColourId, juce::Colours::white);
    presetComboBox.setColour (juce::ComboBox::ColourIds::arrowColourId, juce::Colours::orange);
    
    savePresetButton.setColour (juce::TextButton::ColourIds::buttonColourId, juce::Colours::darkgrey);
    savePresetButton.setColour (juce::TextButton::ColourIds::textColourOffId, juce::Colours::white);
    
    loadPresetButton.setColour (juce::TextButton::ColourIds::buttonColourId, juce::Colours::darkgrey);
    loadPresetButton.setColour (juce::TextButton::ColourIds::textColourOffId, juce::Colours::white);
    
    deletePresetButton.setColour (juce::TextButton::ColourIds::buttonColourId, juce::Colours::darkgrey);
    deletePresetButton.setColour (juce::TextButton::ColourIds::textColourOffId, juce::Colours::white);
    
    presetNameEditor.setColour (juce::TextEditor::ColourIds::backgroundColourId, juce::Colours::darkgrey);
    presetNameEditor.setColour (juce::TextEditor::ColourIds::textColourId, juce::Colours::white);
    presetNameEditor.setText ("New Preset");
    
    // 设置回调
    savePresetButton.addListener (this);
    loadPresetButton.addListener (this);
    deletePresetButton.addListener (this);
    presetComboBox.addListener (this);
    
    updatePresetComboBox();
    
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
    // float hRatio = JUCE_LIVE_CONSTANT (25) / 100.f;
    float hRatio = 25.f / 100.f;

    auto responseArea = bounds.removeFromTop (bounds.getHeight() * hRatio);
    responseCurveComponent.setBounds (responseArea);
    
    bounds.removeFromTop (6);
    
    // 预设区域
    auto presetArea = bounds.removeFromTop (40);
    presetComboBox.setBounds (presetArea.removeFromLeft (presetArea.getWidth() * 0.3));
    presetNameEditor.setBounds (presetArea.removeFromLeft (presetArea.getWidth() * 0.4));
    savePresetButton.setBounds (presetArea.removeFromLeft (presetArea.getWidth() * 0.33));
    loadPresetButton.setBounds (presetArea.removeFromLeft (presetArea.getWidth() * 0.5));
    deletePresetButton.setBounds (presetArea);
    
    bounds.removeFromTop (6);
    
    auto highPassArea = bounds.removeFromLeft (bounds.getWidth() * 0.33);
    auto lowPassArea = bounds.removeFromRight (bounds.getWidth() * 0.5);
    
    // 高通滤波器区域
    highPassLabel.setBounds (highPassArea.removeFromTop (30));
    highPassFreqSlider.setBounds (highPassArea.removeFromTop (highPassArea.getHeight() * 0.35));
    highPassSlopeSlider.setBounds (highPassArea.removeFromTop (highPassArea.getHeight() * 0.6));
    highPassBypassButton.setBounds (highPassArea);
    
    // 低通滤波器区域
    lowPassLabel.setBounds (lowPassArea.removeFromTop (30));
    lowPassFreqSlider.setBounds (lowPassArea.removeFromTop (lowPassArea.getHeight() * 0.35));
    lowPassSlopeSlider.setBounds (lowPassArea.removeFromTop (lowPassArea.getHeight() * 0.6));
    lowPassBypassButton.setBounds (lowPassArea);
    
    // 峰值滤波器区域
    peakLabel.setBounds (bounds.removeFromTop (30));
    peakFreqSlider.setBounds (bounds.removeFromTop (bounds.getHeight() * 0.25));
    peakGainSlider.setBounds (bounds.removeFromTop (bounds.getHeight() * 0.4));
    peakQualitySlider.setBounds (bounds.removeFromTop (bounds.getHeight() * 0.6));
    peakBypassButton.setBounds (bounds);
}

void SimpleEqualizerAudioProcessorEditor::buttonClicked (juce::Button* button)
{
    if (button == &savePresetButton)
    {
        auto presetName = presetNameEditor.getText();
        if (presetName.isNotEmpty())
        {
            audioProcessor.savePreset (presetName);
            updatePresetComboBox();
            presetNameEditor.setText ("");
        }
    }
    else if (button == &loadPresetButton)
    {
        auto selectedPreset = presetComboBox.getText();
        if (selectedPreset.isNotEmpty())
        {
            audioProcessor.loadPreset (selectedPreset);
        }
    }
    else if (button == &deletePresetButton)
    {
        auto selectedPreset = presetComboBox.getText();
        if (selectedPreset.isNotEmpty())
        {
            audioProcessor.deletePreset (selectedPreset);
            updatePresetComboBox();
        }
    }
}

void SimpleEqualizerAudioProcessorEditor::comboBoxChanged (juce::ComboBox* comboBox)
{
    if (comboBox == &presetComboBox)
    {
        auto selectedPreset = presetComboBox.getText();
        if (selectedPreset.isNotEmpty())
        {
            audioProcessor.loadPreset (selectedPreset);
        }
    }
}

void SimpleEqualizerAudioProcessorEditor::updatePresetComboBox()
{
    presetComboBox.clear();
    auto presetNames = audioProcessor.getPresetNames();
    
    for (auto presetName : presetNames)
    {
        presetComboBox.addItem (presetName, presetComboBox.getNumItems() + 1);
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
        &lowPassSlopeSlider,
        &highPassBypassButton,
        &lowPassBypassButton,
        &peakBypassButton,
        &highPassLabel,
        &lowPassLabel,
        &peakLabel,
        &presetComboBox,
        &savePresetButton,
        &loadPresetButton,
        &deletePresetButton,
        &presetNameEditor,
        &responseCurveComponent
    };
}
