#include "NetLoader.h"
#include "decomp.h"
#include "utl/NetLoader_Wii.h"
#include "obj/DataFile.h"
#include "utl/BufStream.h"
#include <obj/Task.h>
#include "utl/NetCacheMgr.h"

NetLoader *NetLoader::Create(const String &path) {
    if (TheNetCacheMgr->IsServerLocal()) {
        return new NetLoaderStub(path);
    } else {
        return new NetLoaderWii(path);
    }
}

NetLoader::NetLoader(const String &pStrRemotePath)
    : mStrRemotePath(pStrRemotePath), mIsLoaded(false), mBuffer(nullptr), mSize(-1),
      unk_0x20(0) {
    MILO_ASSERT(TheNetCacheMgr, 0x30);
}

NetLoader::~NetLoader() {
    if (mBuffer != nullptr) {
        _MemFree(mBuffer);
        mBuffer = nullptr;
    }
}

#pragma push
#pragma auto_inline on
bool NetLoader::IsLoaded() { return mIsLoaded; }
#pragma pop

#pragma push
#pragma auto_inline on
const char *NetLoader::GetRemotePath() const { return mStrRemotePath.c_str(); }
#pragma pop

#pragma push
#pragma auto_inline on
int NetLoader::GetSize() { return mSize; }
#pragma pop

char *NetLoader::GetBuffer() {
    if (mIsLoaded != false) {
        return mBuffer;
    }
    return 0;
}

char *NetLoader::DetachBuffer() {
    if (mIsLoaded == false) {
        return 0;
    }
    char *buf = mBuffer;
    mBuffer = 0;
    return buf;
}

void NetLoader::AttachBuffer(char *pBuf) {
    if (mBuffer != 0) {
        MILO_ASSERT(mIsLoaded, 0x74);
        if (mBuffer != 0) {
            _MemFree(mBuffer);
            mBuffer = 0;
        }
    }
    mBuffer = pBuf;
}

void NetLoader::SetSize(int pSize) { mSize = pSize; }

void NetLoader::PostDownload() { mIsLoaded = mBuffer != 0; }

NetLoaderStub::NetLoaderStub(const String &path) : NetLoader(path), mFileLoader(nullptr) {
    FilePath file(
        MakeString("%s/%s", TheNetCacheMgr->GetServerRoot(), mStrRemotePath.c_str())
    );
    mFileLoader = new FileLoader(file, file.c_str(), kLoadFront, 0, false, true, nullptr);
    MILO_ASSERT(mFileLoader, 0xa2);
    float kilobytes = mFileLoader->GetSize() / 1024.0f;
    mNetSimEndTime = kilobytes / 32.0f + (0.2f + TheTaskMgr.UISeconds());
}

NetLoaderStub::~NetLoaderStub() {
    delete mFileLoader;
    mFileLoader = nullptr;
}

void NetLoaderStub::PollLoading() {
    MILO_ASSERT(mFileLoader, 0xb2);
    if (mIsLoaded == false) {
        if (mFileLoader->IsLoaded() == 0) {
            TheLoadMgr.Poll();
        }
        if (mFileLoader->IsLoaded() != 0) {
            float uiSeconds = TheTaskMgr.UISeconds();
            if (mNetSimEndTime <= uiSeconds) {
                int size = -1;
                const char *buf = mFileLoader->GetBuffer(&size);
                AttachBuffer((char *)buf);
                SetSize(size);
                PostDownload();
            }
        }
        return;
    }
}

bool NetLoaderStub::HasFailed() { return !mBuffer; }

DataNetLoader::DataNetLoader(const String &path) : mLoader(nullptr), unk_0x4(nullptr) {
    if (!TheNetCacheMgr) {
        MILO_FAIL("Tried to create a DataNetLoader, but TheNetCacheMgr is NULL.\n");
    } else {
        mLoader = TheNetCacheMgr->AddNetLoader(path.c_str(), (NetLoaderPos)0);
    }
}

DataNetLoader::~DataNetLoader() {
    if (mLoader) {
        TheNetCacheMgr->DeleteNetLoader(mLoader);
        mLoader = nullptr;
    }
    if (unk_0x4) {
        unk_0x4->Release();
        unk_0x4 = nullptr;
    }
}

void DataNetLoader::PollLoading() {
    if (mLoader) {
        if (mLoader->IsLoaded()) {
            int size = mLoader->GetSize();
            char *buffer = mLoader->GetBuffer();
            const char *path = mLoader->GetRemotePath();
            if (streq(FileGetExt(path), "dtz")) {
                DataArray::SetFile(path);
                unk_0x4 = LoadDtz(buffer, size);
            } else {
                BufStream stream(buffer, size, true);
                unk_0x4 = DataReadStream(&stream);
            }
            TheNetCacheMgr->DeleteNetLoader(mLoader);
            mLoader = nullptr;
        } else if (mLoader->HasFailed()) {
            TheNetCacheMgr->DeleteNetLoader(mLoader);
            mLoader = nullptr;
        }
    }
}

bool DataNetLoader::IsLoaded() {
    bool loaderIsLoaded = true;
    if (mLoader != 0) {
        loaderIsLoaded = mLoader->mIsLoaded;
    }
    bool retVal = false;
    if ((loaderIsLoaded != false) && (unk_0x4 != 0)) {
        retVal = true;
    }
    return retVal;
}

bool DataNetLoader::HasFailed() {
    NetLoader *loader = mLoader;
    if (mLoader != 0) {
        return mLoader->HasFailed();
    }
    return unk_0x4 == 0;
}

bool NetLoaderStub::IsSafeToDelete() const { return 1; }
