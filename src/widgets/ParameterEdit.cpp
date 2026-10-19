#include "ParameterEdit.h"

#include <QFocusEvent>
#include <QKeyEvent>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QTimer>

ParameterEdit::ParameterEdit(QWidget* parent)
    : QLineEdit(parent)
{
    setObjectName(QStringLiteral("parameterEdit"));
    setMaxLength(10);
    setValidator(new QRegularExpressionValidator(
        QRegularExpression(QStringLiteral("\\d{0,10}")), this));
}

void ParameterEdit::setValue(int value)
{
    m_value = qMax(0, value);
    m_valueOnFocus = m_value;
    setText(QString::number(m_value));
}

int ParameterEdit::value() const
{
    return m_value;
}

void ParameterEdit::forceCommit()
{
    if (hasFocus()) {
        commitEditor();
    }
}

void ParameterEdit::focusInEvent(QFocusEvent* event)
{
    m_valueOnFocus = m_value;
    QLineEdit::focusInEvent(event);
    QTimer::singleShot(0, this, [this] {
        if (hasFocus()) {
            selectAll();
        }
    });
}

void ParameterEdit::focusOutEvent(QFocusEvent* event)
{
    commitEditor();
    QLineEdit::focusOutEvent(event);
}

void ParameterEdit::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        commitEditor();
        clearFocus();
        event->accept();
        return;
    }

    // Tab and Backtab go through QLineEdit so the normal focus chain runs.
    // focusOutEvent then performs the commit.
    QLineEdit::keyPressEvent(event);
}

void ParameterEdit::commitEditor()
{
    bool ok = false;
    const int parsed = text().toInt(&ok);
    if (text().isEmpty() || !ok || parsed < 0) {
        setText(QString::number(m_valueOnFocus));
        m_value = m_valueOnFocus;
        return;
    }

    m_value = parsed;
    setText(QString::number(m_value));
}
