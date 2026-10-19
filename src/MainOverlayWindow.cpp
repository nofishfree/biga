#include "MainOverlayWindow.h"

#include <QGuiApplication>
#include <QPainter>
#include <QPaintEvent>
#include <QScreen>
#include <QShowEvent>

#include <cmath>

#include <windows.h>

MainOverlayWindow::MainOverlayWindow(QWidget* parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint
                   | Qt::Tool | Qt::WindowTransparentForInput
                   | Qt::WindowDoesNotAcceptFocus);

    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_ShowWithoutActivating, true);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setFocusPolicy(Qt::NoFocus);

    m_content = Content::Dot;
    setContentSize(QSize(m_dotSize, m_dotSize));
}

void MainOverlayWindow::showDot(int dotSize)
{
    m_dotSize = qMax(0, dotSize);
    m_content = Content::Dot;
    m_image = QImage();
    setContentSize(QSize(m_dotSize, m_dotSize));
    update();
    presentWithoutActivation();
}

void MainOverlayWindow::showImage(const QImage& image)
{
    if (image.isNull() || image.width() <= 0 || image.height() <= 0) {
        return;
    }

    m_content = Content::Image;
    m_image = image;
    m_image.setDevicePixelRatio(1.0);
    setContentSize(m_image.size());
    update();
    presentWithoutActivation();
}

void MainOverlayWindow::applyDotSize(int dotSize)
{
    m_dotSize = qMax(0, dotSize);
    if (m_content == Content::Dot) {
        setContentSize(QSize(m_dotSize, m_dotSize));
        update();
    }
}

void MainOverlayWindow::hideForScope()
{
    hide();
}

void MainOverlayWindow::showCurrent()
{
    if (m_content == Content::Image && m_image.isNull()) {
        m_content = Content::Dot;
        m_image = QImage();
        setContentSize(QSize(m_dotSize, m_dotSize));
    }
    presentWithoutActivation();
}

bool MainOverlayWindow::isShowingImage() const
{
    return m_content == Content::Image && !m_image.isNull();
}

void MainOverlayWindow::setContentSize(const QSize& size)
{
    const QSize safeSize(qMax(0, size.width()), qMax(0, size.height()));
    setFixedSize(safeSize);
    centerOnPrimaryScreen();
}

void MainOverlayWindow::centerOnPrimaryScreen()
{
    QScreen* screen = QGuiApplication::primaryScreen();
    if (screen == nullptr) {
        return;
    }

    const QRect screenGeometry = screen->geometry();
    const int x = screenGeometry.x()
                  + static_cast<int>(std::floor(
                        (screenGeometry.width() - width()) / 2.0));
    const int y = screenGeometry.y()
                  + static_cast<int>(std::floor(
                        (screenGeometry.height() - height()) / 2.0));
    move(x, y);
}

void MainOverlayWindow::applyNativeWindowStyles()
{
    const HWND hwnd = reinterpret_cast<HWND>(winId());
    if (hwnd == nullptr) {
        return;
    }

    LONG_PTR extendedStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    extendedStyle |= WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE
                     | WS_EX_TOOLWINDOW;
    extendedStyle &= ~WS_EX_APPWINDOW;
    SetWindowLongPtrW(hwnd, GWL_EXSTYLE, extendedStyle);

    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE
                     | SWP_FRAMECHANGED);
}

void MainOverlayWindow::presentWithoutActivation()
{
    if (!isVisible()) {
        show();
    }

    applyNativeWindowStyles();

    const HWND hwnd = reinterpret_cast<HWND>(winId());
    if (hwnd != nullptr) {
        ShowWindow(hwnd, SW_SHOWNOACTIVATE);
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE
                         | SWP_SHOWWINDOW);
    }
}

void MainOverlayWindow::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    if (m_content == Content::Image && !m_image.isNull()) {
        painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
        painter.drawImage(QRect(QPoint(0, 0), size()), m_image, m_image.rect());
        return;
    }

    if (width() <= 0 || height() <= 0) {
        return;
    }

    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::white);
    painter.drawEllipse(QRectF(rect()));
}

void MainOverlayWindow::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    applyNativeWindowStyles();
}

bool MainOverlayWindow::nativeEvent(const QByteArray& eventType, void* message,
                                    qintptr* result)
{
    if (eventType == "windows_generic_MSG") {
        MSG* nativeMessage = static_cast<MSG*>(message);
        if (nativeMessage != nullptr && result != nullptr) {
            if (nativeMessage->message == WM_MOUSEACTIVATE) {
                *result = MA_NOACTIVATE;
                return true;
            }
            if (nativeMessage->message == WM_NCHITTEST) {
                *result = HTTRANSPARENT;
                return true;
            }
        }
    }
    return QWidget::nativeEvent(eventType, message, result);
}
