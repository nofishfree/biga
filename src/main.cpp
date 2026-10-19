#include "App.h"
#include "Constants.h"

#include <QApplication>
#include <QCoreApplication>
#include <QIcon>
#include <QString>

#include <windows.h>

int main(int argc, char* argv[])
{
    // CreateMutexW does not necessarily clear LastError when it creates a new
    // object, so set it explicitly before checking for an existing instance.
    SetLastError(ERROR_SUCCESS);
    const HANDLE singleInstanceMutex =
        CreateMutexW(nullptr, FALSE, Constants::kSingleInstanceMutexName);
    const DWORD mutexError = GetLastError();

    bool alreadyRunning =
        singleInstanceMutex != nullptr && mutexError == ERROR_ALREADY_EXISTS;
    if (!alreadyRunning && singleInstanceMutex == nullptr) {
        // If the existing mutex cannot be opened directly because of its
        // security descriptor, OpenMutex at least tells us whether it exists.
        const HANDLE existingMutex =
            OpenMutexW(SYNCHRONIZE, FALSE, Constants::kSingleInstanceMutexName);
        if (existingMutex != nullptr) {
            CloseHandle(existingMutex);
            alreadyRunning = true;
        }
    }

    if (alreadyRunning) {
        if (singleInstanceMutex != nullptr) {
            CloseHandle(singleInstanceMutex);
        }
        return 0;
    }

    QApplication application(argc, argv);
    QApplication::setQuitOnLastWindowClosed(false);
    QCoreApplication::setApplicationName(
        QString::fromLatin1(Constants::kAppName));
    QCoreApplication::setApplicationVersion(
        QString::fromLatin1(Constants::kAppVersion));
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/icons/app.png")));

    App app;
    if (!app.initialize()) {
        if (singleInstanceMutex != nullptr) {
            CloseHandle(singleInstanceMutex);
        }
        return 1;
    }

    const int result = application.exec();

    if (singleInstanceMutex != nullptr) {
        CloseHandle(singleInstanceMutex);
    }
    return result;
}
