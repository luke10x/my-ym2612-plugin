#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// ─────────────────────────────────────────────────────────────────────────────
// Shared drawing function for algorithm diagrams - matches official YM2612 spec
// ─────────────────────────────────────────────────────────────────────────────

static void drawAlgorithm(juce::Graphics& g, int algo, juce::Rectangle<int> area)
{
    juce::Colour lineCol(0xFFAAAAAA);
    juce::Colour opCol(0xFF4fc3f7);
    juce::Colour textCol(0xFF000000);
    juce::Colour outputCol(0xFF00D4AA);
    
    float w = area.getWidth();
    float h = area.getHeight();
    float opSize = 16.0f;
    
    auto drawOp = [&](float cx, float cy, int num, bool hasFeedback = false) {
        // Draw box for operator
        g.setColour(opCol);
        g.fillRect(cx - opSize/2, cy - opSize/2, opSize, opSize);
        
        // Draw feedback loop if applicable
        if (hasFeedback && num == 1) {
            g.setColour(lineCol);
            float loopSize = opSize * 0.8f;
            juce::Path loop;
            loop.addRoundedRectangle(cx - loopSize, cy - loopSize, loopSize*2, loopSize*2, 4.0f);
            g.strokePath(loop, juce::PathStrokeType(1.5f));
        }
        
        // Operator number
        g.setColour(textCol);
        g.setFont(juce::Font(10.0f, juce::Font::bold));
        g.drawText("" + juce::String(num), cx - opSize/2, cy - opSize/2, opSize, opSize, 
                   juce::Justification::centred, false);
    };
    
    auto drawLine = [&](float x1, float y1, float x2, float y2) {
        g.setColour(lineCol);
        g.drawLine(x1, y1, x2, y2, 1.5f);
    };
    
    auto drawOutput = [&](float x, float y) {
        g.setColour(lineCol);
        g.drawLine(x, y, area.getRight(), y, 1.5f);

        // Output dot
        g.setColour(outputCol);
        g.fillEllipse(area.getRight() - 4, y - 4, 8, 8);
        
    };
    
    // Calculate positions based on official diagram layout
    float margin = 5.0f;
    float usableW = w - margin * 2;
    float usableH = h - margin * 2;
    float baseX = area.getX() + margin;
    float baseY = area.getY() + margin;
    
    switch (algo)
    {
    case 0: { // Four serial connection mode: S1→S2→S3→S4
        float spacing = usableW / 4.5f;
        float y = baseY + usableH / 2;
        float x1 = baseX + spacing * 0.7f;
        float x2 = baseX + spacing * 1.7f;
        float x3 = baseX + spacing * 2.7f;
        float x4 = baseX + spacing * 3.7f;
        
        drawLine(x1 + opSize/2, y, x2 - opSize/2, y);
        drawLine(x2 + opSize/2, y, x3 - opSize/2, y);
        drawLine(x3 + opSize/2, y, x4 - opSize/2, y);
        drawOutput(x4 + opSize/2, y);
        
        drawOp(x1, y, 1,  true);
        drawOp(x2, y, 2);
        drawOp(x3, y, 3);
        drawOp(x4, y, 4); // C
        break;
    }
    
    case 1: { // Three double modulation serial: S1+S2→S3→S4
        float x1 = baseX + usableW * 0.25f;
        float x3 = baseX + usableW * 0.55f;
        float x4 = baseX + usableW * 0.85f;
        float y1 = baseY + usableH * 0.3f;
        float y2 = baseY + usableH * 0.7f;
        float yMid = baseY + usableH * 0.5f;
        
        // S1 feedback loop and connection to S3
        drawLine(x1 + opSize/2, y1, x3 - opSize/2, yMid);
        // S2 to S3
        drawLine(x1 + opSize/2, y2, x3 - opSize/2, yMid);
        // S3 to S4
        drawLine(x3 + opSize/2, yMid, x4 - opSize/2, yMid);
        drawOutput(x4 + opSize/2, yMid);
        
        drawOp(x1, y1, 1, true);
        drawOp(x1, y2, 2);
        drawOp(x3, yMid, 3);
        drawOp(x4, yMid, 4); // C
        break;
    }
    
    case 2: { // Double modulation mode 1: S1→S4, S2→S3→S4
        float x1 = baseX + usableW * 0.25f;
        float x3 = baseX + usableW * 0.55f;
        float x4 = baseX + usableW * 0.85f;
        float y1 = baseY + usableH * 0.3f;
        float y2 = baseY + usableH * 0.7f;
        float yMid = baseY + usableH * 0.5f;
        
        // S1 to S4
        drawLine(x1 + opSize/2, y1, x4 - opSize/2, yMid);
        // S2 to S3
        drawLine(x1 + opSize/2, y2, x3 - opSize/2, y2);
        // S3 to S4
        drawLine(x3 + opSize/2, y2, x4 - opSize/2, yMid);
        drawOutput(x4 + opSize/2, yMid);
        
        drawOp(x1, y1, 1, true);
        drawOp(x1, y2, 2);
        drawOp(x3, y2, 3);
        drawOp(x4, yMid, 4); // C
        break;
    }
    
    case 3: { // Double modulation mode 2: S1→S2→S4, S3→S4
        float x1 = baseX + usableW * 0.25f;
        float x2 = baseX + usableW * 0.5f;
        float x4 = baseX + usableW * 0.85f;
        float y1 = baseY + usableH * 0.3f;
        float y3 = baseY + usableH * 0.7f;
        float yMid = baseY + usableH * 0.5f;
        
        // S1 to S2
        drawLine(x1 + opSize/2, y1, x2 - opSize/2, y1);
        // S2 to S4
        drawLine(x2 + opSize/2, y1, x4 - opSize/2, yMid);
        // S3 to S4
        drawLine(x1 + opSize/2, y3, x4 - opSize/2, yMid);
        drawOutput(x4 + opSize/2, yMid);
        
        drawOp(x1, y1, 1, true);
        drawOp(x2, y1, 2);
        drawOp(x1, y3, 3);
        drawOp(x4, yMid, 4); // C
        break;
    }
    
    case 4: { // Two serial connection and two parallel modes: S1→S2, S3→S4
        float x1 = baseX + usableW * 0.3f;
        float x2 = baseX + usableW * 0.65f;
        float y1 = baseY + usableH * 0.35f;
        float y2 = baseY + usableH * 0.65f;
        float xOut = area.getRight();
        
        float y = (y2 + y1 )/ 2.0f;
        // Top stack: S1→S2
        drawLine(x1 + opSize/2, y1, x2 - opSize/2, y1);
        drawOutput(x2 + opSize/2, y1);
        
        // Bottom stack: S3→S4
        drawLine(x1 + opSize/2, y2, x2 - opSize/2, y2);
        drawOutput(x2 + opSize/2, y2);
        
        drawOp(x1, y1, 1, true);
        drawOp(x2, y1, 2); // C
        drawOp(x1, y2, 3);
        drawOp(x2, y2, 4); // C
        break;
    }
    
    case 5: { // Common modulation 3 parallel mode: S1→S2,S3,S4
        float x1 = baseX + usableW * 0.25f;
        float x2 = baseX + usableW * 0.7f;
        float y1 = baseY + usableH * 0.25f;
        float y2 = baseY + usableH * 0.5f;
        float y3 = baseY + usableH * 0.75f;
        
        // S1 to S2
        drawLine(x1 + opSize/2, y2, x2 - opSize/2, y1);
        drawOutput(x2 + opSize/2, y1);
        
        // S1 to S3
        drawLine(x1 + opSize/2, y2, x2 - opSize/2, y2);
        drawOutput(x2 + opSize/2, y2);
        
        // S1 to S4
        drawLine(x1 + opSize/2, y2, x2 - opSize/2, y3);
        drawOutput(x2 + opSize/2, y3);
        
        drawOp(x1, y2, 1, true);
        drawOp(x2, y1, 2); // C
        drawOp(x2, y2, 3); // C
        drawOp(x2, y3, 4); // C
        break;
    }
    
    case 6: { // Two serial connection + two sine mode: S1→S2, S3, S4
        float x1 = baseX + usableW * 0.3f;
        float x2 = baseX + usableW * 0.6f;
        float y1 = baseY + usableH * 0.25f;
        float y2 = baseY + usableH * 0.5f;
        float y3 = baseY + usableH * 0.75f;
        
        
        // S1 to S2
        drawLine(x1 + opSize/2, y1, x2 - opSize/2, y1);
        drawOutput(x2 + opSize/2, y1);
        
        // S3 standalone
        drawOutput(x2 + opSize/2, y2);
        
        // S4 standalone
        drawOutput(x2 + opSize/2, y3);
        
        drawOp(x1, y1, 1, true);
        drawOp(x2, y1, 2); // C
        drawOp(x2, y2, 3); // C
        drawOp(x2, y3, 4); // C
        break;
    }
    
    case 7: { // Four parallel sine synthesis mode
        float spacing = usableW / 5.0f;
        float y = baseY + usableH * 0.4f;
        float yOut = baseY + usableH * 0.75f;
        float xOut = area.getCentreX();
        
        float x1 = baseX + spacing;
        float x2 = baseX + spacing * 2;
        float x3 = baseX + spacing * 3;
        float x4 = baseX + spacing * 4;
        
        // All connect to output below
        drawLine(x1, y + opSize/2, xOut, yOut);
        drawLine(x2, y + opSize/2, xOut, yOut);
        drawLine(x3, y + opSize/2, xOut, yOut);
        drawLine(x4, y + opSize/2, xOut, yOut);
        
        // Output dot
        g.setColour(outputCol);
        g.fillEllipse(xOut - 4, yOut - 4, 8, 8);
        
        drawOp(x1, y, 1, true); // C
        drawOp(x2, y, 2); // C
        drawOp(x3, y, 3); // C
        drawOp(x4, y, 4); // C
        break;
    }
    }
}

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

            // Draw algorithm diagram
            auto diagramArea = box.reduced(1).withTrimmedBottom(10);
            drawAlgorithm(g, i, diagramArea);

            // Algorithm number
            g.setColour(juce::Colour(0xFF556070));
            g.setFont(juce::Font(9.0f));
            g.drawText(juce::String(i), box.withTrimmedTop(box.getHeight() - 18),
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
        
        // Dropdown box below label
        auto boxArea = bounds.reduced(0, 2);
        g.setColour(juce::Colour(0xFF161625));
        g.fillRoundedRectangle(boxArea.toFloat(), 4.0f);
        g.setColour(juce::Colour(0xFF2a2a3e));
        g.drawRoundedRectangle(boxArea.toFloat(), 4.0f, 1.0f);
        
        // Draw current algorithm diagram using same method as popup
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
};
