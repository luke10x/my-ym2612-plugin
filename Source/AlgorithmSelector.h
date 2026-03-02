#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// =============================================================================
// Algorithm diagram drawing
// =============================================================================
// Op colours matching the column accent colours
static const juce::Colour kOpColours[4] = {
    juce::Colour(0xffff6b35),  // OP1 orange
    juce::Colour(0xffffe135),  // OP2 yellow
    juce::Colour(0xff35ff8f),  // OP3 green
    juce::Colour(0xff9b59ff),  // OP4 purple
};

static void drawAlgorithm (juce::Graphics& g, int algo, juce::Rectangle<int> area)
{
    juce::Colour lineCol  (0xFFAAAAAA);
    juce::Colour outputCol(0xFF00D4AA);
    const float opSize = 12.f;

    // Natural canvas height: tallest algos (5,6) use y fractions 0.25..0.75.
    // Content pixel span = 0.5*uh + opSize. Set uh=36 → span=18+16=34, naturalH=34+10=44px.
    // This is the minimum height that fits all diagrams; the component can be taller —
    // the canvas is centred vertically inside whatever area is passed.
    const float naturalH = 64.f;
    const float naturalW = (float)area.getWidth();
    const float canvasY  = area.getY() + (area.getHeight() - naturalH) * 0.5f;
    const float mg = 1.f;
    const float uw = naturalW - mg * 3.f;
    const float uh = naturalH - mg * 4.f;   // = 36px
    const float bx = area.getX() + mg;
    const float by = canvasY + mg;

    auto drawOp = [&](float cx, float cy, int num, bool fb = false)
    {
        const juce::Colour opCol = kOpColours[juce::jlimit(0, 3, num - 1)];
        g.setColour(opCol.withAlpha(0.18f));
        g.fillRect(cx-opSize/2, cy-opSize/2, opSize, opSize);
        g.setColour(opCol);
        g.drawRect(cx-opSize/2, cy-opSize/2, opSize, opSize, 1.f);
        if (fb && num == 1) {
            // Small feedback loop frame — just slightly outside the box
            float ls = opSize * 0.62f;
            juce::Path lp;
            lp.addRoundedRectangle(cx-ls, cy-ls, ls*2.f, ls*2.f, 3.f);
            g.setColour(lineCol);
            g.strokePath(lp, juce::PathStrokeType(1.2f));
        }
        g.setColour(opCol);
        g.setFont(juce::Font("Courier New", 10.f, juce::Font::bold));
        g.drawText(juce::String(num), cx-opSize/2, cy-opSize/2, opSize, opSize,
                   juce::Justification::centred, false);
    };
    auto ln = [&](float x1,float y1,float x2,float y2){
        g.setColour(lineCol); g.drawLine(x1,y1,x2,y2,1.5f);
    };
    auto out = [&](float x, float y, float x2, float y2){
        g.setColour(lineCol); g.drawLine(x,y,x2,y2,1.5f);
        g.setColour(outputCol); g.fillEllipse(x2-4,y2-4,8,8);
    };

    switch (algo) {
    case 0:{float sp=uw/4.8f,y=by+uh/2,x1=bx+sp*.5f,x2=bx+sp*1.5f,x3=bx+sp*2.5f,x4=bx+sp*3.5f,x5=bx+sp*4.2f;
        ln(x1+opSize/2,y,x2-opSize/2,y);ln(x2+opSize/2,y,x3-opSize/2,y);
        ln(x3+opSize/2,y,x4-opSize/2,y);out(x4+opSize/2,y, x5+opSize/2, y);
        drawOp(x1,y,1,true);drawOp(x2,y,2);drawOp(x3,y,3);drawOp(x4,y,4);break;}
    case 1:{float x1=bx+uw*.20f,x3=bx+uw*.40f,x4=bx+uw*.60f, x5=bx+uw*0.75f, y1=by+uh*.3f,y2=by+uh*.7f,ym=by+uh*.5f;
        ln(x1+opSize/2,y1,x3-opSize/2,ym);ln(x1+opSize/2,y2,x3-opSize/2,ym);
        ln(x3+opSize/2,ym,x4-opSize/2,ym);out(x4+opSize/2,ym, x5+opSize/2, ym);
        drawOp(x1,y1,1,true);drawOp(x1,y2,2);drawOp(x3,ym,3);drawOp(x4,ym,4);break;}
    case 2:{float x1=bx+uw*.20f,x3=bx+uw*.40f,x4=bx+uw*.60f, x5=bx+uw*.75f, y1=by+uh*.3f,y2=by+uh*.7f,ym=by+uh*.5f;
        ln(x1+opSize/2,y1,x3+opSize/2,y1);
        ln(x3+opSize/2,y1,x4-opSize/2,ym);
        ln(x1+opSize/2,y2,x3-opSize/2,y2);
        ln(x3+opSize/2,y2,x4-opSize/2,ym);
        out(x4+opSize/2,ym, x5+opSize/2, ym);
        drawOp(x1,y1,1,true);drawOp(x1,y2,2);drawOp(x3,y2,3);drawOp(x4,ym,4);break;}
    case 3:{float x1=bx+uw*.2f,x2=bx+uw*.4f,x4=bx+uw*.6f, x5=bx+uw*.75f, y1=by+uh*.3f,y3=by+uh*.7f,ym=by+uh*.5f;
        ln(x1+opSize/2,y1,x2-opSize/2,y1);
        ln(x2+opSize/2,y1,x4-opSize/2,ym);
        ln(x1+opSize/2,y3,x2+opSize/2,y3);
        ln(x2+opSize/2,y3,x4-opSize/2,ym);
        out(x4+opSize/2,ym, x5+opSize/2, ym);
        drawOp(x1,y1,1,true);drawOp(x2,y1,2);drawOp(x1,y3,3);drawOp(x4,ym,4);break;}
    case 4:{float x1=bx+uw*.2f,x2=bx+uw*.5f,x3=bx+uw*.75f, y1=by+uh*.33f,ym=by+uh*.5f, y2=by+uh*.66f;
        ln(x1+opSize/2,y1,x2-opSize/2,y1);out(x2+opSize/2,y1, x3+opSize/2, ym);
        ln(x1+opSize/2,y2,x2-opSize/2,y2);out(x2+opSize/2,y2, x3+opSize/2, ym);
        drawOp(x1,y1,1,true);drawOp(x2,y1,2);drawOp(x1,y2,3);drawOp(x2,y2,4);break;}
    case 5:{float x1=bx+uw*.25f,x2=bx+uw*.5f, x3=bx+uw*.75f, y1=by+uh*.25f,y2=by+uh*.5f,y3=by+uh*.75f;
        ln(x1+opSize/2,y2,x2-opSize/2,y1);out(x2+opSize/2,y1, x3+opSize/2, y2);
        ln(x1+opSize/2,y2,x2-opSize/2,y2);out(x2+opSize/2,y2, x3+opSize/2, y2);
        ln(x1+opSize/2,y2,x2-opSize/2,y3);out(x2+opSize/2,y3, x3+opSize/2, y2);
        drawOp(x1,y2,1,true);drawOp(x2,y1,2);drawOp(x2,y2,3);drawOp(x2,y3,4);break;}
    case 6:{float x1=bx+uw*.25f,x2=bx+uw*.5f, x3=bx+uw*.75f, y1=by+uh*.25f,y2=by+uh*.5f,y3=by+uh*.75f;
        ln(x1+opSize/2,y1,x2-opSize/2,y1);out(x2+opSize/2,y1, x3+opSize/2, y2);
        out(x2+opSize/2,y2, x3+opSize/2, y2);out(x2+opSize/2,y3, x3+opSize/2, y2 );
        drawOp(x1,y1,1,true);drawOp(x2,y1,2);drawOp(x2,y2,3);drawOp(x2,y3,4);break;}
    case 7:{float sp=uw/5.f,y=by+uh*.4f,yOut=by+uh*.75f,xOut=(float)area.getCentreX();
        float x1=bx+sp,x2=bx+sp*2,x3=bx+sp*3,x4=bx+sp*4;
        ln(x1,y+opSize/2,xOut,yOut);ln(x2,y+opSize/2,xOut,yOut);
        ln(x3,y+opSize/2,xOut,yOut);ln(x4,y+opSize/2,xOut,yOut);
        g.setColour(outputCol);g.fillEllipse(xOut-4,yOut-4,8,8);
        drawOp(x1,y,1,true);drawOp(x2,y,2);drawOp(x3,y,3);drawOp(x4,y,4);break;}
    default:break;
    }
}

