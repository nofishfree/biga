#include "KeyBindingEdit.h"

#include <QContextMenuEvent>
#include <QMouseEvent>
#include <QStyle>

KeyBindingEdit::KeyBindingEdit(QWidget* parent)
    : QLineEdit(parent)
{
    setObjectName(QStringLiteral("bindingEdit"));
    setReadOnly(true);
    setFocusPolicy(Qt::NoFocus);
    setCursor(Qt::PointingHandCursor);
    setContextMenuPolicy(Qt::NoContextMenu);
    setToolTip(QStringLiteral("左键捕获按键，右键清除绑定"));
    refreshAppearance();
}

const KeyBinding& KeyBindingEdit::binding() const
{
    return m_binding;
}

bool KeyBindingEdit::isCapturing() const
{
    return m_capturing;
}

void KeyBindingEdit::setBindingSilently(const KeyBinding& binding)
{
    m_binding = binding;
    m_capturing = false;
    refreshAppearance();
}

void KeyBindingEdit::applyBinding(const KeyBinding& binding)
{
    m_binding = binding;
    m_capturing = false;
    refreshAppearance();
    emit bindingChanged(this);
}

void KeyBindingEdit::clearBinding()
{
    m_binding = KeyBinding{};
    m_capturing = false;
    refreshAppearance();
    emit bindingCleared(this);
}

void KeyBindingEdit::beginCapture()
{
    if (m_capturing) {
        return;
    }
    m_capturing = true;
    refreshAppearance();
    emit captureStarted(this);
}

void KeyBindingEdit::cancelCapture()
{
    if (!m_capturing) {
        return;
    }
    m_capturing = false;
    refreshAppearance();
    emit captureCancelled(this);
}

void KeyBindingEdit::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton) {
        clearBinding();
        event->accept();
        return;
    }
    if (event->button() == Qt::LeftButton) {
        beginCapture();
        event->accept();
        return;
    }
    event->accept();
}

void KeyBindingEdit::contextMenuEvent(QContextMenuEvent* event)
{
    event->accept();
}

void KeyBindingEdit::refreshAppearance()
{
    if (m_capturing) {
        setText(QStringLiteral("请按键…"));
    } else if (m_binding.isEmpty()) {
        setText(QStringLiteral("未绑定"));
    } else {
        setText(m_binding.displayName());
    }

    setProperty("capturing", m_capturing);
    setProperty("emptyBinding", m_binding.isEmpty() && !m_capturing);
    style()->unpolish(this);
    style()->polish(this);
    update();
}
