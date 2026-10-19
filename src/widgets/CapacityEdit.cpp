#include "CapacityEdit.h"

#include <QFocusEvent>
#include <QKeyEvent>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QTimer>

CapacityEdit::CapacityEdit(QWidget* parent)
    : QLineEdit(parent)
{
    setObjectName(QStringLiteral("capacityEdit"));
    setMaxLength(1);
    setValidator(new QRegularExpressionValidator(
        QRegularExpression(QStringLiteral("\\d")), this));
    setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
}

void CapacityEdit::setValue(int value)
{
    m_value = qBound(0, value, 9);
    setText(QString::number(m_value));
}

int CapacityEdit::value() const
{
    return m_value;
}

void CapacityEdit::focusInEvent(QFocusEvent* event)
{
    QLineEdit::focusInEvent(event);
    QTimer::singleShot(0, this, [this] {
        if (hasFocus()) {
            selectAll();
        }
    });
}

void CapacityEdit::keyPressEvent(QKeyEvent* event)
{
    const QString text = event->text();
    const bool blockedModifier =
        (event->modifiers() & (Qt::ControlModifier | Qt::AltModifier
                               | Qt::MetaModifier))
        != 0;

    if (!blockedModifier && text.size() == 1 && text.at(0).isDigit()) {
        m_value = text.at(0).digitValue();
        setText(QString::number(m_value));
        clearFocus();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        clearFocus();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Tab || event->key() == Qt::Key_Backtab) {
        QLineEdit::keyPressEvent(event);
        return;
    }

    // All other keys are intentionally swallowed: keep waiting for a legal
    // single digit while the field keeps focus and its original value.
    event->accept();
}
