#include "NetCacheMgr.h"
#include "decomp.h"
#include "os/System.h"
#include "os/CommerceMgr_Wii.h"

// Preserve the original literal pool while other methods remain incomplete.
DECOMP_FORCEACTIVE(
    NetCacheMgr,
    "NetCacheMgr.cpp",
    "TheNetCacheMgr == NULL",
    "net_cache_mgr",
    "pData",
    "mServers.empty()",
    "netLoaderRef.IsValid()",
    "mLoadCount <= 2",
    "NetCcaheMgr::Load() called before previous load had finished.\n",
    "NetCacheMgr::Unload() called more times than NetCacheMgr::Load()!\n",
    "ref.mNetLoader == NULL",
    "ref.mCacheLoader == NULL",
    "Found loader for %s, but it was not type %d.\n",
    "Unknown ref type %d.\n",
    "Unknown net loader pos %d.\n",
    "pNetLoaderRef",
    "s != mServers.end()",
    "NetCacheMgr attempted to move straight from kNCMS_Nil to kNCMS_Unload!\n",
    "mNetLoaderRefs.empty()",
    "!mCache",
    "mLoadCacheSize",
    "Loader for %s has %d reference(s) left unaccounted for!\n",
    "NetCacheMgr::IsUnloadStateDone: %s still busy\n",
    "%s(%d): %s unhandled msg: %s",
    "IsValid()",
    "IsSafeToDelete()"
)

class NetCacheMgrWii : public NetCacheMgr {
public:
    NetCacheMgrWii() {}
    virtual ~NetCacheMgrWii();
};
NetCacheMgr *TheNetCacheMgr = 0;

void NetCacheMgrInit() {
    MILO_ASSERT(TheNetCacheMgr == NULL, 0x22);
    TheNetCacheMgr = new NetCacheMgrWii();
}

void NetCacheMgrTerminate() {
    delete TheNetCacheMgr;
    TheNetCacheMgr = NULL;
}

NetCacheMgr::NetCacheMgr()
    : mState(kNCMS_Nil), unk_0x20(false), mFailType(kNCMFT_Unknown), mStrXLSPFilter(),
      mXLSPServiceId(0), mServers(), mServerType(), mLoadCacheSize(0), mCache(NULL),
      mNetLoaderRefs(), mLoadCount(0) {
    SetName("net_cache_mgr", ObjectDir::sMainDir);
}

NetCacheMgr::~NetCacheMgr() {}

const char *NetCacheMgr::GetServer() const { return Server().mServer; }
unsigned short NetCacheMgr::GetPort() const { return Server().mPort; }
const char *NetCacheMgr::GetServerRoot() const { return Server().mRoot; }
bool NetCacheMgr::IsServerLocal() const { return Server().mLocal; }

bool NetCacheMgr::IsUnloaded() const { return mState != kNCMS_UnloadWaitForWrite; }
#pragma push
#pragma auto_inline on
bool NetCacheMgr::IsReady() const {
    return mState == kNCMS_Ready && !unk_0x20 && mLoadCount == 1;
}
#pragma pop

const NetCacheMgr::ServerData &NetCacheMgr::Server() const {
    std::list<ServerData>::const_iterator s = mServers.begin();
    while (s != mServers.end() && mServerType != s->mType) {
        ++s;
    }
    MILO_ASSERT(s != mServers.end(), 0x2e2);
    return *s;
}

void NetLoaderRef::AddRef() { ++mRefCount; }
void NetLoaderRef::ReleaseRef() { --mRefCount; }

bool NetLoaderRef::NeedsToDownload() {
    MILO_ASSERT(IsValid(), 0x31e);
    return mNetLoader || (!mCacheLoader || mCacheLoader->NeedsToDownload());
}

bool NetLoaderRef::IsDownloading() {
    MILO_ASSERT(IsValid(), 0x324);
    return mNetLoader || (mCacheLoader && mCacheLoader->IsDownloading());
}

void NetLoaderRef::Poll() {
    MILO_ASSERT(IsValid(), 0x32a);
    if (mCacheLoader)
        mCacheLoader->Poll();
    else
        mNetLoader->PollLoading();
}

bool NetLoaderRef::IsSafeToDelete() {
    MILO_ASSERT(IsValid(), 0x33d);
    if (mCacheLoader)
        return mCacheLoader->IsSafeToDelete();
    else
        return mNetLoader->IsSafeToDelete();
}

