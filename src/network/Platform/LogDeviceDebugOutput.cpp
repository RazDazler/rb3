#include "LogDeviceDebugOutput.h"
#include <stdio.h>

namespace Quazal {
    void LogDeviceDebugOutput::Output(const LogEntry &le) {
        printf("%s\n", le.m_oText.m_szContent);
    }
}