#pragma once
#include "network/Protocol/ClientProtocol.h"
#include "network/Protocol/ProtocolCallContext.h"
#include "net_band/RBBinaryDataDDL_Wii.h"

namespace Quazal {
    class NintendoManagementProtocolClient : public ClientProtocol {
    public:
        NintendoManagementProtocolClient() : ClientProtocol(1) {}
#if defined(VERSION_SZBE69_B8)
        virtual ~NintendoManagementProtocolClient() {}
#else
        virtual ~NintendoManagementProtocolClient();
#endif
        virtual void ExtractCallSpecificResults(Message *, ProtocolCallContext *);

        bool
        CallGetConsoleUsernames(ProtocolCallContext *, const unsigned long long &, qList<String> *);
    };
}