void NetLoaderRef::DeleteLoader() {
    MILO_ASSERT(IsSafeToDelete(), 0x343);
    delete mCacheLoader;
    mCacheLoader = NULL;
    delete mNetLoader;
    mNetLoader = NULL;
}

NetCacheLoader *NetCacheMgr::AddNetCacheLoader(const char *path, NetLoaderPos pos) {
    NetLoaderRef *ref = AddLoaderRef(path, kRefCache, pos);
    if (ref && ref->mCacheLoader)
        return ref->mCacheLoader;
    return NULL;
}

NetLoader *NetCacheMgr::AddNetLoader(const char *path, NetLoaderPos pos) {
    NetLoaderRef *ref = AddLoaderRef(path, kRefNet, pos);
    if (ref && ref->mNetLoader)
        return ref->mNetLoader;
    return NULL;
}

void NetCacheMgr::DeleteNetCacheLoader(NetCacheLoader *loader) {
    if (!loader)
        return;
    std::list<NetLoaderRef>::iterator ref = mNetLoaderRefs.begin();
    std::list<NetLoaderRef>::iterator end = mNetLoaderRefs.end();
    for (; ref != end; ++ref) {
        NetLoaderRef &netLoaderRef = *ref;
        if (netLoaderRef.mCacheLoader == loader) {
            netLoaderRef.ReleaseRef();
            return;
        }
    }
}

void NetCacheMgr::DeleteNetLoader(NetLoader *loader) {
    if (!loader)
        return;
    std::list<NetLoaderRef>::iterator ref = mNetLoaderRefs.begin();
    std::list<NetLoaderRef>::iterator end = mNetLoaderRefs.end();
    for (; ref != end; ++ref) {
        NetLoaderRef &netLoaderRef = *ref;
        if (netLoaderRef.mNetLoader == loader) {
            netLoaderRef.ReleaseRef();
            return;
        }
    }
}

bool NetCacheMgr::IsLocalFile(const char *path) const {
    if (IsReady())
        return mCache->FileCached(path);
    return false;
}

void NetCacheMgr::DebugClearCache() {
    if (IsReady())
        mCache->Clear();
}

bool NetCacheMgr::UseSSL() {
    std::list<ServerData>::const_iterator s = mServers.begin();
    while ((s != mServers.end() && mServerType != s->mType) != false)
        ++s;
    MILO_ASSERT(s != mServers.end(), 0x240);
    return s->mVerifySSL;
}

void NetCacheMgr::OnInit(DataArray *pData) {
    MILO_ASSERT(pData, 0x48);
    mXLSPServiceId = pData->FindInt(xlsp_service_id);
    mStrXLSPFilter = pData->FindStr(xlsp_filter);
    DataArray *pServers = pData->FindArray(servers);
    MILO_ASSERT(mServers.empty(), 0x58);
    for (int i = 1; i < pServers->Size(); ++i) {
        ServerData sd;
        DataArray *pServer = pServers->Array(i);
        sd.mType = pServer->Sym(0);
        sd.mServer = gNullStr;
        const char *pHost = NULL;
        bool bVerifySSL = true;
        bool bLocal;
        pServer->FindData(verify_ssl, bVerifySSL, false);
        sd.mVerifySSL = bVerifySSL;
        bLocal = false;
        pServer->FindData(local, bLocal, false);
        sd.mLocal = bLocal;
        pServer->FindData(server, pHost, false);
        sd.mServer = pHost;
        int iPort = 0;
        pServer->FindData(port, iPort, false);
        sd.mPort = iPort;
        sd.mRoot = pServer->FindStr(root);
        mServers.push_back(sd);
    }
    mServerType = pData->FindSym(default_server);
    for (std::list<ServerData>::iterator s = mServers.begin(); s != mServers.end(); ++s) {
    }
}

Symbol NetCacheMgr::CheatNextServer() {
    std::list<ServerData>::iterator s = mServers.begin();
    while ((s != mServers.end() && mServerType != s->mType) != false)
        ++s;
    MILO_ASSERT(s != mServers.end(), 0x22a);
    ++s;
    if (s == mServers.end())
        s = mServers.begin();
    mServerType = s->mType;
    if (UsingCD() && mServerType == local)
        CheatNextServer();
    return mServerType;
}

