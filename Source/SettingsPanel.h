#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// =============================================================================
// SettingsPanel - Panel for plugin settings
// =============================================================================
class SettingsPanel : public juce::Component
{
public:
    std::function<void()> onClose;
    std::function<void(bool)> onTooltipsChanged;
    
    SettingsPanel(bool tooltipsEnabled)
    {
        setInterceptsMouseClicks(true, true);
        
        // Tooltips toggle
        tooltipsToggle.setButtonText("Show tooltips");
        tooltipsToggle.setToggleState(tooltipsEnabled, juce::dontSendNotification);
        tooltipsToggle.onClick = [this]() {
            if (onTooltipsChanged)
                onTooltipsChanged(tooltipsToggle.getToggleState());
        };
        addAndMakeVisible(tooltipsToggle);
        
        // Close button
        closeButton.setButtonText("Close");
        closeButton.onClick = [this]() {
            if (onClose)
                onClose();
        };
        addAndMakeVisible(closeButton);
    }

    void paint(juce::Graphics& g) override
    {
        // Panel background
        g.setColour(juce::Colour(0xf51a1a2e));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 10.f);
        
        // Border
        g.setColour(juce::Colour(0xFF00D4AA));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(.5f), 10.f, 2.f);
        
        // Title
        g.setColour(juce::Colour(0xFF00D4AA));
        g.setFont(juce::Font("Courier New", 14.f, juce::Font::bold));
        g.drawText("Settings", getLocalBounds().withHeight(40).reduced(16, 0),
                   juce::Justification::centredLeft);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(16);
        bounds.removeFromTop(40); // Title area
        
        bounds.removeFromTop(8); // Spacing
        
        // Tooltips toggle
        tooltipsToggle.setBounds(bounds.removeFromTop(30));
        
        bounds.removeFromTop(16); // Spacing before button
        
        // Close button at bottom
        auto buttonArea = bounds.removeFromBottom(36);
        closeButton.setBounds(buttonArea.withSizeKeepingCentre(120, 36));
    }

private:
    juce::ToggleButton tooltipsToggle;
    juce::TextButton closeButton;
};

// =============================================================================
// SettingsModal - Modal wrapper for settings panel (no backdrop dismiss)
// =============================================================================
class SettingsModal : public juce::Component
{
public:
    SettingsPanel* panel;
    std::function<void()> onDismiss;
    
    SettingsModal(SettingsPanel* settingsPanel, std::function<void()> dismissCallback)
        : panel(settingsPanel), onDismiss(dismissCallback)
    {
        setInterceptsMouseClicks(true, true);
        addAndMakeVisible(panel);
        
        panel->onClose = [this]() {
            if (onDismiss)
                onDismiss();
        };
    }

    void paint(juce::Graphics& g) override
    {
        // Dark semi-transparent backdrop
        g.setColour(juce::Colour(0xcc000000));
        g.fillRect(getLocalBounds());
    }

    // Swallow all mouse events - no dismiss on backdrop click
    void mouseDown(const juce::MouseEvent&) override {}
    void mouseUp(const juce::MouseEvent&) override {}
    void mouseDrag(const juce::MouseEvent&) override {}
    void mouseMove(const juce::MouseEvent&) override {}

    void dismiss()
    {
        if (auto* parent = getParentComponent())
            parent->removeChildComponent(this);
        selfReference.reset();  // Deletes this
    }

    std::unique_ptr<SettingsModal> selfReference;
};
