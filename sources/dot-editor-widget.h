#pragma once
#include <QWidget>
#include <QBitArray>
#include <memory>

class BasicDotEditorWidget : public QWidget {
  Q_OBJECT;
 public:
  explicit BasicDotEditorWidget(int xdots, int ydots, QWidget *parent = nullptr);
  virtual ~BasicDotEditorWidget();

  void initialize();

  void paintEvent(QPaintEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;

  void smooth(double param);
  void window(double param);
  void shiftData(int off);

  enum Side { LeftSide = 1, RightSide = 2,
              BothSides = LeftSide|RightSide };

  void invert(int sides);

  enum MirrorDir { MirLeftToRight, MirRightToLeft };

  void mirror(MirrorDir mirrordir);

  const std::vector<double> &dotData() const;
  std::vector<double> &dotData();

  bool inGridBounds(QPoint gridpos) const;

 signals:
  void hoveredGridCoord(QPoint gridpoint);

 protected:
  struct Impl;
  std::unique_ptr<Impl> P;

  virtual void paintGrid(QPainter &painter) const = 0;
  virtual void paintAxes(QPainter &painter) const = 0;
  virtual void paintDot(int x, int y, QColor color, QPainter &painter) const = 0;
  virtual void paintMouseIndicator(QPainter &painter) const = 0;

  virtual QPoint toGridCoord(QPoint pos) const = 0;

  void activateDot(QPoint gridpos);
  void deactivateDot(int gridx);
  void connectDots(QPoint mousesrc, QPoint mousedst);

  void setPrecisionCursor();

  void paintDots(QPainter &painter) const;
};

///
class DotEditorWidget : public BasicDotEditorWidget {
  Q_OBJECT;
 public:
  DotEditorWidget(int xdots, int ydots, int dotsize, QWidget *parent = nullptr);

  QSize sizeHint() const override;

  void paintGrid(QPainter &painter) const override;
  void paintAxes(QPainter &painter) const override;
  void paintDot(int x, int y, QColor color, QPainter &painter) const override;
  void paintMouseIndicator(QPainter &painter) const override;

  QPoint toGridCoord(QPoint pos) const override;

 private:
  const int dotsize = 2;
};

///
class CompactDotEditorWidget : public BasicDotEditorWidget {
  Q_OBJECT;
 public:
  CompactDotEditorWidget(int xdots, int ydots, QWidget *parent = nullptr);

  QSize sizeHint() const override;

  void paintGrid(QPainter &painter) const override;
  void paintAxes(QPainter &painter) const override;
  void paintDot(int x, int y, QColor color, QPainter &painter) const override;
  void paintMouseIndicator(QPainter &painter) const override;

  QPoint toGridCoord(QPoint pos) const override;
};