// =============================================================================
// AlgorithmPickerPanel  — the 4x2 grid of algorithm diagrams
// =============================================================================
class AlgorithmPickerPanel : public juce::Component
{
public:
    int selectedAlgo = 0;
    int hoveredAlgo  = -1;
    std::function<void(int)> onSelect;

    AlgorithmPickerPanel() { setInterceptsMouseClicks(true, false); }

    void paint (juce::Graphics& g) override
    {
        g.setColour(juce::Colour(0xf51a1a2e));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 10.f);
        g.setColour(juce::Colour(0xFF00D4AA));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(.5f), 10.f, 2.f);

        g.setColour(juce::Colour(0xFF00D4AA));
        g.setFont(juce::Font("Courier New", 11.f, juce::Font::bold));
        g.drawText("Select Algorithm",
                   getLocalBounds().withHeight(24).reduced(10,0),
                   juce::Justification::centredLeft);

        auto grid = getLocalBounds().withTrimmedTop(24).reduced(6);
        const int iw = grid.getWidth()/4, ih = grid.getHeight()/2;

        for (int i = 0; i < 8; ++i)
        {
            int col=i%4, row=i/4;
            auto box = juce::Rectangle<int>(
                grid.getX()+col*iw, grid.getY()+row*ih, iw, ih).reduced(4);

            if (i==hoveredAlgo || i==selectedAlgo) {
                g.setColour(i==selectedAlgo ? juce::Colour(0xFF252550)
                                            : juce::Colour(0xFF202038));
                g.fillRoundedRectangle(box.toFloat(), 5.f);
            }
            if (i==selectedAlgo) {
                g.setColour(juce::Colour(0xFF00D4AA));
                g.drawRoundedRectangle(box.toFloat(), 5.f, 2.f);
            }
            drawAlgorithm(g, i, box.reduced(2).withTrimmedBottom(13));
            g.setColour(i==selectedAlgo ? juce::Colour(0xFF00D4AA)
                                        : juce::Colour(0xFF556070));
            g.setFont(juce::Font("Courier New", 9.f, juce::Font::plain));
            g.drawText("ALG "+juce::String(i),
                       box.withTrimmedTop(box.getHeight()-13),
                       juce::Justification::centred, false);
        }
    }

    void mouseMove (const juce::MouseEvent& e) override
    {
        auto grid = getLocalBounds().withTrimmedTop(24).reduced(6);
        int iw=juce::jmax(1,grid.getWidth()/4), ih=juce::jmax(1,grid.getHeight()/2);
        int n = juce::jlimit(0,1,(e.y-grid.getY())/ih)*4
              + juce::jlimit(0,3,(e.x-grid.getX())/iw);
        if (n!=hoveredAlgo) { hoveredAlgo=n; repaint(); }
    }
    void mouseExit (const juce::MouseEvent&) override { hoveredAlgo=-1; repaint(); }
    void mouseDown (const juce::MouseEvent& e) override
    {
        auto grid = getLocalBounds().withTrimmedTop(24).reduced(6);
        int iw=juce::jmax(1,grid.getWidth()/4), ih=juce::jmax(1,grid.getHeight()/2);
        int algo = juce::jlimit(0,1,(e.y-grid.getY())/ih)*4
                 + juce::jlimit(0,3,(e.x-grid.getX())/iw);
        if (algo>=0 && algo<8 && onSelect) onSelect(algo);
    }
};

