#include "bluetoothmanager.h"
#include <QDebug>
#include <QDataStream>
#include <QMessageBox>

BluetoothManager::BluetoothManager(QObject *parent)
    : QObject(parent),
    m_discoveryAgent(new QBluetoothDeviceDiscoveryAgent(this)),
    m_control(nullptr),
    m_service(nullptr)
{
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this, &BluetoothManager::deviceDiscovered);
    connect(m_discoveryAgent, QOverload<QBluetoothDeviceDiscoveryAgent::Error>::of(&QBluetoothDeviceDiscoveryAgent::errorOccurred),
            [this](QBluetoothDeviceDiscoveryAgent::Error error){
                if (error != QBluetoothDeviceDiscoveryAgent::NoError)
                    emit errorOccurred(QString("Помилка сканування: %1").arg(m_discoveryAgent->errorString()));
            });
}

void BluetoothManager::startScanning(const QString &deviceName)
{
    if (m_discoveryAgent->isActive()) {
        m_discoveryAgent->stop();
    }
    m_targetDeviceName = deviceName;
    m_discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
}

void BluetoothManager::connectToDevice(const QBluetoothDeviceInfo &deviceInfo)
{
    if (m_control) {
        m_control->deleteLater();
    }
    m_control = QLowEnergyController::createCentral(deviceInfo, this);

    connect(m_control, &QLowEnergyController::stateChanged,
            this, &BluetoothManager::controllerStateChanged);
    connect(m_control, &QLowEnergyController::serviceDiscovered,
            this, &BluetoothManager::serviceDiscovered);
    connect(m_control, &QLowEnergyController::disconnected,
            this, &BluetoothManager::disconnected);

    m_control->connectToDevice();
}

void BluetoothManager::disconnectDevice()
{
    if (m_control) {
        m_control->disconnectFromDevice();
    }
}

void BluetoothManager::deviceDiscovered(const QBluetoothDeviceInfo &deviceInfo)
{
    if (deviceInfo.name() == m_targetDeviceName) {
        m_discoveryAgent->stop();
        emit deviceFound(deviceInfo);
    }
}

void BluetoothManager::controllerStateChanged(QLowEnergyController::ControllerState state)
{
    if (state == QLowEnergyController::ConnectedState) {
        emit connected(m_control->remoteName());
        m_control->discoverServices();
    } else if (state == QLowEnergyController::UnconnectedState) {
        // Disconnected сигнал вже обробляється окремо
    }
}

void BluetoothManager::serviceDiscovered(const QUuid &uuid)
{
    if (uuid == QUuid(SERVICE_UUID)) {
        m_service = m_control->createServiceObject(uuid, this);

        if (m_service) {
            connect(m_service, &QLowEnergyService::stateChanged,
                    this, &BluetoothManager::serviceDetailsDiscovered);
            m_service->discoverDetails();
        }
    }
}

void BluetoothManager::serviceDetailsDiscovered(QLowEnergyService::ServiceState newState)
{
    if (newState == QLowEnergyService::RemoteServiceDiscovered) {
        subscribeToCharacteristic();
    } else if (newState == QLowEnergyService::InvalidService) {
        emit errorOccurred("Неправильний BLE-сервіс. Перевірте UUID.");
    }
}

void BluetoothManager::subscribeToCharacteristic()
{
    if (!m_service) return;

    const QLowEnergyCharacteristic dataChar =
        m_service->characteristic(QUuid(CHARACTERISTIC_UUID));

    if (!dataChar.isValid()) {
        emit errorOccurred("Характеристика даних не знайдена. Перевірте UUID.");
        return;
    }

    QLowEnergyDescriptor cccdDesc = dataChar.descriptor(
        QUuid::fromString("00002902-0000-1000-8000-00805f9b34fb"));

    if (cccdDesc.isValid()) {
        // *** ЗАПИС 0x0200 ДЛЯ ІНДИКАЦІЙ ***
        m_service->writeDescriptor(cccdDesc, QByteArray::fromHex("0200"));

        // Підключаємо слот для прийому даних
        connect(m_service, &QLowEnergyService::characteristicChanged,
                this, &BluetoothManager::characteristicChanged);

        qDebug() << "Successfully subscribed to data characteristic using INDICATIONS (0200).";
    } else {
        emit errorOccurred("Не вдалося підписатися: CCCD-дескриптор відсутній.");
    }
}

void BluetoothManager::characteristicChanged(
    const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    if (characteristic.uuid() == QBluetoothUuid(CHARACTERISTIC_UUID)) {
        QVector<float> receivedVals = parseData(value);
        emit dataReceived(receivedVals);
    }
}

QVector<float> BluetoothManager::parseData(const QByteArray &data)
{
    // --- ПАРСИНГ ДАНИХ (ПРИКЛАД: 4-байтний float) ---
    // Налаштуйте цей код відповідно до того, як ESP32 надсилає дані.

    const uint16_t* pData16 = reinterpret_cast<const uint16_t*>(data.constData());
    QVector<float> receivedVals;
    size_t dataSize16 = data.size() / 2;

    for (size_t i = 0; i < dataSize16; ++i) {
        receivedVals.push_back(static_cast<float>(pData16[i]));
    }

    if (receivedVals.size() == 0) {
        qWarning() << "Помилка парсингу або неправильний розмір даних:" << data.toHex();
    }

    return receivedVals;
}
