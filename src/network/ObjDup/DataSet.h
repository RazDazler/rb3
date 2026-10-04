#pragma once
#include "Platform/RootObject.h"
#include "Platform/Time.h"

namespace Quazal {
    class DataSet : public RootObject {
    public:
        DataSet();
        ~DataSet();
        bool Refresh(const Time &);
        // Both B8 and retail derived datasets begin their own fields at offset 4.
        unsigned int unk0;
    };
}
