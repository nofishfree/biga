#pragma once

#include <QImage>
#include <QWidget>

class MainOverlayWindow final : public QWidget {
    Q_OBJECT

public:
    explicit MainOverlayWindow(QWidget* parent = nullptr);

    void showDot(int dotSize);
    void showImage(const QImage& image);

    // Changes the dot size without changing what is currently displayed.
    void applyDotSize(int dotSize);

    void hideForScope();
    void showCurrent();

    bool isShowingImage() const;

protected:
    void paintEvent(QPaintEvent* event) override;
    void showEvent(QShowEvent* event) override;
    bool nativeEvent(const QByteArray& eventType, void* message,
                     qintptr* result) override;

private:
    enum class Content {
        Dot,
        Image,
    };

    void setContentSize(const QSize& size);
    void centerOnPrimaryScreen();
    void applyNativeWindowStyles();
    void presentWithoutActivation();

    Content m_content = Content::Dot;
    int m_dotSize = 3;
    QImage m_image;
};
