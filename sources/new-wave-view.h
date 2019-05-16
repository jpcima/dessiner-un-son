#pragma once
#include <QWidget>
#include <vector>

class NewWaveView : public QWidget
{
    Q_OBJECT

public:
    explicit NewWaveView(QWidget *parent = nullptr);

    void setData(const double *data, unsigned length);

    static void paintIntoRect(
        QPainter &p, const QRectF &r,
        const double *data, int length);

protected:
    void paintEvent(QPaintEvent *event);

private:
    std::vector<double> m_data;
};
