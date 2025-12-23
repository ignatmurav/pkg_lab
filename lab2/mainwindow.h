#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidget>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QDir>
#include <QFileInfo>
#include <QHeaderView>
#include <QMessageBox>
#include <QThread>
#include <QImageReader>
#include <QApplication>
#include <QScreen>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QImage>
#include <QStandardPaths>
#include <QFileDialog>
#include <QBuffer>
#include <QDialog>
#include <QTextEdit>
#include <QDialogButtonBox>
#include <QVBoxLayout>

// Диалог для отображения матрицы квантования
class QuantizationMatrixDialog : public QDialog
{
    Q_OBJECT
public:
    explicit QuantizationMatrixDialog(const QVector<QVector<int>> &matrix,
                                      const QString &title, QWidget *parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle(QString("Матрица квантования - %1").arg(title));
        setMinimumSize(400, 300);

        QVBoxLayout *layout = new QVBoxLayout(this);

        QTextEdit *textEdit = new QTextEdit(this);
        textEdit->setReadOnly(true);
        textEdit->setFont(QFont("Courier New", 10));

        // Форматируем матрицу для отображения
        QString matrixText;
        for (int i = 0; i < matrix.size(); ++i) {
            for (int j = 0; j < matrix[i].size(); ++j) {
                matrixText += QString("%1").arg(matrix[i][j], 3) + " ";
            }
            matrixText += "\n";
        }

        textEdit->setText(matrixText);

        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, this);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);

        layout->addWidget(textEdit);
        layout->addWidget(buttonBox);
    }
};

class ImageInfoReader : public QThread
{
    Q_OBJECT
public:
    explicit ImageInfoReader(const QStringList &files, QObject *parent = nullptr)
        : QThread(parent), m_files(files) {}

    void run() override {
        int total = m_files.size();
        for (int i = 0; i < total; ++i) {
            if (isInterruptionRequested()) return;

            QFileInfo fileInfo(m_files[i]);
            processImageFile(fileInfo);
            emit progressUpdated(i + 1, total);
        }
    }

private:
    void processImageFile(const QFileInfo &fileInfo) {
        QString fileName = fileInfo.fileName();
        QString sizeStr = "Cannot read";
        QString resolution = "Unknown";
        QString depthStr = "Unknown";
        QString compression = "Unknown";
        QString compressionRatio = "N/A";
        QVector<QVector<int>> quantizationMatrix; // Матрица квантования

        QString suffix = fileInfo.suffix().toLower();

        // Для JPEG пытаемся получить матрицу квантования
        if (suffix == "jpg" || suffix == "jpeg") {
            quantizationMatrix = readJpegQuantizationMatrix(fileInfo.absoluteFilePath());
        }

        // Загружаем изображение
        QImage image(fileInfo.absoluteFilePath());
        if (!image.isNull()) {
            // Размер изображения
            sizeStr = QString("%1 x %2").arg(image.width()).arg(image.height());

            // Глубина цвета - исправляем для JPEG
            int depth = image.depth();
            if ((suffix == "jpg" || suffix == "jpeg") && depth == 32) {
                // Проверяем наличие альфа-канала
                if (!image.hasAlphaChannel()) {
                    depth = 24;
                }
            }
            depthStr = QString::number(depth);

            // Разрешение
            int dpmX = image.dotsPerMeterX();
            int dpmY = image.dotsPerMeterY();
            if (dpmX > 0 && dpmY > 0) {
                int dpiX = qRound(dpmX / 39.37);
                int dpiY = qRound(dpmY / 39.37);
                resolution = QString("%1 x %2").arg(dpiX).arg(dpiY);
            } else {
                resolution = "Not specified";
            }

            // Алгоритм сжатия
            compression = getCompressionInfo(fileInfo.suffix());

            // Вычисляем степень сжатия для JPEG
            if (suffix == "jpg" || suffix == "jpeg") {
                compressionRatio = calculateJpegCompressionRatio(image, fileInfo);
            }
        }

        emit fileProcessed(fileName, sizeStr, resolution, depthStr, compression,
                           compressionRatio, quantizationMatrix);
    }

    QString getCompressionInfo(const QString &suffix) {
        QString ext = suffix.toLower();
        if (ext == "jpg" || ext == "jpeg") return "JPEG";
        if (ext == "png") return "Deflate";
        if (ext == "gif") return "LZW";
        if (ext == "tif" || ext == "tiff") return "LZW/JPEG";
        if (ext == "bmp") return "None";
        if (ext == "pcx") return "RLE";
        return "Unknown";
    }

