/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

enum Slope
{
    Slope_6,
    Slope_12,
    Slope_18,
    Slope_24,
    Slope_30,
    Slope_36
};

struct ChainSettings
{
    float highPassFreq {0}, lowPassFreq {0};
    float peakFreq {0}, peakGainInDecibels {0}, peakQuality {0};
    Slope highPassSlope {Slope::Slope_6}, lowPassSlope {Slope::Slope_6};
};

ChainSettings getChainSettings (juce::AudioProcessorValueTreeState& apvts);

//==============================================================================
/**
*/
class SimpleEqualizerAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    SimpleEqualizerAudioProcessor();
    ~SimpleEqualizerAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts {*this, nullptr, "Parameters", createParameterLayout()};

private:
    using Filter = juce::dsp::IIR::Filter<float>;
    using PassFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter, Filter, Filter>;
    using MonoChain = juce::dsp::ProcessorChain<PassFilter, Filter, PassFilter>;
    
    MonoChain leftChain, rightChain;
    
    enum ChainPositions
    {
        HighPass,
        Peak,
        LowPass
    };
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEqualizerAudioProcessor)
};
