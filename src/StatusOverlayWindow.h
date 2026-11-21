#pragma once

#include <QPoint>
#include <QRect>
#include <QSize>
#include <QWidget>

class QByteArray;
class QHideEvent;
class QPaintEvent;
class QShowEvent;
class QTimer;

class StatusOverlayWindow final : public QWidget {
    Q_OBJECT

public:
    explicit StatusOverlayWindow(QWidget* parent = nullptr);

    void setScale(double scale);
    double scale() const;

    void setRuntimeStatus(int currentWeapon, int ammoState, int capacity);

    // Restores a previously persisted top-left position. Falls back to the
    // primary screen's bottom-right corner if the point is not on any screen.
    void restorePosition(const QPoint& savedTopLeft);
    void placeAtDefaultPosition();
    void ensureVisibleOnScreen();

    void showNoActivate();

signals:
    void dragFinished(const QPoint& topLeft);

protected:
    void paintEvent(QPaintEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    bool nativeEvent(const QByteArray& eventType, void* message,
                     qintptr* result) override;

private:
    void updateContentSize();
    QSize contentSizeForScale(double scale) const;
    // 居中的固定 50×50 拖动判定方块（窗口坐标）。窗口对鼠标完全穿透，
    // 只有在这个方块内同时按住左右键才会开始拖动。
    QRect dragRect() const;
    void pollDrag();
    void applyNativeWindowStyles();

    int m_currentWeapon = 0;
    int m_ammoState = 0;
    int m_capacity = 0;
    double m_scale = 1.0;

    QTimer* m_dragTimer = nullptr;
    bool m_dragging = false;
    QPoint m_dragOffset;
};