// =============================================================================
// AlgorithmModal  — full-screen dim backdrop that blocks ALL input (modal).
// Clicking the dim area outside the picker dismisses with NO selection change.
// The picker panel is positioned on the RIGHT side to avoid the left notch.
// =============================================================================
class AlgorithmModal : public juce::Component
{
public:
    AlgorithmPickerPanel picker;
    std::function<void()> onDismiss;

    AlgorithmModal()
    {
        setInterceptsMouseClicks(true, true);
        addAndMakeVisible(picker);
    }

    // Call after bounds are set. Centres picker on screen (lightbox style).
    void layoutPicker (int selectedAlgo)
    {
        picker.selectedAlgo = selectedAlgo;
        const int pw = juce::jmin(430, (int)(getWidth()  * 0.65f));
        const int ph = juce::jmin(250, (int)(getHeight() * 0.80f));
        picker.setBounds((getWidth()-pw)/2, (getHeight()-ph)/2, pw, ph);
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour(juce::Colour(0xcc000000));
        g.fillRect(getLocalBounds());
        g.setColour(juce::Colour(0x88ffffff));
        g.setFont(juce::Font("Courier New", 10.f, juce::Font::plain));
        g.drawText("tap outside to cancel",
                   getLocalBounds().withTrimmedBottom(6),
                   juce::Justification::centredBottom);
    }

