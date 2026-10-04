#include "Platform/OutputFormat.h"
#include "Platform/RefCountedObject.h"
#include <decomp.h>
#include <typeinfo>

#if defined(VERSION_SZBE69_B8)
DECOMP_FORCEACTIVE(CIDLogFilter, &typeid(Quazal::RootObject))
#else
DECOMP_FORCEBLOCK(CIDLogFilter, (void), {Quazal::OutputFormat* r = dynamic_cast<Quazal::OutputFormat*>(new Quazal::RefCountedObject);})
#endif