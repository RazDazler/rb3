#pragma once
#include "network/Core/SystemComponent.h"

namespace Quazal {

    class EndPoint;
    class Message;
    class ProtocolCallContext;
    class ProtocolRequestBroker;

    class Protocol : public SystemComponent {
    public:
        Protocol(unsigned int);
        virtual ~Protocol(); // 0x8
        virtual const char *GetType() const; // 0x14
        virtual bool IsAKindOf(const char *str) const; // 0x18
        virtual void EnforceDeclareSysComponentMacro(); // 0x1C
        virtual void TraceImpl(uint) const; // 0x20
        virtual bool BeginInitialization(); // 0x30
        virtual void DoWork(); // 0x4C
        virtual int GetProtocolType() const = 0; // fix ret type
        virtual void EndPointDisconnected(EndPoint *);
        virtual void FaultDetected(EndPoint *, unsigned int);
        virtual int Clone() const;

        static void AddProtocolKey(Message *, unsigned char);
        static void AddMethodID(Message *, unsigned int);
        static unsigned int ExtractMethodID(Message *);
        static unsigned int ExtractCallContextID(Message *);
        static void ExtractCallOutcome(Message *, qResult *);
        static bool RegisterCallContext(Message *, ProtocolCallContext *);
        void UseLocalLoopback(unsigned int, unsigned int);
        void SetProtocolID(unsigned char);
        void AssociateProtocolRequestBroker(ProtocolRequestBroker *);
        bool FlagIsSet(uint) const;
        void SetFlag(uint);
        EndPoint *GetOutgoingConnection() const;
        void SetOutgoingConnection(EndPoint *);
        int GetCallerPID() const;
        int GetCallerCID() const;

        unsigned char unk18;
        EndPoint *unk1c;
        ProtocolRequestBroker *unk20;
        unsigned int unk24;
        int unk28;
        bool unk2c;
        int unk30;
        int unk34;
    };
}
