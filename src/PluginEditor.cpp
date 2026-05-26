#include "PluginEditor.h"
#include <BinaryData.h>

// ── Constructor ───────────────────────────────────────────────────────────────
BendMeterEditor::BendMeterEditor(BendMeterProcessor& p)
    : AudioProcessorEditor(&p), proc(p)
{
    setResizable(true, true);
    setSize(1280, 200);
    setResizeLimits(350, 100, 4096, 800);

    logoDrawable = juce::Drawable::createFromImageData(
        BinaryData::logo_svg, BinaryData::logo_svgSize);

    screwImage = juce::ImageCache::getFromMemory(
        BinaryData::screw_png, BinaryData::screw_pngSize);

    woodImage = juce::ImageCache::getFromMemory(
        BinaryData::wood_jpeg, BinaryData::wood_jpegSize);

    paintImage = juce::ImageCache::getFromMemory(
        BinaryData::paint_jpg, BinaryData::paint_jpgSize);

    startTimerHz(160);
}
BendMeterEditor::~BendMeterEditor() { stopTimer(); }

// ── Layout ────────────────────────────────────────────────────────────────────
BendMeterEditor::RackLayout BendMeterEditor::getRackLayout() const
{
    const int W    = getWidth();
    const int H    = getHeight();
    const int earW = juce::jmin(60, H);
    const int panelX = earW;
    const int panelW = W - earW * 2;
    const int logoW  = juce::jmax(70, panelW * 15 / 100);
    const int displayX = panelX + logoW;
    const int displayW = panelW - logoW - RackLayout::ledsW;
    const int ledsX    = displayX + displayW;
    return { earW, panelX, panelW, logoW, displayX, displayW, ledsX };
}

juce::Point<int> BendMeterEditor::rackSchemeDotCenter(int i) const
{
    const int H = getHeight();
    const auto L = getRackLayout();
    return { L.ledsX + L.ledsW / 2, H * (i + 1) / 5 };
}

juce::Point<int> BendMeterEditor::rackToggleCenter() const
{
    const int H = getHeight();
    const auto L = getRackLayout();
    return { L.ledsX + L.ledsW / 2, H * 4 / 5 };
}

int BendMeterEditor::rackSchemeHitTest(juce::Point<int> p) const
{
    for (int i = 0; i < 3; ++i)
        if (rackSchemeDotCenter(i).getDistanceFrom(p) <= 12) return i;
    return -1;
}

bool BendMeterEditor::rackToggleHit(juce::Point<int> p) const
{
    return rackToggleCenter().getDistanceFrom(p) <= 14;
}

// ── Colors ────────────────────────────────────────────────────────────────────
BendMeterEditor::ColorSet BendMeterEditor::getWaveColors() const
{
    const juce::Colour waveBg = juce::Colours::black;

    switch (colorScheme) {
    case 1:
        return { waveBg, juce::Colour(0xff00ff88), juce::Colour(0xff0088ff), juce::Colour(0xffff00aa) };
    case 2:
        return { waveBg, juce::Colours::white, juce::Colours::white, juce::Colours::white };
    default:
        return { waveBg, juce::Colour(0xffFFD600), juce::Colour(0xffFF8C00), juce::Colour(0xffC0392B) };
    }
}

// ── Mouse ─────────────────────────────────────────────────────────────────────
void BendMeterEditor::mouseDown(const juce::MouseEvent& e)
{
    const auto p = e.getPosition();
    if (displayMode == 2) {
        // paint mode: posiciones fijas sin rack ears
        static constexpr int kLedsW = 55;
        const int ledCx = getWidth() - kLedsW + kLedsW / 2;
        for (int i = 0; i < 3; ++i) {
            if (juce::Point<int>(ledCx, getHeight() * (i + 1) / 5).getDistanceFrom(p) <= 12) {
                colorScheme = i; repaint(); return;
            }
        }
        if (juce::Point<int>(ledCx, getHeight() * 4 / 5).getDistanceFrom(p) <= 14) {
            displayMode = (displayMode + 1) % 3; repaint();
        }
        return;
    }
    const int hit = rackSchemeHitTest(p);
    if (hit >= 0)         { colorScheme = hit; repaint(); return; }
    if (rackToggleHit(p)) { displayMode = (displayMode + 1) % 3; repaint(); }
}

// ── Timer ─────────────────────────────────────────────────────────────────────
void BendMeterEditor::timerCallback()
{
    ringBuf[writePos] = {
        proc.analysis.amp .load(),
        proc.analysis.bass.load(),
        proc.analysis.mid .load(),
        proc.analysis.high.load()
    };
    writePos = (writePos + 1) % kBufSize;
    repaint();
}

