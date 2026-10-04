#include "BlockMgr.h"
#include "obj/DataFunc.h"
#include "os/AsyncTask.h"
#include "os/CDReader.h"
#include "os/Debug.h"
#include "os/HDCache.h"
#include "os/Archive.h"
#include <string.h>
#include "utl/MemMgr.h"
#include "decomp.h"

#define kNumBlockBuffers 4

BlockMgr TheBlockMgr;
int gLastBlockNum = -1;
int gLastArkNum = -1;
const int kArkBlockSize = 0x10000;
static char *gBuffers;
int gCurrBuffNum;
int Block::sCurrTimestamp = 0;
int gNumPolls;
Timer gReadTime;
int gSeekCount;
float gSeekTimeMs;

namespace {
    bool gReadHD = false;
    static DataNode OnSpinUp(DataArray *) { return TheBlockMgr.SpinUp(); }
}

int GetFreeBuffer() {
    MILO_ASSERT(gCurrBuffNum < kNumBlockBuffers, 0x44);
    return gCurrBuffNum++;
}

#if defined(VERSION_SZBE69_B8)
// Preserve the full original B8 literal pool.
DECOMP_FORCEACTIVE(
    LiteralPoolRecoveredBlockMgr,
    "BlockMgr.cpp",
    "gCurrBuffNum < kNumBlockBuffers",
    "it->Exceeds(ark, block)",
    "",
    "disc_spin_up",
    "!mWritingBlock",
    "mReadingBlock",
    "CD READING ERROR: %x\n",
    "MainThread()",
    " CD READING ERROR!!!  %x\n",
    "BlockMgr Seek: Ark: %2d  Dist: %5d  Seek Time: %3.0f ms  Suspect: %s\n",
    "request != mRequests.end()",
    "blocknum != -1",
    "BlockMgr spinning up...\n"
)
#else
DECOMP_FORCEACTIVE(BlockMgr, "it->Exceeds(ark, block)")
#endif

Block::Block() : mArkfileNum(-1), mBlockNum(-1), mWritten(true), mDebugName("") {
    mBuffer = &gBuffers[GetFreeBuffer() * 0x10000];
    UpdateTimestamp();
}

void Block::UpdateTimestamp() { mTimestamp = ++sCurrTimestamp; }

BlockRequest::BlockRequest(const AsyncTask &task)
    : mArkfileNum(task.mArkfileNum), mBlockNum(task.GetBlockNum()), mStr(task.GetStr()) {
    mTasks.push_back(task);
}

void BlockMgr::Init() {
    gBuffers = (char *)_MemAlloc(0x40000, 0x40);
    gCurrBuffNum = 0;
    mBlockCache.resize(4);
    mReadingBlock = nullptr;
    for (int i = 0; i < mBlockCache.size(); i++) {
        mBlockCache[i] = new Block();
    }
    TheHDCache.Init();
    DataRegisterFunc("disc_spin_up", OnSpinUp);
}

void BlockMgr::GetAssociatedBlocks(
    unsigned long long pos, int bytes, int &firstBlock, int &numBlocks, int &blockSize
) {
    blockSize = kArkBlockSize;
    firstBlock = pos >> 16;
    int remaining = bytes + (int)(pos & 0xffff) - kArkBlockSize;
    if (remaining > 0) {
        numBlocks = remaining / kArkBlockSize + 1;
        if (remaining % kArkBlockSize != 0)
            ++numBlocks;
    } else
        numBlocks = 1;
}

void BlockMgr::KillBlockRequests(ArkFile *owner) {
    std::list<BlockRequest>::iterator request = mRequests.begin();
    while (request != mRequests.end()) {
        std::list<AsyncTask> &tasks = request->mTasks;
        std::list<AsyncTask>::iterator task = tasks.begin();
        while (task != tasks.end()) {
            if (task->mOwner == owner)
                task = tasks.erase(task);
            else
                ++task;
        }
        if (tasks.size() == 0
            && (!mReadingBlock
                || !mReadingBlock->CheckMetadata(request->mArkfileNum, request->mBlockNum)
            )) {
            request = mRequests.erase(request);
        } else
            ++request;
    }
}

