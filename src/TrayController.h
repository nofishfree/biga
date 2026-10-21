#pragma once

#include <QObject>

class QAction;
class QMenu;
class QSystemTrayIcon;

class TrayController final : public QObject {
    Q_OBJECT

public:
    explicit TrayController(QObject* parent = nullptr);
    ~TrayController() override;

    void show();
    void setStatusOverlayChecked(bool checked);

signals:
    void chooseImageRequested();
    void restoreDotRequested();
    void statusOverlayToggled(bool enabled);
    void settingsRequested();
    void quitRequested();

private:
    QSystemTrayIcon* m_trayIcon = nullptr;
    QMenu* m_menu = nullptr;
    QAction* m_statusOverlayAction = nullptr;
};
