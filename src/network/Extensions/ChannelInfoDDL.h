#pragma once
#include "Platform/String.h"
#include "ObjDup/DataSet.h"
#include "Platform/RootObject.h"

namespace Quazal {
    class _DS_ChannelInfo : public DataSet {
    public:
#ifdef VERSION_SZBE69
        _DS_ChannelInfo();
        ~_DS_ChannelInfo();
#else
        _DS_ChannelInfo() {}
        ~_DS_ChannelInfo() {}

#endif
        String unk4;
        unsigned short unk8;
    };
}
