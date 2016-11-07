#include "dot-editor-widget.h"
#include "math-dsp.h"
#include <QPainter>
#include <QMouseEvent>
#include <QDebug>
#include <boost/optional.hpp>
#include <vector>
#include <cmath>

struct BasicDotEditorWidget::Impl {
  Impl(int xdots, int ydots) : xdots(xdots), ydots(ydots) {
    dotdata.resize(xdots);
    for (int i = 0; i < xdots; ++i) {
      double x = double(i) / (xdots - 1);
      dotdata[i] = std::sin(2.0 * M_PI * x);
    }
  }

  const int xdots {};
  const int ydots {};
  std::vector<double> dotdata;
  boost::optional<QPoint> mousepos;
};

BasicDotEditorWidget::BasicDotEditorWidget(int xdots, int ydots, QWidget *parent)
  : QWidget(parent), P(new Impl(xdots, ydots)) {
  this->setMouseTracking(true);
  this->setPrecisionCursor();
}

BasicDotEditorWidget::~BasicDotEditorWidget() {
}

void BasicDotEditorWidget::initialize() {
  QSize size = this->sizeHint();
  this->setMinimumSize(size);
  this->setMaximumSize(size);
}

void BasicDotEditorWidget::smooth(double param) {
  if (param <= 0.0 || param > 1.0)
    return;

  double adj = 100.0;  // adjust cutoff
  double s = std::exp(-1.0 / (adj * (1.0 - param)));

  for (int i = 0; i < P->xdots; ++i) {
    double input = P->dotdata[i];
    double nextinput = (i + 1 < P->xdots) ? P->dotdata[i + 1] : input;
    P->dotdata[i] = nextinput * (1.0 - s) + input * s;
  }

  this->update();
}

void BasicDotEditorWidget::window(double param) {
  for (int i = 0; i < P->xdots; ++i) {
    double x = double(i) / (P->xdots - 1);
    double w = tukey_window(param, x);
    P->dotdata[i] *= w;
  }

  this->update();
}

void BasicDotEditorWidget::shiftData(int off) {
  std::vector<double> newdata(P->xdots);
  for (int x = 0; x < P->xdots; ++x) {
    int newx = x - off;
    double s;
    if (newx < 0)
      s = P->dotdata[0];
    else if (newx >= P->xdots)
      s = P->dotdata[P->xdots - 1];
    else
      s = P->dotdata[newx];
    newdata[x] = s;
  }
  std::copy(newdata.begin(), newdata.end(), P->dotdata.begin());
  this->update();
}

void BasicDotEditorWidget::invert(int sides) {
  int xmid = P->xdots / 2;

  if (sides & LeftSide) {
    for (int i = 0; i < xmid; ++i)
      P->dotdata[i] = -P->dotdata[i];
  }
  if (sides & RightSide) {
    for (int i = xmid; i < P->xdots; ++i)
      P->dotdata[i] = -P->dotdata[i];
  }

  this->update();
}

void BasicDotEditorWidget::mirror(MirrorDir mirrordir) {
  switch (mirrordir) {
    case MirLeftToRight:
      for (int i = 0; i < P->xdots / 2; ++i)
        P->dotdata[P->xdots - i - 1] = P->dotdata[i];
      this->update();
      break;
    case MirRightToLeft:
      for (int i = 0; i < P->xdots / 2; ++i)
        P->dotdata[i] = P->dotdata[P->xdots - i - 1];
      this->update();
      break;
  }
}

const std::vector<double> &BasicDotEditorWidget::dotData() const {
  return P->dotdata;
}

std::vector<double> &BasicDotEditorWidget::dotData() {
  return P->dotdata;
}

bool BasicDotEditorWidget::inGridBounds(QPoint gridpos) const {
  int x = gridpos.x(), y = gridpos.y();
  return x >= 0 && y >= 0 && x < P->xdots && y < P->ydots;
}

void BasicDotEditorWidget::paintEvent(QPaintEvent *event) {
  QPainter painter(this);

  painter.fillRect(this->rect(), Qt::black);

  this->paintGrid(painter);
  this->paintAxes(painter);
  this->paintDots(painter);
  this->paintMouseIndicator(painter);
}

