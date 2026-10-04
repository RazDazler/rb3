#include "meta_band/UIStats.h"
#include "decomp.h"

#if defined(VERSION_SZBE69_B8)
// Preserve original literal ordering while incomplete methods remain.
DECOMP_FORCEACTIVE(
    LiteralPoolUIStats,
    "UIStats.cpp",
    "from",
    "",
    "00",
    "stats/screen_exit",
    "stats/pad_user",
    "name",
    "mode",
    "%x:",
    "padlog",
    "pad_%d",
    "local_user_%d",
    "null",
    "remoteCount < DIM(mLastRemoteID)",
    "remote_user_%d",
    "%s:%s",
    "exit_stats",
    "(rslt->Size() % 2) == 0",
    "dropped_screens",
    "but < 32",
    "pad < 8",
    "state < 2",
    "msg.GetUser()",
    "%s(%d): %s unhandled msg: %s"
)
#endif
#include "obj/Data.h"
#include "obj/ObjMacros.h"
#include "os/Debug.h"
#include "os/JoypadMsgs.h"
#include "os/System.h"
#include "utl/MemMgr.h"

UIStats gUIStats;
UIStats *TheUIStats = &gUIStats;

UIStats::UIStats() {}

void UIStats::Init() {
    void *mem = _MemAlloc(0x10000, 0);
    unkb0 = mem;
    unkb4 = mem;
    unkb8 = 0;
    unka8 = 0;
    unk1c = false;
    unkac = SystemMs();
}

void UIStats::Terminate() {
    if (unkb0) {
        _MemFree(unkb0);
        unkb0 = 0;
    }
    unkb4 = 0;
}

void UIStats::Poll() {}

void UIStats::DropScreen(UIScreen *screen) {
    unkb4 = unkb0;
    unkb8 = 0;
    unka8++;
}

void UIStats::MaybePublish(UIScreen *from) {}

void UIStats::EventLog(unsigned int pad, unsigned int but, unsigned int state) {
    MILO_ASSERT(but < 32, 0x139);
    MILO_ASSERT(pad < 8, 0x13B);
    MILO_ASSERT(state < 2, 0x13D);
}

DataNode UIStats::OnMsg(const ButtonDownMsg &msg) {
    EventLog(msg.GetPadNum(), msg.GetButton(), 0);
    return DataNode(kDataUnhandled, 0);
}

DataNode UIStats::OnMsg(const ButtonUpMsg &msg) {
    EventLog(msg.GetPadNum(), msg.GetButton(), 1);
    return DataNode(kDataUnhandled, 0);
}

DataNode UIStats::OnMsg(const JoypadConnectionMsg &msg) {
    MILO_ASSERT(msg.GetUser(), 0x166);
    EventLog(msg.GetUser()->GetPadNum(), 0x18, msg->Int(3) != 0);
    return DataNode(kDataUnhandled, 0);
}

DataNode UIStats::OnMsg(const UIComponentFocusChangeMsg &) {
    return DataNode(kDataUnhandled, 0);
}

DataNode UIStats::OnMsg(const UIScreenChangeMsg &msg) {
    MaybePublish(msg.GetOldScreen());
    return DataNode(kDataUnhandled, 0);
}

BEGIN_HANDLERS(UIStats)
    HANDLE_MESSAGE(ButtonDownMsg)
    HANDLE_MESSAGE(ButtonUpMsg)
    HANDLE_MESSAGE(JoypadConnectionMsg)
    HANDLE_MESSAGE(UIComponentFocusChangeMsg)
    HANDLE_MESSAGE(UIScreenChangeMsg)
    HANDLE_SUPERCLASS(Hmx::Object)
    HANDLE_CHECK(0x183)
END_HANDLERS