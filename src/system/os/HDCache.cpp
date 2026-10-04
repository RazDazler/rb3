#include "HDCache.h"
#include "decomp.h"

#if defined(VERSION_SZBE69_B8)
// Preserve original literal ordering while incomplete methods remain.
DECOMP_FORCEACTIVE(
    LiteralPoolHDCache,
    "no_hdcache",
    "skip_hdcache",
    "Using the archive cache\n",
    "",
    "HDCache.cpp",
    "mLockId == CurrentThreadId()",
    "HDCache Write Header Failed\n",
    "mHdr[mHdrIdx]->WriteDone()",
    "mHdrBuf->Size() <= finalSize",
    "mHdrBuf->Size() == finalSize",
    "oldSize == newSize",
    "ReadDone()",
    "blockNum < TheArchive->GetArkfileNumBlocks(arkfileNum)",
    "mReadArkFiles[arkfileNum]->Size() >= ((blockNum + 1) * kArkBlockSize)",
    "HDCache Read %d failed\n",
    "WriteDone()",
    "mReadArkFiles[mWriteFileIdx]->Size() == mWriteArkFiles[mWriteFileIdx]->Size()",
    "HDCache Write %d.%d failed\n",
    "numCachedArkfiles <= numArkfiles",
    "max != pendingArkfiles.end()"
)
#endif
#include "os/Debug.h"
#include "os/OSFuncs.h"
#include "os/Archive.h"
#include "utl/Option.h"
#include "os/System.h"
#include "math/SHA1.h"
#include <string.h>

HDCache TheHDCache;

HDCache::HDCache()
    : mBlockState(0), mWriteFileIdx(0), unk18(-1), unk20(0), unk24(0), unk28(-1),
      unk2c(-1), mLockId(0), unk34(0), mCritSec(0), mHdrIdx(0), mHdrBuf(0), unk64(0) {}

HDCache::~HDCache() {}

void HDCache::Init() {
    mCritSec = new CriticalSection();
    if (!TheArchive)
        return;
    OptionBool("no_hdcache", true);
    int numArkfiles = TheArchive->mNumArkfiles;
    mReadArkFiles.resize(numArkfiles);
    mWriteArkFiles.resize(numArkfiles);
    FileStream *header = OpenHeader();
    bool valid = header && header->Size() == HdrSize();
    if (valid) {
        header->EnableReadEncryption();
        int version;
        *header >> version;
        valid = version == 2;
    }
    if (valid) {
        HxGuid headerGuid, archiveGuid;
        *header >> headerGuid;
        TheArchive->GetGuid(archiveGuid);
        valid = headerGuid == archiveGuid;
    }
    int numCachedArkfiles = 0;
    if (valid) {
        *header >> numCachedArkfiles;
        if (numCachedArkfiles < 0 || numCachedArkfiles > numArkfiles) {
            numCachedArkfiles = 0;
            valid = false;
        }
    }
    OpenFiles(numCachedArkfiles);
    mBlockState = new int *[numArkfiles];
    CSHA1 sha;
    unsigned char savedBlocks[4096];
    for (int i = 0; i < numArkfiles; ++i) {
        unsigned int savedBytes = 0;
        if (i < numCachedArkfiles) {
            *header >> savedBytes;
            if (savedBytes > 4096 || savedBytes % 4)
                valid = false;
            else
                header->Read(savedBlocks, savedBytes);
            if (header->Fail() || !valid) {
                savedBytes = 0;
                valid = false;
                numCachedArkfiles = 0;
            }
            if (valid)
                sha.Update(savedBlocks, savedBytes);
        }
        File *&read = mReadArkFiles[i];
        File *&write = mWriteArkFiles[i];
        if (!read || read->Fail() || !write || write->Fail()) {
            delete read;
            read = NULL;
            delete write;
            write = NULL;
        }
        if (read) {
            int bytes = ((TheArchive->GetArkfileNumBlocks(i) + 31) / 32) * 4;
            int *states = (int *)new char[bytes];
            memcpy(states, savedBlocks, savedBytes);
            // Original B8 advances the int pointer by savedBytes, not savedBytes / 4.
            // Keep this observed expression for matching; do not treat it as safe
            // portable code.
            memset(states + savedBytes, 0, bytes - savedBytes);
            mBlockState[i] = states;
        } else
            mBlockState[i] = NULL;
    }
    if (valid) {
        char computedHash[256] = { 0 };
        char savedHash[256] = { 0 };
        sha.Final()->ReportHash(computedHash, 0);
        header->Read(savedHash, 256);
        valid = !header->Fail() && !memcmp(computedHash, savedHash, 256);
    }
    if (OptionBool("skip_hdcache", false))
        valid = false;
    if (valid) {
        unk64 = true;
        MILO_LOG("Using the archive cache\n");
    } else {
        for (int i = 0; i < numArkfiles; ++i) {
            if (mBlockState[i]) {
                int blocks = TheArchive->GetArkfileNumBlocks(i);
                int *states = mBlockState[i];
                memset(states, 0, ((blocks + 31) / 32) * 4);
            }
        }
    }
    delete header;
    mHdrFmt = "";
    mFileFmt = "";
    mHdrBuf = new MemStream(true);
}

