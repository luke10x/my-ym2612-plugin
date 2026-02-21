#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// ─────────────────────────────────────────────────────────────────────────────
// Shared drawing function for algorithm diagrams
// ─────────────────────────────────────────────────────────────────────────────

static void drawAlgorithm(juce::Graphics& g, int algo, juce::Rectangle<int> area)
{
    juce::Colour lineCol(0xFFAAAAAA);
    juce::Colour opCol(0xFF4fc3f7);
    juce::Colour textCol(0xFF000000);
    juce::Colour outputCol(0xFF00D4AA);
    
    auto drawOp = [&](float cx, float cy, int num, bool hasFeedback = false) {
        g.setColour(opCol);
        g.fillEllipse(cx - 9, cy - 9, 18, 18);
        
        // Draw feedback ring around OP1 if applicable
        if (hasFeedback && num == 1) {
            g.setColour(outputCol);
            g.drawEllipse(cx - 12, cy - 12, 24, 24, 2.0f);
        }
        
        g.setColour(textCol);
        g.setFont(juce::Font(11.0f, juce::Font::bold));
        g.drawText(juce::String(num), int(cx - 9), int(cy - 9), 18, 18, juce::Justification::centred);
    };
    
    auto drawLine = [&](float x1, float y1, float x2, float y2) {
        g.setColour(lineCol);
        g.drawLine(x1, y1, x2, y2, 2.0f);
    };
    
    auto drawOutput = [&](float opX, float opY) {
        float outputX = area.getRight();
        g.setColour(lineCol);
        g.drawLine(opX, opY, outputX, opY, 2.0f);
    };
    
    // 3:2 aspect ratio layout
    float w = area.getWidth();
    float h = area.getHeight();
    float cy = area.getCentreY();
    
    switch (algo)
    {
    case 0: { // 1→2→3→4 (only OP4 carrier)
        float x1 = area.getX() + w*0.15f, x2 = area.getX() + w*0.35f, 
              x3 = area.getX() + w*0.55f, x4 = area.getX() + w*0.75f;
        drawLine(x1, cy, x2, cy);
        drawLine(x2, cy, x3, cy);
        drawLine(x3, cy, x4, cy);
        drawOutput(x4, cy);
        drawOp(x1, cy, 1, true);
        drawOp(x2, cy, 2);
        drawOp(x3, cy, 3);
        drawOp(x4, cy, 4);
        break;
    }
    case 1: { // 1→2←3, 2→4 (only OP4 carrier)
        float x1 = area.getX() + w*0.2f, x2 = area.getX() + w*0.5f, x3 = area.getX() + w*0.8f;
        float y1 = area.getY() + h*0.3f, y2 = cy, y3 = area.getY() + h*0.7f;
        drawLine(x1, y1, x2, y2);
        drawLine(x1, y3, x2, y2);
        drawLine(x2, y2, x3, y2);
        drawOutput(x3, y2);
        drawOp(x1, y1, 1, true);
        drawOp(x1, y3, 3);
        drawOp(x2, y2, 2);
        drawOp(x3, y2, 4);
        break;
    }
    case 2: { // 1→4, 2→3→4 (only OP4 carrier)
        float x1 = area.getX() + w*0.2f, x2 = area.getX() + w*0.5f, x3 = area.getX() + w*0.8f;
        float y1 = area.getY() + h*0.3f, y2 = cy, y3 = area.getY() + h*0.7f;
        drawLine(x1, y1, x3, y2);
        drawLine(x1, y3, x2, y3);
        drawLine(x2, y3, x3, y2);
        drawOutput(x3, y2);
        drawOp(x1, y1, 1, true);
        drawOp(x1, y3, 2);
        drawOp(x2, y3, 3);
        drawOp(x3, y2, 4);
        break;
    }
    case 3: { // 1→2→4, 3→4 (only OP4 carrier)
        float x1 = area.getX() + w*0.2f, x2 = area.getX() + w*0.5f, x3 = area.getX() + w*0.8f;
        float y1 = area.getY() + h*0.3f, y2 = cy, y3 = area.getY() + h*0.7f;
        drawLine(x1, y1, x2, y1);
        drawLine(x2, y1, x3, y2);
        drawLine(x1, y3, x3, y2);
        drawOutput(x3, y2);
        drawOp(x1, y1, 1, true);
        drawOp(x2, y1, 2);
        drawOp(x1, y3, 3);
        drawOp(x3, y2, 4);
        break;
    }
    case 4: { // 1→2, 3→4 (OP2 and OP4 carriers)
        float x1 = area.getX() + w*0.3f, x2 = area.getX() + w*0.7f;
        float y1 = area.getY() + h*0.35f, y2 = area.getY() + h*0.65f;
        drawLine(x1, y1, x2, y1);
        drawLine(x1, y2, x2, y2);
        drawOutput(x2, y1);
        drawOutput(x2, y2);
        drawOp(x1, y1, 1, true);
        drawOp(x2, y1, 2);
        drawOp(x1, y2, 3);
        drawOp(x2, y2, 4);
        break;
    }
    case 5: { // 1→2,3,4 (OP2, OP3, OP4 carriers)
        float x1 = area.getX() + w*0.25f, x2 = area.getX() + w*0.75f;
        float y1 = area.getY() + h*0.25f, y2 = cy, y3 = area.getY() + h*0.75f;
        drawLine(x1, y2, x2, y1);
        drawLine(x1, y2, x2, y2);
        drawLine(x1, y2, x2, y3);
        drawOutput(x2, y1);
        drawOutput(x2, y2);
        drawOutput(x2, y3);
        drawOp(x1, y2, 1, true);
        drawOp(x2, y1, 2);
        drawOp(x2, y2, 3);
        drawOp(x2, y3, 4);
        break;
    }
    case 6: { // 1→2, 3, 4 (OP2, OP3, OP4 carriers)
        float x1 = area.getX() + w*0.3f, x2 = area.getX() + w*0.7f;
        float y1 = area.getY() + h*0.25f, y2 = cy, y3 = area.getY() + h*0.75f;
        drawLine(x1, y1, x2, y1);
        drawOutput(x2, y1);
        drawOutput(x2, y2);
        drawOutput(x2, y3);
        drawOp(x1, y1, 1, true);
        drawOp(x2, y1, 2);
        drawOp(x2, y2, 3);
        drawOp(x2, y3, 4);
        break;
    }
    case 7: { // All parallel - horizontal layout with output below
        float x1 = area.getX() + w*0.15f, x2 = area.getX() + w*0.35f,
              x3 = area.getX() + w*0.55f, x4 = area.getX() + w*0.75f;
        float yOps = area.getY() + h*0.35f;
        float yOut = area.getY() + h*0.75f;
        float xOut = area.getCentreX();
        
        // All ops connect down to output dot
        drawLine(x1, yOps, xOut, yOut);
        drawLine(x2, yOps, xOut, yOut);
        drawLine(x3, yOps, xOut, yOut);
        drawLine(x4, yOps, xOut, yOut);
        
        // Output dot
        g.setColour(outputCol);
        g.fillEllipse(xOut - 5, yOut - 5, 10, 10);
        
        drawOp(x1, yOps, 1, true);
        drawOp(x2, yOps, 2);
        drawOp(x3, yOps, 3);
        drawOp(x4, yOps, 4);
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

            // Draw algorithm diagram (3:2 aspect ratio)
            auto diagramArea = box.reduced(8).withTrimmedBottom(20);
            // Force 3:2 aspect ratio
            int targetH = diagramArea.getWidth() * 2 / 3;
            if (diagramArea.getHeight() > targetH) {
                int diff = diagramArea.getHeight() - targetH;
                diagramArea = diagramArea.withTrimmedTop(diff / 2).withTrimmedBottom(diff / 2);
            }
            drawAlgorithm(g, i, diagramArea);
            
            // Output dot on right side (for algos 0-6)
            if (i < 7) {
                float dotX = box.getRight() - 10;
                float dotY = box.getCentreY();
                g.setColour(juce::Colour(0xFF00D4AA));
                g.fillEllipse(dotX - 5, dotY - 5, 10, 10);
                juce::Path arrow;
                arrow.addTriangle(dotX + 6, dotY, dotX, dotY - 5, dotX, dotY + 5);
                g.fillPath(arrow);
            }

            // Algorithm number
            g.setColour(juce::Colour(0xFF556070));
            g.setFont(juce::Font(9.0f));
            g.drawText("Algo " + juce::String(i), box.withTrimmedTop(box.getHeight() - 18),
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
        
        // Dropdown box below label (square)
        auto boxArea = bounds.reduced(0, 2);
        g.setColour(juce::Colour(0xFF161625));
        g.fillRoundedRectangle(boxArea.toFloat(), 4.0f);
        g.setColour(juce::Colour(0xFF2a2a3e));
        g.drawRoundedRectangle(boxArea.toFloat(), 4.0f, 1.0f);
        
        // Draw current algorithm diagram using same method as popup (3:2 aspect ratio)
        auto diagramArea = boxArea.reduced(8);
        int targetH = diagramArea.getWidth() * 2 / 3;
        if (diagramArea.getHeight() > targetH) {
            int diff = diagramArea.getHeight() - targetH;
            diagramArea = diagramArea.withTrimmedTop(diff / 2).withTrimmedBottom(diff / 2);
        }
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
