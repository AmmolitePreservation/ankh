#pragma once

#include <QtGlobal>

// Whether this process has Windows' PreferSystem32 image load policy, which every process it starts inherits and
// which makes a game take System32's copy of the DLL its plugin loader stands in for.
bool prefersSystem32Images();

// Asks the running Windows Explorer to start this program again; Explorer starts it without this process's policies.
void restartThroughExplorer();

// Lets a game Ankh just started take the foreground when its window shows, as a game started from Explorer may.
void letGameTakeForeground(qint64 processID);
