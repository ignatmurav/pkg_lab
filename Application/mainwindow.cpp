#include "mainwindow.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QColorDialog>
#include <algorithm>
#include <cmath>
#include <QToolTip>
#include <QCursor>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), updating(false)
{
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // Настройка RGB группы
    QGroupBox *rgbGroup = new QGroupBox("RGB Model");
    redSlider = new QSlider(Qt::Horizontal);
    greenSlider = new QSlider(Qt::Horizontal);
    blueSlider = new QSlider(Qt::Horizontal);
    redSpin = new QSpinBox();
    greenSpin = new QSpinBox();
    blueSpin = new QSpinBox();
    redEdit = new QLineEdit();
    greenEdit = new QLineEdit();
    blueEdit = new QLineEdit();

    for (auto slider : {redSlider, greenSlider, blueSlider}) {
        slider->setRange(0, 255);
    }
    for (auto spin : {redSpin, greenSpin, blueSpin}) {
        spin->setRange(0, 255);
    }
    for (auto edit : {redEdit, greenEdit, blueEdit}) {
        edit->setMaximumWidth(50);
        edit->setPlaceholderText("0-255");
    }

    QFormLayout *rgbLayout = new QFormLayout;
    rgbLayout->addRow("Red:", createSliderSpinEditLayout(redSlider, redSpin, redEdit));
    rgbLayout->addRow("Green:", createSliderSpinEditLayout(greenSlider, greenSpin, greenEdit));
    rgbLayout->addRow("Blue:", createSliderSpinEditLayout(blueSlider, blueSpin, blueEdit));
    rgbGroup->setLayout(rgbLayout);

    // Настройка CMYK группы
    QGroupBox *cmykGroup = new QGroupBox("CMYK Model");
    cyanSlider = new QSlider(Qt::Horizontal);
    magentaSlider = new QSlider(Qt::Horizontal);
    yellowSlider = new QSlider(Qt::Horizontal);
    blackSlider = new QSlider(Qt::Horizontal);
    cyanSpin = new QSpinBox();
    magentaSpin = new QSpinBox();
    yellowSpin = new QSpinBox();
    blackSpin = new QSpinBox();
    cyanEdit = new QLineEdit();
    magentaEdit = new QLineEdit();
    yellowEdit = new QLineEdit();
    blackEdit = new QLineEdit();

    for (auto slider : {cyanSlider, magentaSlider, yellowSlider, blackSlider}) {
        slider->setRange(0, 100);
    }
    for (auto spin : {cyanSpin, magentaSpin, yellowSpin, blackSpin}) {
        spin->setRange(0, 100);
    }
    for (auto edit : {cyanEdit, magentaEdit, yellowEdit, blackEdit}) {
        edit->setMaximumWidth(50);
        edit->setPlaceholderText("0-100");
    }

    QFormLayout *cmykLayout = new QFormLayout;
    cmykLayout->addRow("Cyan:", createSliderSpinEditLayout(cyanSlider, cyanSpin, cyanEdit));
    cmykLayout->addRow("Magenta:", createSliderSpinEditLayout(magentaSlider, magentaSpin, magentaEdit));
    cmykLayout->addRow("Yellow:", createSliderSpinEditLayout(yellowSlider, yellowSpin, yellowEdit));
    cmykLayout->addRow("Black:", createSliderSpinEditLayout(blackSlider, blackSpin, blackEdit));
    cmykGroup->setLayout(cmykLayout);

    // Настройка HLS группы
    QGroupBox *hlsGroup = new QGroupBox("HLS Model");
    hueSlider = new QSlider(Qt::Horizontal);
    lightnessSlider = new QSlider(Qt::Horizontal);
    saturationSlider = new QSlider(Qt::Horizontal);
    hueSpin = new QSpinBox();
    lightnessSpin = new QSpinBox();
    saturationSpin = new QSpinBox();
    hueEdit = new QLineEdit();
    lightnessEdit = new QLineEdit();
    saturationEdit = new QLineEdit();

    hueSlider->setRange(0, 359); hueSpin->setRange(0, 359);
    lightnessSlider->setRange(0, 100); lightnessSpin->setRange(0, 100);
    saturationSlider->setRange(0, 100); saturationSpin->setRange(0, 100);

    hueEdit->setMaximumWidth(50); hueEdit->setPlaceholderText("0-359");
    lightnessEdit->setMaximumWidth(50); lightnessEdit->setPlaceholderText("0-100");
    saturationEdit->setMaximumWidth(50); saturationEdit->setPlaceholderText("0-100");

    QFormLayout *hlsLayout = new QFormLayout;
    hlsLayout->addRow("Hue:", createSliderSpinEditLayout(hueSlider, hueSpin, hueEdit));
    hlsLayout->addRow("Lightness:", createSliderSpinEditLayout(lightnessSlider, lightnessSpin, lightnessEdit));
    hlsLayout->addRow("Saturation:", createSliderSpinEditLayout(saturationSlider, saturationSpin, saturationEdit));
    hlsGroup->setLayout(hlsLayout);

    // Кнопка выбора цвета
    colorPickerButton = new QPushButton("Выбрать цвет из палитры");
    colorPickerButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; padding: 8px; }");

    // Отображение цвета
    colorDisplay = new QLabel();
    colorDisplay->setFrameStyle(QFrame::Box);
    colorDisplay->setMinimumHeight(80);
    colorDisplay->setStyleSheet("background-color: rgb(0,0,0);");

    // Компоновка
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(rgbGroup);
    mainLayout->addWidget(cmykGroup);
    mainLayout->addWidget(hlsGroup);
    mainLayout->addWidget(colorPickerButton);
    mainLayout->addWidget(colorDisplay);
    centralWidget->setLayout(mainLayout);

    // Соединение сигналов
    connectAll();

    // Инициализация
    updateFromRGB();

    setWindowTitle("Конвертер цветовых моделей");
    resize(500, 600);
}

