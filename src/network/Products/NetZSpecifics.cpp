#include "NetZSpecifics.h"

#ifdef VERSION_SZBE69_B8
// Only the static entrypoints observed in B8 are declared here; no instance layout is
// assumed.
namespace Quazal {
    class SessionClockExtDDLDeclarations {
    public:
        static void Register();
    };
    class DupSpaceExtDDLDeclarations {
    public:
        static void Register();
    };
    class GlobalDiscoveryExtension {
    public:
        static void Register();
    };
    class DupSpaceExtension {
    public:
        static void Register();
    };
    class SessionClockExtension {
    public:
        static void Register();
    };
    class DuplicationSpace {
    public:
        static bool s_bUseSessionSpace;
    };
}
#endif

namespace Quazal {
    NetZSpecifics::NetZSpecifics() {}

    NetZSpecifics::~NetZSpecifics() {}

    u32 NetZSpecifics::GetProductID() { return 1; }

    void NetZSpecifics::RegisterSpecificDDLs() {
#ifdef VERSION_SZBE69_B8
        SessionClockExtDDLDeclarations::Register();
        DupSpaceExtDDLDeclarations::Register();
#endif
    }

    void NetZSpecifics::RegisterSpecificComponents() {
#ifdef VERSION_SZBE69_B8
        GlobalDiscoveryExtension::Register();
        DuplicationSpace::s_bUseSessionSpace = false;
        DupSpaceExtension::Register();
        SessionClockExtension::Register();
#endif
    }
}
