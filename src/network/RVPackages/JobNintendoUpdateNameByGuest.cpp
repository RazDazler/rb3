#include "NintendoManagementProtocolClient.h"

#if defined(VERSION_SZBE69_B8)
#include "decomp.h"
using Quazal::NintendoManagementProtocolClient;
DECOMP_FORCEDTOR(NintendoDtor, NintendoManagementProtocolClient)
#else
namespace Quazal {
    NintendoManagementProtocolClient::~NintendoManagementProtocolClient() {}
}
#endif
