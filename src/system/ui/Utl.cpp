#include "ui/Utl.h"

bool IsNavAction(JoypadAction act) { return act == kAction_Up || act == kAction_Down; }

int PageDirection(JoypadAction act) {
    if (act == kAction_PageDown)
        return 1;
    return act == kAction_PageUp ? -1 : 0;
}