#include "CacheMgr_Wii.h"
#include "decomp.h"

#if defined(VERSION_SZBE69_B8)
// Preserve original literal ordering while incomplete methods remain.
DECOMP_FORCEACTIVE(
    LiteralPoolCacheMgr_Wii,
    "A",
    "Can't delete system file.",
    "Not enough NAND available for VF.",
    "Can't create sytem file.",
    "Can't mount nand drive.",
    "Can't format nand drive",
    "Can't unmount nand drive.",
    "Unknown OpType encountered in CacheMgr::Poll()\n",
    "SearchAsync BAD PARAM: ppCacheID = 0x%X",
    ", *ppCacheID = 0x%X",
    "\n",
    "SearchAsync BAD PARAM: mStrCacheName is empty\n",
    "CacheMgr_Wii.cpp",
    "false",
    "IsDone()",
    "/",
    "cache_mgr_mount_result",
    "cache_mgr_unmount_result"
)
#endif
#include "Cache_Wii.h"
#include "VF.h"
#include "os/Debug.h"
#include "utl/MakeString.h"

const char *unusedStrings[] = {
    "A",
    "Can't delete system file.",
    "Not enough NAND available for VF.",
    "Can't create sytem file.", // Intentional typo
    "Can't mount nand drive.",
    "Can't format nand drive",
    "Can't unmount nand drive.",
};

CacheMgrWii::CacheMgrWii() : mVar1(), mVar2(0), mVar3(0), mVar4(0), mVar5(0) {
    CreateVFCache();
}

void CacheMgrWii::CreateVFCache() {
    VFInitEx();
    bool result = VFMountDriveNANDFlashEx("", "");
}

CacheMgrWii::~CacheMgrWii() {}

void CacheMgrWii::Poll() {
    CacheMgr::OpType op = GetOp();
    if (op != 0) {
        switch (op) {
        case 1:
            PollSearch();
            break;
        case 3:
            PollMount();
            break;
        case 4:
            PollUnmount();
            break;
        default:
            FormatString error("Unknown OpType encountered in CacheMgr::Poll()\n");
            TheDebug.Fail(error.Str());
            break;
        }
    }
}

const char *unusedStrings2[] = { "\n"

};

bool CacheMgrWii::SearchAsync(const char *param_1, CacheID **param_2) {
    if (!IsDone()) {
        SetLastResult(kCache_ErrorBusy);
    } else {
        if (param_2 != NULL && (*param_2) != NULL) {
            SetLastResult(kCache_ErrorBadParam);
            return true;
        }
        TheDebug << "SearchAsync BAD PARAM: mStrCacheName is empty\n";

        if (param_2 != NULL) {
            TheDebug << MakeString(", *ppCacheID = 0x%X", param_2);
        }
        TheDebug << "";
        SetLastResult(kCache_NoError);
        return true;
    }
    TheDebug << MakeString("SearchAsync BAD PARAM: ppCacheID = 0x%X", param_2);
    if (param_2 != NULL) {
        TheDebug << MakeString(", *ppCacheID = 0x%X", param_2);
    }
    SetLastResult(kCache_ErrorBadParam);

    return false;
}

/*
bool CacheMgrWii::CreateCacheID(const char* param_1, const char* param_2, const char*
param_3, const char* param_4, const char* param_5, int param_6, CacheID** param_7) { if
(param_2 == 0 || param_4 == 0) { SetLastResult(kCache_ErrorBadParam); return false; } else
{ CacheIDWii* id = new CacheIDWii(); id->unk2 = param_2; id->unk3 = param_4; id->unk4 =
param_6; return true;
    }
}
*/

/*
bool CacheMgrWii::MountAsync(CacheID*, Cache*, Hmx::Object*) {}
*/
bool CacheMgrWii::UnmountAsync(Cache **, Hmx::Object *) {}
bool CacheMgrWii::DeleteAsync(CacheID *) {
    TheDebug.Fail(MakeString(kAssertStr, "CacheMgr_Wii.cpp", 0x128, "false"));
    return false;
}
void CacheMgrWii::PollSearch() {}
void CacheMgrWii::EndSearch(CacheResult) {}

void CacheMgrWii::PollMount() {}

void CacheMgrWii::PollUnmount() {}
