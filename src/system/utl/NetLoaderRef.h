#pragma once
#include "utl/NetLoader.h"

class NetCacheLoader;

class NetLoaderRef {
public:
    NetLoaderRef()
        : mStrRemotePath(), mRefCount(0), mNetLoader(NULL), mCacheLoader(NULL) {}
    NetLoaderRef(const String &path, NetLoader *loader)
        : mStrRemotePath(path), mRefCount(0), mNetLoader(loader), mCacheLoader(NULL) {}
    NetLoaderRef(const String &path, NetCacheLoader *loader)
        : mStrRemotePath(path), mRefCount(0), mNetLoader(NULL), mCacheLoader(loader) {}
    void AddRef();
    void ReleaseRef();
    bool NeedsToDownload();
    bool IsDownloading();
    void Poll();
    bool IsSafeToDelete();
    void DeleteLoader();

    bool IsValid() const {
        return (mCacheLoader == NULL || mNetLoader == NULL)
            && !(mCacheLoader == NULL && mNetLoader == NULL);
    }

    String mStrRemotePath; // 0x0
    int mRefCount; // 0xc
    NetLoader *mNetLoader; // 0x10
    NetCacheLoader *mCacheLoader; // 0x14
};
