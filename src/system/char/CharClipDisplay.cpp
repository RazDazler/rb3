#include "char/CharClipDisplay.h"
#include "rndobj/Rnd.h"
#include "obj/Msg.h"
#include "decomp.h"
#include "utl/MakeString.h"
extern "C" double floor(double);

float CharClipDisplay::sZoom = 1.0f;
float CharClipDisplay::sEm;
ObjectDir *CharClipDisplay::sDir;

void CharClipDisplay::Init(ObjectDir *dir) {
    sDir = dir;
    sEm = TheRnd->DrawString("", Vector2(0, 0), Hmx::Color(1.0f, 0.0f, 0.0f), false).y;
}

#if defined(VERSION_SZBE69_B8)
// Preserve original literal ordering while incomplete methods remain.
DECOMP_FORCEACTIVE(
    LiteralPoolCharClipDisplay,
    "",
    "left.ikfoot",
    "right.ikfoot",
    "CharClipDisplay.cpp",
    "!rightIk || !leftIk || (rightIk->GetData() == leftIk->GetData())",
    "%.1f",
    "%d",
    "%.1f (%.2f)"
)
#endif

MsgSource *CharClipDisplay::FindSource(Hmx::Object *o) {
    for (ObjDirItr<MsgSource> it(ObjectDir::Main(), false); it != nullptr; ++it) {
        for (std::list<MsgSource::Sink>::iterator lit = it->mSinks.begin();
             lit != it->mSinks.end();
             ++lit) {
            if ((*lit).obj == o)
                return it;
        }
    }
    return 0;
}

void CharClipDisplay::SetClip(CharClip *clip, bool b) {
    unk0 = clip;
    SetText(clip->Name());
    SetStartEnd(clip->StartBeat(), clip->EndBeat(), b);
}

void CharClipDisplay::SetText(const char *text) {
    strcpy(unk24, text);
    float width =
        TheRnd->DrawString(text, Vector2(0, 0), Hmx::Color(1.0f, 0.0f, 0.0f), false).x;
    unk14 = width + sEm;
}

void CharClipDisplay::SetStartEnd(float start, float end, bool center) {
    unk4 = start;
    unk8 = end;
    unkc = start;
    unk10 = end;
    float span = 16.0f / sZoom;
    if (center) {
        float margin = 3.0f * sEm;
        float halfWidth = 0.5f * TheRnd->Width();
        float width = unk64 + unk14;
        float left = width + margin;
        float before = span * (halfWidth - left) / TheRnd->Width();
        float after = span * (TheRnd->Width() - margin - left);
        unkc = unk1c - before;
        unk10 = unkc + after / TheRnd->Width();
        GetX(unk1c);
    } else if (end - start > span) {
        float half = span * 0.5f;
        if (unk1c < start + half)
            unk10 = start + span;
        else if (unk1c > end - half)
            unkc = end - span;
        else {
            unkc = unk1c - half;
            unk10 = unk1c + half;
        }
    } else if (end == start) {
        float half = span * 0.5f;
        unkc = start - half;
        unk10 = end + half;
    }
}

void CharClipDisplay::DrawBlend(float start, float duration) {
    float y = unk18;
    y += 1.0f;
    Hmx::Rect rect(0.0f, y, 0.0f, 2.0f);
    rect.x = GetX(start);
    rect.w = GetX(start + duration) - rect.x;
    TheRnd->DrawRect(rect, Hmx::Color(0.0f, 0.0f, 1.0f, 0.4f), 0, 0, 0);
    rect.y = unk18 - 1.0f;
    rect.h = 4.0f;
    rect.w = 3.0f;
    rect.x = GetX(start + 0.5f * duration) - 1.0f;
    TheRnd->DrawRect(rect, Hmx::Color(0.0f, 0.0f, 1.0f, 1.0f), 0, 0, 0);
}

void CharClipDisplay::DrawBeatString(float beat, const Hmx::Color &color) {
    const char *text;
    if (beat == (float)floor(beat))
        text = MakeString("%d", (int)beat);
    else
        text = MakeString("%.1f", beat);
    DrawBeatString(text, beat, color);
}

void CharClipDisplay::DrawBeatString(
    const char *text, float beat, const Hmx::Color &color
) {
    Vector2 pos;
    GetXY(pos, beat);
    float x = pos.x - 4.0f;
    float y = pos.y - 18.0f;
    TheRnd->DrawString(text, Vector2(x, y), color, true);
}

void CharClipDisplay::DrawCursor() {
    Hmx::Color color(1.0f, 1.0f, 0.0f);
    Vector2 pos;
    GetXY(pos, unk1c);
    float y = pos.y - 3.0f;
    float x = pos.x;
    TheRnd->DrawRect(Hmx::Rect(x, y, 1.0f, 9.0f), color, 0, 0, 0);
    const char *text;
    if (unk20 < 1.0f)
        text = MakeString("%.1f (%.2f)", unk1c, unk20);
    else
        text = MakeString("%.1f", unk1c);
    DrawBeatString(text, unk1c, color);
}

float CharClipDisplay::GetX(float beat) const {
    float start = unkc;
    float end = unk10;
    float span = end > start ? end - start : 1.0f;
    float margin = 3.0f * sEm;
    float width = unk64 + unk14;
    float left = width + margin;
    return left + (beat - start) * (TheRnd->Width() - margin - left) / span;
}

void CharClipDisplay::GetXY(Vector2 &pos, float beat) const {
    float y = unk18;
    pos.x = GetX(beat);
    pos.y = y;
}

float CharClipDisplay::LineSpacing() {
    float em = sEm;
    em *= 2.0f;
    return em;
}