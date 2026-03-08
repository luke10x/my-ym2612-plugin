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
    
    PatchesPanel() : patchList("Patches", nullptr), selectedPatch(0), codeModified(false)
    {
        setInterceptsMouseClicks(true, true);
        
        // Code display (left side) - now editable
        codeDisplay.setMultiLine(true);
        codeDisplay.setReadOnly(false);  // Editable
        codeDisplay.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 11.0f, juce::Font::plain));
        codeDisplay.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xFF0D0D1A));
        codeDisplay.setColour(juce::TextEditor::textColourId, juce::Colour(0xFF00D4AA));
        codeDisplay.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xFF252540));
        codeDisplay.setText(kBuiltInPatches[0].code);
        codeDisplay.onTextChange = [this]() { 
            codeModified = true; 
            validateButton.setEnabled(true);
        };
        addAndMakeVisible(codeDisplay);
        
        // Error display
        errorLabel.setFont(juce::Font("Courier New", 10.0f, juce::Font::plain));
        errorLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFFF4444));
        errorLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xFF1a1a2e));
        errorLabel.setJustificationType(juce::Justification::centredLeft);
        errorLabel.setText("", juce::dontSendNotification);
        addAndMakeVisible(errorLabel);
        
        // Validate & Load button
        validateButton.setButtonText("Validate & Load");
        validateButton.setEnabled(false);  // Disabled until code is edited
        validateButton.onClick = [this]() { validateAndLoadPatch(); };
        addAndMakeVisible(validateButton);
        
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
        
        bounds.removeFromBottom(8); // Space above button
        
        // Validate button and error label above close button
        auto validateArea = bounds.removeFromBottom(30);
        validateButton.setBounds(validateArea.removeFromLeft(140));
        validateArea.removeFromLeft(8);
        errorLabel.setBounds(validateArea);
        
        bounds.removeFromBottom(8); // Space above validate section
        
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
            codeModified = false;
            validateButton.setEnabled(false);
            errorLabel.setText("", juce::dontSendNotification);
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
    
    void validateAndLoadPatch()
    {
        juce::String code = codeDisplay.getText();
        juce::String error;
        int errorLine = 0, errorCol = 0;
        
        if (parsePatchCode(code, error, errorLine, errorCol))
        {
            // Success
            errorLabel.setText("✓ Patch valid and loaded", juce::dontSendNotification);
            errorLabel.setColour(juce::Label::textColourId, juce::Colour(0xFF00FF88));
            codeModified = false;
            validateButton.setEnabled(false);
            
            // TODO: Actually load the parsed patch into the synth
        }
        else
        {
            // Error
            errorLabel.setText(juce::String::formatted("Invalid patch syntax at %d:%d - %s", 
                                                        errorLine, errorCol, error.toRawUTF8()), 
                               juce::dontSendNotification);
            errorLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFFF4444));
        }
    }
    
    bool parsePatchCode(const juce::String& code, juce::String& error, int& errorLine, int& errorCol)
    {
        // Simple parser for YM2612Patch struct format
        auto lines = juce::StringArray::fromLines(code);
        
        errorLine = 1;
        errorCol = 1;
        
        // Track required fields
        bool foundALG = false, foundFB = false, foundAMS = false, foundFMS = false;
        bool foundOp = false;
        int opCount = 0;
        
        for (int i = 0; i < lines.size(); ++i)
        {
            auto line = lines[i].trim();
            errorLine = i + 1;
            
            // Skip empty lines and comments
            if (line.isEmpty() || line.startsWith("//"))
                continue;
            
            // Check for opening brace
            if (line.contains("constexpr") && line.contains("YM2612Patch"))
                continue;
                
            if (line == "{")
                continue;
                
            if (line == "};")
                continue;
            
            // Parse .ALG = value
            if (line.startsWith(".ALG"))
            {
                if (!parseIntField(line, ".ALG", 0, 7, error, errorCol))
                    return false;
                foundALG = true;
                continue;
            }
            
            // Parse .FB = value
            if (line.startsWith(".FB"))
            {
                if (!parseIntField(line, ".FB", 0, 7, error, errorCol))
                    return false;
                foundFB = true;
                continue;
            }
            
            // Parse .AMS = value
            if (line.startsWith(".AMS"))
            {
                if (!parseIntField(line, ".AMS", 0, 3, error, errorCol))
                    return false;
                foundAMS = true;
                continue;
            }
            
            // Parse .FMS = value
            if (line.startsWith(".FMS"))
            {
                if (!parseIntField(line, ".FMS", 0, 7, error, errorCol))
                    return false;
                foundFMS = true;
                continue;
            }
            
            // Parse .op = section
            if (line.startsWith(".op"))
            {
                foundOp = true;
                continue;
            }
            
            // Parse operator arrays like {3,1,34,0,31,0,10,6,4,7,0}
            if (foundOp && line.startsWith("{") && line.contains("}"))
            {
                if (!parseOperatorArray(line, error, errorCol))
                    return false;
                opCount++;
                continue;
            }
        }
        
        // Check all required fields are present
        if (!foundALG)
        {
            error = ".ALG field required";
            return false;
        }
        if (!foundFB)
        {
            error = ".FB field required";
            return false;
        }
        if (!foundAMS)
        {
            error = ".AMS field required";
            return false;
        }
        if (!foundFMS)
        {
            error = ".FMS field required";
            return false;
        }
        if (!foundOp)
        {
            error = ".op array required";
            return false;
        }
        if (opCount != 4)
        {
            error = juce::String::formatted("Expected 4 operators, found %d", opCount);
            return false;
        }
        
        return true;
    }
    
    bool parseIntField(const juce::String& line, const juce::String& fieldName, 
                       int minVal, int maxVal, juce::String& error, int& errorCol)
    {
        auto parts = juce::StringArray::fromTokens(line, "=,", "");
        if (parts.size() < 2)
        {
            error = fieldName + " value expected";
            errorCol = line.length();
            return false;
        }
        
        int value = parts[1].trim().getIntValue();
        if (value < minVal || value > maxVal)
        {
            error = juce::String::formatted("%s must be %d-%d, got %d", 
                                           fieldName.toRawUTF8(), minVal, maxVal, value);
            errorCol = line.indexOf(parts[1]);
            return false;
        }
        
        return true;
    }
    
    bool parseOperatorArray(const juce::String& line, juce::String& error, int& errorCol)
    {
        // Extract values between { }
        int start = line.indexOf("{");
        int end = line.indexOf("}");
        
        if (start < 0 || end < 0)
        {
            error = "Operator array must be enclosed in { }";
            return false;
        }
        
        juce::String values = line.substring(start + 1, end);
        auto parts = juce::StringArray::fromTokens(values, ",", "");
        
        if (parts.size() != 11)
        {
            error = juce::String::formatted("Operator array expects 11 values, got %d", parts.size());
            errorCol = start;
            return false;
        }
        
        // Validate each value (all should be integers)
        for (int i = 0; i < parts.size(); ++i)
        {
            if (!parts[i].trim().containsOnly("0123456789"))
            {
                error = juce::String::formatted("Invalid value '%s' at position %d", 
                                               parts[i].trim().toRawUTF8(), i + 1);
                errorCol = line.indexOf(parts[i]);
                return false;
            }
        }
        
        return true;
    }

private:
    juce::ListBox patchList;
    juce::TextEditor codeDisplay;
    juce::TextButton closeButton;
    juce::TextButton validateButton;
    juce::Label errorLabel;
    int selectedPatch;
    bool codeModified;
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
