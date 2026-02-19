#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// ─────────────────────────────────────────────────────────────────────────────
// SsgEgPopup - Popup window showing all 9 SSG-EG modes (Off + 8 modes)
// ─────────────────────────────────────────────────────────────────────────────
class SsgEgPopup : public juce::Component
{
public:
    SsgEgPopup(int currentMode) : selectedMode(currentMode) {}

    std::function<void(int)> onSelect;

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xFF1a1a2e));
        g.setColour(juce::Colour(0xFF00D4AA));
        g.drawRect(getLocalBounds(), 2);

        int itemW = getWidth() / 3;
        int itemH = getHeight() / 3;

        for (int i = 0; i < 9; i++)
        {
            int col = i % 3;
            int row = i / 3;
            auto box = juce::Rectangle<int>(col * itemW, row * itemH, itemW, itemH).reduced(4);

            // Hover or selected
            if (i == hoveredMode || i == selectedMode)
            {
                g.setColour(i == selectedMode ? juce::Colour(0xFF252540) : juce::Colour(0xFF202030));
                g.fillRoundedRectangle(box.toFloat(), 4.0f);
                if (i == selectedMode)
                {
                    g.setColour(juce::Colour(0xFF00D4AA));
                    g.drawRoundedRectangle(box.toFloat(), 4.0f, 2.0f);
                }
            }

            // Draw SSG-EG diagram
            auto diagramArea = box.reduced(8).withTrimmedBottom(20);
            drawSsgMode(g, i, diagramArea);

            // Mode label
            g.setColour(juce::Colour(0xFF556070));
            g.setFont(juce::Font(9.0f));
            juce::String label = (i == 0) ? "Off" : getSsgModeName(i - 1);
            g.drawText(label, box.withTrimmedTop(box.getHeight() - 18),
                       juce::Justification::centred, false);
        }
    }

    void mouseMove(const juce::MouseEvent& e) override
    {
        int itemW = getWidth() / 3;
        int itemH = getHeight() / 3;
        int col = e.x / itemW;
        int row = e.y / itemH;
        int newHover = row * 3 + col;
        if (newHover >= 0 && newHover < 9 && newHover != hoveredMode)
        {
            hoveredMode = newHover;
            repaint();
        }
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        hoveredMode = -1;
        repaint();
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        int itemW = getWidth() / 3;
        int itemH = getHeight() / 3;
        int col = e.x / itemW;
        int row = e.y / itemH;
        int mode = row * 3 + col;
        if (mode >= 0 && mode < 9 && onSelect)
        {
            onSelect(mode);
            if (auto* parent = getParentComponent())
                parent->exitModalState(0);
        }
    }

