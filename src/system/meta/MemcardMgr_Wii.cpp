#include "meta/MemcardMgr_Wii.h"

MemcardMgr TheMemcardMgr;

MemcardMgr::MemcardMgr()
    : unka4(0), unka8(0), unkac(0), mIsWriteMode(0), unkbc(-1), unkc0(0), unkcc(0),
      unkd0(0), unkd4(-1), unkd8(-1), unkdc(0) {}

MemcardMgr::~MemcardMgr() {}
void MemcardMgr::DisableWriting(bool disabled) {
    if (disabled)
        mIsWriteMode |= 1;
    else
        mIsWriteMode &= ~1;
}
bool MemcardMgr::IsDisableWriting() const { return mIsWriteMode & 1; }
DataNode MemcardMgr::OnMsg(const UIChangedMsg &) { return 0; }
DataNode MemcardMgr::OnMsg(const StorageChangedMsg &) { return 0; }

bool MemcardMgr::IsWriteMode() const { return (mIsWriteMode >> 1) & 1; }