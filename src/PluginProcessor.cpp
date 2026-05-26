#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

BendMeterProcessor::BendMeterProcessor()
    : AudioProcessor(BusesProperties()
        .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{}

BendMeterProcessor::~BendMeterProcessor() {}

void BendMeterProcessor::designLP(Biquad& f, double fc, double fs, double Q)
{
    double w0 = 2.0*M_PI*fc/fs, cosW = std::cos(w0);
    double alpha = std::sin(w0)/(2.0*Q), a0 = 1.0+alpha;
    f.b0 = (1.0-cosW)/2.0/a0;  f.b1 = (1.0-cosW)/a0;  f.b2 = f.b0;
    f.a1 = -2.0*cosW/a0;        f.a2 = (1.0-alpha)/a0;
    f.reset();
}

void BendMeterProcessor::designHP(Biquad& f, double fc, double fs, double Q)
{
    double w0 = 2.0*M_PI*fc/fs, cosW = std::cos(w0);
    double alpha = std::sin(w0)/(2.0*Q), a0 = 1.0+alpha;
    f.b0 =  (1.0+cosW)/2.0/a0;  f.b1 = -(1.0+cosW)/a0;  f.b2 = f.b0;
    f.a1 = -2.0*cosW/a0;         f.a2 = (1.0-alpha)/a0;
    f.reset();
}

void BendMeterProcessor::prepareToPlay(double sampleRate, int)
{
    designLP(fBassLP, 250.0,  sampleRate);
    designHP(fHighHP, 4000.0, sampleRate);
    chunkMaxAmp = chunkBassE = chunkMidE = chunkHighE = 0.0f;
    downsampleCounter = 0;
}

void BendMeterProcessor::releaseResources() {}

void BendMeterProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    const int frames = buffer.getNumSamples();
    const float* L = buffer.getReadPointer(0);
    const float* R = buffer.getNumChannels() > 1 ? buffer.getReadPointer(1) : L;

    for (int i = 0; i < frames; ++i)
    {
        const float s    = (L[i] + R[i]) * 0.5f;
        const float sAbs = s < 0.0f ? -s : s;

        const float bass = fBassLP.process(s);
        const float high = fHighHP.process(s);
        const float mid  = s - bass - high;

        if (sAbs > chunkMaxAmp) chunkMaxAmp = sAbs;
        chunkBassE += bass < 0.0f ? -bass : bass;
        chunkMidE  += mid  < 0.0f ? -mid  : mid;
        chunkHighE += high < 0.0f ? -high : high;

        if (++downsampleCounter >= downsampleRate) {
            downsampleCounter = 0;
            float total = chunkBassE + chunkMidE + chunkHighE;
            if (total < 1e-6f) total = 1.0f;

            analysis.amp .store(chunkMaxAmp > 1.0f ? 1.0f : chunkMaxAmp);
            analysis.bass.store(chunkBassE / total);
            analysis.mid .store(chunkMidE  / total);
            analysis.high.store(chunkHighE / total);

            chunkMaxAmp = chunkBassE = chunkMidE = chunkHighE = 0.0f;
        }
    }
}

juce::AudioProcessorEditor* BendMeterProcessor::createEditor()
{
    return new BendMeterEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BendMeterProcessor();
}