void BasicDotEditorWidget::mousePressEvent(QMouseEvent *event) {
  QPoint pos = event->pos();
  QPoint gridpos = this->toGridCoord(pos);
  bool inbounds = inGridBounds(gridpos);

  if (!inbounds) {
    P->mousepos = boost::optional<QPoint>();
    return;
  }

  if (event->buttons() & Qt::LeftButton) {
    // qDebug() << "paint dot by click" << pos;
    this->activateDot(gridpos);
    P->mousepos = pos;
    this->update();
  }
}

void BasicDotEditorWidget::mouseReleaseEvent(QMouseEvent *event) {
  P->mousepos = boost::optional<QPoint>();
}

void BasicDotEditorWidget::mouseMoveEvent(QMouseEvent *event) {
  QPoint pos = event->pos();
  QPoint gridpos = this->toGridCoord(pos);
  bool inbounds = inGridBounds(gridpos);

  if (inbounds)
    emit hoveredGridCoord(gridpos);

  if (event->buttons() & Qt::LeftButton) {
    if (P->mousepos) {
      // qDebug() << "connect dots by move" << *P->mousepos << pos;
      this->connectDots(*P->mousepos, pos);
    } else {
      // qDebug() << "paint dot by move" << pos;
      this->activateDot(gridpos);
    }
    this->update();
  }

  if (!inbounds) {
    P->mousepos = boost::optional<QPoint>();
  } else {
    P->mousepos = pos;
  }
}

void BasicDotEditorWidget::activateDot(QPoint gridpos) {
  if (!inGridBounds(gridpos))
    return;
  double val = (double(gridpos.y()) / (P->ydots - 1)) * 2.0 - 1.0;
  P->dotdata[gridpos.x()] = val;
}

void BasicDotEditorWidget::deactivateDot(int gridx) {
  if (!inGridBounds(QPoint(gridx, 0)))
    return;
  P->dotdata[gridx] = 0.0;
}

void BasicDotEditorWidget::connectDots(QPoint mousesrc, QPoint mousedst) {
  int x0 = mousesrc.x(), x1 = mousedst.x();
  int y0 = mousesrc.y(), y1 = mousedst.y();

  if (x0 == x1) {
    if (y0 > y1)
      std::swap(y0, y1);
    for (int y = y0; y <= y1; ++y)
      this->activateDot(this->toGridCoord(QPoint(x0, y)));
    return;
  }

  if (x0 > x1) {
    std::swap(x0, x1);
    std::swap(y0, y1);
  }

  int yinc = (y0 < y1) ? +1 : -1;
  int xmax = x1 + 1;
  int ymax = y1 + yinc;

  double deltax = xmax - x0;
  double deltay = ymax - y0;
  double error = -1.0;
  double deltaerr = std::fabs(deltay / deltax);

  for (int x = x0, y = y0; x < xmax; ++x) {
    this->activateDot(this->toGridCoord(QPoint(x, y)));
    error += deltaerr;
    if (error >= 0.0) {
      y += yinc;
      error -= 1.0;
    }
  }
}

void BasicDotEditorWidget::setPrecisionCursor() {
  QPixmap cursorPic(32, 32);
  cursorPic.fill(Qt::transparent);

  QPainter painter(&cursorPic);
  painter.setPen(Qt::white);
  painter.drawLine(16, 0, 16, 31);
  painter.drawLine(0, 16, 31, 16);
  painter.end();

  this->setCursor(cursorPic);
}

void BasicDotEditorWidget::paintDots(QPainter &painter) const {
  double oldval = 0.0;
  for (int x = 0; x < P->xdots; ++x) {
    double val = P->dotdata[x];
    val = (val + 1.0) / 2.0;
    val = (val < 0.0) ? 0.0 : (val > +1.0) ? +1.0 : val;
    // val normalized to [0,1]
    int y = val * (P->ydots - 1);
    int oldy = oldval * (P->ydots - 1);

    for (int y1 = (oldy < y) ? oldy : y,
             y2 = (oldy < y) ? y : oldy;
         y1 < y2; ++y1)
      this->paintDot(x - 1, y1, Qt::red, painter);

    this->paintDot(x, y, Qt::red, painter);
    oldval = val;
  }
}

