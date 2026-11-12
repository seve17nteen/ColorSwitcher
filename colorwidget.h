#ifndef COLORWIDGET_H
#define COLORWIDGET_H

#include <QColor>
#include <QVector>
#include <QWidget>

class ColorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ColorWidget(QWidget *parent = nullptr);
    ~ColorWidget() override;

    void addColor(const QColor &c);
    void resetPalette();
    QColor currentColor() const;

    void stepForward();    // Следующий цвет
    void stepBackward();   // Предыдущий цвет
    void setRandomColor(); // Случайный цвет
    void toggleBlink();    // Мигание

signals:
    void colorChanged(const QColor &color); // Наш собственный сигнал

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    QVector<QColor> m_palette;
    int m_index;
    bool m_blinkOn;
    QColor m_blinkBackup;

    static QVector<QColor> defaultPalette();
    void emitCurrent();
};

#endif // COLORWIDGET_H