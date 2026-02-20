#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>

// ─────────────────────────────────────────────────────────────────────────────
// OscilloscopeDisplay - Shows real-time audio waveform
// ─────────────────────────────────────────────────────────────────────────────
class OscilloscopeDisplay : public juce::Component,
                            private juce::Timer
{
public:
    OscilloscopeDisplay()
    {
        buffer.clear();
        buffer.resize(bufferSize, 0.0f);
        startTimerHz(60);  // 60 FPS refresh
    }

    void pushSample(float sample)
    {
        const juce::ScopedLock sl(bufferLock);
        buffer[writePosition] = sample;
        writePosition = (writePosition + 1) % bufferSize;
        
        // Detect zero crossings for phase lock
        if (phaseLockEnabled && lastSample <= 0.0f && sample > 0.0f)
        {
            triggerPosition = writePosition;
        }
        lastSample = sample;
    }
    
    void setPhaseLock(bool enabled) { phaseLockEnabled = enabled; }
    bool getPhaseLock() const { return phaseLockEnabled; }
    
    void setZoom(float zoomFactor) { zoom = juce::jlimit(1.0f, 5.0f, zoomFactor); }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();
        
        // Background
        g.setColour(juce::Colour(0xFF0a0a15));
        g.fillRect(bounds);
        
        // Border
        g.setColour(juce::Colour(0xFF2a2a3e));
        g.drawRect(bounds, 1);
        
        // Center line
        g.setColour(juce::Colour(0xFF1a1a2e));
        g.drawLine(0, bounds.getCentreY(), static_cast<float>(bounds.getWidth()), 
                   static_cast<float>(bounds.getCentreY()), 1.0f);
        
        // Draw waveform
        const juce::ScopedLock sl(bufferLock);
        
        juce::Path waveform;
        bool started = false;
        
        float width = static_cast<float>(bounds.getWidth());
        float height = static_cast<float>(bounds.getHeight());
        float centerY = bounds.getCentreY();
        float amplitude = height * 0.45f;  // Use 90% of height for waveform (±45% from center)
        
        // Use zoom to reduce number of samples displayed (zooms in on x-axis)
        int samplesToDisplay = static_cast<int>(bufferSize / zoom);
        samplesToDisplay = juce::jmin(samplesToDisplay, bufferSize);
        
        // Start position: use trigger position if phase-locked, otherwise follow write position
        int startPos = phaseLockEnabled ? triggerPosition : writePosition;
        
        for (int i = 0; i < samplesToDisplay; ++i)
        {
            int readPos = (startPos + i) % bufferSize;
            float sample = buffer[readPos];
            
            // Clamp sample to prevent drawing outside bounds
            sample = juce::jlimit(-1.0f, 1.0f, sample);
            
            float x = (i / static_cast<float>(samplesToDisplay)) * width;
            float y = centerY - (sample * amplitude);
            
            if (!started)
            {
                waveform.startNewSubPath(x, y);
                started = true;
            }
            else
            {
                waveform.lineTo(x, y);
            }
        }
        
        // Draw the waveform
        g.setColour(juce::Colour(0xFF00D4AA));
        g.strokePath(waveform, juce::PathStrokeType(1.5f));
        
        // Label
        g.setColour(juce::Colour(0xFF556070));
        g.setFont(juce::Font(9.0f));
        g.drawText("SCOPE", bounds.reduced(4), juce::Justification::topLeft, false);
    }

    void timerCallback() override
    {
        repaint();
    }

private:
    static constexpr int bufferSize = 2048;  // Circular buffer size
    std::vector<float> buffer;
    int writePosition = 0;
    int triggerPosition = 0;
    float lastSample = 0.0f;
    bool phaseLockEnabled = false;
    float zoom = 2.5f;  // 2.5x zoom by default
    juce::CriticalSection bufferLock;
};