///
DotEditorWidget::DotEditorWidget(int xdots, int ydots, int dotsize, QWidget *parent)
  : BasicDotEditorWidget(xdots, ydots, parent)
  , dotsize((dotsize > 2) ? dotsize : 2) {
}

QSize DotEditorWidget::sizeHint() const {
  const QSize size(dotsize * P->xdots + 1, dotsize * P->ydots + 1);
  return size;
}

void DotEditorWidget::paintGrid(QPainter &painter) const {
  const QSize size = this->sizeHint();
  painter.setPen(Qt::gray);
  for (int x = 0; x < P->xdots; ++x)
    painter.drawLine(QLine(dotsize * x, 0, dotsize * x, size.height() - 1));
  for (int y = 0; y < P->ydots; ++y)
    painter.drawLine(QLine(0, dotsize * y, size.width() - 1, dotsize * y));
}

void DotEditorWidget::paintAxes(QPainter &painter) const {
  const QSize size = this->sizeHint();
  painter.setPen(Qt::lightGray);
  int xcenter = size.width() / 2;
  int ycenter = size.height() / 2;
  painter.drawLine(xcenter, 0, xcenter, size.height() - 1);
  painter.drawLine(0, ycenter, size.width() - 1, ycenter);
}

void DotEditorWidget::paintDot(int x, int y, QColor color, QPainter &painter) const {
  QPoint origin(x * dotsize + 1, y * dotsize + 1);
  QRect rect(origin, QSize(dotsize - 1, dotsize - 1));
  painter.fillRect(rect, color);
}

void DotEditorWidget::paintMouseIndicator(QPainter &painter) const {
  if (!P->mousepos)
    return;
  QPoint gridpos = this->toGridCoord(*P->mousepos);
  ///
}

QPoint DotEditorWidget::toGridCoord(QPoint pos) const {
  int x = pos.x() / dotsize;
  int y = pos.y() / dotsize;
  x = (x < 0) ? 0 : (x >= P->xdots) ? (P->xdots - 1) : x;
  y = (y < 0) ? 0 : (y >= P->ydots) ? (P->ydots - 1) : y;
  return QPoint(x, y);
}

///
CompactDotEditorWidget::CompactDotEditorWidget(int xdots, int ydots, QWidget *parent)
  : BasicDotEditorWidget(xdots, ydots, parent) {
}

QSize CompactDotEditorWidget::sizeHint() const {
  return QSize(P->xdots, P->ydots);
}

void CompactDotEditorWidget::paintGrid(QPainter &painter) const {
  const QSize size = this->sizeHint();
  painter.setPen(Qt::gray);
  for (int x = 0; x < P->xdots; x += 2) {
    for (int y = 0; y < P->ydots; y += 2) {
      painter.drawPoint(x, y);
    }
  }
}

void CompactDotEditorWidget::paintAxes(QPainter &painter) const {
  const QSize size = this->sizeHint();
  painter.setPen(Qt::lightGray);
  int xcenter = size.width() / 2;
  int ycenter = size.height() / 2;
  painter.drawLine(xcenter, 0, xcenter, size.height() - 1);
  painter.drawLine(0, ycenter, size.width() - 1, ycenter);
}

void CompactDotEditorWidget::paintDot(int x, int y, QColor color, QPainter &painter) const {
  painter.setPen(color);
  painter.drawPoint(x, y);
}

void CompactDotEditorWidget::paintMouseIndicator(QPainter &painter) const {
  if (!P->mousepos)
    return;
  QPoint gridpos = this->toGridCoord(*P->mousepos);
  ///
}

QPoint CompactDotEditorWidget::toGridCoord(QPoint pos) const {
  return QPoint(pos.x(), pos.y());
}
