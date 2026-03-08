#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ModalBackdrop.h"

static void drawSsgMode(juce::Graphics& g, int dropdownIdx, juce::Rectangle<int> area)
{
    if (dropdownIdx == 0)
    {
        // "None" 
        g.setColour(juce::Colour(0xFF556070));
        g.setFont(juce::Font(12.0f));
        g.drawText("None", area, juce::Justification::centred, false);
        return;
    }

    int mode = dropdownIdx - 1;  // Convert dropdown index to SSG mode (0-7)
    
    g.setColour(juce::Colour(0xFF4fc3f7));
    
    // Use consistent sizing for both popup and dropdown
    float margin = 10.0f;
    float x1 = area.getX() + margin;
    float x2 = area.getRight() - margin;
    float yTop = area.getY() + margin;
    float yBottom = area.getBottom() - margin;
    float yMid = area.getCentreY();
    
    juce::Path path;
    
    float segmentW = (x2 - x1) / 4.0f;
    
    if (mode == 0) {
        // Down-Down pattern (repeat with vertical returns)
        path.startNewSubPath(x1, yBottom);
        path.lineTo(x1, yTop);
        path.lineTo(x1 + segmentW, yBottom);  // First decay
        path.lineTo(x1 + segmentW, yTop);     // Vertical jump back
        path.lineTo(x1 + segmentW * 2, yBottom);
        path.lineTo(x1 + segmentW * 2, yTop);
        path.lineTo(x1 + segmentW * 3, yBottom);
        path.lineTo(x1 + segmentW * 3, yTop);
        path.lineTo(x1 + segmentW * 4, yBottom);
    }
    if (mode == 1) {
        // Down.
        path.startNewSubPath(x1, yBottom);
        path.lineTo(x1, yTop);
        path.lineTo(x1 + segmentW, yBottom);
        path.lineTo(x1 + segmentW * 4, yBottom);
    }
    if (mode == 2) {
        // Down Up Down Up
        path.startNewSubPath(x1, yBottom);
        path.lineTo(x1, yTop);
        path.lineTo(x1 + segmentW, yBottom);
        path.lineTo(x1 + segmentW * 2, yTop);
        path.lineTo(x1 + segmentW * 3, yBottom);
        path.lineTo(x1 + segmentW * 4, yTop);
    }
    if (mode == 3) {
        // Down Up
        path.startNewSubPath(x1, yBottom);
        path.lineTo(x1, yTop);
        path.lineTo(x1 + segmentW, yBottom);
        path.lineTo(x1 + segmentW, yTop);
        path.lineTo(x1 + segmentW * 4, yTop);
    }
    if (mode == 4) {
        path.startNewSubPath(x1, yBottom);
        path.lineTo(x1 + segmentW, yTop);  // First decay
        path.lineTo(x1 + segmentW, yBottom);     // Vertical jump back
        path.lineTo(x1 + segmentW * 2, yTop);
        path.lineTo(x1 + segmentW * 2, yBottom);
        path.lineTo(x1 + segmentW * 3, yTop);
        path.lineTo(x1 + segmentW * 3, yBottom);
        path.lineTo(x1 + segmentW * 4, yTop);
    }
    if (mode == 5) {
        // Up.
        path.startNewSubPath(x1, yBottom);
        path.lineTo(x1 + segmentW, yTop);
        path.lineTo(x1 + segmentW * 4, yTop);
    }
    if (mode == 6) {
        // Up Down Up Down
        path.startNewSubPath(x1, yBottom);
        path.lineTo(x1 + segmentW, yTop);
        path.lineTo(x1 + segmentW * 2, yBottom);
        path.lineTo(x1 + segmentW * 3, yTop);
        path.lineTo(x1 + segmentW * 4, yBottom);
    }
    if (mode == 7) {
        // Down.
        path.startNewSubPath(x1, yBottom);
        path.lineTo(x1 + segmentW, yTop);
        path.lineTo(x1 + segmentW, yBottom);
        path.lineTo(x1 + segmentW * 4, yBottom);
    }
    
    g.strokePath(path, juce::PathStrokeType(2.0f));
    
}
// ─────────────────────────────────────────────────────────────────────────────
// SsgEgPickerPanel - 3x3 grid showing all 9 SSG-EG modes (Off + 8 modes)
// ─────────────────────────────────────────────────────────────────────────────
class SsgEgPickerPanel : public juce::Component
{
public:
    int selectedMode = 0;
    int hoveredMode = -1;
    std::function<void(int)> onSelect;

    SsgEgPickerPanel() { setInterceptsMouseClicks(true, false); }

