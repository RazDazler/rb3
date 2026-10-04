#pragma once
#include "DOHandle.h"
#include "Platform/RefCountedObject.h"

namespace Quazal {
    class DuplicatedObject;
    class DOFilter : public RefCountedObject {
        DOFilter();
        virtual ~DOFilter();
#ifdef VERSION_SZBE69_B8
        // Original derived vtables place these two Filter overloads here.
        virtual bool Filter(DuplicatedObject *) = 0;
        virtual bool Filter(DOHandle) = 0;
#endif
        virtual DOHandle GetMinimumValidHandle(); // these both return a struct
        virtual DOHandle GetMaximumValidHandle();
    };
}