void MainWindow::showRangeWarning(const QString &fieldName, int min, int max)
{
    QString message = QString("%1 должен быть в диапазоне %2-%3").arg(fieldName).arg(min).arg(max);
    QToolTip::showText(QCursor::pos(), message, nullptr, QRect(), 3000);
}

void MainWindow::openColorPicker()
{
    QColor color = QColorDialog::getColor(QColor(redSpin->value(), greenSpin->value(), blueSpin->value()),
                                          this,
                                          "Выберите цвет",
                                          QColorDialog::DontUseNativeDialog);

    if (color.isValid()) {
        updateFromColor(color);
    }
}

void MainWindow::updateFromColor(const QColor &color)
{
    if (updating) return;
    updating = true;

    redSpin->setValue(color.red());
    greenSpin->setValue(color.green());
    blueSpin->setValue(color.blue());

    updateEditFromSpin(redEdit, redSpin);
    updateEditFromSpin(greenEdit, greenSpin);
    updateEditFromSpin(blueEdit, blueSpin);

    int c, m, y, k;
    rgbToCmyk(color.red(), color.green(), color.blue(), c, m, y, k);
    cyanSpin->setValue(c); updateEditFromSpin(cyanEdit, cyanSpin);
    magentaSpin->setValue(m); updateEditFromSpin(magentaEdit, magentaSpin);
    yellowSpin->setValue(y); updateEditFromSpin(yellowEdit, yellowSpin);
    blackSpin->setValue(k); updateEditFromSpin(blackEdit, blackSpin);

    int h, l, s;
    rgbToHls(color.red(), color.green(), color.blue(), h, l, s);
    hueSpin->setValue(h); updateEditFromSpin(hueEdit, hueSpin);
    lightnessSpin->setValue(l); updateEditFromSpin(lightnessEdit, lightnessSpin);
    saturationSpin->setValue(s); updateEditFromSpin(saturationEdit, saturationSpin);

    updateColorDisplay();
    updating = false;
}

QHBoxLayout* MainWindow::createSliderSpinEditLayout(QSlider *slider, QSpinBox *spin, QLineEdit *edit)
{
    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(slider);
    layout->addWidget(spin);
    layout->addWidget(edit);
    return layout;
}

