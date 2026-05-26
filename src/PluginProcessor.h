#pragma once
#include <JuceHeader.h>

class BendMeterProcessor : public juce::AudioProcessor
{
public:
    BendMeterProcessor();
    ~BendMeterProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "BendMeter"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

    // Analysis data — written by processBlock, read by editor
    struct AnalysisData {
        std::atomic<float> amp  { 0.0f };
        std::atomic<float> bass { 0.33f };
        std::atomic<float> mid  { 0.33f };
        std::atomic<float> high { 0.33f };
    };
    AnalysisData analysis;

private:
    struct Biquad {
        double b0=0,b1=0,b2=0,a1=0,a2=0,s1=0,s2=0;
        void reset() { s1=s2=0; }
        float process(float x) {
            double y = b0*x + s1;
            s1 = b1*x - a1*y + s2;
            s2 = b2*x - a2*y;
            return (float)y;
        }
    };
    void designLP(Biquad& f, double fc, double fs, double Q=0.707);
    void designHP(Biquad& f, double fc, double fs, double Q=0.707);

    Biquad fBassLP, fHighHP;
    float chunkMaxAmp=0, chunkBassE=0, chunkMidE=0, chunkHighE=0;
    int   downsampleRate=256, downsampleCounter=0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BendMeterProcessor)
};