const char *BlockMgr::GetBlockData(int ark, int blk) {
    Block *blokc = FindBlock(ark, blk);
    if (blokc != nullptr && blokc != mReadingBlock) {
        blokc->UpdateTimestamp();
        return blokc->mBuffer;
    }
    return nullptr;
}

void BlockMgr::AddTask(const AsyncTask &task) {
    int block = task.GetBlockNum();
    int ark = task.mArkfileNum;
    std::list<BlockRequest>::iterator it = mRequests.begin();
    for (; it != mRequests.end(); ++it) {
        if ((ark == it->mArkfileNum && block == it->mBlockNum) != false) {
            it->mTasks.push_back(task);
            break;
        }
        if ((it->mArkfileNum > ark || (it->mArkfileNum == ark && it->mBlockNum > block))
            != false) {
            mRequests.insert(it, BlockRequest(task));
            break;
        }
    }
    if (it == mRequests.end())
        mRequests.push_back(BlockRequest(task));
}

void BlockMgr::WriteBlock() {
    MILO_ASSERT(!mWritingBlock, 345);
    bool ret;
    Block *blk;
    do {
        blk = FindLRUBlock(true);
        if (blk == nullptr)
            return;
        blk->mWritten = true;
        ret = TheHDCache.WriteAsync(blk->mArkfileNum, blk->mBlockNum, blk->mBuffer);
    } while (!ret);
    mWritingBlock = blk;
}

void BlockMgr::ReadBlock() {
    MILO_ASSERT(mReadingBlock, 364);

    bool x;
    void *buf = (void *)mReadingBlock->mBuffer;
    u32 arknum = mReadingBlock->mArkfileNum;
    u32 blknum = mReadingBlock->mBlockNum;
    if (TheHDCache.ReadAsync(arknum, blknum, buf)) {
        gReadHD = true;
        x = false;
    } else {
        gReadHD = false;
        x = CDRead(arknum, blknum * 32, 32, buf);
    }
    if (!x) {
        mReadingBlock->UpdateTimestamp();
    } else {
        MILO_LOG("CD READING ERROR: %x\n", x);
        mReadingBlock = nullptr;
    }
}

