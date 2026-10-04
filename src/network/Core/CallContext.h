#pragma once
#include "Platform/RefCountedObject.h"
#include "Platform/Result.h"
#include "Platform/Time.h"
#include "Platform/qStd.h"

namespace Quazal {
    class CallbackRoot;
    class UserContext;
    class CallContext;
    void InvokeCallbackOnSuccess(CallContext *, const UserContext *);
    void InvokeCallbackOnCompletion(CallContext *, const UserContext *);

    class CallContext : public RefCountedObject {
    public:
        enum _State {
        };
        CallContext();
        virtual ~CallContext();
        virtual bool FlagsAreValid() const;
        virtual void BeginTransition(_State, qResult, bool);
        virtual void ProcessCallCompletion();

        void RegisterCompletionCallback(CallbackRoot *, bool, bool);
        void SetFlag(unsigned int);
        bool InitiateCall();
        void Reset();
        static bool TransitionIsValid(_State, _State);
        void SignalSuccess(qResult);
        void SignalFailure(qResult);
        void SetStateImpl(_State, qResult, bool);
        void SetDependentConnection(void *, unsigned int);
        void RegisterCancellationCallback(CallbackRoot *);
        void ClearFlag(unsigned int);
        bool FlagIsSet(unsigned int) const;
        void Trace(unsigned int);

        _State GetState() const { return unkc; }

        unsigned int unk8; // 0x8
        _State unkc; // 0xc
        qList<int> unk10; // 0x10
        qVector<int> unk18; // 0x18
        qResult unk20; // 0x20
        int unk2c; // 0x2c
        void *unk30;
        unsigned int unk34;
        CallbackRoot *unk38;
        int unk3c;
        Time unk40; // 0x40
    };
}
