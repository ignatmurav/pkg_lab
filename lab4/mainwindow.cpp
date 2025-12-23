#include "mainwindow.h"
#include "algorithms.h"

#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QComboBox>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QColorDialog>
#include <QElapsedTimer>
#include <QGroupBox>
#include <QMouseEvent>
#include <QPainter>

// --- DrawingWidget: custom widget that draws grid + pixels ---
class DrawingWidget : public QWidget {
    Q_OBJECT
public:
    DrawingWidget(QWidget *parent = nullptr)
        : QWidget(parent), gridSize(30), color(Qt::black)
    {
        setAutoFillBackground(true);
        setMinimumSize(400, 400);
        firstSet = false;
    }

    void setGridSize(int n) { gridSize = std::max(2, n); update(); }
    void setColor(const QColor &c) { color = c; update(); }
    void setPixels(const std::vector<QPoint> &p) { pixels = p; update(); }
    void clearPixels() { pixels.clear(); firstSet=false; startPoint = endPoint = QPoint(-1,-1); update(); }
    void setModeCircle(bool v) { modeCircle = v; }
    QPoint gridFromMouse(const QPoint &pos) const {
        int w = width(), h = height();
        int cell = std::min(w, h) / gridSize;
        if (cell <= 0) return QPoint(-1,-1);

        // origin in center
        int ox = w / 2;
        int oy = h / 2;

        // compute floating coordinates relative to center, y up
        double fx = double(pos.x() - ox) / double(cell);
        double fy = double(oy - pos.y()) / double(cell);

        int gx = int(std::round(fx));
        int gy = int(std::round(fy));

        int half = gridSize / 2;
        // allowed range: [-half .. half] (if gridSize odd, center included)
        if (gx < -half || gx > half) return QPoint(-1,-1);
        if (gy < -half || gy > half) return QPoint(-1,-1);

        return QPoint(gx, gy);
    }


    // used to set points by main window or mouse
    void setStartEnd(const QPoint &a, const QPoint &b) { startPoint = a; endPoint = b; firstSet = (a.x()>=0); update(); }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        // background
        p.fillRect(rect(), Qt::white);

        int w = width(), h = height();
        int cell = std::min(w, h) / gridSize;
        if (cell <= 0) return;

        int ox = w / 2; // origin in center
        int oy = h / 2;

        // draw grid
        p.setPen(QPen(Qt::lightGray, 1));
        for (int i = -gridSize/2; i <= gridSize/2; ++i) {
            p.drawLine(ox + i*cell, 0, ox + i*cell, h);
            p.drawLine(0, oy + i*cell, w, oy + i*cell);
        }

        // axes
        p.setPen(QPen(Qt::black, 2));
        // X axis
        p.drawLine(0, oy, w, oy);
        // Y axis
        p.drawLine(ox, 0, ox, h);

        // axis labels
        p.setFont(QFont("Arial", 12, QFont::Bold));
        p.drawText(w - 20, oy - 5, "X");
        p.drawText(ox + 5, 15, "Y");

        // draw pixels
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        for (const QPoint &pt : pixels) {
            int rx = ox + pt.x() * cell;
            int ry = oy - (pt.y() + 1) * cell;
            p.drawRect(rx, ry, cell, cell);
        }


        // draw start/end points markers
        auto mark = [&](const QPoint &gpt, const QColor &c) {
            if (gpt.x() < 0) return;
            int rx = ox + gpt.x()*cell;
            int ry = oy - gpt.y()*cell;

            p.setBrush(c);
            p.setPen(Qt::black);
            p.drawEllipse(rx - cell/3, ry - cell/3, 2*cell/3, 2*cell/3);
        };
        mark(startPoint, Qt::green);
        if (endPoint.x() >= 0) mark(endPoint, Qt::red);
    }



    void mousePressEvent(QMouseEvent *ev) override {
        QPoint g = gridFromMouse(ev->pos());
        if (g.x() < 0) return;
        if (!firstSet) {
            startPoint = g;
            firstSet = true;
        } else {
            endPoint = g;
            firstSet = false; // ready for next pair
            emit twoPointsSelected(startPoint, endPoint);
        }
        update();
    }

