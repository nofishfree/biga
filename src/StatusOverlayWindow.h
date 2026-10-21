#pragma once

#include <QPoint>
#include <QSize>
#include <QWidget>

class QByteArray;
class QMouseEvent;
class QPaintEvent;
class QShowEvent;

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
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    bool nativeEvent(const QByteArray& eventType, void* message,
                     qintptr* result) override;

private:
    void updateContentSize();
    QSize contentSizeForScale(double scale) const;
    void applyNativeWindowStyles();

    int m_currentWeapon = 0;
    int m_ammoState = 0;
    int m_capacity = 0;
    double m_scale = 1.0;

    bool m_dragging = false;
    QPoint m_dragOffset;
};
