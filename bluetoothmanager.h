#ifndef BLUETOOTHMANAGER_H
#define BLUETOOTHMANAGER_H

#include <QObject>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QUuid>
#include <QVector>

class BluetoothManager : public QObject
{
    Q_OBJECT
public:
    explicit BluetoothManager(QObject *parent = nullptr);

    void startScanning(const QString &deviceName);
    void connectToDevice(const QBluetoothDeviceInfo &deviceInfo);
    void disconnectDevice();

    // UUID ВАШОГО ESP32 (Замініть ці значення!)
    static constexpr char SERVICE_UUID[] = "{0000180A-0000-1000-8000-00805f9b34fb}";
    static constexpr char CHARACTERISTIC_UUID[] = "{00002A58-0000-1000-8000-00805f9b34fb}";

signals:
    void deviceFound(const QBluetoothDeviceInfo &deviceInfo);
    void connected(const QString &deviceName);
    void disconnected();
    void dataReceived(const QVector<float>& receivedVals);
    void errorOccurred(const QString &message);

private slots:
    void deviceDiscovered(const QBluetoothDeviceInfo &deviceInfo);
    void controllerStateChanged(QLowEnergyController::ControllerState state);
    void serviceDiscovered(const QUuid &uuid);
    void serviceDetailsDiscovered(QLowEnergyService::ServiceState newState);
    void characteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);

private:
    QBluetoothDeviceDiscoveryAgent *m_discoveryAgent;
    QLowEnergyController *m_control;
    QLowEnergyService *m_service;
    QString m_targetDeviceName;

    void subscribeToCharacteristic();
    QVector<float> parseData(const QByteArray &data);
};

#endif // BLUETOOTHMANAGER_H