extern int kArkBlockSize;

bool HDCache::ReadAsync(int arkfileNum, int blockNum, void *buffer) {
    MILO_ASSERT(ReadDone(), 0x190);
    if (mBlockState[arkfileNum]) {
        MILO_ASSERT(blockNum < TheArchive->GetArkfileNumBlocks(arkfileNum), 0x195);
        int mask = 1 << (blockNum % 32);
        if (mBlockState[arkfileNum][blockNum / 32] & mask) {
            MILO_ASSERT(mReadArkFiles[arkfileNum]->Size() >= ((blockNum + 1) * kArkBlockSize), 0x19c);
            unk20 = arkfileNum;
            int size = kArkBlockSize;
            mReadArkFiles[arkfileNum]->Seek(blockNum * size, 0);
            return mReadArkFiles[unk20]->ReadAsync(buffer, size);
        }
    }
    return false;
}

bool HDCache::WriteAsync(int arkfileNum, int blockNum, const void *buffer) {
    MILO_ASSERT(WriteDone(), 0x1c0);
    if (mBlockState[arkfileNum]) {
        MILO_ASSERT(blockNum < TheArchive->GetArkfileNumBlocks(arkfileNum), 0x1c5);
        int mask = 1 << (blockNum % 32);
        if (mBlockState[arkfileNum][blockNum / 32] & mask)
            return false;
        int size = kArkBlockSize;
        if (mWriteArkFiles[arkfileNum]->Size() < (blockNum + 1) * size)
            return false;
        if (!LockCache())
            return false;
        unk2c = SystemMs();
        mWriteFileIdx = arkfileNum;
        unk18 = blockNum;
        mWriteArkFiles[arkfileNum]->Seek(blockNum * size, 0);
        bool success = mWriteArkFiles[mWriteFileIdx]->WriteAsync(buffer, size);
        if (!success)
            WriteDone();
        return success;
    }
    return false;
}

void HDCache::Poll() {
    if (mWritingHeader) {
        int bytes;
        if (mHdr[mHdrIdx]->WriteDone(bytes)) {
            UnlockCache();
            if (mHdr[mHdrIdx]->Fail())
                MILO_LOG("HDCache Write Header Failed\n");
            mWritingHeader = false;
        }
    }
    if (unk24) {
        if (mWritingHeader)
            return;
        if (unk24 > 1024 || SystemMs() - unk28 > 60000)
            WriteHdr();
    }
}

int HDCache::HdrSize() {
    int size = 32;
    int numArkfiles = TheArchive->mNumArkfiles;
    for (int i = 0; i < numArkfiles; ++i) {
        if (TheArchive->GetArkfileCachePriority(i) >= 0) {
            int bytes = ((TheArchive->GetArkfileNumBlocks(i) + 31) / 32) * 4;
            size += 4;
            size += bytes;
        }
    }
    size += 256;
    int remainder = size % 4096;
    if (remainder)
        size += 4096 - remainder;
    return size;
}

void HDCache::WriteHdr() {
    if (mHdr[mHdrIdx]->Fail() || !LockCache())
        return;
    MILO_ASSERT(mHdr[mHdrIdx]->WriteDone(), 0x143);
    CSHA1 sha;
    mHdrBuf->Seek(0, BinStream::kSeekBegin);
    mHdrBuf->EnableWriteEncryption();
    *mHdrBuf << 2;
    HxGuid guid;
    TheArchive->GetGuid(guid);
    *mHdrBuf << guid;
    int numArkfiles = TheArchive->mNumArkfiles;
    *mHdrBuf << numArkfiles;
    for (int i = 0; i < numArkfiles; ++i) {
        int bytes = 0;
        if (mBlockState[i])
            bytes = ((TheArchive->GetArkfileNumBlocks(i) + 31) / 32) * 4;
        *mHdrBuf << bytes;
        if (bytes > 0) {
            mHdrBuf->Write(mBlockState[i], bytes);
            sha.Update((const unsigned char *)mBlockState[i], bytes);
        }
    }
    char hash[256] = { 0 };
    sha.Final()->ReportHash(hash, 0);
    mHdrBuf->Write(hash, 256);
    mHdrBuf->DisableEncryption();
    unk24 = 0;
    int finalSize = HdrSize();
    MILO_ASSERT(mHdrBuf->Size() <= finalSize, 0x175);
    char padding[128];
    memset(padding, 0, sizeof(padding));
    while (mHdrBuf->Size() < finalSize) {
        unsigned int bytes = finalSize - mHdrBuf->Size();
        if (bytes > sizeof(padding))
            bytes = sizeof(padding);
        mHdrBuf->Write(padding, bytes);
    }
    MILO_ASSERT(mHdrBuf->Size() == finalSize, 0x182);
    int oldSize = mHdr[mHdrIdx]->Size();
    int newSize = mHdrBuf->Size();
    MILO_ASSERT(oldSize == newSize, 0x185);
    mWritingHeader = true;
    mHdr[mHdrIdx]->Seek(0, 0);
    mHdr[mHdrIdx]->WriteAsync(mHdrBuf->Buffer(), mHdrBuf->Size());
}