    QString calculateJpegCompressionRatio(const QImage &image, const QFileInfo &fileInfo) {
        qint64 originalSize = fileInfo.size();
        if (originalSize <= 0) {
            return "Error";
        }

        QBuffer buffer;
        buffer.open(QIODevice::WriteOnly);
        bool saved = image.save(&buffer, "JPEG", 75);

        if (!saved || buffer.size() <= 0) {
            return "Error";
        }

        double ratio = (1.0 - (double)buffer.size() / originalSize) * 100.0;

        if (ratio > 0) {
            return QString("%1%").arg(ratio, 0, 'f', 1);
        } else {
            return QString("+%1%").arg(-ratio, 0, 'f', 1);
        }
    }

    QVector<QVector<int>> readJpegQuantizationMatrix(const QString &filePath) {
        QVector<QVector<int>> matrix;

        FILE *file = fopen(filePath.toLocal8Bit().constData(), "rb");
        if (!file) {
            return matrix;
        }

        // Читаем JPEG маркеры
        unsigned char marker[2];
        while (fread(marker, 1, 2, file) == 2) {
            if (marker[0] != 0xFF) break;

            if (marker[1] == 0xDB) { // Маркер DQT (Define Quantization Table)
                unsigned char lengthBytes[2];
                if (fread(lengthBytes, 1, 2, file) != 2) break;

                unsigned int length = (lengthBytes[0] << 8) | lengthBytes[1];
                if (length < 2) break;

                // Читаем оставшиеся данные таблицы квантования
                unsigned char *tableData = new unsigned char[length - 2];
                if (fread(tableData, 1, length - 2, file) == length - 2) {
                    // Первый байт - информация о таблице (бит 0-3: номер таблицы, бит 4-7: точность)
                    unsigned char tableInfo = tableData[0];
                    //bool is16Bit = (tableInfo & 0xF0) != 0; // 0 = 8-bit, 1 = 16-bit

                    // Читаем матрицу 8x8
                    matrix.resize(8);
                    for (int i = 0; i < 8; ++i) {
                        matrix[i].resize(8);
                        for (int j = 0; j < 8; ++j) {
                            int index = 1 + i * 8 + j;
                            if (index < length - 2) {
                                matrix[i][j] = tableData[index];
                            }
                        }
                    }

                    delete[] tableData;
                    break; // Нашли первую таблицу, выходим
                }
                delete[] tableData;
            }
            else if (marker[1] == 0xD8) { // Start of Image
                continue;
            }
            else if (marker[1] >= 0xD0 && marker[1] <= 0xD7) { // Restart markers
                continue;
            }
            else if (marker[1] == 0xDA) { // Start of Scan - данные изображения, выходим
                break;
            }
            else { // Пропускаем другие маркеры
                unsigned char lengthBytes[2];
                if (fread(lengthBytes, 1, 2, file) != 2) break;
                unsigned int length = (lengthBytes[0] << 8) | lengthBytes[1];
                if (length < 2) break;
                fseek(file, length - 2, SEEK_CUR);
            }
        }

        fclose(file);
        return matrix;
    }

    QStringList m_files;

signals:
    void fileProcessed(const QString &fileName, const QString &size,
                       const QString &resolution, const QString &colorDepth,
                       const QString &compression, const QString &compressionRatio,
                       const QVector<QVector<int>> &quantizationMatrix);
    void progressUpdated(int current, int total);
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void selectFolder();
    void processFiles();
    void onFileProcessed(const QString &fileName, const QString &size,
                         const QString &resolution, const QString &colorDepth,
                         const QString &compression, const QString &compressionRatio,
                         const QVector<QVector<int>> &quantizationMatrix);
    void onProgressUpdated(int current, int total);
    void onReaderFinished();
    void onCellDoubleClicked(int row, int column);

private:
    void setupUI();
    void setupTable();
    QStringList findImageFiles(const QString &folderPath);
    void showQuantizationMatrix(int row);

    QTableWidget *tableWidget;
    QPushButton *selectButton;
    QPushButton *processButton;
    QProgressBar *progressBar;
    QLabel *statusLabel;
    QLabel *fileCountLabel;
    ImageInfoReader *readerThread;
    QString currentFolder;

    QVector<QVector<QVector<int>>> quantizationMatrices; // Хранит матрицы для всех файлов
};

#endif // MAINWINDOW_H
