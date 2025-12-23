#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QColor>

class DrawingWidget;
class QComboBox;
class QSpinBox;
class QPushButton;
class QLabel;
class QLineEdit;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onRun();
    void onClear();
    void onPickColor();
    void onAlgorithmChanged(int idx);
    void onGridSizeChanged(int size);

private:
    void createUI();

    DrawingWidget *drawArea;

    QComboBox *algCombo;
    QSpinBox *gridSpin;
    QSpinBox *x0Spin, *y0Spin, *x1Spin, *y1Spin, *rSpin;
    QPushButton *runBtn;
    QPushButton *clearBtn;
    QPushButton *colorBtn;
    QLabel *timeLabel;
    QLabel *countLabel;
    QColor currentColor;
};

#endif // MAINWINDOW_H
