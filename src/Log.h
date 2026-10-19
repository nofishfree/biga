#pragma once

#include <QString>

namespace Log {

void info(const QString& message);
void warning(const QString& message);
void error(const QString& message);

}  // namespace Log
