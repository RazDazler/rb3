#include "utl/NetLoader_Wii.h"
#include "utl/HttpWii.h"
#include "utl/NetCacheMgr.h"
#include "decomp.h"

// Preserve the original pooled literals, including the original state names.
DECOMP_FORCEACTIVE(
    NetLoaderWii,
    "https://%s:%u%s",
    "%s%s",
    "kSendRequest",
    "kGetContentLength",
    "kDispatchRequest",
    "kReceiveRequest",
    "kDone",
    "kFailure",
    "kFailureComplete",
    "unknown"
)

template const char *MakeString<
    const char *,
    unsigned short,
    const char *>(const char *, const char *, unsigned short, const char *);

NetLoaderWii::NetLoaderWii(const String &path)
    : NetLoader(path), mHttpHandle(-1), mHttpBuffer(nullptr), mUrl() {
    mUrl = MakeString(
        "%s%s",
        MakeString(
            "https://%s:%u%s",
            TheNetCacheMgr->GetServer(),
            TheNetCacheMgr->GetPort(),
            TheNetCacheMgr->GetServerRoot()
        ),
        path
    );
    SetState(kDispatchRequest);
}

NetLoaderWii::~NetLoaderWii() {
    if (mHttpHandle >= 0) {
        TheHttpWii.CancelAsync(mHttpHandle);
    }
    if (mHttpBuffer) {
        _MemFree(mHttpBuffer);
        mHttpBuffer = nullptr;
    }
}

bool NetLoaderWii::SendRequest() {
    mLength = 0;
    mHttpHandle = TheHttpWii.GetFileAsync(mUrl.c_str(), nullptr, 0);
    return mHttpHandle >= 0;
}

bool NetLoaderWii::GetContentLength() {
    unsigned long size = 0;
    int result = TheHttpWii.CompleteAsync(mHttpHandle, size);
    if (result < 0) {
        mHttpHandle = -1;
        return false;
    }
    if (result == 100) {
        mLength = size;
    }
    return true;
}

bool NetLoaderWii::DispatchDownload() {
    mReceived = 0;
    mHttpHandle = TheHttpWii.GetFileAsync(mUrl.c_str(), nullptr, 0);
    return mHttpHandle >= 0;
}

bool NetLoaderWii::ReceiveResponse() {
    unsigned long size = 0;
    int result = TheHttpWii.CompleteAsync(mHttpHandle, size);
    if (result >= 0 && result < 100) {
        return true;
    }
    if (result == 100) {
        mReceived = mLength = TheHttpWii.mDataBufferSize;
        mHttpHandle = -1;
        return true;
    }
    mHttpHandle = -1;
    return false;
}

void NetLoaderWii::FinishTransaction() {
    if (!IsLoaded()) {
        AttachBuffer(mHttpBuffer);
        mHttpBuffer = nullptr;
        SetSize(mLength);
        PostDownload();
    }
}

void NetLoaderWii::SetState(State state) { mState = state; }

void NetLoaderWii::PollLoading() {
    Timer::Sleep(5);
    switch (mState) {
    case kSendRequest:
        if (SendRequest())
            SetState(kGetContentLength);
        else
            SetState(kFailure);
        break;
    case kGetContentLength:
        if (GetContentLength()) {
            if (mLength != 0)
                SetState(kDispatchRequest);
        } else
            SetState(kFailure);
        break;
    case kDispatchRequest:
        if (DispatchDownload())
            SetState(kReceiveRequest);
        else {
            if (mHttpBuffer) {
                _MemFree(mHttpBuffer);
                mHttpBuffer = nullptr;
            }
            SetState(kFailure);
        }
        break;
    case kReceiveRequest:
        if (ReceiveResponse()) {
            if (mReceived == mLength && mReceived != 0) {
                mHttpBuffer = (char *)TheHttpWii.mDataBuffer;
                SetState(kDone);
            }
        } else {
            if (mHttpBuffer) {
                _MemFree(mHttpBuffer);
                mHttpBuffer = nullptr;
            }
            SetState(kFailure);
        }
        break;
    case kDone:
        FinishTransaction();
        break;
    default:
        break;
    }
}

bool NetLoaderWii::HasFailed() {
    return mState == kFailure || mState == kFailureComplete;
}

bool NetLoaderWii::IsSafeToDelete() const { return true; }