signals:
    void twoPointsSelected(const QPoint &a, const QPoint &b);

private:
    int gridSize;
    QColor color;
    std::vector<QPoint> pixels;
    bool firstSet;
    QPoint startPoint;
    QPoint endPoint;
    bool modeCircle = false;
};

// ---------- MainWindow implementation ----------

#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), currentColor(Qt::black)
{
    createUI();
}

void MainWindow::createUI()
{
    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QHBoxLayout *mainLay = new QHBoxLayout(central);

    // Left: controls
    QVBoxLayout *ctrlLay = new QVBoxLayout();
    ctrlLay->setSpacing(8);

    algCombo = new QComboBox();
    algCombo->addItems(QStringList() << "Step (parametric)" << "DDA" << "Bresenham (line)" << "Bresenham (circle)" << "Castle-Pitway");
    connect(algCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onAlgorithmChanged);

    gridSpin = new QSpinBox();
    gridSpin->setRange(15, 120);
    gridSpin->setValue(45);
    gridSpin->setSuffix(" cells");
    connect(gridSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onGridSizeChanged);

    x0Spin = new QSpinBox(); x0Spin->setRange(-100, 100); x0Spin->setValue(2);
    y0Spin = new QSpinBox(); y0Spin->setRange(-100, 100); y0Spin->setValue(2);
    x1Spin = new QSpinBox(); x1Spin->setRange(-100, 100); x1Spin->setValue(20);
    y1Spin = new QSpinBox(); y1Spin->setRange(-100, 100); y1Spin->setValue(18);
    rSpin  = new QSpinBox(); rSpin->setRange(-100, 100); rSpin->setValue(8);

    runBtn = new QPushButton("Run");
    clearBtn = new QPushButton("Clear");
    colorBtn = new QPushButton("Pick Color");

    timeLabel = new QLabel("Time: -");
    countLabel = new QLabel("Points: -");

    ctrlLay->addWidget(new QLabel("<b>Algorithm</b>"));
    ctrlLay->addWidget(algCombo);
    ctrlLay->addWidget(new QLabel("<b>Grid size</b>"));
    ctrlLay->addWidget(gridSpin);

    QGroupBox *coordsBox = new QGroupBox("Coordinates (grid)");
    QGridLayout *coordsGrid = new QGridLayout();
    coordsGrid->addWidget(new QLabel("x0"), 0, 0); coordsGrid->addWidget(x0Spin, 0, 1);
    coordsGrid->addWidget(new QLabel("y0"), 1, 0); coordsGrid->addWidget(y0Spin, 1, 1);
    coordsGrid->addWidget(new QLabel("x1"), 2, 0); coordsGrid->addWidget(x1Spin, 2, 1);
    coordsGrid->addWidget(new QLabel("y1"), 3, 0); coordsGrid->addWidget(y1Spin, 3, 1);
    coordsGrid->addWidget(new QLabel("r (circle)"), 4, 0); coordsGrid->addWidget(rSpin, 4, 1);
    coordsBox->setLayout(coordsGrid);
    ctrlLay->addWidget(coordsBox);

    ctrlLay->addWidget(colorBtn);
    ctrlLay->addWidget(runBtn);
    ctrlLay->addWidget(clearBtn);
    ctrlLay->addStretch();
    ctrlLay->addWidget(timeLabel);
    ctrlLay->addWidget(countLabel);

    // Right: drawing area
    drawArea = new DrawingWidget();
    drawArea->setGridSize(gridSpin->value());

    mainLay->addLayout(ctrlLay, 0);
    mainLay->addWidget(drawArea, 1);

    // signals
    connect(runBtn, &QPushButton::clicked, this, &MainWindow::onRun);
    connect(clearBtn, &QPushButton::clicked, this, &MainWindow::onClear);
    connect(colorBtn, &QPushButton::clicked, this, &MainWindow::onPickColor);
    connect(drawArea, &DrawingWidget::twoPointsSelected, [this](const QPoint &a, const QPoint &b){
        // update spin boxes and auto-run
        x0Spin->setValue(a.x());
        y0Spin->setValue(a.y());
        x1Spin->setValue(b.x());
        y1Spin->setValue(b.y());
        onRun();
    });

    // initial state
    onAlgorithmChanged(algCombo->currentIndex());
}