void BlockMgr::Poll() {
    MILO_ASSERT(MainThread(), 0x192);
    TheHDCache.Poll();
    mSpinDownTimer.Split();
    if (mWritingBlock && TheHDCache.WriteDone()) {
        mWritingBlock = NULL;
        WriteBlock();
    }
    if (mReadingBlock) {
        ++gNumPolls;
        int error = gReadHD ? TheHDCache.ReadFail() : CDGetError();
        if (error) {
            MILO_LOG(" CD READING ERROR!!!  %x\n", error);
            ReadBlock();
            return;
        }
        bool done = gReadHD ? TheHDCache.ReadDone() : CDReadDone();
        if (done) {
            if (Archive::DebugArkOrder()) {
                gReadTime.Split();
                int distance = mReadingBlock->mBlockNum - gLastBlockNum;
                if (mReadingBlock->mArkfileNum != gLastArkNum)
                    distance = 99999;
                if (distance != 1) {
                    ++gSeekCount;
                    gSeekTimeMs += gReadTime.Ms();
                } else {
                    gSeekCount = 0;
                    gSeekTimeMs = 0;
                }
                if (gSeekCount >= 1 || gSeekTimeMs >= 240.0f) {
                    char suspect[100];
                    strncpy(suspect, mReadingBlock->mDebugName, 99);
                    suspect[99] = '\0';
                    MILO_LOG(
                        "BlockMgr Seek: Ark: %2d  Dist: %5d  Seek Time: %3.0f ms  Suspect: %s\n",
                        mReadingBlock->mArkfileNum,
                        distance,
                        gSeekTimeMs,
                        suspect
                    );
                }
                gLastBlockNum = mReadingBlock->mBlockNum;
                gLastArkNum = mReadingBlock->mArkfileNum;
            }
            if (!gReadHD)
                MarkDiscRead();
            mReadingBlock->UpdateTimestamp();
            std::list<BlockRequest>::iterator request = mRequests.begin();
            for (; request != mRequests.end(); ++request) {
                if (mReadingBlock->CheckMetadata(request->mArkfileNum, request->mBlockNum))
                    break;
            }
            MILO_ASSERT(request != mRequests.end(), 0x1e0);
            mReadingBlock = NULL;
            for (std::list<AsyncTask>::iterator task = request->mTasks.begin();
                 task != request->mTasks.end();
                 ++task)
                task->FillData();
            mRequests.erase(request);
            if (!mWritingBlock)
                WriteBlock();
        }
    }
    if (!mReadingBlock && mRequests.size() != 0) {
        Block *block = FindLRUBlock(false);
        int blocknum = mRequests.front().mBlockNum;
        int ark = mRequests.front().mArkfileNum;
        const char *name = mRequests.front().mStr;
        MILO_ASSERT(blocknum != -1, 0x204);
        mReadingBlock = block;
        block->mBlockNum = blocknum;
        mReadingBlock->mArkfileNum = ark;
        mReadingBlock->mWritten = false;
        mReadingBlock->mDebugName = name;
        gReadTime.Restart();
        gNumPolls = 0;
        ReadBlock();
    }
}

Block *BlockMgr::FindBlock(int i1, int i2) {
    for (int i = 0; i < mBlockCache.size(); i++) {
        if (mBlockCache[i]->CheckMetadata(i1, i2))
            return mBlockCache[i];
    }
    return nullptr;
}

Block *BlockMgr::FindLRUBlock(bool b) {
    int time = Block::sCurrTimestamp;
    Block *ret = nullptr;
    for (int i = 0; i < mBlockCache.size(); i++) {
        if (mBlockCache[i] != mWritingBlock && mBlockCache[i] != mReadingBlock
            && (!b || !mBlockCache[i]->mWritten) && mBlockCache[i]->mTimestamp < time) {
            ret = mBlockCache[i];
            time = mBlockCache[i]->mTimestamp;
        }
    }
    return ret;
}

Block *BlockMgr::FindMRUBlock() {
    int time = -1;
    Block *ret = nullptr;
    for (int i = 0; i < mBlockCache.size(); i++) {
        if (mBlockCache[i]->mTimestamp > time) {
            ret = mBlockCache[i];
            time = mBlockCache[i]->mTimestamp;
        }
    }
    return ret;
}

bool BlockMgr::SpinUp() {
    TheBlockMgr.Poll();
    if (UsingCD()) {
        if (mSpinDownTimer.Ms() > 120000.000f) {
            if (mReadingBlock == nullptr) {
                MILO_LOG("BlockMgr spinning up...\n");
                Block *blk = FindMRUBlock();
                mReadingBlock = blk;
                AsyncTask at(blk->mArkfileNum, blk->mBlockNum);
                AddTask(at);
                gReadHD = false;
                bool x = CDRead(
                    mReadingBlock->mArkfileNum,
                    ((mReadingBlock->mBlockNum + 1) << 5) - 1,
                    1,
                    (void *)(mReadingBlock->mBuffer + 0xF800)
                );
                if (!x) {
                    mReadingBlock->UpdateTimestamp();
                } else {
                    MILO_LOG("CD READING ERROR: %x\n", x);
                    mReadingBlock = nullptr;
                }
            }
            return false;
        }
    }
    return true;
}

void BlockMgr::MarkDiscRead() { mSpinDownTimer.Restart(); }
