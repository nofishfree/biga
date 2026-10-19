#pragma once

#include <QObject>

class QMenu;
class QSystemTrayIcon;

class TrayController final : public QObject {
    Q_OBJECT

public:
    explicit TrayController(QObject* parent = nullptr);
    ~TrayController() override;

    void show();

signals:
    void chooseImageRequested();
    void restoreDotRequested();
    void settingsRequested();
    void quitRequested();

private:
    QSystemTrayIcon* m_trayIcon = nullptr;
    QMenu* m_menu = nullptr;
};
