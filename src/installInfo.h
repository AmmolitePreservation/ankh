#pragma once

#include <QString>

// Where the Ankh installer put the running copy, read from the installer's own record; notInstalled for a copy it
// did not install, such as a build folder.
enum class InstallScope { allUsers, thisUser, notInstalled };

InstallScope installScope();
QString installFolder();