void MainWindow::onAlgorithmChanged(int idx)
{
    // enable/disable inputs depending on algorithm
    bool circle = (idx == 3);
    x1Spin->setEnabled(!circle);
    y1Spin->setEnabled(!circle);
    rSpin->setEnabled(circle);
    // For circle we will use x0,y0 as center and r as radius
}

void MainWindow::onGridSizeChanged(int size)
{
    drawArea->setGridSize(size);
}

void MainWindow::onPickColor()
{
    QColor c = QColorDialog::getColor(currentColor, this, "Pick pixel color");
    if (c.isValid()) {
        currentColor = c;
        drawArea->setColor(currentColor);
    }
}

#include <QElapsedTimer>
void MainWindow::onRun()
{
    int gridN = gridSpin->value();
    drawArea->setGridSize(gridN);

    int alg = algCombo->currentIndex();
    int ux0 = x0Spin->value();
    int uy0 = y0Spin->value();
    int ux1 = x1Spin->value();
    int uy1 = y1Spin->value();
    int ur  = rSpin->value();

    int half = gridN / 2;

    auto toAlgX = [&](int cx)->int { return cx + half; };
    auto toAlgY = [&](int cy)->int { return half - cy; };

    int ax0 = toAlgX(ux0);
    int ay0 = toAlgY(uy0);
    int ax1 = toAlgX(ux1);
    int ay1 = toAlgY(uy1);

    auto clampAlg = [&](int &v){ if (v < 0) v = 0; if (v >= gridN) v = gridN-1; };
    clampAlg(ax0); clampAlg(ay0); clampAlg(ax1); clampAlg(ay1);
    if (ur < 0) ur = 0;
    if (ur > gridN) ur = gridN;

    std::vector<QPoint> algPts;
    QElapsedTimer t; t.start();

    if (alg == 0) {
        algPts = step_line(ax0, ay0, ax1, ay1);
    } else if (alg == 1) {
        algPts = dda_line(ax0, ay0, ax1, ay1);
    } else if (alg == 2) {
        algPts = bresenham_line(ax0, ay0, ax1, ay1);
    } else if (alg == 3) {
        algPts = bresenham_circle(toAlgX(ux0), toAlgY(uy0), ur);
    } else if (alg == 4) {
    algPts = kastl_ptv_line(ax0, ay0, ax1, ay1);
}
    qint64 elapsed = t.elapsed();

    // --- ensure start point is present (if algorithm didn't include it) ---
    bool hasStart = false;
    for (const QPoint &p : algPts) if (p.x() == ax0 && p.y() == ay0) { hasStart = true; break; }
    if (!hasStart) {
        // insert at beginning
        algPts.insert(algPts.begin(), QPoint(ax0, ay0));
    }

    // Debug: print algPts (algorithm coordinate system)
    qDebug() << "Alg pts (alg coords):";
    for (const QPoint &p : algPts) qDebug() << p;

    // convert alg points -> centered coordinates for drawing
    std::vector<QPoint> drawPts;
    drawPts.reserve(algPts.size());
    for (const QPoint &p : algPts) {
        int ax = p.x(), ay = p.y();
        int cx = ax - half;
        int cy = half - ay;
        // only keep visible ones
        if (cx < -half || cx > half) continue;
        if (cy < -half || cy > half) continue;
        drawPts.emplace_back(cx, cy);
    }

    qDebug() << "Draw pts (center coords):";
    for (const QPoint &p : drawPts) qDebug() << p;

    drawArea->setPixels(drawPts);

    // set start/end markers in centered coordinates
    drawArea->setStartEnd(QPoint(ux0, uy0), (alg==3?QPoint(-9999,-9999):QPoint(ux1, uy1)));

    timeLabel->setText(QString("Time: %1 ms").arg(elapsed));
    countLabel->setText(QString("Points: %1").arg(drawPts.size()));
}


void MainWindow::onClear()
{
    drawArea->clearPixels();
    timeLabel->setText("Time: -");
    countLabel->setText("Points: -");
}

#include "mainwindow.moc"
