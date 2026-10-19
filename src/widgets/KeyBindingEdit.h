#pragma once

#include "KeyBinding.h"

#include <QLineEdit>

class KeyBindingEdit final : public QLineEdit {
    Q_OBJECT

public:
    explicit KeyBindingEdit(QWidget* parent = nullptr);

    const KeyBinding& binding() const;
    bool isCapturing() const;

    void setBindingSilently(const KeyBinding& binding);
    void applyBinding(const KeyBinding& binding);
    void clearBinding();
    void beginCapture();
    void cancelCapture();

signals:
    void captureStarted(KeyBindingEdit* editor);
    void captureCancelled(KeyBindingEdit* editor);
    void bindingChanged(KeyBindingEdit* editor);
    void bindingCleared(KeyBindingEdit* editor);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    void refreshAppearance();

    KeyBinding m_binding;
    bool m_capturing = false;
};
