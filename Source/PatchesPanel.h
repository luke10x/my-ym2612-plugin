#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// =============================================================================
// Built-in YM2612 Patches
// =============================================================================
struct BuiltInPatch
{
    juce::String name;
    juce::String category;  // Bass, Lead, Pad, etc.
};

static const BuiltInPatch kBuiltInPatches[] = {
    // Bass
    { "Slap Bass", "Bass" },
    { "Synth Bass", "Bass" },
    { "Electric Bass", "Bass" },
    { "Acoustic Bass", "Bass" },
    
    // Lead
    { "Square Lead", "Lead" },
    { "Saw Lead", "Lead" },
    { "FM Lead", "Lead" },
    { "Trumpet", "Lead" },
    
    // Pad
    { "Soft Pad", "Pad" },
    { "Strings", "Pad" },
    { "Choir", "Pad" },
    { "Brass Section", "Pad" },
    
    // Pluck
    { "Piano", "Pluck" },
    { "Marimba", "Pluck" },
    { "Steel Drums", "Pluck" },
    { "Harp", "Pluck" },
    
    // SFX
    { "Bell", "SFX" },
    { "Metallic", "SFX" },
    { "Noise Sweep", "SFX" },
    { "Thunder", "SFX" },
};

// =============================================================================
// PatchesPanel - List of built-in patches
// =============================================================================
class PatchesPanel : public juce::Component, public juce::ListBoxModel
{
public:
    std::function<void()> onClose;
    std::function<void(int)> onPatchSelected;
    
    PatchesPanel() : patchList("Patches", nullptr)
    {
        setInterceptsMouseClicks(true, true);
        
        // Patch list
        patchList.setModel(this);
        patchList.setRowHeight(24);
        patchList.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xFF0D0D1A));
        patchList.setColour(juce::ListBox::outlineColourId, juce::Colour(0xFF252540));
        addAndMakeVisible(patchList);
        
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
        g.drawText("Built-in Patches", getLocalBounds().withHeight(40).reduced(16, 0),
                   juce::Justification::centredLeft);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(16);
        bounds.removeFromTop(40); // Title area
        
        bounds.removeFromTop(8); // Spacing
        
        // Patch list
        auto listArea = bounds;
        listArea.removeFromBottom(52); // Space for button
        patchList.setBounds(listArea);
        
        // Close button at bottom
        auto buttonArea = bounds.removeFromBottom(36);
        closeButton.setBounds(buttonArea.withSizeKeepingCentre(120, 36));
    }
    
    // ListBoxModel methods
    int getNumRows()
    {
        return sizeof(kBuiltInPatches) / sizeof(kBuiltInPatches[0]);
    }
    
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
    {
        if (rowNumber < 0 || rowNumber >= getNumRows())
            return;
            
        auto& patch = kBuiltInPatches[rowNumber];
        
        // Background
        if (rowIsSelected)
            g.fillAll(juce::Colour(0xFF252550));
        else if (rowNumber % 2 == 0)
            g.fillAll(juce::Colour(0xFF161625));
        else
            g.fillAll(juce::Colour(0xFF0D0D1A));
        
        // Patch name
        g.setColour(rowIsSelected ? juce::Colour(0xFF00D4AA) : juce::Colour(0xFFCCCCCC));
        g.setFont(juce::Font("Courier New", 12.f, juce::Font::plain));
        g.drawText(patch.name, 8, 0, width - 120, height, juce::Justification::centredLeft);
        
        // Category tag
        g.setColour(juce::Colour(0xFF556070));
        g.setFont(juce::Font("Courier New", 10.f, juce::Font::plain));
        g.drawText("[" + patch.category + "]", width - 110, 0, 100, height, juce::Justification::centredLeft);
    }
    
    void listBoxItemClicked(int row, const juce::MouseEvent&)
    {
        if (onPatchSelected)
            onPatchSelected(row);
    }
    
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&)
    {
        if (onPatchSelected)
            onPatchSelected(row);
        if (onClose)
            onClose();
    }

private:
    juce::ListBox patchList;
    juce::TextButton closeButton;
};

// =============================================================================
// PatchesModal - Modal wrapper for patches panel
// =============================================================================
class PatchesModal : public juce::Component
{
public:
    PatchesPanel* panel;
    std::function<void()> onDismiss;
    
    PatchesModal(PatchesPanel* patchesPanel, std::function<void()> dismissCallback)
        : panel(patchesPanel), onDismiss(dismissCallback)
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

    std::unique_ptr<PatchesModal> selfReference;
};