// ── Paint ─────────────────────────────────────────────────────────────────────
void BendMeterEditor::paint(juce::Graphics& g)
{
    if (displayMode == 2) { drawPaintMode(g); return; }
    drawRackMode(g, displayMode == 1);
}
void BendMeterEditor::resized() {}

// ── Waveform ──────────────────────────────────────────────────────────────────
void BendMeterEditor::drawWaveformInArea(juce::Graphics& g, juce::Rectangle<float> area)
{
    const auto cs    = getWaveColors();
    const float W    = area.getWidth();
    const float H    = area.getHeight();
    const float offY = area.getY();

    g.setColour(cs.waveBg);
    g.fillRect(area);

    const bool isBw = (colorScheme == 2);
    const int  cols = (int)W;

    for (int col = 0; col < cols; ++col)
    {
        const int age = cols - 1 - col;
        const int idx = ((writePos - 1 - age) % kBufSize + kBufSize) % kBufSize;
        const Frame& fr = ringBuf[idx];
        if (fr.amp < 1e-4f) continue;

        const float barH = fr.amp * H;
        const float top  = (H - barH) * 0.5f;
        const float x    = area.getX() + (float)col;

        if (isBw) {
            g.setColour(cs.bass);
            g.fillRect(x, offY + top, 1.0f, barH);
        } else {
            const float bassH = barH * fr.bass;
            const float midH  = barH * fr.mid;
            const float highH = barH * fr.high;
            g.setColour(cs.bass);
            g.fillRect(x, offY + top + barH - bassH, 1.0f, bassH);
            g.setColour(cs.mid);
            g.fillRect(x, offY + top + barH - bassH - midH, 1.0f, midH);
            g.setColour(cs.high);
            g.fillRect(x, offY + top, 1.0f, highH);
        }
    }
}

// ── Tornillo ──────────────────────────────────────────────────────────────────
void BendMeterEditor::drawScrew(juce::Graphics& g, float cx, float cy, float radius)
{
    if (screwImage.isValid()) {
        juce::Graphics::ScopedSaveState ss(g);
        juce::Path clip;
        clip.addEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
        g.reduceClipRegion(clip);
        g.drawImage(screwImage,
                    (int)(cx - radius), (int)(cy - radius),
                    (int)(radius * 2.0f), (int)(radius * 2.0f),
                    0, 0, screwImage.getWidth(), screwImage.getHeight());
    } else {
        // fallback si no carga la imagen
        g.setColour(juce::Colour(0xff909090));
        g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
        g.setColour(juce::Colour(0xff555555));
        g.drawLine(cx - radius * 0.6f, cy, cx + radius * 0.6f, cy, radius * 0.2f);
    }
}

