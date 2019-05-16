#include "new-wave-view.h"
#include <QPainter>
#include <QDebug>

NewWaveView::NewWaveView(QWidget *parent)
    : QWidget(parent)
{
}

void NewWaveView::setData(const double *data, unsigned length)
{
    m_data.assign(data, data + length);
    repaint();
}

void NewWaveView::paintIntoRect(
    QPainter &p, const QRectF &r, const double *data, int length)
{
    qreal w = r.width();
    qreal h = r.height();

    p.fillRect(r, Qt::black);

    QPen penAxis(Qt::blue);
    p.setPen(penAxis);
    p.drawLine(r.topLeft() + QPointF(0.25 * w, 0),
               r.bottomLeft() + QPointF(0.25 * w, 0));
    p.drawLine(r.topRight() - QPointF(0.25 * w, 0),
               r.bottomRight() - QPointF(0.25 * w, 0));
    p.drawLine(r.topLeft() + QPointF(0, 0.25 * h),
               r.topRight() + QPointF(0, 0.25 * h));
    p.drawLine(r.bottomLeft() - QPointF(0, 0.25 * h),
               r.bottomRight() - QPointF(0, 0.25 * h));
    penAxis.setWidthF(1.5);
    p.setPen(penAxis);
    p.drawLine(QPointF(0.5 * w, r.top()), QPointF(0.5 * w, r.bottom()));
    p.drawLine(QPointF(r.left(), 0.5 * h), QPointF(r.right(), 0.5 * h));

    if (length <= 0)
        return;

    QPen penCurve(Qt::red, 3.0);
    p.setPen(penCurve);

    int step = qMax(1, (int)(length * (1 / w)));

    qreal sample1 = data[0];
    qreal x1 = 0;
    qreal y1 = h * (1 - (sample1 + 1) * 0.5);
    for(int i = 1; i < length; i += step)
    {
        i = (i < length) ? i : (length - 1);
        qreal ri = i * (1.0 / length);

        qreal sample2 = data[i];
        qreal x2 = ri * w;
        qreal y2 = h * (1 - (sample2 + 1) * 0.5);
        p.drawLine(QPointF(x1, y1), QPointF(x2, y2));

        sample1 = sample2;
        x1 = x2;
        y1 = y2;
    }
}

void NewWaveView::paintEvent(QPaintEvent *event)
{
    QPainter p(this);
    paintIntoRect(p, rect(), m_data.data(), (int)m_data.size());
}
