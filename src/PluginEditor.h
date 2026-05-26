#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class BendMeterEditor : public juce::AudioProcessorEditor,
                        private juce::Timer
{
public:
    explicit BendMeterEditor(BendMeterProcessor&);
    ~BendMeterEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;

private:
    void timerCallback() override;

    // ── Colors ────────────────────────────────────────────────────────────────
    // colorScheme: 0=bend  1=vivid  2=bw
    // displayMode: 0=metal 1=wood
    struct ColorSet { juce::Colour waveBg, bass, mid, high; };
    ColorSet getWaveColors() const;

    // ── Draw ──────────────────────────────────────────────────────────────────
    void drawRackMode  (juce::Graphics&, bool wood);
    void drawPaintMode (juce::Graphics&);
    void drawWaveformInArea (juce::Graphics&, juce::Rectangle<float>);
    void drawScrew     (juce::Graphics&, float cx, float cy, float radius);

    // ── Rack geometry helpers ─────────────────────────────────────────────────
    struct RackLayout {
        int earW, panelX, panelW, logoW, displayX, displayW, ledsX;
        static constexpr int ledsW = 55;
    };
    RackLayout getRackLayout() const;

    juce::Point<int> rackSchemeDotCenter(int i) const;
    juce::Point<int> rackToggleCenter()         const;
    int  rackSchemeHitTest (juce::Point<int>) const;
    bool rackToggleHit     (juce::Point<int>) const;

    // ── State ─────────────────────────────────────────────────────────────────
    BendMeterProcessor& proc;

    static constexpr int kBufSize = 2048;
    struct Frame { float amp, bass, mid, high; };
    Frame ringBuf[kBufSize]{};
    int   writePos = 0;

    int  colorScheme = 0;   // 0=bend 1=vivid 2=bw
    int  displayMode = 0;   // 0=metal 1=wood 2=paint

    std::unique_ptr<juce::Drawable> logoDrawable;
    juce::Image screwImage;
    juce::Image woodImage;
    juce::Image paintImage;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BendMeterEditor)
};
