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
    
    void updatePeakFilter (const ChainSettings& chainSettings);
    using Coefficients = Filter::CoefficientsPtr;
    static void updateCoefficients (Coefficients& old, const Coefficients& replacements);
    
    template<int Index, typename ChainType, typename CoefficientType>
    void update (ChainType& chain, const CoefficientType& Coefficients)
    {
        updateCoefficients(chain.template get<Index>().coefficients, Coefficients[Index]);
        chain.template setBypassed<Index>(false);
    }
    
    template<typename ChainType, typename CoefficientType>
    void updatePassFilter (ChainType& leftHighPass,
                           const CoefficientType& passCoefficients,
                           const Slope& highPassSlope)
    {
        leftHighPass.template setBypassed<0>(true);
        leftHighPass.template setBypassed<1>(true);
        leftHighPass.template setBypassed<2>(true);
        leftHighPass.template setBypassed<3>(true);
        leftHighPass.template setBypassed<4>(true);
        leftHighPass.template setBypassed<5>(true);
        
        switch (highPassSlope) {
            case Slope_36:
            {
                update<5>(leftHighPass, passCoefficients);
            }
            case Slope_30:
            {
                update<4>(leftHighPass, passCoefficients);
            }
            case Slope_24:
            {
                update<3>(leftHighPass, passCoefficients);
            }
            case Slope_18:
            {
                update<2>(leftHighPass, passCoefficients);
            }
            case Slope_12:
            {
                update<1>(leftHighPass, passCoefficients);
            }
            case Slope_6:
            {
                update<0>(leftHighPass, passCoefficients);
            }
        }
    }
    
    void updateHighPassFilters (const ChainSettings& chainsettings);
    void updateLowPassFilters (const ChainSettings& chainsettings);
    void updateFilters();
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEqualizerAudioProcessor)
};
