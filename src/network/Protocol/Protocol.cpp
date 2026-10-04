#include "Protocol/Protocol.h"
#include "Core/SystemComponent.h"
#include "Protocol.h"
#include "Plugins/Message.h"
#include "Protocol/ProtocolCallContext.h"

namespace Quazal {

    Protocol::Protocol(unsigned int ui)
        : SystemComponent(String("<unknown>")), unk18(0), unk24(ui) {
        unk20 = 0;
        unk28 = 0;
        unk1c = 0;
        unk2c = 0;
        unk30 = 0;
        unk34 = 0;
    }

    Protocol::~Protocol() {}

    void Protocol::AddProtocolKey(Message *message, unsigned char key) {
        message->Append(&key, 1, 1);
    }
    void Protocol::AddMethodID(Message *message, unsigned int id) {
        message->Append((const unsigned char *)&id, 4, 1);
    }
    unsigned int Protocol::ExtractMethodID(Message *message) {
        unsigned int id;
        message->Extract((unsigned char *)&id, 4, 1);
        return id;
    }
    unsigned int Protocol::ExtractCallContextID(Message *message) {
        unsigned int id;
        message->Extract((unsigned char *)&id, 4, 1);
        return id;
    }
    void Protocol::ExtractCallOutcome(Message *message, qResult *result) {
        unsigned char success;
        message->Extract(&success, 1, 1);
        if (!success) {
            int code;
            message->Extract((unsigned char *)&code, 4, 1);
            qResult transmitted(code);
            *result = transmitted;
        } else {
            *result = qResult(0x10001);
        }
    }
    bool Protocol::RegisterCallContext(Message *message, ProtocolCallContext *context) {
        if (!context->InitiateCall())
            return false;
        unsigned int id = context->unk2c;
        message->Append((const unsigned char *)&id, 4, 1);
        return true;
    }
    void Protocol::UseLocalLoopback(unsigned int pid, unsigned int cid) {
        unk2c = true;
        unk28 = 0;
        unk30 = pid;
        unk34 = cid;
    }

    void Protocol::SetProtocolID(unsigned char id) { unk18 = id; }
    void Protocol::AssociateProtocolRequestBroker(ProtocolRequestBroker *broker) {
        unk20 = broker;
    }
    void Protocol::TraceImpl(uint) const {}
    bool Protocol::FlagIsSet(uint mask) const { return (mask & unk24) == mask; }
    void Protocol::SetFlag(uint mask) { unk24 |= mask; }
    EndPoint *Protocol::GetOutgoingConnection() const { return unk1c; }
    void Protocol::SetOutgoingConnection(EndPoint *endpoint) { unk1c = endpoint; }
    int Protocol::GetCallerPID() const { return unk30; }
    int Protocol::GetCallerCID() const { return unk34; }
    void Protocol::EnforceDeclareSysComponentMacro() {}

}