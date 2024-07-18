/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimpleEqualizerAudioProcessor::SimpleEqualizerAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

SimpleEqualizerAudioProcessor::~SimpleEqualizerAudioProcessor()
{
}

//==============================================================================
const juce::String SimpleEqualizerAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SimpleEqualizerAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SimpleEqualizerAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SimpleEqualizerAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SimpleEqualizerAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SimpleEqualizerAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SimpleEqualizerAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SimpleEqualizerAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SimpleEqualizerAudioProcessor::getProgramName (int index)
{
    return {};
}

void SimpleEqualizerAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void SimpleEqualizerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;
    spec.sampleRate = sampleRate;
    
    leftChain.prepare(spec);
    rightChain.prepare(spec);
    
    auto chainSettings = getChainSettings(apvts);
    
    updatePeakFilter(chainSettings);
    
    auto passCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.highPassFreq,
                                                                                                        sampleRate,
                                                                                                        2 * (chainSettings.highPassSlope + 1));
    
    auto& leftHighPass = leftChain.get<ChainPositions::HighPass>();
    updatePassFilter(leftHighPass, passCoefficients, chainSettings.highPassSlope);
    
    auto& rightHighPass = rightChain.get<ChainPositions::HighPass>();
    updatePassFilter(rightHighPass, passCoefficients, chainSettings.highPassSlope);
}

void SimpleEqualizerAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SimpleEqualizerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void SimpleEqualizerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    {
        buffer.clear (i, 0, buffer.getNumSamples());
    }
    
    
    auto chainSettings = getChainSettings(apvts);
    
    updatePeakFilter(chainSettings);
    
    auto passCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.highPassFreq,
                                                                                                        getSampleRate(),
                                                                                                        2 * (chainSettings.highPassSlope + 1));
    
    auto& leftHighPass = leftChain.get<ChainPositions::HighPass>();
    updatePassFilter(leftHighPass, passCoefficients, chainSettings.highPassSlope);
    
    auto& rightHighPass = rightChain.get<ChainPositions::HighPass>();
    updatePassFilter(rightHighPass, passCoefficients, chainSettings.highPassSlope);
    
    juce::dsp::AudioBlock<float> block(buffer);
    
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);
    
    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);
    
    leftChain.process(leftContext);
    rightChain.process(rightContext);

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

        // ..do something to the data...
    }
}

//==============================================================================
bool SimpleEqualizerAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SimpleEqualizerAudioProcessor::createEditor()
{
    //return new SimpleEqualizerAudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor (*this);
}

//==============================================================================
void SimpleEqualizerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void SimpleEqualizerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

ChainSettings getChainSettings (juce::AudioProcessorValueTreeState& apvts)
{
    ChainSettings settings;
    using namespace juce;
    
    settings.highPassFreq = apvts.getRawParameterValue("HighPass Freq")->load();
    settings.lowPassFreq = apvts.getRawParameterValue("LowPass Freq")->load();
    settings.peakFreq = apvts.getRawParameterValue("Peak Freq")->load();
    settings.peakGainInDecibels = apvts.getRawParameterValue("Peak Gain")->load();
    settings.peakQuality = apvts.getRawParameterValue("Peak Quality")->load();
    settings.highPassSlope = static_cast<Slope>(apvts.getRawParameterValue("HighPass Slope")->load());
    settings.lowPassSlope = static_cast<Slope>(apvts.getRawParameterValue("LowPass Slope")->load());
    
    return settings;
}

void SimpleEqualizerAudioProcessor::updatePeakFilter(const ChainSettings &chainSettings)
{
    auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
                                                                                chainSettings.peakFreq,
                                                                                chainSettings.peakQuality,
                                                                                juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));
    
    updateCoefficients(leftChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
    updateCoefficients(rightChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
}

void SimpleEqualizerAudioProcessor::updateCoefficients(Coefficients &old, const Coefficients &replacements)
{
    *old = *replacements;
}

juce::AudioProcessorValueTreeState::ParameterLayout SimpleEqualizerAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    using namespace juce;
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(ParameterID{"HighPass Freq", 1},
                                                           "HighPass Freq",
                                                           juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 20.f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(ParameterID{"LowPass Freq", 1},
                                                           "LowPass Freq",
                                                           juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 20000.f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(ParameterID{"Peak Freq", 1},
                                                           "Peak Freq",
                                                           juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 1000.f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(ParameterID{"Peak Gain", 1},
                                                           "Peak Gain",
                                                           juce::NormalisableRange<float>(-24.f, 24.f, 0.1f, 1.f), 0.0f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(ParameterID{"Peak Quality", 1},
                                                           "Peak Quality",
                                                           juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f), 1.f));
    
    juce::StringArray stringArray;
    for (int i = 0; i < 6; ++i)
    {
        juce::String str;
        str << (6 + i * 6);
        str << " db/Oct";
        stringArray.add(str);
    }
    
    layout.add(std::make_unique<juce::AudioParameterChoice>(ParameterID{"HighPass Slope", 1}, "HighPass Slope", stringArray, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>(ParameterID{"LowPass Slope", 1}, "LowPass Slope", stringArray, 0));
    
    layout.add(std::make_unique<juce::AudioParameterBool>(ParameterID{"HighPass Bypass", 1}, "HighPass Bypass", false));
    layout.add(std::make_unique<juce::AudioParameterBool>(ParameterID{"LowPass Bypass", 1}, "LowPass Bypass", false));
    layout.add(std::make_unique<juce::AudioParameterBool>(ParameterID{"Peak Bypass", 1}, "Peak Bypass", false));
    layout.add(std::make_unique<juce::AudioParameterBool>(ParameterID{"Analyzer Enabled", 1}, "Analyzer Enabled", true));
    
    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SimpleEqualizerAudioProcessor();
}
