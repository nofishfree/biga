#pragma once

#include <QWidget>

class QPushButton;

class FramelessTitleBar final : public QWidget {
    Q_OBJECT

public:
    explicit FramelessTitleBar(QWidget* parent = nullptr);

signals:
    void closeRequested();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    QPushButton* m_closeButton = nullptr;
    QPoint m_dragOffset;
    bool m_dragging = false;
};