void NetCacheMgr::PollLoaders() {
    // The original inlines WiiCommerceMgr::IsBusy here.
    bool bCommerceBusy = TheWiiCommerceMgr.mCommerceAsyncOpId != -1;
    std::list<NetLoaderRef>::iterator it = mNetLoaderRefs.begin();
    std::list<NetLoaderRef>::iterator end = mNetLoaderRefs.end();
    bool bCanStartDownload = true;
    NetLoaderRef *pDownloading = NULL;
    NetLoaderRef *pNeedsDownload = NULL;
    for (; it != end; ++it) {
        NetLoaderRef &netLoaderRef = *it;
        MILO_ASSERT(netLoaderRef.IsValid(), 0xea);
        if (netLoaderRef.IsDownloading()) {
            pDownloading = &netLoaderRef;
            bCanStartDownload = false;
        } else if (netLoaderRef.NeedsToDownload()) {
            if (bCanStartDownload) {
                pNeedsDownload = &netLoaderRef;
                bCanStartDownload = false;
            }
        } else {
            netLoaderRef.Poll();
        }
    }
    if (pDownloading)
        pDownloading->Poll();
    else if (pNeedsDownload && !bCommerceBusy)
        pNeedsDownload->Poll();
    it = mNetLoaderRefs.begin();
    end = mNetLoaderRefs.end();
    while (it != end) {
        NetLoaderRef &ref = *it;
        if (ref.mRefCount < 1 && ref.IsSafeToDelete()) {
            ref.DeleteLoader();
            it = mNetLoaderRefs.erase(it);
        } else {
            ++it;
        }
    }
}

NetLoaderRef *
NetCacheMgr::AddLoaderRef(const char *path, RefType type, NetLoaderPos pos) {
    if (*path == 0 || !IsReady())
        return NULL;
    NetLoaderRef *pNetLoaderRef = NULL;
    std::list<NetLoaderRef>::iterator it = mNetLoaderRefs.begin();
    std::list<NetLoaderRef>::iterator end = mNetLoaderRefs.end();
    for (; it != end; ++it) {
        NetLoaderRef &ref = *it;
        if (stricmp(ref.mStrRemotePath.c_str(), path) == 0) {
            if (type == kRefCache && ref.mCacheLoader) {
                MILO_ASSERT(ref.mNetLoader == NULL, 0x181);
                pNetLoaderRef = &ref;
                break;
            } else if (type == kRefNet && ref.mNetLoader) {
                MILO_ASSERT(ref.mCacheLoader == NULL, 0x187);
                pNetLoaderRef = &ref;
                break;
            } else {
                MILO_LOG(
                    "Found loader for %s, but it was not type %d.\n",
                    ref.mStrRemotePath.c_str(),
                    type
                );
            }
        }
    }
    NetLoaderRef newRef;
    if (!pNetLoaderRef) {
        switch (type) {
        case kRefCache: {
            NetCacheLoader *pLoader = new NetCacheLoader(mCache, path);
            newRef = NetLoaderRef(path, pLoader);
            break;
        }
        case kRefNet: {
            NetLoader *pLoader = NetLoader::Create(path);
            newRef = NetLoaderRef(path, pLoader);
            break;
        }
        default:
            MILO_FAIL("Unknown ref type %d.\n", type);
            break;
        }
        switch (pos) {
        case 0:
            mNetLoaderRefs.push_front(newRef);
            pNetLoaderRef = &mNetLoaderRefs.front();
            break;
        case 1:
            pNetLoaderRef = &*mNetLoaderRefs.insert(mNetLoaderRefs.end(), newRef);
            break;
        default:
            MILO_FAIL("Unknown net loader pos %d.\n", pos);
            break;
        }
    }
    MILO_ASSERT(pNetLoaderRef, 0x1c1);
    pNetLoaderRef->AddRef();
    return pNetLoaderRef;
}

void NetCacheMgr::Poll() {
    PollLoaders();
    switch (mState) {
    case kNCMS_Load:
        if (IsDoneLoading())
            SetState(kNCMS_Ready);
        break;
    case kNCMS_UnloadWaitForWrite:
        if (IsUnloadStateDone())
            SetState(kNCMS_Nil);
        break;
    }
}

