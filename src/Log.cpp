#include "Log.h"

#include "AppPaths.h"

#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <QStringConverter>
#include <QTextStream>

namespace Log {
namespace {

QMutex g_logMutex;

void writeLine(const QLatin1StringView level, const QString& message,
               QtMsgType messageType)
{
    const QString line = QStringLiteral("[%1] [%2] %3\n")
                             .arg(QDateTime::currentDateTime().toString(
                                      QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")),
                                  QString(level), message);

    {
        QMutexLocker locker(&g_logMutex);
        AppPaths::ensureDataDir();
        QFile file(AppPaths::logFilePath());
        if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            QTextStream stream(&file);
            stream.setEncoding(QStringConverter::Utf8);
            stream << line;
            stream.flush();
            file.flush();
        }
    }

    // Also send the message to the debugger / stderr when one is attached.
    switch (messageType) {
    case QtInfoMsg:
        qInfo().noquote() << line.trimmed();
        break;
    case QtCriticalMsg:
    case QtFatalMsg:
        qCritical().noquote() << line.trimmed();
        break;
    case QtDebugMsg:
    case QtWarningMsg:
    default:
        qWarning().noquote() << line.trimmed();
        break;
    }
}

}  // namespace

void info(const QString& message)
{
    writeLine(QLatin1StringView("INFO"), message, QtInfoMsg);
}

void warning(const QString& message)
{
    writeLine(QLatin1StringView("WARN"), message, QtWarningMsg);
}

void error(const QString& message)
{
    writeLine(QLatin1StringView("ERROR"), message, QtCriticalMsg);
}

}  // namespace Log
