#include "network/Extensions/ChannelInfo.h"

namespace Quazal {
    ChannelInfo::ChannelInfo() {}
#ifdef VERSION_SZBE69
    _DS_ChannelInfo::~_DS_ChannelInfo() {}
    _DS_ChannelInfo::_DS_ChannelInfo() {}
#endif
    ChannelInfo::~ChannelInfo() {}
}
