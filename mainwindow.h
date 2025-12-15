#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QChartView>
#include <QLineSeries>
#include <QValueAxis>
#include <QVBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QVector>
#include "graphsession.h"
#include "bluetoothmanager.h"

    class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // BLE Slots
    void startBleScanAndConnect();
    void deviceFoundAndConnect(const QBluetoothDeviceInfo &deviceInfo);
    void handleConnected(const QString &deviceName);
    void handleDisconnected();
    void handleError(const QString &message);

    // Graph Slots
    void handleNewData(const QVector<float>& data);
    void startNewGraphSession();
    void stopCurrentGraphSession();
    void loadSelectedHistoryGraph(int index);

private:
    void setupUi();
    void setupChart();

    uint32_t minY;
    uint32_t maxY;

    BluetoothManager *m_btManager;
    QChart *m_chart;
    QChartView *m_chartView;
    QLineSeries *m_currentSeries;
    QValueAxis *m_axisX;
    QValueAxis *m_axisY;

    QListWidget *m_historyList;
    QPushButton *m_startButton;
    QPushButton *m_stopButton;

    QLineEdit *m_deviceNameEdit;
    QLabel *m_statusLabel;
    QPushButton *m_scanButton;
    QBluetoothDeviceInfo m_targetDevice;

    QVector<GraphSession> m_history;
    double m_currentX; // Лічильник для осі X
    int m_sessionCounter;
};
#endif // MAINWINDOW_H
