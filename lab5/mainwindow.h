#pragma once
#include <QWidget>
#include <QVector>
#include <QPointF>
#include <QPushButton>
#include <QFileDialog>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QMouseEvent>

struct Seg {
    QPointF a, b;
};

struct Rect {
    double xmin, ymin, xmax, ymax;
};

class MainWindow : public QWidget {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *ev) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void openFile();
    void zoomIn();
    void zoomOut();
    void resetView();

private:
    bool loadFromFile(const QString &path);

    QVector<Seg> segments;
    QVector<QPointF> polygon;
    Rect clipWindow;
    bool hasClip = false;

    QPushButton *openBtn;
    QPushButton *zoomInBtn;
    QPushButton *zoomOutBtn;
    QPushButton *resetBtn;

    // Мировые координаты
    double worldMinX, worldMaxX, worldMinY, worldMaxY;
    double originalWorldMinX, originalWorldMaxX, originalWorldMinY, originalWorldMaxY;

    // Параметры отображения
    double scaleFactor = 1.0;
    double scaleX, scaleY, offsetX, offsetY;
    void computeViewTransform();
    QPointF worldToScreen(const QPointF &p) const;
    QPointF screenToWorld(const QPointF &p) const;

    // Для перемещения
    bool isDragging = false;
    QPoint lastMousePos;
    double dragStartWorldMinX, dragStartWorldMaxX, dragStartWorldMinY, dragStartWorldMaxY;

    bool cohenSutherlandClip(const Seg &in, Seg &out) const;
    QVector<QPointF> sutherlandHodgmanPolygonClip(const QVector<QPointF> &poly, const Rect &clip) const;

    void drawAxes(QPainter &p);
    void updateWorldBounds();
    void zoom(double factor, const QPointF &center = QPointF());
};