void MainWindow::connectAll()
{
    // RGB connections
    connect(redSlider, &QSlider::valueChanged, redSpin, &QSpinBox::setValue);
    connect(redSpin, QOverload<int>::of(&QSpinBox::valueChanged), redSlider, &QSlider::setValue);
    connect(redSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::updateFromRGB);
    connect(redEdit, &QLineEdit::editingFinished, [this]() {
        bool ok;
        int value = redEdit->text().toInt(&ok);
        if (ok) {
            if (value < 0 || value > 255) {
                showRangeWarning("Красный компонент", 0, 255);
                value = qBound(0, value, 255);
                redEdit->setText(QString::number(value));
            }
            redSpin->setValue(value);
        } else if (!redEdit->text().isEmpty()) {
            showRangeWarning("Красный компонент", 0, 255);
            updateEditFromSpin(redEdit, redSpin);
        }
    });

    connect(greenSlider, &QSlider::valueChanged, greenSpin, &QSpinBox::setValue);
    connect(greenSpin, QOverload<int>::of(&QSpinBox::valueChanged), greenSlider, &QSlider::setValue);
    connect(greenSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::updateFromRGB);
    connect(greenEdit, &QLineEdit::editingFinished, [this]() {
        bool ok;
        int value = greenEdit->text().toInt(&ok);
        if (ok) {
            if (value < 0 || value > 255) {
                showRangeWarning("Зеленый компонент", 0, 255);
                value = qBound(0, value, 255);
                greenEdit->setText(QString::number(value));
            }
            greenSpin->setValue(value);
        } else if (!greenEdit->text().isEmpty()) {
            showRangeWarning("Зеленый компонент", 0, 255);
            updateEditFromSpin(greenEdit, greenSpin);
        }
    });

    connect(blueSlider, &QSlider::valueChanged, blueSpin, &QSpinBox::setValue);
    connect(blueSpin, QOverload<int>::of(&QSpinBox::valueChanged), blueSlider, &QSlider::setValue);
    connect(blueSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::updateFromRGB);
    connect(blueEdit, &QLineEdit::editingFinished, [this]() {
        bool ok;
        int value = blueEdit->text().toInt(&ok);
        if (ok) {
            if (value < 0 || value > 255) {
                showRangeWarning("Синий компонент", 0, 255);
                value = qBound(0, value, 255);
                blueEdit->setText(QString::number(value));
            }
            blueSpin->setValue(value);
        } else if (!blueEdit->text().isEmpty()) {
            showRangeWarning("Синий компонент", 0, 255);
            updateEditFromSpin(blueEdit, blueSpin);
        }
    });

    // CMYK connections
    connect(cyanSlider, &QSlider::valueChanged, cyanSpin, &QSpinBox::setValue);
    connect(cyanSpin, QOverload<int>::of(&QSpinBox::valueChanged), cyanSlider, &QSlider::setValue);
    connect(cyanSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::updateFromCMYK);
    connect(cyanEdit, &QLineEdit::editingFinished, [this]() {
        bool ok;
        int value = cyanEdit->text().toInt(&ok);
        if (ok) {
            if (value < 0 || value > 100) {
                showRangeWarning("Голубой компонент", 0, 100);
                value = qBound(0, value, 100);
                cyanEdit->setText(QString::number(value));
            }
            cyanSpin->setValue(value);
        } else if (!cyanEdit->text().isEmpty()) {
            showRangeWarning("Голубой компонент", 0, 100);
            updateEditFromSpin(cyanEdit, cyanSpin);
        }
    });

    connect(magentaSlider, &QSlider::valueChanged, magentaSpin, &QSpinBox::setValue);
    connect(magentaSpin, QOverload<int>::of(&QSpinBox::valueChanged), magentaSlider, &QSlider::setValue);
    connect(magentaSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::updateFromCMYK);
    connect(magentaEdit, &QLineEdit::editingFinished, [this]() {
        bool ok;
        int value = magentaEdit->text().toInt(&ok);
        if (ok) {
            if (value < 0 || value > 100) {
                showRangeWarning("Пурпурный компонент", 0, 100);
                value = qBound(0, value, 100);
                magentaEdit->setText(QString::number(value));
            }
            magentaSpin->setValue(value);
        } else if (!magentaEdit->text().isEmpty()) {
            showRangeWarning("Пурпурный компонент", 0, 100);
            updateEditFromSpin(magentaEdit, magentaSpin);
        }
    });

    connect(yellowSlider, &QSlider::valueChanged, yellowSpin, &QSpinBox::setValue);
    connect(yellowSpin, QOverload<int>::of(&QSpinBox::valueChanged), yellowSlider, &QSlider::setValue);
    connect(yellowSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::updateFromCMYK);
    connect(yellowEdit, &QLineEdit::editingFinished, [this]() {
        bool ok;
        int value = yellowEdit->text().toInt(&ok);
        if (ok) {
            if (value < 0 || value > 100) {
                showRangeWarning("Желтый компонент", 0, 100);
                value = qBound(0, value, 100);
                yellowEdit->setText(QString::number(value));
            }
            yellowSpin->setValue(value);
        } else if (!yellowEdit->text().isEmpty()) {
            showRangeWarning("Желтый компонент", 0, 100);
            updateEditFromSpin(yellowEdit, yellowSpin);
        }
    });

    connect(blackSlider, &QSlider::valueChanged, blackSpin, &QSpinBox::setValue);
    connect(blackSpin, QOverload<int>::of(&QSpinBox::valueChanged), blackSlider, &QSlider::setValue);
    connect(blackSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::updateFromCMYK);
    connect(blackEdit, &QLineEdit::editingFinished, [this]() {
        bool ok;
        int value = blackEdit->text().toInt(&ok);
        if (ok) {
            if (value < 0 || value > 100) {
                showRangeWarning("Черный компонент", 0, 100);
                value = qBound(0, value, 100);
                blackEdit->setText(QString::number(value));
            }
            blackSpin->setValue(value);
        } else if (!blackEdit->text().isEmpty()) {
            showRangeWarning("Черный компонент", 0, 100);
            updateEditFromSpin(blackEdit, blackSpin);
        }
    });

    // HLS connections
    connect(hueSlider, &QSlider::valueChanged, hueSpin, &QSpinBox::setValue);
    connect(hueSpin, QOverload<int>::of(&QSpinBox::valueChanged), hueSlider, &QSlider::setValue);
    connect(hueSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::updateFromHLS);
    connect(hueEdit, &QLineEdit::editingFinished, [this]() {
        bool ok;
        int value = hueEdit->text().toInt(&ok);
        if (ok) {
            if (value < 0 || value > 359) {
                showRangeWarning("Оттенок", 0, 359);
                value = qBound(0, value, 359);
                hueEdit->setText(QString::number(value));
            }
            hueSpin->setValue(value);
        } else if (!hueEdit->text().isEmpty()) {
            showRangeWarning("Оттенок", 0, 359);
            updateEditFromSpin(hueEdit, hueSpin);
        }
    });

    connect(lightnessSlider, &QSlider::valueChanged, lightnessSpin, &QSpinBox::setValue);
    connect(lightnessSpin, QOverload<int>::of(&QSpinBox::valueChanged), lightnessSlider, &QSlider::setValue);
    connect(lightnessSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::updateFromHLS);
    connect(lightnessEdit, &QLineEdit::editingFinished, [this]() {
        bool ok;
        int value = lightnessEdit->text().toInt(&ok);
        if (ok) {
            if (value < 0 || value > 100) {
                showRangeWarning("Яркость", 0, 100);
                value = qBound(0, value, 100);
                lightnessEdit->setText(QString::number(value));
            }
            lightnessSpin->setValue(value);
        } else if (!lightnessEdit->text().isEmpty()) {
            showRangeWarning("Яркость", 0, 100);
            updateEditFromSpin(lightnessEdit, lightnessSpin);
        }
    });

    connect(saturationSlider, &QSlider::valueChanged, saturationSpin, &QSpinBox::setValue);
    connect(saturationSpin, QOverload<int>::of(&QSpinBox::valueChanged), saturationSlider, &QSlider::setValue);
    connect(saturationSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::updateFromHLS);
    connect(saturationEdit, &QLineEdit::editingFinished, [this]() {
        bool ok;
        int value = saturationEdit->text().toInt(&ok);
        if (ok) {
            if (value < 0 || value > 100) {
                showRangeWarning("Насыщенность", 0, 100);
                value = qBound(0, value, 100);
                saturationEdit->setText(QString::number(value));
            }
            saturationSpin->setValue(value);
        } else if (!saturationEdit->text().isEmpty()) {
            showRangeWarning("Насыщенность", 0, 100);
            updateEditFromSpin(saturationEdit, saturationSpin);
        }
    });

    // Color picker connection
    connect(colorPickerButton, &QPushButton::clicked, this, &MainWindow::openColorPicker);
}

