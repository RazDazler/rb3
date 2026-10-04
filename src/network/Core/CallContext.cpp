#include "Core/CallContext.h"
#include "Platform/Result.h"
#include "Scheduler.h"
#include "Platform/Callback.h"
#include "Platform/UserContext.h"

namespace Quazal {

    CallContext::CallContext() : unkc((_State)0) {
        unk2c = 0;
        unk30 = 0;
        unk38 = 0;
        unk20 = qResult(0x80010001);
        unk8 = 0;
        unk40 = 0;
    }

    CallContext::~CallContext() { Scheduler::GetInstance(); }

    void InvokeCallbackOnSuccess(CallContext *call, const UserContext *context) {
        CallbackRoot *callback = (CallbackRoot *)context->GetPointer();
        if (call->GetState() == 2)
            callback->Call();
        delete callback;
    }
    void InvokeCallbackOnCompletion(CallContext *, const UserContext *context) {
        CallbackRoot *callback = (CallbackRoot *)context->GetPointer();
        callback->Call();
        delete callback;
    }

    bool CallContext::FlagsAreValid() const {
        if ((unk8 & 8) == 8 && (unk8 & 4) != 4)
            return false;
        return true;
    }
    bool CallContext::TransitionIsValid(_State from, _State to) {
        if (from == 0 && to == 1)
            return true;
        if (from == 1 && to == 2)
            return true;
        if (from == 1 && to == 3)
            return true;
        if (from == 1 && to == 4)
            return true;
        if (from == 2 && to == 0)
            return true;
        if (from == 3 && to == 0)
            return true;
        if (from == 4 && to == 0)
            return true;
        return false;
    }
    void CallContext::SignalSuccess(qResult result) {
        SetStateImpl((_State)2, result, true);
    }
    void CallContext::SignalFailure(qResult result) {
        SetStateImpl((_State)3, result, true);
    }

    void CallContext::SetDependentConnection(void *connection, unsigned int id) {
        unk30 = connection;
        unk34 = id;
    }
    void CallContext::BeginTransition(_State, qResult, bool) {}
    void CallContext::ProcessCallCompletion() {}
    void CallContext::RegisterCancellationCallback(CallbackRoot *callback) {
        unk38 = callback;
    }
    void CallContext::SetFlag(unsigned int mask) { unk8 |= mask; }
    void CallContext::ClearFlag(unsigned int mask) { unk8 &= ~mask; }
    bool CallContext::FlagIsSet(unsigned int mask) const { return (mask & unk8) == mask; }
    void CallContext::Trace(unsigned int) {}

}