#include "mainwindow.h"
#include <QFileDialog>
#include <QImage>
#include <QScreen>
#include <QHeaderView>
#include <QMessageBox>
#include <QDir>
#include <QStandardPaths>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , readerThread(nullptr)
{
    setupUI();
    setupTable();

    // Центрируем окно
    resize(1200, 700);
    QScreen *screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();
    int x = (screenGeometry.width() - width()) / 2;
    int y = (screenGeometry.height() - height()) / 2;
    move(x, y);
}

MainWindow::~MainWindow()
{
    if (readerThread && readerThread->isRunning()) {
        readerThread->requestInterruption();
        readerThread->wait();
    }
}

void MainWindow::setupUI()
{
    setMinimumSize(1000, 600);

    // Центральный виджет
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // Основной layout
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // Группа управления
    QGroupBox *controlGroup = new QGroupBox("Управление", this);
    QHBoxLayout *controlLayout = new QHBoxLayout(controlGroup);

    selectButton = new QPushButton("Выбрать папку", this);
    processButton = new QPushButton("Обработать файлы", this);
    processButton->setEnabled(false);

    fileCountLabel = new QLabel("Файлов не выбрано", this);
    statusLabel = new QLabel("Готово", this);

    controlLayout->addWidget(selectButton);
    controlLayout->addWidget(processButton);
    controlLayout->addWidget(fileCountLabel);
    controlLayout->addStretch();
    controlLayout->addWidget(statusLabel);

    // Прогресс бар
    progressBar = new QProgressBar(this);
    progressBar->setVisible(false);

    // Таблица
    tableWidget = new QTableWidget(this);

    mainLayout->addWidget(controlGroup);
    mainLayout->addWidget(progressBar);
    mainLayout->addWidget(tableWidget);

    // Подключаем сигналы
    connect(selectButton, &QPushButton::clicked, this, &MainWindow::selectFolder);
    connect(processButton, &QPushButton::clicked, this, &MainWindow::processFiles);
    connect(tableWidget, &QTableWidget::cellDoubleClicked, this, &MainWindow::onCellDoubleClicked);
}

void MainWindow::setupTable()
{
    tableWidget->setColumnCount(7); // Увеличили количество колонок до 7
    tableWidget->setHorizontalHeaderLabels({
        "Имя файла",
        "Размер (пиксели)",
        "Разрешение (DPI)",
        "Глубина цвета",
        "Сжатие",
        "Степень сжатия",
        "Матрица квантования" // Новая колонка
    });

    tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableWidget->setSortingEnabled(true);
    tableWidget->setAlternatingRowColors(true);

    // Настраиваем ширину колонок
    tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    tableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    tableWidget->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    tableWidget->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    tableWidget->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
}

void MainWindow::selectFolder()
{
    QFileDialog dialog(this);
    dialog.setWindowTitle("Выберите папку с изображениями");
    dialog.setDirectory(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
    dialog.setFileMode(QFileDialog::Directory);
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);
    dialog.setOption(QFileDialog::ShowDirsOnly, false);

    // Устанавливаем фильтр для изображений
    QStringList filters;
    filters << "Image files (*.jpg *.jpeg *.png *.gif *.tif *.tiff *.bmp *.pcx)"
            << "JPEG (*.jpg *.jpeg)"
            << "PNG (*.png)"
            << "GIF (*.gif)"
            << "TIFF (*.tif *.tiff)"
            << "BMP (*.bmp)"
            << "PCX (*.pcx)"
            << "All files (*.*)";
    dialog.setNameFilters(filters);

    if (dialog.exec() == QDialog::Accepted) {
        QString folder = dialog.directory().absolutePath();

        if (!folder.isEmpty()) {
            currentFolder = folder;
            QStringList files = findImageFiles(folder);

            fileCountLabel->setText(QString("Найдено файлов: %1").arg(files.size()));
            processButton->setEnabled(!files.isEmpty());

            // Очищаем таблицу
            tableWidget->setRowCount(0);
            quantizationMatrices.clear();

            if (files.isEmpty()) {
                QMessageBox::information(this, "Информация",
                                         "В выбранной папке не найдено поддерживаемых изображений.\n\n"
                                         "Поддерживаемые форматы:\n"
                                         "• JPG, JPEG\n"
                                         "• PNG\n"
                                         "• GIF\n"
                                         "• TIFF, TIF\n"
                                         "• BMP\n"
                                         "• PCX"
                                         );
            } else {
                statusLabel->setText(QString("Выбрана папка: %1").arg(QDir(folder).dirName()));
            }
        }
    }
}

