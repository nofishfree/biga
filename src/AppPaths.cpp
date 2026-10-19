#include "AppPaths.h"

#include "Constants.h"

#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QStandardPaths>

namespace AppPaths {
namespace {

QString appDataRoot()
{
    const QString appData = QProcessEnvironment::systemEnvironment().value(
        QStringLiteral("APPDATA"));
    if (!appData.isEmpty()) {
        return appData;
    }

    // Fallback only. The normal Windows location is %APPDATA%.
    const QString fallback = QStandardPaths::writableLocation(
        QStandardPaths::AppDataLocation);
    const QFileInfo info(fallback);
    if (info.fileName().compare(QString::fromLatin1(Constants::kAppName),
                                Qt::CaseInsensitive)
        == 0) {
        return info.absolutePath();
    }
    return fallback;
}

}  // namespace

QString dataDir()
{
    return QDir(appDataRoot()).filePath(QString::fromLatin1(Constants::kAppName));
}

QString configFilePath()
{
    return QDir(dataDir()).filePath(QStringLiteral("user-config.json"));
}

QString imageFilePath()
{
    return QDir(dataDir()).filePath(QStringLiteral("current-image.img"));
}

QString logFilePath()
{
    return QDir(dataDir()).filePath(QStringLiteral("app.log"));
}

bool ensureDataDir()
{
    return QDir().mkpath(dataDir());
}

}  // namespace AppPaths
