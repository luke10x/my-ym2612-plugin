#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// =============================================================================
// Built-in YM2612 Patches
// =============================================================================
struct BuiltInPatch
{
    juce::String name;
    juce::String code;  // C++ code for the patch
};

static const BuiltInPatch kBuiltInPatches[] = {
    // Bass
    { 
        "Slap Bass",
        "constexpr YM2612Patch SLAP_BASS =\n"
        "{\n"
        "    .ALG = 4,\n"
        "    .FB  = 5,\n"
        "    .AMS = 2,\n"
        "    .FMS = 3,\n"
        "\n"
        "    .op =\n"
        "    {\n"
        "        {3,1,34,0,31,0,10,6,4,7,0},\n"
        "        {0,2,18,1,25,0,12,5,5,6,0},\n"
        "        {0,1,0,0,31,0,6,3,6,5,0},\n"
        "        {0,1,0,0,31,0,7,2,5,5,0}\n"
        "    }\n"
        "};"
    },
    { 
        "Synth Bass",
        "constexpr YM2612Patch SYNTH_BASS =\n"
        "{\n"
        "    .ALG = 5,\n"
        "    .FB  = 7,\n"
        "    .AMS = 0,\n"
        "    .FMS = 4,\n"
        "\n"
        "    .op =\n"
        "    {\n"
        "        {0,1,20,0,31,0,15,7,3,8,0},\n"
        "        {0,1,15,0,28,0,12,6,4,7,0},\n"
        "        {0,0,0,0,31,0,0,0,0,0,0},\n"
        "        {0,0,0,0,31,0,0,0,0,0,0}\n"
        "    }\n"
        "};"
    },
    { 
        "Electric Bass",
        "constexpr YM2612Patch ELECTRIC_BASS =\n"
        "{\n"
        "    .ALG = 4,\n"
        "    .FB  = 6,\n"
        "    .AMS = 1,\n"
        "    .FMS = 2,\n"
        "\n"
        "    .op =\n"
        "    {\n"
        "        {2,1,28,0,31,0,12,5,4,6,0},\n"
        "        {0,1,22,0,26,0,10,4,5,5,0},\n"
        "        {0,1,0,0,31,0,5,3,6,4,0},\n"
        "        {0,0,0,0,31,0,0,0,0,0,0}\n"
        "    }\n"
        "};"
    },
    { 
        "Acoustic Bass",
        "constexpr YM2612Patch ACOUSTIC_BASS =\n"
        "{\n"
        "    .ALG = 2,\n"
        "    .FB  = 3,\n"
        "    .AMS = 0,\n"
        "    .FMS = 1,\n"
        "\n"
        "    .op =\n"
        "    {\n"
        "        {1,1,24,0,30,0,8,4,3,5,0},\n"
        "        {0,2,16,1,24,0,10,5,4,6,0},\n"
        "        {0,1,12,0,28,0,6,3,5,4,0},\n"
        "        {0,0,0,0,31,0,0,0,0,0,0}\n"
        "    }\n"
        "};"
    },
};

// =============================================================================
// PatchesPanel - List of built-in patches with code preview
// =============================================================================
class PatchesPanel : public juce::Component, public juce::ListBoxModel
{
public:
    std::function<void()> onClose;
    std::function<void(int)> onPatchSelected;
    
    PatchesPanel() : patchList("Patches", nullptr), selectedPatch(0)
    {
        setInterceptsMouseClicks(true, true);
        
        // Code display (left side)
        codeDisplay.setMultiLine(true);
        codeDisplay.setReadOnly(true);
        codeDisplay.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 11.0f, juce::Font::plain));
        codeDisplay.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xFF0D0D1A));
        codeDisplay.setColour(juce::TextEditor::textColourId, juce::Colour(0xFF00D4AA));
        codeDisplay.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xFF252540));
        codeDisplay.setText(kBuiltInPatches[0].code);
        addAndMakeVisible(codeDisplay);
        
        // Patch list (right side)
        patchList.setModel(this);
        patchList.setRowHeight(28);
        patchList.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xFF0D0D1A));
        patchList.setColour(juce::ListBox::outlineColourId, juce::Colour(0xFF252540));
        patchList.selectRow(0);
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
        bounds.removeFromTop(8);  // Spacing
        
        // Close button at bottom
        auto buttonArea = bounds.removeFromBottom(36);
        closeButton.setBounds(buttonArea.withSizeKeepingCentre(120, 36));
        
        bounds.removeFromBottom(16); // Space above button
        
        // Split remaining area: code on left (60%), list on right (40%)
        auto listArea = bounds.removeFromRight(bounds.getWidth() * 0.4f);
        listArea.removeFromLeft(8); // Gap between code and list
        
        codeDisplay.setBounds(bounds);
        patchList.setBounds(listArea);
    }
    
    // ListBoxModel methods
    int getNumRows() override
    {
        return sizeof(kBuiltInPatches) / sizeof(kBuiltInPatches[0]);
    }
    
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override
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
        
        // Patch name (centered)
        g.setColour(rowIsSelected ? juce::Colour(0xFF00D4AA) : juce::Colour(0xFFCCCCCC));
        g.setFont(juce::Font("Courier New", 12.f, juce::Font::plain));
        g.drawText(patch.name, 8, 0, width - 16, height, juce::Justification::centredLeft);
    }
    
    void listBoxItemClicked(int row, const juce::MouseEvent&) override
    {
        if (row >= 0 && row < getNumRows())
        {
            selectedPatch = row;
            codeDisplay.setText(kBuiltInPatches[row].code);
        }
        
        if (onPatchSelected)
            onPatchSelected(row);
    }
    
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override
    {
        if (onPatchSelected)
            onPatchSelected(row);
        if (onClose)
            onClose();
    }

private:
    juce::ListBox patchList;
    juce::TextEditor codeDisplay;
    juce::TextButton closeButton;
    int selectedPatch;
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
