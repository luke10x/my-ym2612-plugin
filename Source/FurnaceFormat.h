#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// FurnaceFormat.h  –  Read/write Furnace .fui for OPN2/YM2612
//
// Based EXACTLY on Furnace source:
//   writeFeatureFM / readFeatureFM / writeFeatureNA / readFeatureNA
//
// .fui layout:
//   [0]  4B  "FINS"
//   [4]  2B  uint16 version LE
//   [6]  1B  uint8  type (1 = DIV_INS_FM)
//   [7]  1B  uint8  reserved = 0
//   feature blocks until "EN":
//     [+0] 2B  feature ID
//     [+2] 2B  uint16 featLen
//     [+4] N   data
//
// Feature "NA":
//   writeString(name, false)  →  uint16 length LE, then UTF-8 bytes (no null)
//   (SafeWriter::writeString writes the length then the chars)
//
// Feature "FM" – all bit-packed:
//   Byte 0:  opEnable[3..0] in bits 7..4, opCount in bits 3..0
//   Byte 1:  (alg&7)<<4 | (fb&7)
//   Byte 2:  (fms2&7)<<5 | (ams&3)<<3 | (fms&7)
//   Byte 3:  (ams2&3)<<6 | (ops==4?32:0) | (opllPreset&31)
//   Byte 4:  block&15   (always written, even for OPN2 where block=0)
//   then for each operator (8 bytes each):
//     Byte+0: (ksr?128:0) | (dt&7)<<4 | (mult&15)
//     Byte+1: (sus?128:0) | (tl&127)
//     Byte+2: (rs&3)<<6  | (vib?32:0) | (ar&31)
//     Byte+3: (am?128:0) | (ksl&3)<<5 | (dr&31)
//     Byte+4: (egt?128:0)| (kvs&3)<<5 | (d2r&31)
//     Byte+5: (sl&15)<<4 | (rr&15)
//     Byte+6: (dvb&15)<<4| (ssgEnv&15)
//     Byte+7: (dam&7)<<5 | (dt2&3)<<3 | (ws&7)
//
// ssgEnv bits 3..0: bit3=enable, bits2..0=mode
// ─────────────────────────────────────────────────────────────────────────────

#include <juce_core/juce_core.h>
#include <cstring>