void NetCacheMgr::Load(CacheSize size) {
    ++mLoadCount;
    MILO_ASSERT(mLoadCount <= 2, 0x12b);
    if (mState == kNCMS_Load && !unk_0x20) {
        MILO_WARN("NetCcaheMgr::Load() called before previous load had finished.\n");
    }
    mLoadCacheSize = size == 0 ? 0x100000 : 0x500000;
    if (mLoadCount == 1 && mState == kNCMS_Nil)
        SetState(kNCMS_Load);
}

void NetCacheMgr::Unload() {
    --mLoadCount;
    if (mLoadCount < 0) {
        MILO_LOG("NetCacheMgr::Unload() called more times than NetCacheMgr::Load()!\n");
        mLoadCount = 0;
    } else {
        SetState(kNCMS_UnloadWaitForWrite);
    }
}

void NetCacheMgr::SetState(NetCacheMgrState state) {
    if (mState == state)
        return;
    if (mState == kNCMS_UnloadWaitForWrite)
        unk_0x20 = false;
    if (mState == kNCMS_Nil && state == kNCMS_UnloadWaitForWrite) {
        MILO_FAIL(
            "NetCacheMgr attempted to move straight from kNCMS_Nil to kNCMS_Unload!\n"
        );
    }
    mState = state;
    switch (state) {
    case kNCMS_Load:
        EnterLoadState();
        break;
    case kNCMS_Ready:
        ReadyInit();
        break;
    case kNCMS_UnloadWaitForWrite:
        EnterUnloadState();
        break;
    case kNCMS_Nil:
        MILO_ASSERT(mNetLoaderRefs.empty(), 0x28a);
        if (mLoadCount > 0)
            SetState(kNCMS_Load);
        break;
    }
}

void NetCacheMgr::EnterLoadState() {
    unk_0x20 = false;
    LoadInit();
    if (!unk_0x20) {
        MILO_ASSERT(!mCache, 0x2aa);
        MILO_ASSERT(mLoadCacheSize, 0x2ab);
        mCache = new FileCache(mLoadCacheSize, (LoaderPos)3, true);
        mLoadCacheSize = 0;
    }
}

void NetCacheMgr::EnterUnloadState() {
    UnloadInit();
    for (std::list<NetLoaderRef>::iterator it = mNetLoaderRefs.begin();
         it != mNetLoaderRefs.end();
         ++it) {
        NetLoaderRef &ref = *it;
        if (ref.mRefCount > 0) {
            MILO_WARN(
                "Loader for %s has %d reference(s) left unaccounted for!\n",
                ref.mStrRemotePath,
                ref.mRefCount
            );
            ref.mRefCount = 0;
        }
    }
    delete mCache;
    mCache = NULL;
}

void NetCacheMgr::UnloadInit() {}

bool NetCacheMgr::IsUnloadStateDone() const {
    for (std::list<NetLoaderRef>::const_iterator it = mNetLoaderRefs.begin();
         it != mNetLoaderRefs.end();
         ++it) {
        if (it->mCacheLoader && it->mCacheLoader->IsDownloading()) {
            MILO_LOG(
                "NetCacheMgr::IsUnloadStateDone: %s still busy\n",
                it->mStrRemotePath.c_str()
            );
            it->mCacheLoader->Poll();
            return false;
        }
    }
    return IsDoneUnloading() && mNetLoaderRefs.empty();
}

bool NetCacheMgr::IsDoneLoading() const { return 1; }

bool NetCacheMgr::IsDoneUnloading() const { return 1; }

void NetCacheMgr::LoadInit() { return; }

void NetCacheMgr::ReadyInit() { return; }

NetCacheMgrFailType NetCacheMgr::GetFailType() const { return mFailType; }

#pragma push
#pragma auto_inline on
BEGIN_HANDLERS(NetCacheMgr)
    HANDLE_ACTION(init, OnInit(_msg->Array(2)))
    HANDLE_ACTION(debug_clear_cache, DebugClearCache())
    HANDLE_EXPR(cheat_next_server, CheatNextServer())
    HANDLE_EXPR(server_type, mServerType);
    HANDLE_CHECK(0x2f3)
END_HANDLERS
#pragma pop

NetCacheMgrWii::~NetCacheMgrWii() {}
