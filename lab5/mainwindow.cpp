#include "mainwindow.h"
#include <QPainter>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QHBoxLayout>
#include "clipping.h"

MainWindow::MainWindow(QWidget *parent) : QWidget(parent)
{
    setMinimumSize(900, 700);
    setMouseTracking(true);

    // Создаем кнопки
    openBtn = new QPushButton("Открыть файл", this);
    zoomInBtn = new QPushButton("+", this);
    zoomOutBtn = new QPushButton("-", this);
    resetBtn = new QPushButton("Сброс", this);

    // Устанавливаем размеры кнопок масштабирования
    zoomInBtn->setFixedSize(30, 30);
    zoomOutBtn->setFixedSize(30, 30);
    resetBtn->setFixedSize(60, 30);

    // Подключаем сигналы
    connect(openBtn, &QPushButton::clicked, this, &MainWindow::openFile);
    connect(zoomInBtn, &QPushButton::clicked, this, &MainWindow::zoomIn);
    connect(zoomOutBtn, &QPushButton::clicked, this, &MainWindow::zoomOut);
    connect(resetBtn, &QPushButton::clicked, this, &MainWindow::resetView);

    // Создаем layout
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(openBtn);
    buttonLayout->addWidget(zoomOutBtn);
    buttonLayout->addWidget(zoomInBtn);
    buttonLayout->addWidget(resetBtn);
    buttonLayout->addStretch();

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addStretch();
    setLayout(mainLayout);

    // Инициализируем границы
    worldMinX = -100; worldMaxX = 100;
    worldMinY = -100; worldMaxY = 100;
    originalWorldMinX = worldMinX;
    originalWorldMaxX = worldMaxX;
    originalWorldMinY = worldMinY;
    originalWorldMaxY = worldMaxY;
}

void MainWindow::openFile()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Открыть файл",
        "",
        "Text files (*.txt);;All files (*)"
        );

    if (!fileName.isEmpty()) {
        loadFromFile(fileName);
    }
}

bool MainWindow::loadFromFile(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
    QTextStream in(&f);
    segments.clear();
    polygon.clear();
    hasClip = false;

    int n;
    in >> n;
    for (int i=0;i<n;i++){
        double x1,y1,x2,y2;
        in >> x1 >> y1 >> x2 >> y2;
        segments.append({QPointF(x1,y1), QPointF(x2,y2)});
    }
    double xmin,ymin,xmax,ymax;
    in >> xmin >> ymin >> xmax >> ymax;
    clipWindow = {xmin,ymin,xmax,ymax};
    hasClip = true;

    if (!in.atEnd()){
        int m;
        in >> m;
        if (m>0){
            for (int i=0;i<m;i++){
                double x,y; in>>x>>y;
                polygon.append(QPointF(x,y));
            }
        }
    }

    // Сохраняем оригинальные границы
    updateWorldBounds();
    originalWorldMinX = worldMinX;
    originalWorldMaxX = worldMaxX;
    originalWorldMinY = worldMinY;
    originalWorldMaxY = worldMaxY;

    // Сбрасываем масштаб
    scaleFactor = 1.0;
    computeViewTransform();
    update();
    return true;
}

void MainWindow::updateWorldBounds()
{
    worldMinX = worldMinY = 1e9;
    worldMaxX = worldMaxY = -1e9;

    auto extend = [&](const QPointF &p){
        if (p.x() < worldMinX) worldMinX = p.x();
        if (p.x() > worldMaxX) worldMaxX = p.x();
        if (p.y() < worldMinY) worldMinY = p.y();
        if (p.y() > worldMaxY) worldMaxY = p.y();
    };

    for (auto &s: segments){ extend(s.a); extend(s.b); }
    extend(QPointF(clipWindow.xmin, clipWindow.ymin));
    extend(QPointF(clipWindow.xmax, clipWindow.ymax));
    for (auto &p: polygon) extend(p);

    // Добавляем отступы
    double padx = (worldMaxX - worldMinX) * 0.1 + 1.0;
    double pady = (worldMaxY - worldMinY) * 0.1 + 1.0;
    worldMinX -= padx; worldMaxX += padx;
    worldMinY -= pady; worldMaxY += pady;
}

void MainWindow::zoomIn()
{
    zoom(1.2);
}

void MainWindow::zoomOut()
{
    zoom(1.0 / 1.2);
}

void MainWindow::resetView()
{
    worldMinX = originalWorldMinX;
    worldMaxX = originalWorldMaxX;
    worldMinY = originalWorldMinY;
    worldMaxY = originalWorldMaxY;
    scaleFactor = 1.0;
    computeViewTransform();
    update();
}

