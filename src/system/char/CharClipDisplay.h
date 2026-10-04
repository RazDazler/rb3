#pragma once
#include "obj/Object.h"
#include "char/CharClip.h"

class MsgSource;

class CharClipDisplay { // size 0x68
public:
    CharClipDisplay()
        : unk0(0), unk4(0), unk8(0), unkc(0), unk10(0), unk14(0), unk18(0), unk1c(0),
          unk20(0), unk64(0) {}

    MsgSource *FindSource(Hmx::Object *);
    void SetClip(CharClip *, bool);
    void SetText(const char *);
    void SetStartEnd(float, float, bool);

    void DrawBlend(float, float);
    void DrawCursor();
    float GetX(float) const;
    void GetXY(Vector2 &, float) const;
    void DrawBeatString(float, const Hmx::Color &);
    void DrawBeatString(const char *, float, const Hmx::Color &);

    static void Init(ObjectDir *);
    static float LineSpacing();

    static float sZoom;
    static float sEm;
    static ObjectDir *sDir;

    CharClip *unk0;
    float unk4;
    float unk8;
    float unkc;
    float unk10;
    float unk14;
    float unk18;
    float unk1c;
    float unk20;
    char unk24[64]; // inline text buffer, 0x24..0x63
    float unk64;
};
