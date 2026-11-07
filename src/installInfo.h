#pragma once

#include <QString>

enum class InstallScope { allUsers, thisUser, buildFolder };

InstallScope installScope();
QString installFolder();