void MainWindow::updateEditFromSpin(QLineEdit* edit, QSpinBox* spin)
{
    edit->setText(QString::number(spin->value()));
}

void MainWindow::updateSpinFromEdit(QLineEdit* edit, QSpinBox* spin)
{
    bool ok;
    int value = edit->text().toInt(&ok);
    if (ok) {
        spin->setValue(value);
    } else {
        updateEditFromSpin(edit, spin);
    }
}

void MainWindow::rgbToCmyk(int r, int g, int b, int &c, int &m, int &y, int &k)
{
    double dr = r / 255.0, dg = g / 255.0, db = b / 255.0;
    double k_val = 1.0 - std::max({dr, dg, db});

    if (std::abs(k_val - 1.0) < 1e-6) {
        c = m = y = 0;
        k = 100;
    } else {
        c = qRound(((1.0 - dr - k_val) / (1.0 - k_val)) * 100.0);
        m = qRound(((1.0 - dg - k_val) / (1.0 - k_val)) * 100.0);
        y = qRound(((1.0 - db - k_val) / (1.0 - k_val)) * 100.0);
        k = qRound(k_val * 100.0);

        c = qBound(0, c, 100);
        m = qBound(0, m, 100);
        y = qBound(0, y, 100);
        k = qBound(0, k, 100);
    }
}

