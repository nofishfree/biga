#include "StatusOverlayWindow.h"

#include <QGuiApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QScreen>
#include <QShowEvent>

#include <algorithm>
#include <cmath>

#include <windows.h>

namespace {

// Visual style constants. All sizes are for a 1.0 scale and are multiplied
// uniformly by StatusOverlayWindow::m_scale.
constexpr int kMainFontPixels = 40;
constexpr int kSubFontPixels = 16;
constexpr qreal kContentPadding = 2.0;
constexpr double kSubscriptOverlap = 0.10;
constexpr double kMinimumScale = 0.5;
constexpr double kMaximumScale = 3.0;

const QColor kTextFill(255, 255, 255);

struct ContentLayout {
    QFont mainFont;
    QFont subFont;
    QString mainText;
    QString superscriptText;
    QString subscriptText;
    QPointF mainBaseline;
    QPointF superscriptBaseline;
    QPointF subscriptBaseline;
    qreal superscriptX = 0.0;
    qreal subscriptX = 0.0;
    qreal width = 0.0;
    qreal height = 0.0;
    bool hasSubscripts = false;
};

QFont overlayFont(int pixelSize)
{
    QFont font(QStringLiteral("Segoe UI"));
    font.setStyleHint(QFont::SansSerif);
    font.setWeight(QFont::DemiBold);
    font.setPixelSize(qMax(1, pixelSize));
    return font;
}

ContentLayout makeContentLayout(int currentWeapon, int ammoState, int capacity,
                                double scale)
{
    ContentLayout layout;
    const int mainPixelSize =
        qMax(1, qRound(static_cast<double>(kMainFontPixels) * scale));
    const int subPixelSize =
        qMax(1, qRound(static_cast<double>(kSubFontPixels) * scale));

    layout.mainFont = overlayFont(mainPixelSize);
    layout.subFont = overlayFont(subPixelSize);

    const qreal margin = std::max(1.0, kContentPadding * scale);
    const QFontMetricsF mainMetrics(layout.mainFont);
    const QFontMetricsF subMetrics(layout.subFont);

    const int weaponNumber =
        std::clamp(currentWeapon, 0, 3) + 1;
    layout.mainText = QString::number(weaponNumber);
    layout.hasSubscripts = currentWeapon != 2;

    const qreal mainAdvance =
        mainMetrics.horizontalAdvance(layout.mainText);
    const qreal mainHeight = mainMetrics.height();
    const qreal mainTop = margin;
    const qreal mainLeft = margin;

    layout.mainBaseline =
        QPointF(mainLeft, mainTop + mainMetrics.ascent());
    layout.width = mainLeft + mainAdvance + margin;
    layout.height = mainTop + mainHeight + margin;

    if (layout.hasSubscripts) {
        layout.superscriptText = QString::number(std::clamp(ammoState, 0, 99));
        layout.subscriptText = QString::number(std::clamp(capacity, 0, 99));

        const qreal sideX =
            mainLeft + mainAdvance * (1.0 - kSubscriptOverlap);
        layout.superscriptX = sideX;
        layout.subscriptX = sideX;

        layout.superscriptBaseline =
            QPointF(sideX, mainTop + subMetrics.ascent());
        layout.subscriptBaseline =
            QPointF(sideX, mainTop + mainHeight - subMetrics.descent());

        const qreal superscriptAdvance =
            subMetrics.horizontalAdvance(layout.superscriptText);
        const qreal subscriptAdvance =
            subMetrics.horizontalAdvance(layout.subscriptText);
        const qreal contentRight =
            std::max(mainLeft + mainAdvance,
                     sideX + std::max(superscriptAdvance, subscriptAdvance));
        layout.width = contentRight + margin;
    }

    return layout;
}

}  // namespace

StatusOverlayWindow::StatusOverlayWindow(QWidget* parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint
                   | Qt::Tool | Qt::WindowDoesNotAcceptFocus);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_ShowWithoutActivating, true);
    setFocusPolicy(Qt::NoFocus);
    setCursor(Qt::SizeAllCursor);

    updateContentSize();
    placeAtDefaultPosition();
}

void StatusOverlayWindow::setScale(double scale)
{
    const double normalized =
        std::round(std::clamp(scale, kMinimumScale, kMaximumScale) * 10.0)
        / 10.0;
    if (qFuzzyCompare(m_scale, normalized)) {
        return;
    }

    m_scale = normalized;
    updateContentSize();
    ensureVisibleOnScreen();
}

double StatusOverlayWindow::scale() const
{
    return m_scale;
}

void StatusOverlayWindow::setRuntimeStatus(int currentWeapon, int ammoState,
                                           int capacity)
{
    const int normalizedWeapon = std::clamp(currentWeapon, 0, 3);
    const int normalizedAmmo = std::clamp(ammoState, 0, 9);
    const int normalizedCapacity = std::clamp(capacity, 0, 9);

    if (normalizedWeapon == m_currentWeapon
        && normalizedAmmo == m_ammoState
        && normalizedCapacity == m_capacity) {
        return;
    }

    m_currentWeapon = normalizedWeapon;
    m_ammoState = normalizedAmmo;
    m_capacity = normalizedCapacity;
    updateContentSize();
    update();
}

