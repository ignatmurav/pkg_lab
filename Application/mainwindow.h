#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSlider>
#include <QSpinBox>
#include <QLineEdit>
#include <QGroupBox>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QColorDialog>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

private slots:
    void updateFromRGB();
    void updateFromCMYK();
    void updateFromHLS();
    void updateEditFromSpin(QLineEdit* edit, QSpinBox* spin);
    void updateSpinFromEdit(QLineEdit* edit, QSpinBox* spin);
    void openColorPicker();
    void updateFromColor(const QColor &color);

private:
    void connectAll();
    QHBoxLayout* createSliderSpinEditLayout(QSlider *slider, QSpinBox *spin, QLineEdit *edit);

    // Методы конвертации
    void rgbToCmyk(int r, int g, int b, int &c, int &m, int &y, int &k);
    void rgbToHls(int r, int g, int b, int &h, int &l, int &s);
    void cmykToRgb(int c, int m, int y, int k, int &r, int &g, int &b);
    void hlsToRgb(int h, int l, int s, int &r, int &g, int &b);

    void updateColorDisplay();
    void showRangeWarning(const QString &fieldName, int min, int max);

    QWidget *centralWidget;
    bool updating;

    // RGB элементы
    QSlider *redSlider, *greenSlider, *blueSlider;
    QSpinBox *redSpin, *greenSpin, *blueSpin;
    QLineEdit *redEdit, *greenEdit, *blueEdit;

    // CMYK элементы
    QSlider *cyanSlider, *magentaSlider, *yellowSlider, *blackSlider;
    QSpinBox *cyanSpin, *magentaSpin, *yellowSpin, *blackSpin;
    QLineEdit *cyanEdit, *magentaEdit, *yellowEdit, *blackEdit;

    // HLS элементы
    QSlider *hueSlider, *lightnessSlider, *saturationSlider;
    QSpinBox *hueSpin, *lightnessSpin, *saturationSpin;
    QLineEdit *hueEdit, *lightnessEdit, *saturationEdit;

    QLabel *colorDisplay;
    QPushButton *colorPickerButton;
};

#endif // MAINWINDOW_H
