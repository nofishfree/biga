#include "FramelessTitleBar.h"

#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPushButton>

FramelessTitleBar::FramelessTitleBar(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("titleBar"));
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedHeight(38);
    setCursor(Qt::SizeAllCursor);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addStretch(1);

    m_closeButton = new QPushButton(QStringLiteral("×"), this);
    m_closeButton->setObjectName(QStringLiteral("closeButton"));
    m_closeButton->setFixedSize(44, 38);
    m_closeButton->setCursor(Qt::PointingHandCursor);
    m_closeButton->setToolTip(QStringLiteral("关闭"));
    m_closeButton->setAccessibleName(QStringLiteral("关闭"));
    layout->addWidget(m_closeButton);

    connect(m_closeButton, &QPushButton::clicked, this,
            &FramelessTitleBar::closeRequested);
}

void FramelessTitleBar::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragOffset = event->globalPosition().toPoint()
                       - window()->frameGeometry().topLeft();
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void FramelessTitleBar::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton) != 0) {
        window()->move(event->globalPosition().toPoint() - m_dragOffset);
        event->accept();
        return;
    }
    QWidget::mouseMoveEvent(event);
}

void FramelessTitleBar::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}