// ── Rack mode (metal y madera comparten estructura) ───────────────────────────
void BendMeterEditor::drawRackMode(juce::Graphics& g, bool wood)
{
    const int W    = getWidth();
    const int H    = getHeight();
    const auto L   = getRackLayout();
    const float sr = juce::jmax(6.0f, (float)H * 0.075f); // radio tornillo

    // ── Fondo principal ───────────────────────────────────────────────────────
    if (wood && woodImage.isValid()) {
        // textura de madera estirada + barniz oscuro encima
        g.drawImage(woodImage, 0, 0, W, H,
                    0, 0, woodImage.getWidth(), woodImage.getHeight());
        g.setColour(juce::Colour(0x55000000));
        g.fillRect(0, 0, W, H);
    } else {
        // metal: gradiente gris
        juce::ColourGradient metalGrad(
            juce::Colour(0xff3c3c3c), 0.0f, 0.0f,
            juce::Colour(0xff1e1e1e), 0.0f, (float)H, false);
        metalGrad.addColour(0.45, juce::Colour(0xff2a2a2a));
        metalGrad.addColour(0.55, juce::Colour(0xff262626));
        g.setGradientFill(metalGrad);
        g.fillRect(0, 0, W, H);
        // textura de líneas horizontales (brushed aluminium)
        g.setColour(juce::Colour(0x08ffffff));
        for (int y = 0; y < H; y += 2)
            g.drawHorizontalLine(y, 0.0f, (float)W);
    }

    // ── Rack ears ─────────────────────────────────────────────────────────────
    auto drawEar = [&](int x, int w) {
        const bool isLeft = (x == 0);

        if (wood && woodImage.isValid()) {
            // ear: misma textura pero más oscura
            {
                juce::Graphics::ScopedSaveState ss(g);
                g.reduceClipRegion(x, 0, w, H);
                g.drawImage(woodImage, x, 0, w, H,
                            x * woodImage.getWidth() / W, 0,
                            w * woodImage.getWidth() / W, woodImage.getHeight());
            }
            g.setColour(juce::Colour(0x88000000));
            g.fillRect(x, 0, w, H);
        } else {
            juce::ColourGradient earGrad(
                juce::Colour(0xff1c1c1c), (float)x,       0.0f,
                juce::Colour(0xff131313), (float)(x + w), 0.0f, false);
            g.setGradientFill(earGrad);
            g.fillRect(x, 0, w, H);
        }

        // borde interior del ear
        const int bx = isLeft ? x + w - 1 : x;
        g.setColour(juce::Colour(0xff050505));
        g.drawVerticalLine(bx, 0.0f, (float)H);
        g.setColour(wood ? juce::Colour(0xff5a3a1a) : juce::Colour(0xff404040));
        g.drawVerticalLine(bx + (isLeft ? 1 : -1), 0.0f, (float)H);

        // tornillos
        drawScrew(g, (float)(x + w / 2), (float)(H / 4),      sr);
        drawScrew(g, (float)(x + w / 2), (float)(H * 3 / 4),  sr);
    };

    drawEar(0, L.earW);
    drawEar(W - L.earW, L.earW);

    // ── Separadores logo | display ────────────────────────────────────────────
    g.setColour(juce::Colour(0xff050505));
    g.drawVerticalLine(L.displayX - 1, 2.0f, (float)(H - 2));
    g.setColour(wood ? juce::Colour(0xff6a4020) : juce::Colour(0xff3a3a3a));
    g.drawVerticalLine(L.displayX, 2.0f, (float)(H - 2));

    // ── Logo ──────────────────────────────────────────────────────────────────
    if (logoDrawable) {
        const int sz = juce::jmin(L.logoW - 12, H - 12);
        const int lx = L.panelX + (L.logoW - sz) / 2;
        const int ly = (H - sz) / 2;
        logoDrawable->setTransformToFit(
            juce::Rectangle<float>((float)lx, (float)ly, (float)sz, (float)sz),
            juce::RectanglePlacement::centred);
        logoDrawable->draw(g, 1.0f);
    }

    // ── Display inset ─────────────────────────────────────────────────────────
    const int dvM = juce::jmax(6, H * 10 / 100);
    const int dhM = 10;
    const juce::Rectangle<int> dispRect(L.displayX + dhM, dvM,
                                        L.displayW - dhM * 2, H - dvM * 2);

    // sombra de profundidad
    for (int i = 3; i >= 0; --i) {
        g.setColour(juce::Colour(0xff000000).withAlpha(0.4f - i * 0.08f));
        g.drawRoundedRectangle(dispRect.expanded(i).toFloat(), 3.0f, 1.0f);
    }

    // bisel: oscuro arriba/izq, claro abajo/der
    {
        const auto df = dispRect.expanded(1).toFloat();
        g.setColour(juce::Colour(0xff050505));
        g.drawLine(df.getX(), df.getY(), df.getRight(), df.getY(), 1.5f);
        g.drawLine(df.getX(), df.getY(), df.getX(), df.getBottom(), 1.5f);
        g.setColour(wood ? juce::Colour(0xff7a5030) : juce::Colour(0xff484848));
        g.drawLine(df.getX(), df.getBottom(), df.getRight(), df.getBottom(), 1.0f);
        g.drawLine(df.getRight(), df.getY(), df.getRight(), df.getBottom(), 1.0f);
    }

    // waveform
    g.setColour(juce::Colours::black);
    g.fillRect(dispRect);
    {
        juce::Graphics::ScopedSaveState ss(g);
        g.reduceClipRegion(dispRect);
        drawWaveformInArea(g, dispRect.toFloat());
    }

    // reflejo de vidrio superior
    {
        const auto dr = dispRect.toFloat();
        const juce::Rectangle<float> glassR(dr.getX(), dr.getY(),
                                             dr.getWidth(), dr.getHeight() * 0.18f);
        g.setGradientFill(juce::ColourGradient(
            juce::Colours::white.withAlpha(0.08f), glassR.getX(), glassR.getY(),
            juce::Colours::transparentWhite,        glassR.getX(), glassR.getBottom(), false));
        g.fillRect(glassR);
    }

    // label "BendMeter"
    if (dvM >= 12) {
        g.setFont(juce::Font(juce::FontOptions().withHeight((float)dvM * 0.58f)));
        g.setColour(wood ? juce::Colour(0xffb08060) : juce::Colour(0xff686868));
        g.drawText("BendMeter",
                   juce::Rectangle<float>((float)(L.displayX + dhM), 0.0f,
                                          (float)(L.displayW - dhM), (float)dvM),
                   juce::Justification::centredRight, false);
    }

    // ── Separador display | LEDs ──────────────────────────────────────────────
    g.setColour(juce::Colour(0xff050505));
    g.drawVerticalLine(L.ledsX, 2.0f, (float)(H - 2));
    g.setColour(wood ? juce::Colour(0xff6a4020) : juce::Colour(0xff3a3a3a));
    g.drawVerticalLine(L.ledsX + 1, 2.0f, (float)(H - 2));

    // ── LEDs de color scheme ──────────────────────────────────────────────────
    const juce::Colour ledFill[3] = {
        juce::Colour(0xffFFD600), juce::Colour(0xff00ff88), juce::Colours::white,
    };

    for (int i = 0; i < 3; ++i) {
        const auto c   = rackSchemeDotCenter(i);
        const float lr = 5.5f;
        const auto lr2 = juce::Rectangle<float>((float)c.x - lr, (float)c.y - lr, lr * 2.0f, lr * 2.0f);

        if (i == colorScheme) {
            g.setColour(ledFill[i].withAlpha(0.18f));
            g.fillEllipse(lr2.expanded(5.0f));
            g.setColour(ledFill[i].withAlpha(0.35f));
            g.fillEllipse(lr2.expanded(2.5f));
            g.setColour(ledFill[i]);
            g.fillEllipse(lr2);
            g.setColour(juce::Colours::white.withAlpha(0.55f));
            g.fillEllipse(lr2.reduced(lr * 0.45f));
        } else {
            g.setColour(ledFill[i].withAlpha(0.12f));
            g.fillEllipse(lr2);
            g.setColour(ledFill[i].withAlpha(0.35f));
            g.drawEllipse(lr2, 0.7f);
        }
    }

    // ── Toggle de modo (metal ↔ madera) ───────────────────────────────────────
    {
        const auto c  = rackToggleCenter();
        const float tx = (float)c.x;
        const float ty = (float)c.y;

        if (wood) {
            // ícono de veta de madera: 3 líneas onduladas
            g.setColour(juce::Colour(0xffc09060));
            for (int l = 0; l < 3; ++l) {
                const float ly = ty - 4.0f + l * 4.0f;
                juce::Path wave;
                wave.startNewSubPath(tx - 8.0f, ly);
                wave.cubicTo(tx - 4.0f, ly - 2.0f, tx + 4.0f, ly + 2.0f, tx + 8.0f, ly);
                g.strokePath(wave, juce::PathStrokeType(1.1f));
            }
        } else {
            // ícono de rack: 3 líneas rectas
            g.setColour(juce::Colour(0xff909090));
            for (int l = 0; l < 3; ++l)
                g.drawLine(tx - 8.0f, ty - 4.0f + l * 4.0f,
                           tx + 8.0f, ty - 4.0f + l * 4.0f, 1.2f);
        }
        g.setColour(juce::Colour(0x22ffffff));
        g.drawRoundedRectangle(tx - 10.0f, ty - 8.0f, 20.0f, 16.0f, 2.0f, 0.7f);
    }
}