private:
    int selectedMode = 0;
    int hoveredMode = -1;

    juce::String getSsgModeName(int mode)
    {
        const char* names[] = {
            "\\\\\\", "\\__", "\\/\\/", "\\/_",
            "///", "/__", "/\\/\\", "/\\__"
        };
        return (mode >= 0 && mode < 8) ? names[mode] : "";
    }

    void drawSsgMode(juce::Graphics& g, int dropdownIdx, juce::Rectangle<int> area)
    {
        if (dropdownIdx == 0)
        {
            // "Off" - just show text
            g.setColour(juce::Colour(0xFF556070));
            g.setFont(juce::Font(14.0f));
            g.drawText("Off", area, juce::Justification::centred, false);
            return;
        }

        int mode = dropdownIdx - 1;  // Convert dropdown index to SSG mode (0-7)
        
        g.setColour(juce::Colour(0xFF4fc3f7));
        
        float x1 = area.getX() + 8;
        float x2 = area.getRight() - 8;
        float yTop = area.getY() + 8;
        float yBottom = area.getBottom() - 8;
        float yMid = area.getCentreY();
        
        juce::Path path;
        
        // Bit flags: mode = (hold<<2) | (attack<<1) | alternate
        bool alternate = (mode & 1) != 0;
        bool attack = (mode & 2) != 0;
        bool hold = (mode & 4) != 0;
        
        float segmentW = (x2 - x1) / (hold ? 2.0f : 4.0f);
        
        if (!attack)  // Start from top (decay)
        {
            path.startNewSubPath(x1, yTop);
            path.lineTo(x1 + segmentW, yBottom);
            
            if (!hold)
            {
                if (alternate)
                {
                    // Down-Up pattern
                    path.lineTo(x1 + segmentW * 2, yTop);
                    path.lineTo(x1 + segmentW * 3, yBottom);
                    path.lineTo(x1 + segmentW * 4, yTop);
                }
                else
                {
                    // Down-Down pattern (repeat)
                    path.startNewSubPath(x1 + segmentW * 1.2f, yTop);
                    path.lineTo(x1 + segmentW * 2, yBottom);
                    path.startNewSubPath(x1 + segmentW * 2.2f, yTop);
                    path.lineTo(x1 + segmentW * 3, yBottom);
                    path.startNewSubPath(x1 + segmentW * 3.2f, yTop);
                    path.lineTo(x1 + segmentW * 4, yBottom);
                }
            }
            else
            {
                // Hold at bottom or top
                if (alternate)
                    path.lineTo(x2, yTop);  // Hold high
                else
                    path.lineTo(x2, yBottom);  // Hold low
            }
        }
        else  // Start from bottom (attack)
        {
            path.startNewSubPath(x1, yBottom);
            path.lineTo(x1 + segmentW, yTop);
            
            if (!hold)
            {
                if (alternate)
                {
                    // Up-Down pattern
                    path.lineTo(x1 + segmentW * 2, yBottom);
                    path.lineTo(x1 + segmentW * 3, yTop);
                    path.lineTo(x1 + segmentW * 4, yBottom);
                }
                else
                {
                    // Up-Up pattern (repeat)
                    path.startNewSubPath(x1 + segmentW * 1.2f, yBottom);
                    path.lineTo(x1 + segmentW * 2, yTop);
                    path.startNewSubPath(x1 + segmentW * 2.2f, yBottom);
                    path.lineTo(x1 + segmentW * 3, yTop);
                    path.startNewSubPath(x1 + segmentW * 3.2f, yBottom);
                    path.lineTo(x1 + segmentW * 4, yTop);
                }
            }
            else
            {
                // Hold at bottom or top
                if (alternate)
                    path.lineTo(x2, yBottom);  // Hold low
                else
                    path.lineTo(x2, yTop);  // Hold high
            }
        }
        
        g.strokePath(path, juce::PathStrokeType(2.0f));
        
        // Draw axis
        g.setColour(juce::Colour(0xFF556070));
        g.drawLine(x1, yBottom, x2, yBottom, 1.0f);  // Time axis
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// SsgEgSelector - Dropdown-style selector with label above diagram
// ─────────────────────────────────────────────────────────────────────────────
class SsgEgSelector : public juce::Component
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
        
        // Dropdown box below label
        auto boxArea = bounds.reduced(0, 1);
        g.setColour(juce::Colour(0xFF161625));
        g.fillRoundedRectangle(boxArea.toFloat(), 3.0f);
        g.setColour(juce::Colour(0xFF2a2a3e));
        g.drawRoundedRectangle(boxArea.toFloat(), 3.0f, 1.0f);
        
        // Draw current mode diagram or "Off"
        auto diagramArea = boxArea.reduced(4);
        drawSsgMode(g, selectedMode, diagramArea);
        
        // Dropdown arrow (bottom right corner)
        g.setColour(juce::Colour(0xFF556070));
        g.setFont(juce::Font(8.0f));
        g.drawText("▼", boxArea.withLeft(boxArea.getRight() - 12), juce::Justification::centred, false);
    }

    void mouseDown(const juce::MouseEvent&) override
    {
        // Show popup directly below this component
        auto* popup = new SsgEgPopup(selectedMode);
        popup->setSize(360, 270);  // 3x3 grid
        popup->onSelect = [this](int mode) {
            setSelectedMode(mode);
            if (onChange)
                onChange(mode);
        };
        
        juce::CallOutBox::launchAsynchronously(std::unique_ptr<juce::Component>(popup),
                                                getScreenBounds(), nullptr);
    }

private:
    int selectedMode = 0;

    void drawSsgMode(juce::Graphics& g, int dropdownIdx, juce::Rectangle<int> area)
    {
        if (dropdownIdx == 0)
        {
            // "Off"
            g.setColour(juce::Colour(0xFF888888));
            g.setFont(juce::Font(11.0f));
            g.drawText("Off", area, juce::Justification::centred, false);
            return;
        }

        int mode = dropdownIdx - 1;
        
        g.setColour(juce::Colour(0xFF4fc3f7));
        
        float x1 = area.getX() + 4;
        float x2 = area.getRight() - 14;  // Leave space for arrow
        float yTop = area.getY() + 4;
        float yBottom = area.getBottom() - 4;
        
        juce::Path path;
        
        bool alternate = (mode & 1) != 0;
        bool attack = (mode & 2) != 0;
        bool hold = (mode & 4) != 0;
        
        float segmentW = (x2 - x1) / (hold ? 1.5f : 3.0f);
        
        if (!attack)
        {
            path.startNewSubPath(x1, yTop);
            path.lineTo(x1 + segmentW, yBottom);
            
            if (!hold)
            {
                if (alternate)
                {
                    path.lineTo(x1 + segmentW * 2, yTop);
                    path.lineTo(x1 + segmentW * 3, yBottom);
                }
                else
                {
                    path.startNewSubPath(x1 + segmentW * 1.1f, yTop);
                    path.lineTo(x1 + segmentW * 2, yBottom);
                    path.startNewSubPath(x1 + segmentW * 2.1f, yTop);
                    path.lineTo(x1 + segmentW * 3, yBottom);
                }
            }
            else
            {
                path.lineTo(x2, alternate ? yTop : yBottom);
            }
        }
        else
        {
            path.startNewSubPath(x1, yBottom);
            path.lineTo(x1 + segmentW, yTop);
            
            if (!hold)
            {
                if (alternate)
                {
                    path.lineTo(x1 + segmentW * 2, yBottom);
                    path.lineTo(x1 + segmentW * 3, yTop);
                }
                else
                {
                    path.startNewSubPath(x1 + segmentW * 1.1f, yBottom);
                    path.lineTo(x1 + segmentW * 2, yTop);
                    path.startNewSubPath(x1 + segmentW * 2.1f, yBottom);
                    path.lineTo(x1 + segmentW * 3, yTop);
                }
            }
            else
            {
                path.lineTo(x2, alternate ? yBottom : yTop);
            }
        }
        
        g.strokePath(path, juce::PathStrokeType(1.5f));
    }
};
