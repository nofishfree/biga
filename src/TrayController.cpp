#include "TrayController.h"

#include "Log.h"

#include <QAction>
#include <QApplication>
#include <QDateTime>
#include <QGuiApplication>
#include <QIcon>
#include <QMenu>
#include <QSignalBlocker>
#include <QStyleHints>
#include <QSystemTrayIcon>
#include <QTimer>

TrayController::TrayController(QObject* parent)
    : QObject(parent)
{
    m_menu = new QMenu();
    m_menu->setObjectName(QStringLiteral("trayMenu"));

    QAction* chooseImage =
        m_menu->addAction(QStringLiteral("选择图片"));
    QAction* restoreDot =
        m_menu->addAction(QStringLiteral("还原白点"));
    m_statusOverlayAction =
        m_menu->addAction(QStringLiteral("状态显示"));
    m_statusOverlayAction->setCheckable(true);
    m_statusOverlayAction->setChecked(false);
    QAction* settings = m_menu->addAction(QStringLiteral("设置"));
    QAction* quit = m_menu->addAction(QStringLiteral("退出"));

    connect(chooseImage, &QAction::triggered, this,
            &TrayController::chooseImageRequested);
    connect(restoreDot, &QAction::triggered, this,
            &TrayController::restoreDotRequested);
    connect(m_statusOverlayAction, &QAction::triggered, this,
            &TrayController::statusOverlayToggled);
    connect(settings, &QAction::triggered, this,
            &TrayController::settingsRequested);
    connect(quit, &QAction::triggered, this, &TrayController::quitRequested);

    // PNG is the primary tray format because it is built into Qt and does not
    // depend on the optional ICO image plugin. The ICO is still used for the
    // executable and as a fallback.
    QIcon icon(QStringLiteral(":/icons/app.png"));
    if (icon.isNull()) {
        icon = QIcon(QStringLiteral(":/icons/app.ico"));
    }
    if (icon.isNull()) {
        icon = QApplication::windowIcon();
    }

    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        Log::warning(QStringLiteral("系统通知区域不可用，托盘图标无法创建"));
    } else if (icon.isNull()) {
        Log::warning(QStringLiteral("托盘图标资源加载失败"));
    }

    m_trayIcon = new QSystemTrayIcon(icon, this);
    m_trayIcon->setToolTip(
        QStringLiteral("BigA\n单击显示/隐藏状态，双击打开设置"));
    m_trayIcon->setContextMenu(m_menu);

    // Delay a single click until the system double-click interval has passed.
    // A double click cancels it, so opening settings never also toggles status.
    m_singleClickTimer = new QTimer(this);
    m_singleClickTimer->setSingleShot(true);
    const int doubleClickInterval =
        QGuiApplication::styleHints() != nullptr
            ? QGuiApplication::styleHints()->mouseDoubleClickInterval()
            : 500;
    m_singleClickTimer->setInterval(doubleClickInterval + 75);
    connect(m_singleClickTimer, &QTimer::timeout, this,
            &TrayController::toggleStatusOverlayRequested);

    connect(m_trayIcon, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason reason) {
                if (reason == QSystemTrayIcon::Trigger) {
                    // Some Windows/Qt combinations deliver a Trigger after the
                    // DoubleClick event. Ignore those trailing clicks.
                    if (QDateTime::currentMSecsSinceEpoch()
                        < m_ignoreSingleClicksUntil) {
                        return;
                    }
                    m_singleClickTimer->start();
                } else if (reason == QSystemTrayIcon::DoubleClick) {
                    m_singleClickTimer->stop();
                    m_ignoreSingleClicksUntil =
                        QDateTime::currentMSecsSinceEpoch()
                        + m_singleClickTimer->interval();
                    emit settingsRequested();
                }
            });
}

TrayController::~TrayController()
{
    if (m_trayIcon != nullptr) {
        m_trayIcon->hide();
        m_trayIcon->setContextMenu(nullptr);
    }
    delete m_menu;
    m_menu = nullptr;
}

void TrayController::show()
{
    if (m_trayIcon == nullptr) {
        return;
    }
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        Log::warning(QStringLiteral("系统通知区域不可用，跳过托盘显示"));
        return;
    }

    m_trayIcon->show();
    QTimer::singleShot(1200, this, [this] {
        if (m_trayIcon == nullptr || !m_trayIcon->isVisible()) {
            return;
        }
        if (m_trayIcon->geometry().isEmpty()) {
            Log::warning(QStringLiteral(
                "托盘图标未注册到通知区域。若程序从受限工作区启动，"
                "Windows 可能以低完整性运行它；请从普通桌面位置启动。"));
            return;
        }
        Log::info(QStringLiteral("托盘图标已注册: %1,%2 %3x%4")
                      .arg(m_trayIcon->geometry().x())
                      .arg(m_trayIcon->geometry().y())
                      .arg(m_trayIcon->geometry().width())
                      .arg(m_trayIcon->geometry().height()));
    });
}

void TrayController::setStatusOverlayChecked(bool checked)
{
    if (m_statusOverlayAction == nullptr) {
        return;
    }
    const QSignalBlocker blocker(m_statusOverlayAction);
    m_statusOverlayAction->setChecked(checked);
}