// ── Paint mode — fondo de pinceladas, sin rack ears ───────────────────────────
void BendMeterEditor::drawPaintMode(juce::Graphics& g)
{
    const int W = getWidth();
    const int H = getHeight();
    static constexpr int kLogoW = 100;
    static constexpr int kLedsW = 55;
    const int displayX = kLogoW;
    const int displayW = W - kLogoW - kLedsW;
    const int ledsX    = displayX + displayW;

    // fondo: imagen de pintura estirada
    if (paintImage.isValid())
        g.drawImage(paintImage, 0, 0, W, H,
                    0, 0, paintImage.getWidth(), paintImage.getHeight());
    else {
        g.setColour(juce::Colour(0xfffffaec));
        g.fillRect(0, 0, W, H);
    }

    // separador logo | display
    g.setColour(juce::Colour(0x88000000));
    g.drawVerticalLine(displayX - 1, 0.0f, (float)H);

    // logo
    if (logoDrawable) {
        const int sz = juce::jmin(kLogoW - 10, H - 10);
        const int lx = (kLogoW - sz) / 2;
        const int ly = (H - sz) / 2;
        logoDrawable->setTransformToFit(
            juce::Rectangle<float>((float)lx, (float)ly, (float)sz, (float)sz),
            juce::RectanglePlacement::centred);
        logoDrawable->draw(g, 1.0f);
    }

    // display inset con fondo negro semi-transparente para contraste del waveform
    const int dvM = juce::jmax(6, H * 8 / 100);
    const int dhM = 10;
    const juce::Rectangle<int> dispRect(displayX + dhM, dvM,
                                        displayW - dhM * 2, H - dvM * 2);

    // sombra exterior
    for (int i = 3; i >= 0; --i) {
        g.setColour(juce::Colour(0xff000000).withAlpha(0.35f - i * 0.07f));
        g.drawRoundedRectangle(dispRect.expanded(i).toFloat(), 3.0f, 1.0f);
    }

    // borde del display: blanco/crema para que contraste con el fondo colorido
    {
        const auto df = dispRect.expanded(1).toFloat();
        g.setColour(juce::Colour(0xfffaf0dc));
        g.drawLine(df.getX(), df.getY(), df.getRight(), df.getY(), 2.0f);
        g.drawLine(df.getX(), df.getY(), df.getX(), df.getBottom(), 2.0f);
        g.setColour(juce::Colour(0xffd4c8a8));
        g.drawLine(df.getX(), df.getBottom(), df.getRight(), df.getBottom(), 1.5f);
        g.drawLine(df.getRight(), df.getY(), df.getRight(), df.getBottom(), 1.5f);
    }

    // waveform
    g.setColour(juce::Colours::black);
    g.fillRect(dispRect);
    {
        juce::Graphics::ScopedSaveState ss(g);
        g.reduceClipRegion(dispRect);
        drawWaveformInArea(g, dispRect.toFloat());
    }

    // reflejo de vidrio
    {
        const auto dr = dispRect.toFloat();
        g.setGradientFill(juce::ColourGradient(
            juce::Colours::white.withAlpha(0.10f), dr.getX(), dr.getY(),
            juce::Colours::transparentWhite,        dr.getX(), dr.getY() + dr.getHeight() * 0.2f, false));
        g.fillRect(dr.withHeight(dr.getHeight() * 0.2f));
    }

    // separador display | LEDs
    g.setColour(juce::Colour(0x88000000));
    g.drawVerticalLine(ledsX, 0.0f, (float)H);

    // LEDs
    const juce::Colour ledFill[3] = {
        juce::Colour(0xffFFD600), juce::Colour(0xff00ff88), juce::Colours::white,
    };
    const int ledCx = ledsX + kLedsW / 2;

    for (int i = 0; i < 3; ++i) {
        const float lr  = 5.5f;
        const float lcy = (float)(H * (i + 1) / 5);
        const auto lr2  = juce::Rectangle<float>((float)ledCx - lr, lcy - lr, lr * 2.0f, lr * 2.0f);

        if (i == colorScheme) {
            g.setColour(ledFill[i].withAlpha(0.25f));
            g.fillEllipse(lr2.expanded(5.0f));
            g.setColour(ledFill[i].withAlpha(0.45f));
            g.fillEllipse(lr2.expanded(2.5f));
            g.setColour(ledFill[i]);
            g.fillEllipse(lr2);
            g.setColour(juce::Colours::white.withAlpha(0.6f));
            g.fillEllipse(lr2.reduced(lr * 0.45f));
        } else {
            g.setColour(ledFill[i].withAlpha(0.18f));
            g.fillEllipse(lr2);
            g.setColour(ledFill[i].withAlpha(0.5f));
            g.drawEllipse(lr2, 0.8f);
        }
    }

    // toggle — círculo dividido en 4 colores (ícono pintura)
    {
        const float tx = (float)ledCx;
        const float ty = (float)(H * 4 / 5);
        const float pr = 7.0f;
        const auto rc  = juce::Rectangle<float>(tx - pr, ty - pr, pr * 2.0f, pr * 2.0f);
        const juce::Colour cols[4] = {
            juce::Colour(0xffFF3B30), juce::Colour(0xff34C759),
            juce::Colour(0xff007AFF), juce::Colour(0xffFFCC00),
        };
        for (int s = 0; s < 4; ++s) {
            juce::Path sector;
            sector.addPieSegment(rc,
                s * juce::MathConstants<float>::halfPi,
                (s + 1) * juce::MathConstants<float>::halfPi, 0.0f);
            g.setColour(cols[s]);
            g.fillPath(sector);
        }
        g.setColour(juce::Colour(0x66000000));
        g.drawEllipse(rc, 1.0f);
    }
}
