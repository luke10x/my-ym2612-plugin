#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// ─────────────────────────────────────────────────────────────────────────────
// AlgorithmPopup - Popup window showing all 8 algorithm diagrams
// ─────────────────────────────────────────────────────────────────────────────
class AlgorithmPopup : public juce::Component
{
public:
    AlgorithmPopup(int currentAlgo) : selectedAlgo(currentAlgo) {}

    std::function<void(int)> onSelect;

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xFF1a1a2e));
        g.setColour(juce::Colour(0xFF00D4AA));
        g.drawRect(getLocalBounds(), 2);

        int itemW = getWidth() / 4;
        int itemH = getHeight() / 2;

        for (int i = 0; i < 8; i++)
        {
            int col = i % 4;
            int row = i / 4;
            auto box = juce::Rectangle<int>(col * itemW, row * itemH, itemW, itemH).reduced(4);

            // Hover or selected
            if (i == hoveredAlgo || i == selectedAlgo)
            {
                g.setColour(i == selectedAlgo ? juce::Colour(0xFF252540) : juce::Colour(0xFF202030));
                g.fillRoundedRectangle(box.toFloat(), 4.0f);
                if (i == selectedAlgo)
                {
                    g.setColour(juce::Colour(0xFF00D4AA));
                    g.drawRoundedRectangle(box.toFloat(), 4.0f, 2.0f);
                }
            }

            // Draw algorithm diagram (with output arrow on right)
            auto diagramArea = box.reduced(8).withTrimmedBottom(20).withTrimmedRight(15);
            drawAlgorithm(g, i, diagramArea);
            
            // Output dot on right side (bigger)
            float arrowX = box.getRight() - 10;
            float arrowY = box.getCentreY();
            g.setColour(juce::Colour(0xFF00D4AA));
            g.fillEllipse(arrowX - 5, arrowY - 5, 10, 10);  // Bigger dot
            juce::Path arrow;
            arrow.addTriangle(arrowX + 6, arrowY, arrowX, arrowY - 5, arrowX, arrowY + 5);
            g.fillPath(arrow);

            // Algorithm number
            g.setColour(juce::Colour(0xFF556070));
            g.setFont(juce::Font(10.0f));
            g.drawText("Algo " + juce::String(i), box.withTrimmedTop(box.getHeight() - 16),
                       juce::Justification::centred, false);
        }
    }

    void mouseMove(const juce::MouseEvent& e) override
    {
        int itemW = getWidth() / 4;
        int itemH = getHeight() / 2;
        int col = e.x / itemW;
        int row = e.y / itemH;
        int newHover = row * 4 + col;
        if (newHover >= 0 && newHover < 8 && newHover != hoveredAlgo)
        {
            hoveredAlgo = newHover;
            repaint();
        }
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        hoveredAlgo = -1;
        repaint();
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        int itemW = getWidth() / 4;
        int itemH = getHeight() / 2;
        int col = e.x / itemW;
        int row = e.y / itemH;
        int algo = row * 4 + col;
        if (algo >= 0 && algo < 8 && onSelect)
        {
            onSelect(algo);
            if (auto* parent = getParentComponent())
                parent->exitModalState(0);
        }
    }

