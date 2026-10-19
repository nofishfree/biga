#pragma once

#include <QLineEdit>

class CapacityEdit final : public QLineEdit {
    Q_OBJECT

public:
    explicit CapacityEdit(QWidget* parent = nullptr);

    void setValue(int value);
    int value() const;

protected:
    void focusInEvent(QFocusEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    int m_value = 0;
};