    void paint(juce::Graphics& g) override
    {
        g.setColour(juce::Colour(0xf51a1a2e));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 10.f);
        g.setColour(juce::Colour(0xFF00D4AA));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(.5f), 10.f, 2.f);

        g.setColour(juce::Colour(0xFF00D4AA));
        g.setFont(juce::Font("Courier New", 11.f, juce::Font::bold));
        g.drawText("Select SSG-EG Mode",
                   getLocalBounds().withHeight(24).reduced(10,0),
                   juce::Justification::centredLeft);

        auto grid = getLocalBounds().withTrimmedTop(24).reduced(6);
        int itemW = grid.getWidth() / 3;
        int itemH = grid.getHeight() / 3;

        for (int i = 0; i < 9; i++)
        {
            int col = i % 3;
            int row = i / 3;
            auto box = juce::Rectangle<int>(
                grid.getX() + col * itemW, 
                grid.getY() + row * itemH, 
                itemW, itemH).reduced(4);

            if (i == hoveredMode || i == selectedMode)
            {
                g.setColour(i == selectedMode ? juce::Colour(0xFF252550)
                                              : juce::Colour(0xFF202038));
                g.fillRoundedRectangle(box.toFloat(), 5.f);
            }
            if (i == selectedMode)
            {
                g.setColour(juce::Colour(0xFF00D4AA));
                g.drawRoundedRectangle(box.toFloat(), 5.f, 2.f);
            }

            auto diagramArea = box.reduced(8).withTrimmedBottom(20);
            drawSsgMode(g, i, diagramArea);

            g.setColour(i == selectedMode ? juce::Colour(0xFF00D4AA)
                                          : juce::Colour(0xFF556070));
            g.setFont(juce::Font("Courier New", 9.f, juce::Font::plain));
            juce::String label = (i == 0) ? "Off" : getSsgModeName(i - 1);
            g.drawText(label, box.withTrimmedTop(box.getHeight() - 18),
                       juce::Justification::centred, false);
        }
    }

    void mouseMove(const juce::MouseEvent& e) override
    {
        auto grid = getLocalBounds().withTrimmedTop(24).reduced(6);
        int itemW = juce::jmax(1, grid.getWidth() / 3);
        int itemH = juce::jmax(1, grid.getHeight() / 3);
        int col = juce::jlimit(0, 2, (e.x - grid.getX()) / itemW);
        int row = juce::jlimit(0, 2, (e.y - grid.getY()) / itemH);
        int n = row * 3 + col;
        if (n != hoveredMode) { hoveredMode = n; repaint(); }
    }

    void mouseExit(const juce::MouseEvent&) override { hoveredMode = -1; repaint(); }

    void mouseDown(const juce::MouseEvent& e) override
    {
        auto grid = getLocalBounds().withTrimmedTop(24).reduced(6);
        int itemW = juce::jmax(1, grid.getWidth() / 3);
        int itemH = juce::jmax(1, grid.getHeight() / 3);
        int col = juce::jlimit(0, 2, (e.x - grid.getX()) / itemW);
        int row = juce::jlimit(0, 2, (e.y - grid.getY()) / itemH);
        int mode = row * 3 + col;
        if (mode >= 0 && mode < 9 && onSelect) onSelect(mode);
    }

private:
    juce::String getSsgModeName(int mode)
    {
        const char* names[] = {
            "Down Down Down",  // Mode 0
            "Down.",           // Mode 1
            "Down Up Down Up", // Mode 2
            "Down UP",         // Mode 3
            "Up Up Up",        // Mode 4
            "Up.",             // Mode 5
            "Up Down Up Down", // Mode 6
            "Up DOWN"          // Mode 7
        };
        return (mode >= 0 && mode < 8) ? names[mode] : "?";
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// SsgEgSelector - Dropdown-style selector with label above diagram
// ─────────────────────────────────────────────────────────────────────────────
class SsgEgSelector : public juce::Component, public juce::SettableTooltipClient
{
public:
    SsgEgSelector() = default;

    void setSelectedMode(int mode)
    {
        selectedMode = juce::jlimit(0, 8, mode);
        repaint();
    }

    int getSelectedMode() const { return selectedMode; }

    std::function<void(int)> onChange;

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();
        
        // Label "SSG-EG" at top
        auto labelArea = bounds.removeFromTop(14);
        g.setColour(juce::Colour(0xFF556070));
        g.setFont(juce::Font(9.5f));
        g.drawText("SSG-EG", labelArea, juce::Justification::centredLeft, false);
        
        // Dropdown box below label (square aspect ratio)
        auto boxArea = bounds.reduced(0, 1);
        g.setColour(juce::Colour(0xFF161625));
        g.fillRoundedRectangle(boxArea.toFloat(), 3.0f);
        g.setColour(juce::Colour(0xFF2a2a3e));
        g.drawRoundedRectangle(boxArea.toFloat(), 3.0f, 1.0f);
        
        // Draw current mode diagram using same method as popup
        auto diagramArea = boxArea.reduced(4);
        drawSsgMode(g, selectedMode, diagramArea);
    }

    void mouseDown(const juce::MouseEvent&) override { showModal(); }

private:
    int selectedMode = 0;
    ModalBackdrop<SsgEgPickerPanel>* activeModal = nullptr;

    void showModal()
    {
        if (activeModal) return;
        auto* root = getTopLevelComponent();
        if (!root) return;

        auto panel = std::make_unique<SsgEgPickerPanel>();
        panel->selectedMode = selectedMode;
        
        panel->onSelect = [this](int mode)
        {
            setSelectedMode(mode);
            if (onChange) onChange(mode);
            closeModal();
        };

        const int pw = juce::jmin(400, (int)(root->getWidth() * 0.65f));
        const int ph = juce::jmin(300, (int)(root->getHeight() * 0.70f));

        activeModal = new ModalBackdrop<SsgEgPickerPanel>(
            panel.release(),
            [this]() { closeModal(); }
        );
        
        activeModal->setBounds(root->getLocalBounds());
        activeModal->pickerPanel->setBounds(
            (activeModal->getWidth() - pw) / 2,
            (activeModal->getHeight() - ph) / 2,
            pw, ph
        );
        
        root->addAndMakeVisible(activeModal);
        activeModal->toFront(true);
    }

    void closeModal()
    {
        if (!activeModal) return;
        activeModal->dismiss();
        activeModal = nullptr;
    }
};
