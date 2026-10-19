#pragma once

#include <QString>

namespace AppPaths {

QString dataDir();
QString configFilePath();
QString imageFilePath();
QString logFilePath();

bool ensureDataDir();

}  // namespace AppPaths