QStringList MainWindow::findImageFiles(const QString &folderPath)
{
    QStringList filters;
    filters << "*.jpg" << "*.jpeg" << "*.png" << "*.gif"
            << "*.tif" << "*.tiff" << "*.bmp" << "*.pcx";

    QDir dir(folderPath);
    QStringList files = dir.entryList(filters, QDir::Files | QDir::Readable, QDir::Name);

    for (int i = 0; i < files.size(); ++i) {
        files[i] = dir.absoluteFilePath(files[i]);
    }

    return files;
}

void MainWindow::processFiles()
{
    if (currentFolder.isEmpty()) return;

    QStringList files = findImageFiles(currentFolder);
    if (files.isEmpty()) return;

    selectButton->setEnabled(false);
    processButton->setEnabled(false);
    progressBar->setVisible(true);
    progressBar->setRange(0, files.size());
    progressBar->setValue(0);

    tableWidget->setRowCount(0);
    quantizationMatrices.clear();

    if (readerThread && readerThread->isRunning()) {
        readerThread->requestInterruption();
        readerThread->wait();
    }

    readerThread = new ImageInfoReader(files, this);
    connect(readerThread, &ImageInfoReader::fileProcessed,
            this, &MainWindow::onFileProcessed);
    connect(readerThread, &ImageInfoReader::progressUpdated,
            this, &MainWindow::onProgressUpdated);
    connect(readerThread, &ImageInfoReader::finished,
            this, &MainWindow::onReaderFinished);

    readerThread->start();
}

void MainWindow::onFileProcessed(const QString &fileName, const QString &size,
                                 const QString &resolution, const QString &colorDepth,
                                 const QString &compression, const QString &compressionRatio,
                                 const QVector<QVector<int>> &quantizationMatrix)
{
    int row = tableWidget->rowCount();
    tableWidget->insertRow(row);

    tableWidget->setItem(row, 0, new QTableWidgetItem(fileName));
    tableWidget->setItem(row, 1, new QTableWidgetItem(size));
    tableWidget->setItem(row, 2, new QTableWidgetItem(resolution));
    tableWidget->setItem(row, 3, new QTableWidgetItem(colorDepth));
    tableWidget->setItem(row, 4, new QTableWidgetItem(compression));
    tableWidget->setItem(row, 5, new QTableWidgetItem(compressionRatio));

    // Отображаем информацию о матрице квантования
    QString matrixInfo;
    if (quantizationMatrix.isEmpty()) {
        matrixInfo = "N/A";
    } else {
        matrixInfo = "Доступна (двойной клик)";
    }
    tableWidget->setItem(row, 6, new QTableWidgetItem(matrixInfo));

    // Сохраняем матрицу квантования для этого ряда
    quantizationMatrices.append(quantizationMatrix);

    tableWidget->scrollToBottom();
}

void MainWindow::onProgressUpdated(int current, int total)
{
    progressBar->setValue(current);
    statusLabel->setText(QString("Обработано: %1/%2").arg(current).arg(total));
}

void MainWindow::onReaderFinished()
{
    selectButton->setEnabled(true);
    processButton->setEnabled(true);
    progressBar->setVisible(false);
    statusLabel->setText("Обработка завершена");

    readerThread->deleteLater();
    readerThread = nullptr;
}

void MainWindow::onCellDoubleClicked(int row, int column)
{
    if (column == 6) { // Колонка "Матрица квантования"
        showQuantizationMatrix(row);
    }
}

void MainWindow::showQuantizationMatrix(int row)
{
    if (row < 0 || row >= quantizationMatrices.size()) {
        return;
    }

    QVector<QVector<int>> matrix = quantizationMatrices[row];
    if (matrix.isEmpty()) {
        QMessageBox::information(this, "Матрица квантования",
                                 "Матрица квантования не доступна для этого файла.");
        return;
    }

    QString fileName = tableWidget->item(row, 0)->text();
    QuantizationMatrixDialog dialog(matrix, fileName, this);
    dialog.exec();
}