bool HDCache::ReadDone() {
    int done;
    File *readArkFile = mReadArkFiles[unk20];
    if (readArkFile == 0) {
        return true;
    } else {
        return readArkFile->ReadDone(done);
    }
}

bool HDCache::ReadFail() {
    File *readArkFile = mReadArkFiles[unk20];
    if (readArkFile != nullptr) {
        if (readArkFile->Fail()) {
            TheDebug << MakeString("HDCache Read %d failed\n", unk20);
            return true;
        }
    }
    return false;
}

bool HDCache::WriteDone() {
    if (unk18 >= 0) {
        int bytes;
        if (mWriteArkFiles[mWriteFileIdx]->WriteDone(bytes)) {
            MILO_ASSERT(mReadArkFiles[mWriteFileIdx]->Size() == mWriteArkFiles[mWriteFileIdx]->Size(), 0x1f2);
            UnlockCache();
            if (mWriteArkFiles[mWriteFileIdx]->Fail()) {
                MILO_LOG("HDCache Write %d.%d failed\n", mWriteFileIdx, unk18);
            } else {
                int word = unk18 / 32;
                int mask = 1 << (unk18 % 32);
                ++unk24;
                if (unk24 == 1)
                    unk28 = SystemMs();
                mBlockState[mWriteFileIdx][word] |= mask;
            }
            unk18 = -1;
        }
    }
    return unk18 == -1;
}

bool HDCache::LockCache() {
    CritSecTracker cst(mCritSec);
    if (mLockId == 0 || mLockId == CurrentThreadId()) {
        mLockId = CurrentThreadId();
        unk34++;
        return true;
    } else
        return false;
}

void HDCache::UnlockCache() {
    CritSecTracker cst(mCritSec);
    MILO_ASSERT(mLockId == CurrentThreadId(), 0xF9);
    if (!--unk34)
        mLockId = 0;
}

void HDCache::OpenFiles(int numCachedArkfiles) {
    if (!mFileFmt.mStr[0])
        return;
    int numArkfiles = TheArchive->mNumArkfiles;
    MILO_ASSERT(numCachedArkfiles <= numArkfiles, 0x22e);
    FileMkDir(FileGetPath(mFileFmt.mStr, NULL));
    std::vector<int> pendingArkfiles;
    for (int i = 0; i < numArkfiles; ++i) {
        const char *filename = MakeString(mFileFmt.mStr, i);
        bool exists = FileExists(filename, 0x10000);
        int priority = TheArchive->GetArkfileCachePriority(i);
        if (exists && i > numCachedArkfiles)
            FileDelete(filename);
        if (priority >= 0)
            pendingArkfiles.push_back(i);
    }
    const char *filename = MakeString(mHdrFmt.mStr, 0);
    mHdr[0] = NewFile(filename, 0x50204);
    bool success = mHdr[0] && !mHdr[0]->Fail();
    if (success) {
        int size = HdrSize();
        mHdr[0]->Truncate(size);
        delete mHdr[0];
        mHdr[0] = NULL;
        mHdr[0] = NewFile(filename, 0x50004);
        success = mHdr[0]->Size() == size;
    }
    if (!success) {
        delete mHdr[0];
        mHdr[0] = NULL;
        return;
    }
    int blockSize = kArkBlockSize;
    while (!pendingArkfiles.empty()) {
        int priority = -1;
        std::vector<int>::iterator max = pendingArkfiles.end();
        for (std::vector<int>::iterator it = pendingArkfiles.begin();
             it != pendingArkfiles.end();
             ++it) {
            int nextPriority = TheArchive->GetArkfileCachePriority(*it);
            if (nextPriority > priority) {
                priority = nextPriority;
                max = it;
            }
        }
        MILO_ASSERT(max != pendingArkfiles.end(), 0x26e);
        int arkfile = *max;
        const char *filename = MakeString(mFileFmt.mStr, arkfile);
        File *file = NewFile(filename, 0x50204);
        bool success =
            file && file->Truncate(blockSize * TheArchive->GetArkfileNumBlocks(arkfile));
        if (file) {
            delete file;
            if (!success)
                FileDelete(filename);
        }
        pendingArkfiles.erase(max);
    }
    for (int i = 0; i < numArkfiles; ++i) {
        const char *filename = MakeString(mFileFmt.mStr, i);
        File *read = NewFile(filename, 0x50002);
        File *write = NewFile(filename, 0x50004);
        if (!read || !write || read->Fail() || write->Fail()) {
            delete read;
            read = NULL;
            delete write;
            write = NULL;
        }
        mReadArkFiles[i] = read;
        mWriteArkFiles[i] = write;
    }
}

FileStream *HDCache::OpenHeader() {
    if (mHdrFmt.mStr[0] == '\0')
        return NULL;
    const char *str;
    int i;
    for (i = 0; i < 2; ++i) {
        str = MakeString(mHdrFmt.mStr, 0);
        if (FileExists(str, 0x10000))
            break;
    }
    if (i == 2)
        return NULL;
    return new FileStream(str, FileStream::kReadNoArk, true);
}
