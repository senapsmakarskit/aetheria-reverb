#include "PluginProcessor.h"
#include "PluginEditor.h"

AetheriaAudioProcessor::AetheriaAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout()),
      delayLines(numDelays)
{
    feedbackBuffers.resize(numDelays, 0.0f);
}

AetheriaAudioProcessor::~AetheriaAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout AetheriaAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"decay", 1}, "Decay Time", juce::NormalisableRange<float>(0.1f, 20.0f, 0.1f, 0.5f), 6.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"mix", 1}, "Mix", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 55.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"shimmer", 1}, "Shimmer", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 25.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"damp", 1}, "Damping", juce::NormalisableRange<float>(200.0f, 20000.0f, 1.0f, 0.3f), 7500.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"size", 1}, "Room Size", juce::NormalisableRange<float>(0.1f, 2.0f, 0.01f), 1.2f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"swell", 1}, "Swell Time", juce::NormalisableRange<float>(0.1f, 3.0f, 0.05f), 1.2f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"freeze", 1}, "Freeze", false));

    return { params.begin(), params.end() };
}

void AetheriaAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;

    const float delayTimesMs[numDelays] = { 147.0f, 191.0f, 233.0f, 281.0f };

    for (int i = 0; i < numDelays; ++i)
    {
        delayLines[i].reset();
        delayLines[i].prepare(spec);
        delayLines[i].setMaximumDelayInSamples(static_cast<int>(sampleRate * 3.0));
        delayLines[i].setDelay(static_cast<float>(sampleRate * (delayTimesMs[i] / 1000.0f)));
    }

    lowPassFilter.reset();
    lowPassFilter.prepare(spec);
    lowPassFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);

    highPassFilter.reset();
    highPassFilter.prepare(spec);
    highPassFilter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    highPassFilter.setCutoffFrequency(80.0f);

    swellGain = 1.0f;
    envelope = 0.0f;
}

void AetheriaAudioProcessor::releaseResources() {}

bool AetheriaAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void AetheriaAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    for (const auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();
        if (msg.isController())
        {
            int ccNum = msg.getControllerNumber();
            int ccVal = msg.getControllerValue();

            if (ccNum == 1)
            {
                if (auto* param = apvts.getParameter("shimmer"))
                    param->setValueNotifyingHost(ccVal / 127.0f);
            }
            else if (ccNum == 64)
            {
                if (auto* param = apvts.getParameter("freeze"))
                    param->setValueNotifyingHost(ccVal > 63 ? 1.0f : 0.0f);
            }
        }
    }

    const float decay = apvts.getRawParameterValue("decay")->load();
    const float mix = apvts.getRawParameterValue("mix")->load() / 100.0f;
    const float shimmerAmount = apvts.getRawParameterValue("shimmer")->load() / 100.0f;
    const float dampCutoff = apvts.getRawParameterValue("damp")->load();
    const float sizeScale = apvts.getRawParameterValue("size")->load();
    const float swellTimeSec = apvts.getRawParameterValue("swell")->load();
    const bool freeze = apvts.getRawParameterValue("freeze")->load() > 0.5f;

    lowPassFilter.setCutoffFrequency(dampCutoff);

    const float baseFeedback = freeze ? 1.0f : juce::jlimit(0.0f, 0.98f, 1.0f - (1.0f / (decay + 0.001f)));
    const float currentSampleRate = static_cast<float>(getSampleRate());

    const float delayTimesMs[numDelays] = { 147.0f, 191.0f, 233.0f, 281.0f };
    for (int i = 0; i < numDelays; ++i)
    {
        delayLines[i].setDelay(currentSampleRate * ((delayTimesMs[i] * sizeScale) / 1000.0f));
    }

    const int numSamples = buffer.getNumSamples();
    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

    const float swellAttack = 1.0f / (currentSampleRate * juce::jmax(0.05f, swellTimeSec));
    const float threshold = 0.02f;

    constexpr float twoPi = 6.283185307179586f;

    for (int s = 0; s < numSamples; ++s)
    {
        const float inL = leftChannel[s];
        const float inR = rightChannel ? rightChannel[s] : inL;
        const float rawInputMono = (inL + inR) * 0.5f;

        float absIn = std::abs(rawInputMono);
        if (absIn > threshold && envelope < threshold)
        {
            swellGain = 0.0f;
        }
        envelope = absIn * 0.1f + envelope * 0.9f;

        if (swellGain < 1.0f)
            swellGain += swellAttack;

        const float inputMono = rawInputMono * swellGain;

        float wetL = 0.0f;
        float wetR = 0.0f;

        for (int i = 0; i < numDelays; ++i)
        {
            float delayed = delayLines[i].popSample(0);

            if (shimmerAmount > 0.0f)
            {
                pitchPhase += 0.001f;
                if (pitchPhase >= 1.0f) pitchPhase -= 1.0f;
                float shimmerMod = std::sin(pitchPhase * twoPi);
                delayed += delayed * shimmerMod * shimmerAmount * 0.7f;
            }

            float filtered = lowPassFilter.processSample(0, delayed);
            feedbackBuffers[i] = freeze ? filtered : (inputMono * 0.25f + filtered * baseFeedback);

            delayLines[i].pushSample(0, feedbackBuffers[i]);

            if (i % 2 == 0) wetL += delayed;
            else wetR += delayed;
        }

        wetL *= 0.5f;
        wetR *= 0.5f;

        leftChannel[s] = inL * (1.0f - mix) + wetL * mix;
        if (rightChannel)
            rightChannel[s] = inR * (1.0f - mix) + wetR * mix;
    }
}

juce::AudioProcessorEditor* AetheriaAudioProcessor::createEditor()
{
    return new AetheriaAudioProcessorEditor(*this);
}

void AetheriaAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void AetheriaAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AetheriaAudioProcessor();
}