    // Backdrop click — dismiss without changing selection
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (!picker.getBounds().contains(e.getPosition()))
            if (onDismiss) onDismiss();
    }
    // Swallow all other mouse events so nothing beneath receives them
    void mouseUp   (const juce::MouseEvent&) override {}
    void mouseDrag (const juce::MouseEvent&) override {}
    void mouseMove (const juce::MouseEvent&) override {}
};

// =============================================================================
// AlgorithmSelector  — the preview widget shown in the global column
// =============================================================================
class AlgorithmSelector : public juce::Component
{
public:
    std::function<void(int)> onChange;

    void setSelectedAlgorithm (int a) { selectedAlgo=juce::jlimit(0,7,a); repaint(); }
    int  getSelectedAlgorithm() const  { return selectedAlgo; }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds();
        g.setColour(juce::Colour(0xFF556070));
        g.setFont(juce::Font("Courier New", 9.f, juce::Font::plain));
        g.drawText("Algorithm", b.removeFromTop(14), juce::Justification::centredLeft);

        g.setColour(juce::Colour(0xFF161625));
        g.fillRoundedRectangle(b.toFloat(), 4.f);
        g.setColour(juce::Colour(0xFF2a2a3e));
        g.drawRoundedRectangle(b.toFloat(), 4.f, 1.f);

        drawAlgorithm(g, selectedAlgo, b.reduced(16));
    }

    void mouseDown (const juce::MouseEvent&) override { showModal(); }

private:
    int             selectedAlgo  = 0;
    AlgorithmModal* activeModal   = nullptr;

    void showModal()
    {
        if (activeModal) return;
        auto* root = getTopLevelComponent();
        if (!root) return;

        activeModal = new AlgorithmModal();
        activeModal->setBounds(root->getLocalBounds());
        activeModal->layoutPicker(selectedAlgo);

        activeModal->picker.onSelect = [this](int algo)
        {
            setSelectedAlgorithm(algo);
            if (onChange) onChange(algo);
            closeModal();
        };
        activeModal->onDismiss = [this]() { closeModal(); };

        root->addAndMakeVisible(activeModal);
        activeModal->toFront(true);
    }

    void closeModal()
    {
        if (!activeModal) return;
        if (auto* p = activeModal->getParentComponent())
            p->removeChildComponent(activeModal);
        delete activeModal;
        activeModal = nullptr;
    }
};
