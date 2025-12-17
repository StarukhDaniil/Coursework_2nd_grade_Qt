#include "mainwindow.h"
#include <QDateTime>
#include <QDebug>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QVector>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    m_btManager(new BluetoothManager(this)),
    m_currentSeries(nullptr),
    m_currentX(0.0),
    m_sessionCounter(0),
    minY(2000),
    maxY(0)
{
    setupUi();
    setupChart();

    // --- Підключення BLE ---
    connect(m_scanButton, &QPushButton::clicked, this, &MainWindow::startBleScanAndConnect);
    connect(m_btManager, &BluetoothManager::deviceFound, this, &MainWindow::deviceFoundAndConnect);
    connect(m_btManager, &BluetoothManager::connected, this, &MainWindow::handleConnected);
    connect(m_btManager, &BluetoothManager::disconnected, this, &MainWindow::handleDisconnected);
    connect(m_btManager, &BluetoothManager::errorOccurred, this, &MainWindow::handleError);

    // --- Підключення Даних та Інтерфейсу ---
    connect(m_btManager, &BluetoothManager::dataReceived, this, &MainWindow::handleNewData);
    connect(m_startButton, &QPushButton::clicked, this, &MainWindow::startNewGraphSession);
    connect(m_stopButton, &QPushButton::clicked, this, &MainWindow::stopCurrentGraphSession);
    connect(m_historyList, &QListWidget::currentRowChanged, this, &MainWindow::loadSelectedHistoryGraph);

    m_startButton->setEnabled(false);
    m_stopButton->setEnabled(false);
}

MainWindow::~MainWindow()
{
    m_btManager->disconnectDevice();
}

void MainWindow::setupUi()
{
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);

    // Ліва частина: Графік
    m_chartView = new QChartView();
    m_chartView->setRenderHint(QPainter::Antialiasing);
    mainLayout->addWidget(m_chartView, 4);

    // Права частина: Управління та Історія
    QVBoxLayout *controlLayout = new QVBoxLayout();

    // Секція Bluetooth
    controlLayout->addWidget(new QLabel("Налаштування Bluetooth:"));
    m_deviceNameEdit = new QLineEdit("ESP32_Graph"); // Типове ім'я
    controlLayout->addWidget(new QLabel("Ім'я пристрою:"));
    controlLayout->addWidget(m_deviceNameEdit);
    m_scanButton = new QPushButton("Пошук та Підключення");
    m_statusLabel = new QLabel("Очікування підключення");
    controlLayout->addWidget(m_scanButton);
    controlLayout->addWidget(m_statusLabel);

    // Секція Управління Сесією
    controlLayout->addWidget(new QLabel("Сесія Графіку:"));
    m_startButton = new QPushButton("Почати запис");
    m_stopButton = new QPushButton("Зупинити запис");
    controlLayout->addWidget(m_startButton);
    controlLayout->addWidget(m_stopButton);

    // Секція Історії
    controlLayout->addWidget(new QLabel("Історія графіків:"));
    m_historyList = new QListWidget();
    controlLayout->addWidget(m_historyList);

    mainLayout->addLayout(controlLayout, 1);
    setWindowTitle("Qt BLE Graph Viewer (Indications)");
    resize(1000, 600);
}

void MainWindow::setupChart()
{
    m_chart = new QChart();
    m_chart->setTitle("Дані з ESP32 (Очікування підключення)");
    m_chart->setAnimationOptions(QChart::SeriesAnimations);

    m_axisX = new QValueAxis;
    m_axisX->setRange(0, 10);
    m_axisX->setTitleText("Час/Лічильник");
    m_chart->addAxis(m_axisX, Qt::AlignBottom);

    m_axisY = new QValueAxis;
    m_axisY->setRange(0, 100);
    m_axisY->setTitleText("Значення Y");
    m_chart->addAxis(m_axisY, Qt::AlignLeft);

    m_chartView->setChart(m_chart);
}

// --- BLE Слоти ---

void MainWindow::startBleScanAndConnect()
{
    m_targetDevice = QBluetoothDeviceInfo();
    m_statusLabel->setText("Статус: Шукаю пристрій...");
    m_scanButton->setEnabled(false);

    m_btManager->startScanning(m_deviceNameEdit->text().trimmed());
}

void MainWindow::deviceFoundAndConnect(const QBluetoothDeviceInfo &deviceInfo)
{
    m_targetDevice = deviceInfo;
    m_statusLabel->setText(QString("Статус: Знайдено! Підключаюсь до %1...").arg(deviceInfo.name()));
    m_btManager->connectToDevice(deviceInfo);
}