namespace FurnaceFormat {

static constexpr uint8_t  INS_FM  = 1;
static constexpr uint16_t ENG_VER = 224;  // must be >=224 so Furnace reads the block byte

struct Op {
    uint8_t  am=0, ar=0, dr=0, mult=1, rr=0, sl=0, tl=0, dt2=0;
    uint8_t  rs=0, dt=0, d2r=0, ssgEnv=0;
    uint8_t  dam=0, dvb=0, egt=0, ksl=0, sus=0, vib=0, ws=0, ksr=0, kvs=2;
    bool     enable=true;
};

struct Instrument {
    juce::String name;
    uint8_t alg=0, fb=0, fms=0, ams=0, fms2=0, ams2=0;
    uint8_t ops=4, opllPreset=0, block=0;
    Op op[4];
};

// ─── tiny cursor ─────────────────────────────────────────────────────────────
struct Cur {
    const uint8_t* p;
    const uint8_t* end;
    bool    ok(size_t n=1) const { return p+n<=end; }
    uint8_t  u8()  { return ok()?*p++:0; }
    uint16_t u16() { uint8_t a=u8(),b=u8(); return uint16_t(a|(b<<8)); }
    void     seek(const uint8_t* to){ p=(to<=end)?to:end; }
};

// ─────────────────────────────────────────────────────────────────────────────
inline bool readFui(const juce::File& file, Instrument& ins)
{
    if (!file.existsAsFile()) return false;
    juce::FileInputStream fs(file);
    if (!fs.openedOk()) return false;
    juce::MemoryOutputStream mb;
    mb.writeFromInputStream(fs, file.getSize()+16);

    const uint8_t* data = static_cast<const uint8_t*>(mb.getData());
    size_t         size = mb.getDataSize();
    if (size < 8 || memcmp(data,"FINS",4)!=0) return false;

    Cur c { data+4, data+size };
    uint16_t version = c.u16();
    uint8_t  type    = c.u8();
    /* reserved */     c.u8();
    if (type != INS_FM) return false;

    bool gotFM = false;

    while (c.ok(2)) {
        char id0=char(c.u8()), id1=char(c.u8());
        if (id0=='E'&&id1=='N') break;
        if (!c.ok(2)) break;
        uint16_t       flen   = c.u16();
        const uint8_t* fstart = c.p;
        const uint8_t* fend   = fstart+flen;
        if (fend > c.end) fend = c.end;

        // ── NA ──────────────────────────────────────────────────────────────
        if (id0=='N'&&id1=='A') {
            // SafeWriter::writeString(str, false) writes:
            //   uint16 LE length, then UTF-8 bytes (no null terminator)
            uint16_t slen = c.u16();
            if (slen > 0 && c.ok(slen))
                ins.name = juce::String::fromUTF8(
                    reinterpret_cast<const char*>(c.p), slen);
        }

        // ── FM ──────────────────────────────────────────────────────────────
        else if (id0=='F'&&id1=='M') {
            // Byte 0: op enable flags + opCount
            uint8_t b0 = c.u8();
            ins.op[0].enable = (b0&16)!=0;
            ins.op[1].enable = (b0&32)!=0;
            ins.op[2].enable = (b0&64)!=0;
            ins.op[3].enable = (b0&128)!=0;
            int opCount = b0 & 15;

            // Byte 1: alg + fb
            uint8_t b1 = c.u8();
            ins.alg = (b1>>4)&7;
            ins.fb  = b1&7;

            // Byte 2: fms2 + ams + fms
            uint8_t b2 = c.u8();
            ins.fms2 = (b2>>5)&7;
            ins.ams  = (b2>>3)&3;
            ins.fms  = b2&7;

            // Byte 3: ams2 + ops flag + opllPreset
            uint8_t b3 = c.u8();
            ins.ams2       = (b3>>6)&3;
            ins.ops        = (b3&32)?4:2;
            ins.opllPreset = b3&31;

            // Byte 4: block – only present if version >= 224
            if (version >= 224) ins.block = c.u8()&15;

            // Operators – 8 bytes each
            int n = juce::jlimit(0,4,opCount);
            for (int i=0; i<n; i++) {
                if (fend-c.p < 8) break;
                Op& op = ins.op[i];

                uint8_t o0 = c.u8();  // ksr | dt | mult
                op.ksr  = (o0&128)?1:0;
                op.dt   = (o0>>4)&7;
                op.mult = o0&15;

                uint8_t o1 = c.u8();  // sus | tl
                op.sus = (o1&128)?1:0;
                op.tl  = o1&127;

                uint8_t o2 = c.u8();  // rs | vib | ar
                op.rs  = (o2>>6)&3;
                op.vib = (o2&32)?1:0;
                op.ar  = o2&31;

                uint8_t o3 = c.u8();  // am | ksl | dr
                op.am  = (o3&128)?1:0;
                op.ksl = (o3>>5)&3;
                op.dr  = o3&31;

                uint8_t o4 = c.u8();  // egt | kvs | d2r
                op.egt = (o4&128)?1:0;
                op.kvs = (o4>>5)&3;
                op.d2r = o4&31;

                uint8_t o5 = c.u8();  // sl | rr
                op.sl = (o5>>4)&15;
                op.rr = o5&15;

                uint8_t o6 = c.u8();  // dvb | ssgEnv
                op.dvb    = (o6>>4)&15;
                op.ssgEnv = o6&15;    // bit3=enable, bits2:0=mode

                uint8_t o7 = c.u8();  // dam | dt2 | ws
                op.dam = (o7>>5)&7;
                op.dt2 = (o7>>3)&3;
                op.ws  = o7&7;
            }
            gotFM = true;
        }

        c.seek(fend);
    }
    return gotFM;
}

// ─────────────────────────────────────────────────────────────────────────────
inline bool writeFui(const juce::File& file, const Instrument& ins)
{
    juce::MemoryOutputStream out;
    auto w8  = [&](uint8_t  v){ out.writeByte(char(v)); };
    auto w16 = [&](uint16_t v){ w8(v&0xFF); w8(uint8_t(v>>8)); };

    // Header
    out.write("FINS",4);
    w16(ENG_VER);
    w8(INS_FM);
    w8(0);

    // Feature NA – SafeWriter::writeString(name,false) = uint16 len + bytes
    {
        auto utf8 = ins.name.toUTF8();
        uint16_t slen = uint16_t(strlen(utf8));
        out.write("NA",2); w16(uint16_t(slen+2));  // featLen includes the 2-byte length field
        w16(slen);
        out.write(utf8, slen);
    }

    // Feature FM
    int opCount = juce::jlimit(0,4,int(ins.ops));
    // featLen = 5 header bytes + opCount*8 op bytes
    uint16_t fmLen = uint16_t(5 + opCount*8);
    out.write("FM",2); w16(fmLen);

    // Byte 0: op enable + opCount
    uint8_t b0 = uint8_t(opCount & 15);
    if (ins.op[0].enable) b0 |= 16;
    if (ins.op[1].enable) b0 |= 32;
    if (ins.op[2].enable) b0 |= 64;
    if (ins.op[3].enable) b0 |= 128;
    w8(b0);

    // Byte 1: alg + fb
    w8(uint8_t(((ins.alg&7)<<4)|(ins.fb&7)));

    // Byte 2: fms2 + ams + fms
    w8(uint8_t(((ins.fms2&7)<<5)|((ins.ams&3)<<3)|(ins.fms&7)));

    // Byte 3: ams2 + ops flag + opllPreset
    w8(uint8_t(((ins.ams2&3)<<6)|((ins.ops==4)?32:0)|(ins.opllPreset&31)));

    // Byte 4: block
    w8(ins.block&15);

    // Operator bytes (8 each)
    for (int i=0; i<opCount; i++) {
        const Op& op = ins.op[i];
        w8(uint8_t((op.ksr?128:0)|((op.dt&7)<<4)|(op.mult&15)));
        w8(uint8_t((op.sus?128:0)|(op.tl&127)));
        w8(uint8_t(((op.rs&3)<<6)|(op.vib?32:0)|(op.ar&31)));
        w8(uint8_t((op.am?128:0)|((op.ksl&3)<<5)|(op.dr&31)));
        w8(uint8_t((op.egt?128:0)|((op.kvs&3)<<5)|(op.d2r&31)));
        w8(uint8_t(((op.sl&15)<<4)|(op.rr&15)));
        w8(uint8_t(((op.dvb&15)<<4)|(op.ssgEnv&15)));
        w8(uint8_t(((op.dam&7)<<5)|((op.dt2&3)<<3)|(op.ws&7)));
    }

    // End marker
    out.write("EN",2);

    return file.replaceWithData(out.getData(), out.getDataSize());
}

} // namespace FurnaceFormat
