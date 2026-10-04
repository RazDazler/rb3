#pragma once
#include "obj/Object.h"
#include "obj/Dir.h"
#include "utl/Symbols.h"
#include "utl/Cache.h"
#include "utl/NetLoader.h"
#include "utl/NetLoaderRef.h"
#include "utl/NetCacheLoader.h"
#include <vector>
#include <list>

enum NetCacheMgrFailType {
    kNCMFT_Unknown,
    kNCMFT_StoreServer,
    kNCMFT_NoSpace,
    kNCMFT_StorageDeviceMissing,
    kNCMFT_Max
};

enum NetCacheMgrState {
    kNCMS_Load,
    kNCMS_Ready,
    kNCMS_UnloadWaitForWrite,
    kNCMS_UnloadUnmount,
    kNCMS_Failure,
    kNCMS_Max,
    kNCMS_Nil = -1
};

enum LoadState {
    kLS_None,
    kLS_Mount,
    kLS_Delete,
    kLS_ReMount,
    kLS_Resync
};

class NetCacheMgr : public Hmx::Object {
public:
    enum CacheSize {
    };
    enum RefType {
        kRefCache,
        kRefNet
    };
    NetLoaderRef *AddLoaderRef(const char *, RefType, NetLoaderPos);
    bool UseSSL();

    NetCacheMgr();
    virtual ~NetCacheMgr();
    virtual DataNode Handle(DataArray *, bool);
    virtual void Poll();
    virtual void LoadInit();
    virtual bool IsDoneLoading() const;
    virtual void ReadyInit();
    virtual void UnloadInit();
    virtual bool IsDoneUnloading() const;

    const char *GetServerRoot() const;
    const char *GetServer() const;
    bool IsServerLocal() const;
    NetLoader *AddNetLoader(const char *, NetLoaderPos);
    void DeleteNetLoader(NetLoader *);
    unsigned short GetPort() const;
    NetCacheMgrFailType GetFailType() const;
    void SetState(NetCacheMgrState);
    void PollLoaders();
    void EnterLoadState();
    void EnterUnloadState();
    bool IsUnloadStateDone() const;
    void Unload();
    bool IsLocalFile(const char *) const;
    void OnInit(DataArray *);
    Symbol CheatNextServer();
    void DebugClearCache();
    void DeleteNetCacheLoader(NetCacheLoader *);
    void Load(CacheSize);
    bool IsUnloaded() const;
    bool IsReady() const;
    NetCacheLoader *AddNetCacheLoader(const char *, NetLoaderPos);

    NetCacheMgrState mState; // 0x1c
    bool unk_0x20; // 0x20
    NetCacheMgrFailType mFailType; // 0x24
    String mStrXLSPFilter; // 0x28 unsure if this name is still correct, but there is
                           // indeed a String here
    struct ServerData {
        Symbol mType; // 0x0
        bool mLocal; // 0x4
        const char *mServer; // 0x8
        unsigned short mPort; // 0xc
        const char *mRoot; // 0x10
        bool mVerifySSL; // 0x14
    };

    const ServerData &Server() const;

    int mXLSPServiceId; // 0x34
    std::list<ServerData> mServers; // 0x38
    Symbol mServerType; // 0x40
    unsigned int mLoadCacheSize; // 0x44
    FileCache *mCache; // 0x48
    std::list<NetLoaderRef> mNetLoaderRefs; // 0x4c
    int mLoadCount; // 0x54
};

extern NetCacheMgr *TheNetCacheMgr;
void NetCacheMgrInit();
void NetCacheMgrTerminate();
