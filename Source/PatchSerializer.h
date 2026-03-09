#pragma once

#include <juce_core/juce_core.h>
#include "BuiltInPatches.h"

// =============================================================================
// YM2612 Patch Serializer - Parse and serialize patch code
// =============================================================================

class PatchSerializer
{
public:
    // Serialize a YM2612Patch to C++ code string
    static juce::String serializePatch(const YM2612Patch& patch, const juce::String& patchName,
                                       int block = 0, int lfoEnable = 0, int lfoFreq = 0)
    {
        juce::String code;
        
        code << "constexpr YM2612Patch " << patchName << " =\n";
        code << "{\n";
        code << "    .ALG = " << patch.ALG << ",\n";
        code << "    .FB  = " << patch.FB << ",\n";
        code << "    .AMS = " << patch.AMS << ",\n";
        code << "    .FMS = " << patch.FMS << ",\n";
        code << "\n";
        code << "    .op =\n";
        code << "    {\n";
        
        for (int i = 0; i < 4; ++i)
        {
            const auto& op = patch.op[i];
            code << "        { ";
            code << ".DT = " << op.DT << ", ";
            code << ".MUL = " << op.MUL << ", ";
            code << ".TL = " << op.TL << ", ";
            code << ".RS = " << op.RS << ", ";
            code << ".AR = " << op.AR << ", ";
            code << ".AM = " << op.AM << ", ";
            code << ".DR = " << op.DR << ", ";
            code << ".SR = " << op.SR << ", ";
            code << ".SL = " << op.SL << ", ";
            code << ".RR = " << op.RR << ", ";
            code << ".SSG = " << op.SSG;
            code << " }";
            if (i < 3)
                code << ",";
            code << "\n";
        }
        
        code << "    }\n";
        code << "};\n";
        code << "constexpr int " << patchName << "_BLOCK = " << block << ";      // Octave offset\n";
        code << "constexpr int " << patchName << "_LFO_ENABLE = " << lfoEnable << "; // LFO on/off\n";
        code << "constexpr int " << patchName << "_LFO_FREQ = " << lfoFreq << ";   // LFO frequency (0-7)";
        
        return code;
    }
    
