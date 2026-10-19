#pragma once

#include <QLineEdit>

class ParameterEdit final : public QLineEdit {
    Q_OBJECT

public:
    explicit ParameterEdit(QWidget* parent = nullptr);

    void setValue(int value);
    int value() const;

    // Used when the settings window closes while this editor still has focus.
    void forceCommit();

protected:
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void commitEditor();

    int m_value = 0;
    int m_valueOnFocus = 0;
};
