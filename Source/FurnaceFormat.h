#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// FurnaceFormat.h  –  Furnace .fui read/write for OPN2/YM2612
// Based on: Furnace src/engine/instrument.cpp  putInsData2/readInsData2
//
// Wire format (all little-endian):
//   [4]  "FINS"
//   [2]  version uint16
//   [1]  type byte  (1 = DIV_INS_FM)
//   [1]  reserved 0x00
//   then feature blocks:
//     [2]  feature ID  e.g. "NA", "FM"
//     [2]  featLen (bytes following)
//     [featLen] data
//   terminated by feature ID "EN" (no length/data)
//
// "FM" block: 8 header bytes + ops*21 bytes
//   header: alg fb fms ams fms2 ams2 ops opllPreset
//   per op: am ar dr mult rr sl tl dt2 rs dt d2r ssgEnv dam dvb egt ksl sus vib ws ksr kvs
// ─────────────────────────────────────────────────────────────────────────────

#include <juce_core/juce_core.h>
#include <cstring>

namespace FurnaceFormat {

static constexpr uint8_t  INS_FM    = 1;
static constexpr uint16_t ENG_VER   = 220;

struct Op {
    uint8_t am=0,ar=0,dr=0,mult=1,rr=0,sl=0,tl=0,dt2=0,
            rs=0,dt=0,d2r=0,ssgEnv=0,dam=0,dvb=0,egt=0,
            ksl=0,sus=0,vib=0,ws=0,ksr=0,kvs=2;
};
struct Instrument {
    juce::String name;
    uint8_t alg=0,fb=0,fms=0,ams=0,fms2=0,ams2=0,ops=4,opllPreset=0;
    Op op[4];
};

// Possible parse failure reasons (for debugging)
enum class ParseResult { OK, FILE_NOT_FOUND, FILE_EMPTY, BAD_MAGIC, WRONG_TYPE, NO_FM_BLOCK };

// ─────────────────────────────────────────────────────────────────────────────
// Inner parser — operates on raw bytes, no JUCE stream API involved at all
// ─────────────────────────────────────────────────────────────────────────────
inline ParseResult parseBytes(const uint8_t* data, size_t size, Instrument& ins)
{
    if (size < 8) return ParseResult::BAD_MAGIC;

    // Check magic
    if (data[0]!='F'||data[1]!='I'||data[2]!='N'||data[3]!='S')
        return ParseResult::BAD_MAGIC;

    size_t pos = 4;
    auto r8  = [&]() -> uint8_t  { return pos < size ? data[pos++] : 0; };
    auto r16 = [&]() -> uint16_t { uint8_t a=r8(), b=r8(); return (uint16_t)(a|(b<<8)); };

    /* version */ r16();
    uint8_t type = r8();
    /* reserved */ r8();

    if (type != INS_FM) return ParseResult::WRONG_TYPE;

    bool gotFM = false;

    while (pos + 2 <= size) {
        char id0 = (char)r8(), id1 = (char)r8();

        if (id0=='E' && id1=='N') break;   // end marker

        if (pos + 2 > size) break;
        uint16_t featLen = r16();
        size_t   featEnd = pos + featLen;
        if (featEnd > size) featEnd = size;

        if (id0=='N' && id1=='A') {
            size_t avail = featEnd - pos;
            if (avail > 0)
                ins.name = juce::String::fromUTF8(
                    reinterpret_cast<const char*>(data + pos), (int)avail);
        }
        else if (id0=='F' && id1=='M') {
            ins.alg        = r8(); ins.fb         = r8();
            ins.fms        = r8(); ins.ams        = r8();
            ins.fms2       = r8(); ins.ams2       = r8();
            ins.ops        = r8(); ins.opllPreset = r8();

            int n = (int)ins.ops;
            if (n < 0) n = 0;
            if (n > 4) n = 4;

            for (int i = 0; i < n; i++) {
                if (featEnd - pos < 20) break;
                Op& op   = ins.op[i];
                op.am    = r8(); op.ar    = r8(); op.dr    = r8(); op.mult  = r8();
                op.rr    = r8(); op.sl    = r8(); op.tl    = r8(); op.dt2   = r8();
                op.rs    = r8(); op.dt    = r8(); op.d2r   = r8(); op.ssgEnv= r8();
                op.dam   = r8(); op.dvb   = r8(); op.egt   = r8(); op.ksl   = r8();
                op.sus   = r8(); op.vib   = r8(); op.ws    = r8(); op.ksr   = r8();
                op.kvs   = (featEnd > pos) ? r8() : 2;
            }
            gotFM = true;
        }

        pos = featEnd;   // skip to next feature regardless of consumption
    }

    return gotFM ? ParseResult::OK : ParseResult::NO_FM_BLOCK;
}

// ─────────────────────────────────────────────────────────────────────────────
// Public read: load file → parse bytes
// ─────────────────────────────────────────────────────────────────────────────
inline bool readFui(const juce::File& file, Instrument& ins)
{
    if (!file.existsAsFile()) {
        DBG("FurnaceFormat::readFui - file not found: " + file.getFullPathName());
        return false;
    }

    // Read via FileInputStream byte-by-byte into a MemoryBlock
    // Use createInputStream so we don't rely on loadFileAsData existing
    std::unique_ptr<juce::FileInputStream> stream (new juce::FileInputStream(file));
    if (!stream->openedOk()) {
        DBG("FurnaceFormat::readFui - failed to open stream");
        return false;
    }

    juce::MemoryOutputStream mb;
    mb.writeFromInputStream(*stream, (int64_t)file.getSize() + 1024);

    const uint8_t* data = static_cast<const uint8_t*>(mb.getData());
    size_t size = mb.getDataSize();

    DBG("FurnaceFormat::readFui - read " + juce::String((int)size) + " bytes");
    if (size >= 4) {
        DBG("FurnaceFormat::readFui - magic bytes: "
            + juce::String::toHexString(data[0]) + " "
            + juce::String::toHexString(data[1]) + " "
            + juce::String::toHexString(data[2]) + " "
            + juce::String::toHexString(data[3]));
    }

    auto result = parseBytes(data, size, ins);

    if (result != ParseResult::OK) {
        const char* reason = "unknown";
        switch (result) {
            case ParseResult::BAD_MAGIC:    reason = "bad magic (not FINS)"; break;
            case ParseResult::WRONG_TYPE:   reason = "wrong type (not FM/OPN2)"; break;
            case ParseResult::NO_FM_BLOCK:  reason = "no FM feature block found"; break;
            default: break;
        }
        DBG("FurnaceFormat::readFui - parse failed: " + juce::String(reason));
        return false;
    }

    DBG("FurnaceFormat::readFui - OK: name='" + ins.name
        + "' alg=" + juce::String(ins.alg)
        + " fb="   + juce::String(ins.fb)
        + " ops="  + juce::String(ins.ops));
    for (int i = 0; i < 4; i++)
        DBG("  OP" + juce::String(i+1)
            + " ar=" + juce::String(ins.op[i].ar)
            + " tl=" + juce::String(ins.op[i].tl)
            + " mult=" + juce::String(ins.op[i].mult)
            + " rr=" + juce::String(ins.op[i].rr));
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Public write
// ─────────────────────────────────────────────────────────────────────────────
inline bool writeFui(const juce::File& file, const Instrument& ins)
{
    juce::MemoryOutputStream out;

    auto w8  = [&](uint8_t v)  { out.writeByte((char)v); };
    auto w16 = [&](uint16_t v) { w8(v&0xFF); w8((v>>8)&0xFF); };

    out.write("FINS", 4);
    w16(ENG_VER);
    w8(INS_FM);
    w8(0);

    // NA block
    if (ins.name.isNotEmpty()) {
        auto utf8 = ins.name.toUTF8();
        uint16_t nlen = (uint16_t)(std::strlen(utf8) + 1);
        out.write("NA", 2); w16(nlen);
        out.write(utf8, nlen);
    }

    // FM block
    uint16_t fmLen = 8 + (uint16_t)ins.ops * 21;
    out.write("FM", 2); w16(fmLen);
    w8(ins.alg);  w8(ins.fb);   w8(ins.fms);  w8(ins.ams);
    w8(ins.fms2); w8(ins.ams2); w8(ins.ops);  w8(ins.opllPreset);
    for (int i = 0; i < (int)ins.ops; i++) {
        const Op& op = ins.op[i];
        w8(op.am);  w8(op.ar);  w8(op.dr);  w8(op.mult);
        w8(op.rr);  w8(op.sl);  w8(op.tl);  w8(op.dt2);
        w8(op.rs);  w8(op.dt);  w8(op.d2r); w8(op.ssgEnv);
        w8(op.dam); w8(op.dvb); w8(op.egt); w8(op.ksl);
        w8(op.sus); w8(op.vib); w8(op.ws);  w8(op.ksr);
        w8(op.kvs);
    }

    out.write("EN", 2);

    return file.replaceWithData(out.getData(), out.getDataSize());
}

} // namespace FurnaceFormat
