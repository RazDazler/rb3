#pragma once
#include "utl/NetLoader.h"

class NetLoaderWii : public NetLoader {
public:
    enum State {
        kSendRequest,
        kGetContentLength,
        kDispatchRequest,
        kReceiveRequest,
        kDone,
        kFailure,
        kFailureComplete,
        kMax,
        kNil = -1
    };

    NetLoaderWii(const String &);
    virtual ~NetLoaderWii();
    virtual void PollLoading();
    virtual bool HasFailed();
    virtual bool IsSafeToDelete() const;

    bool SendRequest();
    bool GetContentLength();
    bool DispatchDownload();
    bool ReceiveResponse();
    void FinishTransaction();
    void SetState(State);

    int mHttpHandle; // 0x24
    State mState; // 0x28
    unsigned long long mLength; // 0x30
    unsigned long long mReceived; // 0x38
    char *mHttpBuffer; // 0x40
    String mUrl; // 0x44
};