    // Parse C++ code string to YM2612Patch
    static bool parsePatch(const juce::String& code, YM2612Patch& outPatch, 
                          int& outBlock, int& outLfoEnable, int& outLfoFreq,
                          juce::String& error, int& errorLine, int& errorCol)
    {
        auto lines = juce::StringArray::fromLines(code);
        
        errorLine = 1;
        errorCol = 1;
        
        // Track required fields
        bool foundALG = false, foundFB = false, foundAMS = false, foundFMS = false;
        bool foundOp = false;
        int opCount = 0;
        
        // Optional fields with defaults
        bool foundBlock = false;
        bool foundLfoEnable = false;
        bool foundLfoFreq = false;
        
        outBlock = 0;
        outLfoEnable = 0;
        outLfoFreq = 0;
        
        for (int i = 0; i < lines.size(); ++i)
        {
            auto line = lines[i].trim();
            errorLine = i + 1;
            
            // Skip empty lines and comments
            if (line.isEmpty() || line.startsWith("//"))
                continue;
            
            // Skip declaration and braces
            if (line.contains("constexpr") && line.contains("YM2612Patch"))
                continue;
                
            if (line == "{" || line == "};")
                continue;
            
            // Parse BLOCK variable
            if (line.contains("_BLOCK"))
            {
                if (!parseConstInt(line, outBlock, error, errorCol))
                    return false;
                foundBlock = true;
                continue;
            }
            
            // Parse LFO_ENABLE variable
            if (line.contains("_LFO_ENABLE"))
            {
                if (!parseConstInt(line, outLfoEnable, error, errorCol))
                    return false;
                foundLfoEnable = true;
                continue;
            }
            
            // Parse LFO_FREQ variable
            if (line.contains("_LFO_FREQ"))
            {
                if (!parseConstInt(line, outLfoFreq, error, errorCol))
                    return false;
                foundLfoFreq = true;
                continue;
            }
            
            // Parse .ALG = value
            if (line.startsWith(".ALG"))
            {
                if (!parseIntField(line, ".ALG", 0, 7, outPatch.ALG, error, errorCol))
                    return false;
                foundALG = true;
                continue;
            }
            
            // Parse .FB = value
            if (line.startsWith(".FB"))
            {
                if (!parseIntField(line, ".FB", 0, 7, outPatch.FB, error, errorCol))
                    return false;
                foundFB = true;
                continue;
            }
            
            // Parse .AMS = value
            if (line.startsWith(".AMS"))
            {
                if (!parseIntField(line, ".AMS", 0, 3, outPatch.AMS, error, errorCol))
                    return false;
                foundAMS = true;
                continue;
            }
            
            // Parse .FMS = value
            if (line.startsWith(".FMS"))
            {
                if (!parseIntField(line, ".FMS", 0, 7, outPatch.FMS, error, errorCol))
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
            
            // Parse operator with explicit properties: { .DT = 3, .MUL = 1, ... }
            if (foundOp && line.contains("{") && line.contains("}"))
            {
                if (opCount >= 4)
                {
                    error = "Too many operators (expected 4)";
                    return false;
                }
                
                if (!parseOperatorWithProps(line, outPatch.op[opCount], error, errorCol))
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

private:
    static bool parseConstInt(const juce::String& line, int& outValue,
                             juce::String& error, int& errorCol)
    {
        // Parse lines like: constexpr int PATCH_NAME_BLOCK = 0;
        auto parts = juce::StringArray::fromTokens(line, "=;", "");
        if (parts.size() < 2)
        {
            error = "Const int value expected";
            errorCol = line.length();
            return false;
        }
        
        outValue = parts[1].trim().getIntValue();
        return true;
    }
    
    static bool parseIntField(const juce::String& line, const juce::String& fieldName, 
                             int minVal, int maxVal, int& outValue,
                             juce::String& error, int& errorCol)
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
        
        outValue = value;
        return true;
    }
    
    static bool parseOperatorArray(const juce::String& line, YM2612Operator& outOp,
                                   juce::String& error, int& errorCol)
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
        
        // Parse values: DT, MUL, TL, RS, AR, AM, DR, SR, SL, RR, SSG
        int vals[11];
        for (int i = 0; i < 11; ++i)
        {
            auto trimmed = parts[i].trim();
            if (!trimmed.containsOnly("0123456789"))
            {
                error = juce::String::formatted("Invalid value '%s' at position %d", 
                                               trimmed.toRawUTF8(), i + 1);
                errorCol = line.indexOf(parts[i]);
                return false;
            }
            vals[i] = trimmed.getIntValue();
        }
        
        // Assign to operator
        outOp.DT = vals[0];
        outOp.MUL = vals[1];
        outOp.TL = vals[2];
        outOp.RS = vals[3];
        outOp.AR = vals[4];
        outOp.AM = vals[5];
        outOp.DR = vals[6];
        outOp.SR = vals[7];
        outOp.SL = vals[8];
        outOp.RR = vals[9];
        outOp.SSG = vals[10];
        
        return true;
    }
    
    static bool parseOperatorWithProps(const juce::String& line, YM2612Operator& outOp,
                                       juce::String& error, int& errorCol)
    {
        // Parse lines like: { .DT = 3, .MUL = 1, .TL = 34, ... }
        
        // Extract content between { }
        int start = line.indexOf("{");
        int end = line.indexOf("}");
        
        if (start < 0 || end < 0)
        {
            error = "Operator must be enclosed in { }";
            return false;
        }
        
        juce::String content = line.substring(start + 1, end);
        
        // Initialize to defaults
        outOp.DT = 0; outOp.MUL = 0; outOp.TL = 0; outOp.RS = 0; outOp.AR = 0;
        outOp.AM = 0; outOp.DR = 0; outOp.SR = 0; outOp.SL = 0; outOp.RR = 0; outOp.SSG = 0;
        
        // Split by commas
        auto assignments = juce::StringArray::fromTokens(content, ",", "");
        
        for (auto& assignment : assignments)
        {
            auto parts = juce::StringArray::fromTokens(assignment, "=", "");
            if (parts.size() != 2)
                continue;
                
            auto prop = parts[0].trim();
            int value = parts[1].trim().getIntValue();
            
            if (prop == ".DT")       outOp.DT = value;
            else if (prop == ".MUL") outOp.MUL = value;
            else if (prop == ".TL")  outOp.TL = value;
            else if (prop == ".RS")  outOp.RS = value;
            else if (prop == ".AR")  outOp.AR = value;
            else if (prop == ".AM")  outOp.AM = value;
            else if (prop == ".DR")  outOp.DR = value;
            else if (prop == ".SR")  outOp.SR = value;
            else if (prop == ".SL")  outOp.SL = value;
            else if (prop == ".RR")  outOp.RR = value;
            else if (prop == ".SSG") outOp.SSG = value;
            else
            {
                error = "Unknown operator property: " + prop;
                errorCol = line.indexOf(prop);
                return false;
            }
        }
        
        return true;
    }
};