void StatusOverlayWindow::restorePosition(const QPoint& savedTopLeft)
{
    const auto screens = QGuiApplication::screens();
    const bool isVisible = std::any_of(
        screens.cbegin(), screens.cend(),
        [&savedTopLeft](const QScreen* screen) {
            return screen != nullptr
                   && screen->availableGeometry().contains(savedTopLeft);
        });

    if (!isVisible) {
        placeAtDefaultPosition();
        return;
    }

    move(savedTopLeft);
    ensureVisibleOnScreen();
}

void StatusOverlayWindow::placeAtDefaultPosition()
{
    QScreen* primary = QGuiApplication::primaryScreen();
    if (primary == nullptr) {
        move(0, 0);
        return;
    }

    const QRect available = primary->availableGeometry();
    const QPoint bottomRight = available.bottomRight();
    move(bottomRight.x() - width() + 1, bottomRight.y() - height() + 1);
    ensureVisibleOnScreen();
}

void StatusOverlayWindow::ensureVisibleOnScreen()
{
    QScreen* screen = QGuiApplication::screenAt(pos());
    if (screen == nullptr) {
        const auto screens = QGuiApplication::screens();
        const auto it = std::find_if(
            screens.cbegin(), screens.cend(),
            [this](const QScreen* candidate) {
                return candidate != nullptr
                       && candidate->availableGeometry().contains(pos());
            });
        if (it != screens.cend()) {
            screen = *it;
        }
    }
    if (screen == nullptr) {
        screen = QGuiApplication::primaryScreen();
    }
    if (screen == nullptr) {
        return;
    }

    const QRect available = screen->availableGeometry();
    const int maximumX = available.right() - width() + 1;
    const int maximumY = available.bottom() - height() + 1;
    const int x = std::clamp(pos().x(), available.left(),
                             std::max(available.left(), maximumX));
    const int y = std::clamp(pos().y(), available.top(),
                             std::max(available.top(), maximumY));
    move(x, y);
}

void StatusOverlayWindow::showNoActivate()
{
    show();
    applyNativeWindowStyles();

    const HWND hwnd = reinterpret_cast<HWND>(winId());
    if (hwnd != nullptr) {
        ShowWindow(hwnd, SW_SHOWNOACTIVATE);
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE
                         | SWP_SHOWWINDOW);
    }
}

void StatusOverlayWindow::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    const ContentLayout layout =
        makeContentLayout(m_currentWeapon, m_ammoState, m_capacity, m_scale);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(kTextFill);

    auto drawText = [&painter](const QPointF& baseline, const QString& text,
                               const QFont& font) {
        QPainterPath path;
        path.addText(baseline, font, text);
        painter.fillPath(path, kTextFill);
    };

    drawText(layout.mainBaseline, layout.mainText, layout.mainFont);
    if (layout.hasSubscripts) {
        drawText(layout.superscriptBaseline, layout.superscriptText,
                 layout.subFont);
        drawText(layout.subscriptBaseline, layout.subscriptText,
                 layout.subFont);
    }
}

void StatusOverlayWindow::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    applyNativeWindowStyles();
}

void StatusOverlayWindow::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragOffset = event->globalPosition().toPoint()
                       - frameGeometry().topLeft();
        setCursor(Qt::ClosedHandCursor);
        applyNativeWindowStyles();
        event->accept();
        return;
    }
    event->accept();
}

void StatusOverlayWindow::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton) != 0) {
        move(event->globalPosition().toPoint() - m_dragOffset);
        applyNativeWindowStyles();
        event->accept();
        return;
    }
    QWidget::mouseMoveEvent(event);
}

void StatusOverlayWindow::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_dragging) {
        m_dragging = false;
        setCursor(Qt::SizeAllCursor);
        emit dragFinished(pos());
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

bool StatusOverlayWindow::nativeEvent(const QByteArray& eventType,
                                       void* message, qintptr* result)
{
    if (eventType == "windows_generic_MSG") {
        MSG* nativeMessage = static_cast<MSG*>(message);
        if (nativeMessage != nullptr && result != nullptr
            && nativeMessage->message == WM_MOUSEACTIVATE) {
            *result = MA_NOACTIVATE;
            return true;
        }
    }
    return QWidget::nativeEvent(eventType, message, result);
}

void StatusOverlayWindow::updateContentSize()
{
    const QSize size = contentSizeForScale(m_scale);
    setFixedSize(size);
    update();
}

QSize StatusOverlayWindow::contentSizeForScale(double scale) const
{
    const ContentLayout layout =
        makeContentLayout(m_currentWeapon, m_ammoState, m_capacity, scale);
    return QSize(qMax(1, static_cast<int>(std::ceil(layout.width))),
                 qMax(1, static_cast<int>(std::ceil(layout.height))));
}

void StatusOverlayWindow::applyNativeWindowStyles()
{
    const HWND hwnd = reinterpret_cast<HWND>(winId());
    if (hwnd == nullptr) {
        return;
    }

    LONG_PTR extendedStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    extendedStyle |= WS_EX_LAYERED | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW;
    extendedStyle &= ~(WS_EX_TRANSPARENT | WS_EX_APPWINDOW);
    SetWindowLongPtrW(hwnd, GWL_EXSTYLE, extendedStyle);

    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE
                     | SWP_FRAMECHANGED);
}