void MainWindow::rgbToHls(int r, int g, int b, int &h, int &l, int &s)
{
    double dr = r / 255.0, dg = g / 255.0, db = b / 255.0;
    double cmax = std::max({dr, dg, db});
    double cmin = std::min({dr, dg, db});
    double delta = cmax - cmin;

    l = qRound(((cmax + cmin) / 2.0) * 100.0);

    if (delta < 1e-6) {
        h = 0;
    } else if (cmax == dr) {
        h = qRound(60.0 * fmod((dg - db) / delta, 6.0));
    } else if (cmax == dg) {
        h = qRound(60.0 * ((db - dr) / delta + 2.0));
    } else {
        h = qRound(60.0 * ((dr - dg) / delta + 4.0));
    }

    if (h < 0) h += 360;
    h = qBound(0, h, 359);

    if (delta < 1e-6) {
        s = 0;
    } else {
        s = qRound((delta / (1.0 - std::abs(2.0 * (l / 100.0) - 1.0))) * 100.0);
    }

    s = qBound(0, s, 100);
    l = qBound(0, l, 100);
}

void MainWindow::cmykToRgb(int c, int m, int y, int k, int &r, int &g, int &b)
{
    double dc = c / 100.0, dm = m / 100.0, dy = y / 100.0, dk = k / 100.0;

    r = qRound(255.0 * (1.0 - dc) * (1.0 - dk));
    g = qRound(255.0 * (1.0 - dm) * (1.0 - dk));
    b = qRound(255.0 * (1.0 - dy) * (1.0 - dk));

    r = qBound(0, r, 255);
    g = qBound(0, g, 255);
    b = qBound(0, b, 255);
}

void MainWindow::hlsToRgb(int h, int l, int s, int &r, int &g, int &b)
{
    double dh = h / 360.0, dl = l / 100.0, ds = s / 100.0;

    if (s == 0) {
        r = g = b = qRound(dl * 255.0);
        return;
    }

    double q = (dl < 0.5) ? dl * (1.0 + ds) : dl + ds - dl * ds;
    double p = 2.0 * dl - q;

    auto hueToRgb = [](double p, double q, double t) {
        if (t < 0.0) t += 1.0;
        if (t > 1.0) t -= 1.0;
        if (t < 1.0/6.0) return p + (q - p) * 6.0 * t;
        if (t < 1.0/2.0) return q;
        if (t < 2.0/3.0) return p + (q - p) * (2.0/3.0 - t) * 6.0;
        return p;
    };

    double dr = hueToRgb(p, q, dh + 1.0/3.0);
    double dg = hueToRgb(p, q, dh);
    double db = hueToRgb(p, q, dh - 1.0/3.0);

    r = qRound(dr * 255.0);
    g = qRound(dg * 255.0);
    b = qRound(db * 255.0);

    r = qBound(0, r, 255);
    g = qBound(0, g, 255);
    b = qBound(0, b, 255);
}