void MainWindow::handleConnected(const QString &deviceName)
{
    m_statusLabel->setText(QString("Підключено до %1. Готовий до запису.").arg(deviceName));
    m_chart->setTitle(QString("Підключено до %1. Очікування запису.").arg(deviceName));
    m_startButton->setEnabled(true);
    m_scanButton->setEnabled(true); // Дозволяємо сканувати, щоб була можливість відключитися
}

void MainWindow::handleDisconnected()
{
    m_statusLabel->setText("Відключено. Перепідключіться.");
    m_chart->setTitle("Відключено. Дані недоступні.");
    m_scanButton->setEnabled(true);
    m_startButton->setEnabled(false);
    stopCurrentGraphSession();
}

void MainWindow::handleError(const QString &message)
{
    m_statusLabel->setText(QString("Помилка: %1").arg(message));
    QMessageBox::critical(this, "BLE Помилка", message);
    m_scanButton->setEnabled(true);
    m_startButton->setEnabled(false);
}

// --- Графік Слоти ---

void MainWindow::handleNewData(const QVector<float>& data)
{
    // Приймаємо дані лише якщо активна поточна сесія запису
    if (!m_currentSeries) return;

    bool shouldSaveHistory = !m_history.isEmpty();

    // 1. Додаємо точку до поточної серії
    for (size_t i = 0; i < data.size(); ++i) {
        if (shouldSaveHistory) {
            m_history.last().appendData(m_currentX, data[i]);
        }
        m_currentSeries->append(m_currentX, data[i]);
        m_currentX += 1; // Збільшення лічильника X

        if (data[i] > maxY) {
            maxY = data[i];
        }

        if (data[i] < minY) {
            minY = data[i];
        }
    }

    // 3. Масштабування осі X
    m_axisX->setRange(m_currentX - 100, m_currentX + 1);
    m_axisY->setRange(minY, maxY);
}

void MainWindow::startNewGraphSession()
{
    if (m_currentSeries) {
        return;
    }

    // Приховуємо всі попередні серії
    m_chart->removeAllSeries();

    m_sessionCounter++;
    QString sessionName = QString("Графік %1 (%2)").arg(m_sessionCounter).arg(QDateTime::currentDateTime().toString("hh:mm:ss"));

    // 1. Створюємо та додаємо нову серію на графік
    m_currentSeries = new QLineSeries();
    m_currentSeries->setName(sessionName);
    m_chart->addSeries(m_currentSeries);

    // 2. Прив'язуємо серію до осей
    m_currentSeries->attachAxis(m_axisX);
    m_currentSeries->attachAxis(m_axisY);

    // 3. Створюємо новий об'єкт історії
    m_history.append(GraphSession(sessionName));
    m_historyList->addItem(sessionName);

    // 4. Скидаємо лічильник X
    m_currentX = 0.0;

    m_startButton->setEnabled(false);
    m_stopButton->setEnabled(true);
    m_chart->setTitle(QString("АКТИВНИЙ ЗАПИС: %1").arg(sessionName));
}

void MainWindow::stopCurrentGraphSession()
{
    if (!m_currentSeries) return;

    m_currentSeries = nullptr; // Припиняємо додавати дані до серії

    // Перевіряємо, чи є в історії що відобразити
    if (m_history.size() > 0) {
        m_historyList->setCurrentRow(m_history.size() - 1);
    }

    m_startButton->setEnabled(true);
    m_stopButton->setEnabled(false);
    m_chart->setTitle("Запис зупинено. Можна розпочати новий або переглянути історію.");
}

void MainWindow::loadSelectedHistoryGraph(int index)
{
    if (index < 0 || index >= m_history.size() || m_currentSeries) return;

    // 1. Отримуємо вибрану сесію
    const GraphSession& session = m_history.at(index);

    // 2. Видаляємо всі поточні серії з Chart
    m_chart->removeAllSeries();

    // 3. Створюємо нову серію для відображення історії
    QLineSeries *historySeries = new QLineSeries();
    historySeries->setName(session.getName());

    double maxX = 0.0;

    // 4. Додаємо всі збережені точки
    for (const QPointF& point : session.getData()) {
        historySeries->append(point);
        if (point.x() > maxX) maxX = point.x();
    }

    // 5. Додаємо серію до Chart та прив'язуємо до осей
    m_chart->addSeries(historySeries);
    historySeries->attachAxis(m_axisX);
    historySeries->attachAxis(m_axisY);

    // 6. Оновлюємо діапазон осі X для відображення всієї історії
    m_axisX->setRange(0, maxX > 0 ? maxX + 1 : 10);
    m_chart->setTitle(QString("Історичні дані: %1").arg(session.getName()));
}