private:
    int selectedAlgo = 0;
    int hoveredAlgo = -1;

    void drawAlgorithm(juce::Graphics& g, int algo, juce::Rectangle<int> area)
    {
        juce::Colour lineCol(0xFFAAAAAA);
        juce::Colour opCol(0xFF4fc3f7);
        juce::Colour textCol(0xFF000000);

        switch (algo)
        {
        case 0: drawAlgo0(g, area, lineCol, opCol, textCol); break;
        case 1: drawAlgo1(g, area, lineCol, opCol, textCol); break;
        case 2: drawAlgo2(g, area, lineCol, opCol, textCol); break;
        case 3: drawAlgo3(g, area, lineCol, opCol, textCol); break;
        case 4: drawAlgo4(g, area, lineCol, opCol, textCol); break;
        case 5: drawAlgo5(g, area, lineCol, opCol, textCol); break;
        case 6: drawAlgo6(g, area, lineCol, opCol, textCol); break;
        case 7: drawAlgo7(g, area, lineCol, opCol, textCol); break;
        }
    }

    void drawOp(juce::Graphics& g, float cx, float cy, int num, juce::Colour opCol, juce::Colour textCol)
    {
        g.setColour(opCol);
        g.fillEllipse(cx - 9, cy - 9, 18, 18);
        g.setColour(textCol);
        g.setFont(juce::Font(11.0f, juce::Font::bold));
        g.drawText(juce::String(num), int(cx - 9), int(cy - 9), 18, 18, juce::Justification::centred);
    }

    void drawLine(juce::Graphics& g, float x1, float y1, float x2, float y2, juce::Colour col)
    {
        g.setColour(col);
        g.drawLine(x1, y1, x2, y2, 2.0f);
    }
    
    void drawOutputConnection(juce::Graphics& g, float opX, float opY, juce::Rectangle<int> r, juce::Colour col)
    {
        // Draw line from operator to right edge (output)
        float outputX = r.getRight();
        g.setColour(col);
        g.drawLine(opX, opY, outputX, opY, 2.0f);
    }

    // Algorithm drawing functions with carrier outputs
    void drawAlgo0(juce::Graphics& g, juce::Rectangle<int> r, juce::Colour lc, juce::Colour oc, juce::Colour tc)
    {
        // Algorithm 0: 1→2→3→4 (only OP4 is carrier)
        float cy = r.getCentreY();
        float x1 = r.getX() + 15, x2 = r.getX() + 40, x3 = r.getX() + 65, x4 = r.getX() + 90;
        drawLine(g, x1, cy, x2, cy, lc);
        drawLine(g, x2, cy, x3, cy, lc);
        drawLine(g, x3, cy, x4, cy, lc);
        drawOutputConnection(g, x4, cy, r, lc);  // OP4 to output
        drawOp(g, x1, cy, 1, oc, tc);
        drawOp(g, x2, cy, 2, oc, tc);
        drawOp(g, x3, cy, 3, oc, tc);
        drawOp(g, x4, cy, 4, oc, tc);
    }

    void drawAlgo1(juce::Graphics& g, juce::Rectangle<int> r, juce::Colour lc, juce::Colour oc, juce::Colour tc)
    {
        // Algorithm 1: 1→2←3, 2→4 (only OP4 is carrier)
        float x1 = r.getX() + 15, x2 = r.getX() + 45, x3 = r.getX() + 75;
        float y1 = r.getY() + 20, y2 = r.getCentreY(), y3 = r.getBottom() - 20;
        drawLine(g, x1, y1, x2, y2, lc);
        drawLine(g, x1, y3, x2, y2, lc);
        drawLine(g, x2, y2, x3, y2, lc);
        drawOutputConnection(g, x3, y2, r, lc);  // OP4 to output
        drawOp(g, x1, y1, 1, oc, tc);
        drawOp(g, x1, y3, 3, oc, tc);
        drawOp(g, x2, y2, 2, oc, tc);
        drawOp(g, x3, y2, 4, oc, tc);
    }

    void drawAlgo2(juce::Graphics& g, juce::Rectangle<int> r, juce::Colour lc, juce::Colour oc, juce::Colour tc)
    {
        // Algorithm 2: 1→4, 2→3→4 (only OP4 is carrier)
        float x1 = r.getX() + 15, x2 = r.getX() + 45, x3 = r.getX() + 75;
        float y1 = r.getY() + 20, y2 = r.getCentreY(), y3 = r.getBottom() - 20;
        drawLine(g, x1, y1, x3, y2, lc);
        drawLine(g, x1, y3, x2, y3, lc);
        drawLine(g, x2, y3, x3, y2, lc);
        drawOutputConnection(g, x3, y2, r, lc);  // OP4 to output
        drawOp(g, x1, y1, 1, oc, tc);
        drawOp(g, x1, y3, 2, oc, tc);
        drawOp(g, x2, y3, 3, oc, tc);
        drawOp(g, x3, y2, 4, oc, tc);
    }

    void drawAlgo3(juce::Graphics& g, juce::Rectangle<int> r, juce::Colour lc, juce::Colour oc, juce::Colour tc)
    {
        // Algorithm 3: 1→2→4, 3→4 (only OP4 is carrier)
        float x1 = r.getX() + 15, x2 = r.getX() + 45, x3 = r.getX() + 75;
        float y1 = r.getY() + 20, y2 = r.getCentreY(), y3 = r.getBottom() - 20;
        drawLine(g, x1, y1, x2, y1, lc);
        drawLine(g, x2, y1, x3, y2, lc);
        drawLine(g, x1, y3, x3, y2, lc);
        drawOutputConnection(g, x3, y2, r, lc);  // OP4 to output
        drawOp(g, x1, y1, 1, oc, tc);
        drawOp(g, x2, y1, 2, oc, tc);
        drawOp(g, x1, y3, 3, oc, tc);
        drawOp(g, x3, y2, 4, oc, tc);
    }

    void drawAlgo4(juce::Graphics& g, juce::Rectangle<int> r, juce::Colour lc, juce::Colour oc, juce::Colour tc)
    {
        // Algorithm 4: 1→2, 3→4 (OP2 and OP4 are carriers)
        float x1 = r.getX() + 20, x2 = r.getX() + 50;
        float y1 = r.getY() + 20, y2 = r.getBottom() - 20;
        drawLine(g, x1, y1, x2, y1, lc);
        drawLine(g, x1, y2, x2, y2, lc);
        drawOutputConnection(g, x2, y1, r, lc);  // OP2 to output
        drawOutputConnection(g, x2, y2, r, lc);  // OP4 to output
        drawOp(g, x1, y1, 1, oc, tc);
        drawOp(g, x2, y1, 2, oc, tc);
        drawOp(g, x1, y2, 3, oc, tc);
        drawOp(g, x2, y2, 4, oc, tc);
    }

    void drawAlgo5(juce::Graphics& g, juce::Rectangle<int> r, juce::Colour lc, juce::Colour oc, juce::Colour tc)
    {
        // Algorithm 5: 1→2,3,4 (OP2, OP3, OP4 are carriers)
        float x1 = r.getX() + 20, x2 = r.getX() + 60;
        float cy = r.getCentreY();
        float y1 = r.getY() + 10, y2 = cy, y3 = r.getBottom() - 10;
        drawLine(g, x1, cy, x2, y1, lc);
        drawLine(g, x1, cy, x2, y2, lc);
        drawLine(g, x1, cy, x2, y3, lc);
        drawOutputConnection(g, x2, y1, r, lc);  // OP2 to output
        drawOutputConnection(g, x2, y2, r, lc);  // OP3 to output
        drawOutputConnection(g, x2, y3, r, lc);  // OP4 to output
        drawOp(g, x1, cy, 1, oc, tc);
        drawOp(g, x2, y1, 2, oc, tc);
        drawOp(g, x2, y2, 3, oc, tc);
        drawOp(g, x2, y3, 4, oc, tc);
    }

    void drawAlgo6(juce::Graphics& g, juce::Rectangle<int> r, juce::Colour lc, juce::Colour oc, juce::Colour tc)
    {
        // Algorithm 6: 1→2, 3, 4 (OP2, OP3, OP4 are carriers)
        float x1 = r.getX() + 20, x2 = r.getX() + 50;
        float y1 = r.getY() + 10, y2 = r.getCentreY(), y3 = r.getBottom() - 10;
        drawLine(g, x1, y1, x2, y1, lc);
        drawOutputConnection(g, x2, y1, r, lc);  // OP2 to output
        drawOutputConnection(g, x2, y2, r, lc);  // OP3 to output
        drawOutputConnection(g, x2, y3, r, lc);  // OP4 to output
        drawOp(g, x1, y1, 1, oc, tc);
        drawOp(g, x2, y1, 2, oc, tc);
        drawOp(g, x2, y2, 3, oc, tc);
        drawOp(g, x2, y3, 4, oc, tc);
    }

    void drawAlgo7(juce::Graphics& g, juce::Rectangle<int> r, juce::Colour lc, juce::Colour oc, juce::Colour tc)
    {
        // Algorithm 7: 1, 2, 3, 4 (all are carriers)
        float cx = r.getCentreX();
        float y1 = r.getY() + 10, y2 = r.getY() + 35, y3 = r.getY() + 60, y4 = r.getY() + 85;
        drawOutputConnection(g, cx, y1, r, lc);  // OP1 to output
        drawOutputConnection(g, cx, y2, r, lc);  // OP2 to output
        drawOutputConnection(g, cx, y3, r, lc);  // OP3 to output
        drawOutputConnection(g, cx, y4, r, lc);  // OP4 to output
        drawOp(g, cx, y1, 1, oc, tc);
        drawOp(g, cx, y2, 2, oc, tc);
        drawOp(g, cx, y3, 3, oc, tc);
        drawOp(g, cx, y4, 4, oc, tc);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// AlgorithmSelector - Dropdown-style selector with label above diagram
// ─────────────────────────────────────────────────────────────────────────────
class AlgorithmSelector : public juce::Component
{
public:
    AlgorithmSelector() = default;

    void setSelectedAlgorithm(int algo)
    {
        selectedAlgo = juce::jlimit(0, 7, algo);
        repaint();
    }

    int getSelectedAlgorithm() const { return selectedAlgo; }

    std::function<void(int)> onChange;

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();
        
        // Label "Algorithm" at top
        auto labelArea = bounds.removeFromTop(18);
        g.setColour(juce::Colour(0xFF556070));
        g.setFont(juce::Font(11.0f));
        g.drawText("Algorithm", labelArea, juce::Justification::centredLeft, false);
        
        // Dropdown box below label (square)
        auto boxArea = bounds.reduced(0, 2);
        g.setColour(juce::Colour(0xFF161625));
        g.fillRoundedRectangle(boxArea.toFloat(), 4.0f);
        g.setColour(juce::Colour(0xFF2a2a3e));
        g.drawRoundedRectangle(boxArea.toFloat(), 4.0f, 1.0f);
        
        // Draw current algorithm diagram using same sizing as popup
        auto diagramArea = boxArea.reduced(8);
        drawAlgorithm(g, selectedAlgo, diagramArea);
        
        // Dropdown arrow (bottom right corner)
        g.setColour(juce::Colour(0xFF556070));
        g.setFont(juce::Font(10.0f));
        g.drawText("▼", boxArea.withLeft(boxArea.getRight() - 16).withTop(boxArea.getBottom() - 16), 
                   juce::Justification::centred, false);
    }

    void mouseDown(const juce::MouseEvent&) override
    {
        // Show popup directly below this component
        auto* popup = new AlgorithmPopup(selectedAlgo);
        popup->setSize(480, 240);
        popup->onSelect = [this](int algo) {
            setSelectedAlgorithm(algo);
            if (onChange)
                onChange(algo);
        };
        
        juce::CallOutBox::launchAsynchronously(std::unique_ptr<juce::Component>(popup),
                                                getScreenBounds(), nullptr);
    }

private:
    int selectedAlgo = 0;

    void drawAlgorithm(juce::Graphics& g, int algo, juce::Rectangle<int> area)
    {
        juce::Colour lineCol(0xFFAAAAAA);
        juce::Colour opCol(0xFF4fc3f7);
        juce::Colour textCol(0xFF000000);

        switch (algo)
        {
        case 0: drawAlgo0(g, area, lineCol, opCol, textCol); break;
        case 1: drawAlgo1(g, area, lineCol, opCol, textCol); break;
        case 2: drawAlgo2(g, area, lineCol, opCol, textCol); break;
        case 3: drawAlgo3(g, area, lineCol, opCol, textCol); break;
        case 4: drawAlgo4(g, area, lineCol, opCol, textCol); break;
        case 5: drawAlgo5(g, area, lineCol, opCol, textCol); break;
        case 6: drawAlgo6(g, area, lineCol, opCol, textCol); break;
        case 7: drawAlgo7(g, area, lineCol, opCol, textCol); break;
        }
    }

    void drawOp(juce::Graphics& g, float cx, float cy, int num, juce::Colour opCol, juce::Colour textCol)
    {
        g.setColour(opCol);
        g.fillEllipse(cx - 9, cy - 9, 18, 18);
        g.setColour(textCol);
        g.setFont(juce::Font(11.0f, juce::Font::bold));
        g.drawText(juce::String(num), int(cx - 9), int(cy - 9), 18, 18, juce::Justification::centred);
    }

    void drawLine(juce::Graphics& g, float x1, float y1, float x2, float y2, juce::Colour col)
    {
        g.setColour(col);
        g.drawLine(x1, y1, x2, y2, 2.0f);
    }

    // Same algorithm drawing functions as AlgorithmPopup
    void drawAlgo0(juce::Graphics& g, juce::Rectangle<int> r, juce::Colour lc, juce::Colour oc, juce::Colour tc)
    {
        float cy = r.getCentreY();
        float x1 = r.getX() + 10, x2 = r.getX() + 30, x3 = r.getX() + 50, x4 = r.getX() + 70;
        drawLine(g, x1, cy, x2, cy, lc);
        drawLine(g, x2, cy, x3, cy, lc);
        drawLine(g, x3, cy, x4, cy, lc);
        drawOp(g, x1, cy, 1, oc, tc);
        drawOp(g, x2, cy, 2, oc, tc);
        drawOp(g, x3, cy, 3, oc, tc);
        drawOp(g, x4, cy, 4, oc, tc);
    }

    void drawAlgo1(juce::Graphics& g, juce::Rectangle<int> r, juce::Colour lc, juce::Colour oc, juce::Colour tc)
    {
        float x1 = r.getX() + 10, x2 = r.getX() + 35, x3 = r.getX() + 60;
        float y1 = r.getY() + 15, y2 = r.getCentreY(), y3 = r.getBottom() - 15;
        drawLine(g, x1, y1, x2, y2, lc);
        drawLine(g, x1, y3, x2, y2, lc);
        drawLine(g, x2, y2, x3, y2, lc);
        drawOp(g, x1, y1, 1, oc, tc);
        drawOp(g, x1, y3, 3, oc, tc);
        drawOp(g, x2, y2, 2, oc, tc);
        drawOp(g, x3, y2, 4, oc, tc);
    }

    void drawAlgo2(juce::Graphics& g, juce::Rectangle<int> r, juce::Colour lc, juce::Colour oc, juce::Colour tc)
    {
        float x1 = r.getX() + 10, x2 = r.getX() + 35, x3 = r.getX() + 60;
        float y1 = r.getY() + 15, y2 = r.getCentreY(), y3 = r.getBottom() - 15;
        drawLine(g, x1, y1, x3, y2, lc);
        drawLine(g, x1, y3, x2, y3, lc);
        drawLine(g, x2, y3, x3, y2, lc);
        drawOp(g, x1, y1, 1, oc, tc);
        drawOp(g, x1, y3, 2, oc, tc);
        drawOp(g, x2, y3, 3, oc, tc);
        drawOp(g, x3, y2, 4, oc, tc);
    }

    void drawAlgo3(juce::Graphics& g, juce::Rectangle<int> r, juce::Colour lc, juce::Colour oc, juce::Colour tc)
    {
        float x1 = r.getX() + 10, x2 = r.getX() + 35, x3 = r.getX() + 60;
        float y1 = r.getY() + 15, y2 = r.getCentreY(), y3 = r.getBottom() - 15;
        drawLine(g, x1, y1, x2, y1, lc);
        drawLine(g, x2, y1, x3, y2, lc);
        drawLine(g, x1, y3, x3, y2, lc);
        drawOp(g, x1, y1, 1, oc, tc);
        drawOp(g, x2, y1, 2, oc, tc);
        drawOp(g, x1, y3, 3, oc, tc);
        drawOp(g, x3, y2, 4, oc, tc);
    }

    void drawAlgo4(juce::Graphics& g, juce::Rectangle<int> r, juce::Colour lc, juce::Colour oc, juce::Colour tc)
    {
        float x1 = r.getX() + 15, x2 = r.getX() + 40;
        float y1 = r.getY() + 15, y2 = r.getBottom() - 15;
        drawLine(g, x1, y1, x2, y1, lc);
        drawLine(g, x1, y2, x2, y2, lc);
        drawOp(g, x1, y1, 1, oc, tc);
        drawOp(g, x2, y1, 2, oc, tc);
        drawOp(g, x1, y2, 3, oc, tc);
        drawOp(g, x2, y2, 4, oc, tc);
    }

    void drawAlgo5(juce::Graphics& g, juce::Rectangle<int> r, juce::Colour lc, juce::Colour oc, juce::Colour tc)
    {
        float x1 = r.getX() + 15, x2 = r.getX() + 50;
        float cy = r.getCentreY();
        float y1 = r.getY() + 8, y2 = cy, y3 = r.getBottom() - 8;
        drawLine(g, x1, cy, x2, y1, lc);
        drawLine(g, x1, cy, x2, y2, lc);
        drawLine(g, x1, cy, x2, y3, lc);
        drawOp(g, x1, cy, 1, oc, tc);
        drawOp(g, x2, y1, 2, oc, tc);
        drawOp(g, x2, y2, 3, oc, tc);
        drawOp(g, x2, y3, 4, oc, tc);
    }

    void drawAlgo6(juce::Graphics& g, juce::Rectangle<int> r, juce::Colour lc, juce::Colour oc, juce::Colour tc)
    {
        float x1 = r.getX() + 15, x2 = r.getX() + 40;
        float y1 = r.getY() + 8, y2 = r.getCentreY(), y3 = r.getBottom() - 8;
        drawLine(g, x1, y1, x2, y1, lc);
        drawOp(g, x1, y1, 1, oc, tc);
        drawOp(g, x2, y1, 2, oc, tc);
        drawOp(g, x2, y2, 3, oc, tc);
        drawOp(g, x2, y3, 4, oc, tc);
    }

    void drawAlgo7(juce::Graphics& g, juce::Rectangle<int> r, juce::Colour lc, juce::Colour oc, juce::Colour tc)
    {
        float cx = r.getCentreX();
        float y1 = r.getY() + 8, y2 = r.getY() + 28, y3 = r.getY() + 48, y4 = r.getY() + 68;
        drawOp(g, cx, y1, 1, oc, tc);
        drawOp(g, cx, y2, 2, oc, tc);
        drawOp(g, cx, y3, 3, oc, tc);
        drawOp(g, cx, y4, 4, oc, tc);
    }
};