void MainWindow::updateFromRGB()
{
    if (updating) return;
    updating = true;

    int r = redSpin->value(), g = greenSpin->value(), b = blueSpin->value();

    updateEditFromSpin(redEdit, redSpin);
    updateEditFromSpin(greenEdit, greenSpin);
    updateEditFromSpin(blueEdit, blueSpin);

    int c, m, y, k;
    rgbToCmyk(r, g, b, c, m, y, k);
    cyanSpin->setValue(c); updateEditFromSpin(cyanEdit, cyanSpin);
    magentaSpin->setValue(m); updateEditFromSpin(magentaEdit, magentaSpin);
    yellowSpin->setValue(y); updateEditFromSpin(yellowEdit, yellowSpin);
    blackSpin->setValue(k); updateEditFromSpin(blackEdit, blackSpin);

    int h, l, s;
    rgbToHls(r, g, b, h, l, s);
    hueSpin->setValue(h); updateEditFromSpin(hueEdit, hueSpin);
    lightnessSpin->setValue(l); updateEditFromSpin(lightnessEdit, lightnessSpin);
    saturationSpin->setValue(s); updateEditFromSpin(saturationEdit, saturationSpin);

    updateColorDisplay();
    updating = false;
}

void MainWindow::updateFromCMYK()
{
    if (updating) return;
    updating = true;

    int c = cyanSpin->value(), m = magentaSpin->value(),
        y = yellowSpin->value(), k = blackSpin->value();

    updateEditFromSpin(cyanEdit, cyanSpin);
    updateEditFromSpin(magentaEdit, magentaSpin);
    updateEditFromSpin(yellowEdit, yellowSpin);
    updateEditFromSpin(blackEdit, blackSpin);

    int r, g, b;
    cmykToRgb(c, m, y, k, r, g, b);
    redSpin->setValue(r); updateEditFromSpin(redEdit, redSpin);
    greenSpin->setValue(g); updateEditFromSpin(greenEdit, greenSpin);
    blueSpin->setValue(b); updateEditFromSpin(blueEdit, blueSpin);

    int h, l, s;
    rgbToHls(r, g, b, h, l, s);
    hueSpin->setValue(h); updateEditFromSpin(hueEdit, hueSpin);
    lightnessSpin->setValue(l); updateEditFromSpin(lightnessEdit, lightnessSpin);
    saturationSpin->setValue(s); updateEditFromSpin(saturationEdit, saturationSpin);

    updateColorDisplay();
    updating = false;
}

void MainWindow::updateFromHLS()
{
    if (updating) return;
    updating = true;

    int h = hueSpin->value(), l = lightnessSpin->value(), s = saturationSpin->value();

    updateEditFromSpin(hueEdit, hueSpin);
    updateEditFromSpin(lightnessEdit, lightnessSpin);
    updateEditFromSpin(saturationEdit, saturationSpin);

    int r, g, b;
    hlsToRgb(h, l, s, r, g, b);
    redSpin->setValue(r); updateEditFromSpin(redEdit, redSpin);
    greenSpin->setValue(g); updateEditFromSpin(greenEdit, greenSpin);
    blueSpin->setValue(b); updateEditFromSpin(blueEdit, blueSpin);

    int c, m, y, k;
    rgbToCmyk(r, g, b, c, m, y, k);
    cyanSpin->setValue(c); updateEditFromSpin(cyanEdit, cyanSpin);
    magentaSpin->setValue(m); updateEditFromSpin(magentaEdit, magentaSpin);
    yellowSpin->setValue(y); updateEditFromSpin(yellowEdit, yellowSpin);
    blackSpin->setValue(k); updateEditFromSpin(blackEdit, blackSpin);

    updateColorDisplay();
    updating = false;
}

void MainWindow::updateColorDisplay()
{
    int r = redSpin->value(), g = greenSpin->value(), b = blueSpin->value();
    QString style = QString("background-color: rgb(%1, %2, %3); border: 2px solid black;")
                        .arg(r).arg(g).arg(b);
    colorDisplay->setStyleSheet(style);

    QString text = QString("RGB: (%1, %2, %3)\nCMYK: (%4%, %5%, %6%, %7%)\nHLS: (%8°, %9%, %10%)")
                       .arg(r).arg(g).arg(b)
                       .arg(cyanSpin->value()).arg(magentaSpin->value())
                       .arg(yellowSpin->value()).arg(blackSpin->value())
                       .arg(hueSpin->value()).arg(lightnessSpin->value())
                       .arg(saturationSpin->value());
    colorDisplay->setText(text);
    colorDisplay->setAlignment(Qt::AlignCenter);
}
