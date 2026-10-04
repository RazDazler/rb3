#include "bandobj/CharKeyHandMidi.h"
#include "decomp.h"

#if defined(VERSION_SZBE69_B8)
// Preserve original literal ordering while incomplete methods remain.
DECOMP_FORCEACTIVE(
    LiteralPoolCharKeyHandMidi,
    "CharKeyHandMidi.cpp",
    "key > kNoKey && key <= kKeyC4",
    "CharKeyHandMidi: Trying to key non-existent finger",
    "CharKeyHandMidi: Trying to put finger on non-existent key",
    "finger >= 0 && finger < CharIKFingers::kFingerNone",
    "Too many keyboard keys down in one poll: %d\n",
    "Keyboard fingers: not enough free fingers to play a note, please check the authoring!",
    "0",
    "%s can't load new %s version %d > %d",
    "%s can't load new %s alt version %d > %d",
    "%s(%d): %s unhandled msg: %s",
    "PropSync_p.h",
    "i == prop->Size() && op <= kPropInsert"
)
#endif
#include "utl/Symbols.h"

CharKeyHandMidi::CharKeyHandMidi()
    : mIKObject(this, 0), mFirstSpot(this, 0), mSecondSpot(this, 0), unk7c(this, 0) {}

CharKeyHandMidi::~CharKeyHandMidi() {}

BEGIN_HANDLERS(CharKeyHandMidi)
    HANDLE(fingers_up, OnFingersUp)
    HANDLE(fingers_down, OnFingersDown)
    HANDLE_ACTION(run_test, RunTest())
    HANDLE_ACTION(end_test, EndTest())
    HANDLE_SUPERCLASS(CharWeightable)
    HANDLE_SUPERCLASS(Hmx::Object)
    HANDLE_CHECK(0x319)
END_HANDLERS

BEGIN_PROPSYNCS(CharKeyHandMidi)
    SYNC_PROP(ik_object, mIKObject)
    SYNC_PROP(first_spot, mFirstSpot)
    SYNC_PROP(second_spot, mSecondSpot)
    SYNC_PROP(is_right_hand, mIsRightHand)
    SYNC_SUPERCLASS(CharWeightable)
END_PROPSYNCS