void MainWindow::zoom(double factor, const QPointF &center)
{
    scaleFactor *= factor;

    if (!center.isNull()) {
        // Масштабируем относительно центра
        QPointF worldCenter = screenToWorld(center);
        double width = (worldMaxX - worldMinX) / factor;
        double height = (worldMaxY - worldMinY) / factor;

        worldMinX = worldCenter.x() - width / 2;
        worldMaxX = worldCenter.x() + width / 2;
        worldMinY = worldCenter.y() - height / 2;
        worldMaxY = worldCenter.y() + height / 2;
    } else {
        // Масштабируем относительно центра экрана
        double width = (worldMaxX - worldMinX) / factor;
        double height = (worldMaxY - worldMinY) / factor;
        double centerX = (worldMinX + worldMaxX) / 2;
        double centerY = (worldMinY + worldMaxY) / 2;

        worldMinX = centerX - width / 2;
        worldMaxX = centerX + width / 2;
        worldMinY = centerY - height / 2;
        worldMaxY = centerY + height / 2;
    }

    computeViewTransform();
    update();
}

void MainWindow::wheelEvent(QWheelEvent *event)
{
    QPoint numDegrees = event->angleDelta() / 8;
    if (!numDegrees.isNull()) {
        double factor = (numDegrees.y() > 0) ? 1.2 : 1.0 / 1.2;
        zoom(factor, event->position());
    }
    event->accept();
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        isDragging = true;
        lastMousePos = event->pos();
        dragStartWorldMinX = worldMinX;
        dragStartWorldMaxX = worldMaxX;
        dragStartWorldMinY = worldMinY;
        dragStartWorldMaxY = worldMaxY;
        setCursor(Qt::ClosedHandCursor);
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (isDragging) {
        QPoint delta = event->pos() - lastMousePos;

        // Преобразуем смещение экрана в мировые координаты
        double dx = delta.x() / scaleX;
        double dy = -delta.y() / scaleY; // Инвертируем Y

        worldMinX = dragStartWorldMinX - dx;
        worldMaxX = dragStartWorldMaxX - dx;
        worldMinY = dragStartWorldMinY - dy;
        worldMaxY = dragStartWorldMaxY - dy;

        computeViewTransform();
        update();
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        isDragging = false;
        setCursor(Qt::ArrowCursor);
    }
}

void MainWindow::computeViewTransform()
{
    double w = width();
    double h = height() - openBtn->height() - 10;
    if (h <= 0) h = 1;

    scaleX = w / (worldMaxX - worldMinX);
    scaleY = h / (worldMaxY - worldMinY);

    // Сохраняем аспект
    double s = qMin(scaleX, scaleY);
    scaleX = scaleY = s;

    offsetX = -worldMinX * scaleX;
    offsetY = worldMaxY * scaleY;
}

QPointF MainWindow::worldToScreen(const QPointF &p) const {
    double sx = p.x() * scaleX + offsetX;
    double sy = offsetY - p.y() * scaleY;
    return QPointF(sx, sy);
}

QPointF MainWindow::screenToWorld(const QPointF &p) const {
    double wx = (p.x() - offsetX) / scaleX;
    double wy = (offsetY - p.y()) / scaleY;
    return QPointF(wx, wy);
}

void MainWindow::paintEvent(QPaintEvent *ev)
{
    Q_UNUSED(ev);
    QPainter p(this);
    p.fillRect(rect(), Qt::white);
    p.setRenderHint(QPainter::Antialiasing, true);

    int topMargin = openBtn->height() + 10;
    p.translate(0, topMargin);

    computeViewTransform();
    drawAxes(p);

    if (hasClip) {
        // Рисуем прямоугольник отсечения
        QPointF p1 = worldToScreen(QPointF(clipWindow.xmin, clipWindow.ymin));
        QPointF p2 = worldToScreen(QPointF(clipWindow.xmax, clipWindow.ymax));
        QRectF rectScreen(QPointF(qMin(p1.x(), p2.x()), qMin(p1.y(), p2.y())),
                          QPointF(qMax(p1.x(), p2.x()), qMax(p1.y(), p2.y())));
        p.setPen(QPen(Qt::darkGreen, 2));
        p.drawRect(rectScreen);

        QColor fillc = Qt::green;
        fillc.setAlpha(30);
        p.fillRect(rectScreen, fillc);
    }

    // Рисуем оригинальные отрезки
    p.setPen(QPen(Qt::gray, 1, Qt::DashLine));
    for (const auto &s: segments){
        p.drawLine(worldToScreen(s.a), worldToScreen(s.b));
    }

    // Отсекаем и рисуем видимые части
    p.setPen(QPen(Qt::blue, 2));
    for (const auto &s: segments){
        Seg out;
        if (cohenSutherlandClip(s, out)){
            p.drawLine(worldToScreen(out.a), worldToScreen(out.b));
        }
    }

    // Многоугольник
    if (!polygon.isEmpty()){
        p.setPen(QPen(Qt::red, 1, Qt::DashLine));
        QPolygonF qp;
        for (auto &pt: polygon) qp << worldToScreen(pt);
        if (qp.size()>=2) p.drawPolygon(qp);

        if (hasClip){
            QVector<QPointF> clipped = sutherlandHodgmanPolygonClip(polygon, clipWindow);
            if (!clipped.isEmpty()){
                p.setPen(QPen(Qt::magenta, 2));
                QPolygonF cp;
                for (auto &pt: clipped) cp << worldToScreen(pt);
                p.drawPolygon(cp);

                QColor fill = Qt::magenta;
                fill.setAlpha(80);
                p.setBrush(fill);
                p.drawPolygon(cp);
                p.setBrush(Qt::NoBrush);
            }
        }
    }
}

