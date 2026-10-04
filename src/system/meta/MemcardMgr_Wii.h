#pragma once
#include "meta/Profile.h"
#include "obj/Msg.h"
#include "os/ThreadCall.h"

class UIChangedMsg;
class StorageChangedMsg;
class MemcardMgr : public MsgSource, public ThreadCallback {
public:
    MemcardMgr();
    virtual DataNode Handle(DataArray *, bool);
    virtual ~MemcardMgr();
    virtual int ThreadStart();
    virtual void ThreadDone(int);
    bool IsWriteMode() const;
    bool IsDisableWriting() const;
    void DisableWriting(bool);
    DataNode OnMsg(const UIChangedMsg &);
    DataNode OnMsg(const StorageChangedMsg &);

    bool unk20;
    char unk21[64];
    char unk61[64];
    int unka4;
    int unka8; // bufstreamnand
    int unkac;
    int unkb0;
    int unkb4;
    bool unkb8;
    unsigned char mIsWriteMode; // 0xB9: disable bit0, write mode bit1
    int unkbc;
    int unkc0; // mState
    int unkc4;
    int unkc8;
    int unkcc; // memcardaction*
    int unkd0;
    int unkd4;
    int unkd8;
    Profile *unkdc;
};

extern MemcardMgr TheMemcardMgr;