void MainWindow::drawAxes(QPainter &p)
{
    p.setRenderHint(QPainter::Antialiasing, true);

    // Рассчитываем шаг сетки в зависимости от масштаба
    double worldWidth = worldMaxX - worldMinX;
    double worldHeight = worldMaxY - worldMinY;

    // Базовый шаг для сетки (автоматически подстраивается под масштаб)
    double logStep = log10(worldWidth / 10.0);
    double stepSize = pow(10.0, floor(logStep));

    // Минимальный и максимальный шаг для хорошего отображения
    if (stepSize < 0.1) stepSize = 0.1;
    if (stepSize > 1000) stepSize = 1000;

    // Если шаг слишком мелкий, увеличиваем его
    int numLines = worldWidth / stepSize;
    if (numLines > 50) stepSize *= 2;
    if (numLines > 100) stepSize *= 2;

    // Если шаг слишком крупный, уменьшаем его
    if (numLines < 5) stepSize /= 2;
    if (numLines < 3) stepSize /= 2;

    // Сетка
    p.setPen(QPen(QColor(230, 230, 230), 1));

    // Вертикальные линии
    double startX = floor(worldMinX / stepSize) * stepSize;
    for (double x = startX; x <= worldMaxX; x += stepSize) {
        QPointF p1 = worldToScreen(QPointF(x, worldMinY));
        QPointF p2 = worldToScreen(QPointF(x, worldMaxY));
        p.drawLine(p1, p2);
    }

    // Горизонтальные линии
    double startY = floor(worldMinY / stepSize) * stepSize;
    for (double y = startY; y <= worldMaxY; y += stepSize) {
        QPointF p1 = worldToScreen(QPointF(worldMinX, y));
        QPointF p2 = worldToScreen(QPointF(worldMaxX, y));
        p.drawLine(p1, p2);
    }

    // Основные оси
    p.setPen(QPen(Qt::black, 2));

    // Ось Y
    if (worldMinX <= 0 && worldMaxX >= 0) {
        p.drawLine(worldToScreen(QPointF(0, worldMinY)),
                   worldToScreen(QPointF(0, worldMaxY)));
    }

    // Ось X
    if (worldMinY <= 0 && worldMaxY >= 0) {
        p.drawLine(worldToScreen(QPointF(worldMinX, 0)),
                   worldToScreen(QPointF(worldMaxX, 0)));
    }

    // Подписи осей (только если масштаб не слишком мелкий)
    if (stepSize <= 100) {
        QFont font = p.font();
        font.setPointSize(8);
        p.setFont(font);
        p.setPen(QPen(Qt::black, 1));

        // Подписи по оси X
        for (double x = startX; x <= worldMaxX; x += stepSize) {
            if (fabs(x) < 1e-10) continue; // Пропускаем 0 (ось уже отмечена)

            QPointF axisP = worldToScreen(QPointF(x, 0));
            if (worldMinY <= 0 && worldMaxY >= 0) {
                // Рисуем риску
                p.drawLine(QPointF(axisP.x(), axisP.y() - 3),
                           QPointF(axisP.x(), axisP.y() + 3));

                // Подпись
                QString label;
                if (stepSize >= 1) {
                    label = QString::number((int)x);
                } else {
                    label = QString::number(x, 'f', 2);
                }
                p.drawText(QPointF(axisP.x() - 10, axisP.y() + 15), label);
            }
        }

        // Подписи по оси Y
        for (double y = startY; y <= worldMaxY; y += stepSize) {
            if (fabs(y) < 1e-10) continue; // Пропускаем 0

            QPointF axisP = worldToScreen(QPointF(0, y));
            if (worldMinX <= 0 && worldMaxX >= 0) {
                // Рисуем риску
                p.drawLine(QPointF(axisP.x() - 3, axisP.y()),
                           QPointF(axisP.x() + 3, axisP.y()));

                // Подпись
                QString label;
                if (stepSize >= 1) {
                    label = QString::number((int)y);
                } else {
                    label = QString::number(y, 'f', 2);
                }
                p.drawText(QPointF(axisP.x() + 6, axisP.y() + 4), label);
            }
        }
    }
}

bool MainWindow::cohenSutherlandClip(const Seg &in, Seg &out) const {
    QPointF o0, o1;
    return cohenSutherlandClipSegment(in.a, in.b, clipWindow, o0, o1) && (out = Seg{o0,o1}, true);
}

QVector<QPointF> MainWindow::sutherlandHodgmanPolygonClip(const QVector<QPointF> &poly, const Rect &clip) const {
    return sutherlandHodgmanClip(poly, clip);
}
