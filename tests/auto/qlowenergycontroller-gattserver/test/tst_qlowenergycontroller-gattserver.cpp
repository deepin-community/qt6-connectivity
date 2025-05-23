// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtBluetooth/qbluetoothaddress.h>
#include <QtBluetooth/qbluetoothdevicediscoveryagent.h>
#include <QtBluetooth/qbluetoothdeviceinfo.h>
#include <QtBluetooth/qbluetoothlocaldevice.h>
#include <QtBluetooth/qlowenergyadvertisingdata.h>
#include <QtBluetooth/qlowenergyadvertisingparameters.h>
#include <QtBluetooth/qlowenergyconnectionparameters.h>
#include <QtBluetooth/qlowenergycontroller.h>
#include <QtBluetooth/qlowenergycharacteristicdata.h>
#include <QtBluetooth/qlowenergydescriptordata.h>
#include <QtBluetooth/qlowenergyservicedata.h>
#include <QtCore/qendian.h>
#include <QtCore/qscopedpointer.h>
//#include <QtCore/qloggingcategory.h>
#include <QtTest/qsignalspy.h>
#include <QtTest/QtTest>

#ifdef Q_OS_LINUX
#include <QtBluetooth/private/lecmaccalculator_p.h>
#endif

#include <algorithm>
#include <cstring>

using namespace QBluetooth;

class TestQLowEnergyControllerGattServer : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // Static, local stuff goes here.
    void advertisingParameters();
    void advertisingData();
    void cmacVerifier();
    void cmacVerifier_data();
    void connectionParameters();
    void controllerType();
    void serviceData();

    // Interaction with actual GATT server goes here. Order is relevant.
    void advertisedData();
    void serverCommunication();

private:
    QBluetoothAddress m_serverAddress;
    QBluetoothDeviceInfo m_serverInfo;
    QScopedPointer<QLowEnergyController> m_leController;

#if defined(CHECK_CMAC_SUPPORT)
    bool checkCmacSupport(const QUuid::Id128Bytes& csrkMsb);
#endif
};


void TestQLowEnergyControllerGattServer::initTestCase()
{
    //QLoggingCategory::setFilterRules(QStringLiteral("qt.bluetooth* = true"));
    const QString serverAddress = qgetenv("QT_BT_GATTSERVER_TEST_ADDRESS");
    if (serverAddress.isEmpty())
        return;
    m_serverAddress = QBluetoothAddress(serverAddress);
    QVERIFY(!m_serverAddress.isNull());
}

void TestQLowEnergyControllerGattServer::advertisingParameters()
{
    QLowEnergyAdvertisingParameters params;
    QCOMPARE(params, QLowEnergyAdvertisingParameters());
    QCOMPARE(params.filterPolicy(), QLowEnergyAdvertisingParameters::IgnoreWhiteList);
    QCOMPARE(params.minimumInterval(), 1280);
    QCOMPARE(params.maximumInterval(), 1280);
    QCOMPARE(params.mode(), QLowEnergyAdvertisingParameters::AdvInd);
    QVERIFY(params.whiteList().isEmpty());

    params.setInterval(100, 200);
    QCOMPARE(params.minimumInterval(), 100);
    QCOMPARE(params.maximumInterval(), 200);
    params.setInterval(200, 100);
    QCOMPARE(params.minimumInterval(), 200);
    QCOMPARE(params.maximumInterval(), 200);

    params.setMode(QLowEnergyAdvertisingParameters::AdvScanInd);
    QCOMPARE(params.mode(), QLowEnergyAdvertisingParameters::AdvScanInd);

    const auto whiteList = QList<QLowEnergyAdvertisingParameters::AddressInfo>()
            << QLowEnergyAdvertisingParameters::AddressInfo(QBluetoothAddress(),
                                                            QLowEnergyController::PublicAddress);
    params.setWhiteList(whiteList, QLowEnergyAdvertisingParameters::UseWhiteListForConnecting);
    QCOMPARE(params.whiteList(), whiteList);
    QCOMPARE(params.filterPolicy(), QLowEnergyAdvertisingParameters::UseWhiteListForConnecting);
    QVERIFY(params != QLowEnergyAdvertisingParameters());

    // verify default ctor
    QLowEnergyAdvertisingParameters::AddressInfo info;
    QVERIFY(info.address == QBluetoothAddress());
    QVERIFY(info.type == QLowEnergyController::PublicAddress);
}

void TestQLowEnergyControllerGattServer::advertisingData()
{
    QLowEnergyAdvertisingData data;
    QCOMPARE(data, QLowEnergyAdvertisingData());
    QCOMPARE(data.discoverability(), QLowEnergyAdvertisingData::DiscoverabilityNone);
    QCOMPARE(data.includePowerLevel(), false);
    QCOMPARE(data.localName(), QString());
    QCOMPARE(data.manufacturerData(), QByteArray());
    QCOMPARE(data.manufacturerId(), QLowEnergyAdvertisingData::invalidManufacturerId());
    QVERIFY(data.services().isEmpty());

    data.setDiscoverability(QLowEnergyAdvertisingData::DiscoverabilityLimited);
    QCOMPARE(data.discoverability(), QLowEnergyAdvertisingData::DiscoverabilityLimited);

    data.setIncludePowerLevel(true);
    QCOMPARE(data.includePowerLevel(), true);

    data.setLocalName("A device name");
    QCOMPARE(data.localName(), QString("A device name"));

    data.setManufacturerData(0xfffd, "some data");
    QCOMPARE(data.manufacturerId(), quint16(0xfffd));
    QCOMPARE(data.manufacturerData(), QByteArray("some data"));

    const auto services = QList<QBluetoothUuid>() << QBluetoothUuid::ServiceClassUuid::CurrentTimeService
                                                  << QBluetoothUuid::ServiceClassUuid::DeviceInformation;
    data.setServices(services);
    QCOMPARE(data.services(), services);

    QByteArray rawData(7, 'x');
    data.setRawData(rawData);
    QCOMPARE(data.rawData(), rawData);

    QVERIFY(data != QLowEnergyAdvertisingData());
}

void TestQLowEnergyControllerGattServer::cmacVerifier()
{
#if defined(CONFIG_LINUX_CRYPTO_API) && defined(QT_BUILD_INTERNAL) && defined(CONFIG_BLUEZ_LE)
    // Test data comes from spec v4.2, Vol 3, Part H, Appendix D.1
    const QUuid::Id128Bytes csrk = {
        { 0x3c, 0x4f, 0xcf, 0x09, 0x88, 0x15, 0xf7, 0xab,
          0xa6, 0xd2, 0xae, 0x28, 0x16, 0x15, 0x7e, 0x2b }
    };
    QFETCH(QByteArray, message);
    QFETCH(quint64, expectedMac);

#if defined(CHECK_CMAC_SUPPORT)
    if (!checkCmacSupport(csrk)) {
        QSKIP("Needed socket options not available. Running qemu?");
    }
#endif

    const bool success = LeCmacCalculator().verify(message, csrk, expectedMac);
    QVERIFY(success);
#else // CONFIG_LINUX_CRYPTO_API
    QSKIP("CMAC verification test only applicable for developer builds on Linux "
          "with BlueZ and crypto API");
#endif // Q_OS_LINUX
}

#if defined(CHECK_CMAC_SUPPORT)
#include <sys/socket.h>
#include <linux/if_alg.h>
#include <unistd.h>

bool TestQLowEnergyControllerGattServer::checkCmacSupport(const QUuid::Id128Bytes& csrk)
{
    bool retval = false;
#if defined(CONFIG_LINUX_CRYPTO_API) && defined(QT_BUILD_INTERNAL) && defined(CONFIG_BLUEZ_LE)
    QUuid::Id128Bytes csrkMsb;
    std::reverse_copy(std::begin(csrk.data), std::end(csrk.data), std::begin(csrkMsb.data));

    int testSocket = socket(AF_ALG, SOCK_SEQPACKET, 0);
    if (testSocket != -1) {
        sockaddr_alg sa;
        using namespace std;
        memset(&sa, 0, sizeof sa);
        sa.salg_family = AF_ALG;
        strcpy(reinterpret_cast<char *>(sa.salg_type), "hash");
        strcpy(reinterpret_cast<char *>(sa.salg_name), "cmac(aes)");
        if (::bind(testSocket, reinterpret_cast<sockaddr *>(&sa), sizeof sa) != -1) {
            if (setsockopt(testSocket, 279 /* SOL_ALG */, ALG_SET_KEY, csrkMsb.data, sizeof csrkMsb) != -1) {
                 retval = true;
            } else {
                 qWarning("Needed socket options (SOL_ALG) not available");
            }
        } else {
            qWarning("bind() failed for crypto socket:");
        }
        close(testSocket);
    } else {
         qWarning("Unable to create test socket");
    }
#endif
    return retval;
}
#endif

void TestQLowEnergyControllerGattServer::cmacVerifier_data()
{
    QTest::addColumn<QByteArray>("message");
    QTest::addColumn<quint64>("expectedMac");
    QTest::newRow("D1.1") << QByteArray() << Q_UINT64_C(0xbb1d6929e9593728);
    QTest::newRow("D1.2") << QByteArray::fromHex("2a179373117e3de9969f402ee2bec16b")
            << Q_UINT64_C(0x070a16b46b4d4144);
    QByteArray messageD13 = QByteArray::fromHex("6bc1bee22e409f96e93d7e117393172aae2d8a57"
                                                "1e03ac9c9eb76fac45af8e5130c81c46a35ce411");
    std::reverse(messageD13.begin(), messageD13.end());
    QTest::newRow("D1.3") << messageD13 << Q_UINT64_C(0xdfa66747de9ae630);
    QByteArray messageD14 = QByteArray::fromHex("6bc1bee22e409f96e93d7e117393172a"
                                                "ae2d8a571e03ac9c9eb76fac45af8e51"
                                                "30c81c46a35ce411e5fbc1191a0a52ef"
                                                "f69f2445df4f9b17ad2b417be66c3710");
    std::reverse(messageD14.begin(), messageD14.end());
    QTest::newRow("D1.4") << messageD14 << Q_UINT64_C(0x51f0bebf7e3b9d92);
}

void TestQLowEnergyControllerGattServer::connectionParameters()
{
    QLowEnergyConnectionParameters connParams;
    QCOMPARE(connParams, QLowEnergyConnectionParameters());
    connParams.setIntervalRange(8, 9);
    QCOMPARE(connParams.minimumInterval(), double(8));
    QCOMPARE(connParams.maximumInterval(), double(9));
    connParams.setIntervalRange(9, 8);
    QCOMPARE(connParams.minimumInterval(), double(9));
    QCOMPARE(connParams.maximumInterval(), double(9));
    connParams.setLatency(50);
    QCOMPARE(connParams.latency(), 50);
    connParams.setSupervisionTimeout(1000);
    QCOMPARE(connParams.supervisionTimeout(), 1000);
    const QLowEnergyConnectionParameters cp2 = connParams;
    QCOMPARE(cp2, connParams);
    QLowEnergyConnectionParameters cp3;
    QVERIFY(cp3 != connParams);
    cp3 = connParams;
    QCOMPARE(cp3, connParams);
}

void TestQLowEnergyControllerGattServer::advertisedData()
{
    if (m_serverAddress.isNull())
        QSKIP("No server address provided");
    QBluetoothDeviceDiscoveryAgent discoveryAgent;
    discoveryAgent.start();
    QSignalSpy spy(&discoveryAgent, SIGNAL(finished()));
    QVERIFY(spy.wait(30000));
    const QList<QBluetoothDeviceInfo> devices = discoveryAgent.discoveredDevices();
    const auto it = std::find_if(devices.constBegin(), devices.constEnd(),
        [this](const QBluetoothDeviceInfo &device) { return device.address() == m_serverAddress; });
    QVERIFY(it != devices.constEnd());
    m_serverInfo = *it;

    // BlueZ seems to interfere with the advertising in some way, so that in addition to the name
    // we set, the host name of the machine is also sent. Therefore we cannot guarantee that "our"
    // name is seen on the scanning machine.
    // QCOMPARE(m_serverInfo.name(), QString("Qt GATT server"));

    QVERIFY(m_serverInfo.serviceUuids().size() >= 3);
    QVERIFY(m_serverInfo.serviceUuids().contains(QBluetoothUuid::ServiceClassUuid::GenericAccess));
    QVERIFY(m_serverInfo.serviceUuids().contains(QBluetoothUuid::ServiceClassUuid::RunningSpeedAndCadence));
    QVERIFY(m_serverInfo.serviceUuids().contains(QBluetoothUuid(quint16(0x2000))));
}

void TestQLowEnergyControllerGattServer::serverCommunication()
{
    if (m_serverAddress.isNull())
        QSKIP("No server address provided");
    m_leController.reset(QLowEnergyController::createCentral(m_serverInfo));
    QVERIFY(!m_leController.isNull());
    m_leController->connectToDevice();
    QScopedPointer<QSignalSpy> spy(new QSignalSpy(m_leController.data(),
                                                  &QLowEnergyController::connected));
    QVERIFY(spy->wait(30000));
    m_leController->discoverServices();
    spy.reset(new QSignalSpy(m_leController.data(), &QLowEnergyController::discoveryFinished));
    QVERIFY(spy->wait(30000));
    const QList<QBluetoothUuid> serviceUuids = m_leController->services();
    QCOMPARE(serviceUuids.size(), 4);
    QVERIFY(serviceUuids.contains(QBluetoothUuid::ServiceClassUuid::GenericAccess));
    QVERIFY(serviceUuids.contains(QBluetoothUuid::ServiceClassUuid::RunningSpeedAndCadence));
    QVERIFY(serviceUuids.contains(QBluetoothUuid(quint16(0x2000))));
    QVERIFY(serviceUuids.contains(QBluetoothUuid(QString("c47774c7-f237-4523-8968-e4ae75431daf"))));

    const QScopedPointer<QLowEnergyService> genericAccessService(
                m_leController->createServiceObject(QBluetoothUuid::ServiceClassUuid::GenericAccess));
    QVERIFY(!genericAccessService.isNull());
    genericAccessService->discoverDetails();
    while (genericAccessService->state() != QLowEnergyService::RemoteServiceDiscovered) {
        spy.reset(new QSignalSpy(genericAccessService.data(), &QLowEnergyService::stateChanged));
        QVERIFY(spy->wait(3000));
    }
    QCOMPARE(genericAccessService->includedServices().size(), 1);
    QCOMPARE(genericAccessService->includedServices().first(),
             QBluetoothUuid(QBluetoothUuid::ServiceClassUuid::RunningSpeedAndCadence));
    QCOMPARE(genericAccessService->characteristics().size(), 2);
    const QLowEnergyCharacteristic deviceNameChar
            = genericAccessService->characteristic(QBluetoothUuid::CharacteristicType::DeviceName);
    QVERIFY(deviceNameChar.isValid());
    QCOMPARE(deviceNameChar.descriptors().size(), 0);
    QCOMPARE(deviceNameChar.properties(),
             QLowEnergyCharacteristic::Read | QLowEnergyCharacteristic::Write);
    QCOMPARE(deviceNameChar.value().constData(), "Qt GATT server");
    const QLowEnergyCharacteristic appearanceChar
            = genericAccessService->characteristic(QBluetoothUuid::CharacteristicType::Appearance);
    QVERIFY(appearanceChar.isValid());
    QCOMPARE(appearanceChar.descriptors().size(), 0);
    QCOMPARE(appearanceChar.properties(), QLowEnergyCharacteristic::Read);
    auto value = qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(
                                                appearanceChar.value().constData()));
    QCOMPARE(value, quint16(128));

    const QScopedPointer<QLowEnergyService> runningSpeedService(
                m_leController->createServiceObject(QBluetoothUuid::ServiceClassUuid::RunningSpeedAndCadence));
    QVERIFY(!runningSpeedService.isNull());
    runningSpeedService->discoverDetails();
    while (runningSpeedService->state() != QLowEnergyService::RemoteServiceDiscovered) {
        spy.reset(new QSignalSpy(runningSpeedService.data(), &QLowEnergyService::stateChanged));
        QVERIFY(spy->wait(3000));
    }
    QCOMPARE(runningSpeedService->includedServices().size(), 0);
    QCOMPARE(runningSpeedService->characteristics().size(), 2);
    QLowEnergyCharacteristic measurementChar
            = runningSpeedService->characteristic(QBluetoothUuid::CharacteristicType::RSCMeasurement);
    QVERIFY(measurementChar.isValid());
    QCOMPARE(measurementChar.descriptors().size(), 1);
    const QLowEnergyDescriptor clientConfigDesc
            = measurementChar.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
    QVERIFY(clientConfigDesc.isValid());
    QCOMPARE(clientConfigDesc.value(), QByteArray(2, 0));
    QCOMPARE(measurementChar.properties(), QLowEnergyCharacteristic::Notify);
    QCOMPARE(measurementChar.value(), QByteArray()); // Empty because Read property not set
    QLowEnergyCharacteristic featureChar
            = runningSpeedService->characteristic(QBluetoothUuid::CharacteristicType::RSCFeature);
    QVERIFY(featureChar.isValid());
    QCOMPARE(featureChar.descriptors().size(), 0);
    QCOMPARE(featureChar.properties(), QLowEnergyCharacteristic::Read);
    value = qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(
                                           featureChar.value().constData()));
    QCOMPARE(value, quint16(1 << 2));

    // 128 bit custom uuid service
    QBluetoothUuid serviceUuid128(QString("c47774c7-f237-4523-8968-e4ae75431daf"));
    QBluetoothUuid charUuid128(QString("c0ad61b1-79e7-42f9-ace0-0a9aa0d0a4f8"));
    QScopedPointer<QLowEnergyService> customService128(
                m_leController->createServiceObject(serviceUuid128));
    QVERIFY(!customService128.isNull());
    customService128->discoverDetails();
    while (customService128->state() != QLowEnergyService::RemoteServiceDiscovered) {
        spy.reset(new QSignalSpy(customService128.data(), &QLowEnergyService::stateChanged));
        QVERIFY(spy->wait(5000));
    }
    QCOMPARE(customService128->serviceUuid(), serviceUuid128);
    QCOMPARE(customService128->includedServices().size(), 0);
    QCOMPARE(customService128->characteristics().size(), 1);
    QLowEnergyCharacteristic customChar128
            = customService128->characteristic(charUuid128);
    QVERIFY(customChar128.isValid());
    QCOMPARE(customChar128.descriptors().size(), 0);
    QCOMPARE(customChar128.value(), QByteArray(15, 'a'));

    QScopedPointer<QLowEnergyService> customService(
                m_leController->createServiceObject(QBluetoothUuid(quint16(0x2000))));
    QVERIFY(!customService.isNull());
    customService->discoverDetails();
    while (customService->state() != QLowEnergyService::RemoteServiceDiscovered) {
        spy.reset(new QSignalSpy(customService.data(), &QLowEnergyService::stateChanged));
        QVERIFY(spy->wait(5000));
    }
    QCOMPARE(customService->includedServices().size(), 0);
    QCOMPARE(customService->characteristics().size(), 5);
    QLowEnergyCharacteristic customChar
            = customService->characteristic(QBluetoothUuid(quint16(0x5000)));
    QVERIFY(customChar.isValid());
    QCOMPARE(customChar.descriptors().size(), 0);
    QCOMPARE(customChar.value(), QByteArray(1024, 'x'));

    QLowEnergyCharacteristic customChar2
            = customService->characteristic(QBluetoothUuid(quint16(0x5001)));
    QVERIFY(customChar2.isValid());
    QCOMPARE(customChar2.descriptors().size(), 0);
    QCOMPARE(customChar2.value(), QByteArray()); // Was not readable due to authorization requirement.

    QLowEnergyCharacteristic customChar3
            = customService->characteristic(QBluetoothUuid(quint16(0x5002)));
    QVERIFY(customChar3.isValid());
    QCOMPARE(customChar3.properties(),
             QLowEnergyCharacteristic::Read | QLowEnergyCharacteristic::Indicate);
    QCOMPARE(customChar3.descriptors().size(), 1);
    QLowEnergyDescriptor cc3ClientConfig
            = customChar3.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
    QVERIFY(cc3ClientConfig.isValid());

    QLowEnergyCharacteristic customChar4
            = customService->characteristic(QBluetoothUuid(quint16(0x5003)));
    QVERIFY(customChar4.isValid());
    QCOMPARE(customChar4.properties(),
             QLowEnergyCharacteristic::Read | QLowEnergyCharacteristic::Notify);
    QCOMPARE(customChar4.descriptors().size(), 1);
    QLowEnergyDescriptor cc4ClientConfig
            = customChar4.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
    QVERIFY(cc4ClientConfig.isValid());

    QLowEnergyCharacteristic customChar5
            = customService->characteristic(QBluetoothUuid(quint16(0x5004)));
    QVERIFY(customChar5.isValid());
    QCOMPARE(customChar5.properties(),
             QLowEnergyCharacteristic::Read | QLowEnergyCharacteristic::WriteSigned);
    QCOMPARE(customChar5.descriptors().size(), 0);
    QCOMPARE(customChar5.value(), QByteArray("initial"));

    customService->writeCharacteristic(customChar, "whatever");
    spy.reset(new QSignalSpy(customService.data(), &QLowEnergyService::errorOccurred));
    QVERIFY(spy->wait(3000));
    QCOMPARE(customService->error(), QLowEnergyService::CharacteristicWriteError);

    spy.reset(new QSignalSpy(customService.data(), &QLowEnergyService::errorOccurred));
    customService->writeCharacteristic(customChar5, "1", QLowEnergyService::WriteSigned);

    // error might happen immediately or once event loop comes back
    bool wasError = (!spy->isEmpty() || spy->wait(3000)); //

    if (!wasError) {
        // Signed write is done twice to test the sign counter stuff.
        // Note: We assume here that the link is not encrypted, as this information is not exported.
        customService->readCharacteristic(customChar5);
        spy.reset(new QSignalSpy(customService.data(), &QLowEnergyService::characteristicRead));
        QVERIFY(spy->wait(3000));
        QCOMPARE(customChar5.value(), QByteArray("1"));
        customService->writeCharacteristic(customChar5, "2", QLowEnergyService::WriteSigned);
        customService->readCharacteristic(customChar5);
        spy.reset(new QSignalSpy(customService.data(), &QLowEnergyService::characteristicRead));
        QVERIFY(spy->wait(3000));
        QCOMPARE(customChar5.value(), QByteArray("2"));
    } else {
        QCOMPARE(customService->error(), QLowEnergyService::CharacteristicWriteError);
    }

    QByteArray indicateValue(2, 0);
    qToLittleEndian<quint16>(2, reinterpret_cast<uchar *>(indicateValue.data()));
    customService->writeDescriptor(cc3ClientConfig, indicateValue);
    spy.reset(new QSignalSpy(customService.data(), &QLowEnergyService::descriptorWritten));
    QVERIFY(spy->wait(3000));

    QByteArray notifyValue(2, 0);
    qToLittleEndian<quint16>(1, reinterpret_cast<uchar *>(notifyValue.data()));
    customService->writeDescriptor(cc4ClientConfig, notifyValue);
    spy.reset(new QSignalSpy(customService.data(), &QLowEnergyService::descriptorWritten));
    QVERIFY(spy->wait(3000));

    // Server now changes the characteristic values.

    spy.reset(new QSignalSpy(customService.data(), &QLowEnergyService::characteristicChanged));
    QVERIFY(spy->wait(3000));
    if (spy->size() == 1)
        QVERIFY(spy->wait(3000));
    QCOMPARE(customChar3.value().constData(), "indicated");
    QCOMPARE(customChar4.value().constData(), "notified");

    // signal requires root privileges on Linux
    spy.reset(new QSignalSpy(m_leController.data(), &QLowEnergyController::connectionUpdated));
    QVERIFY(spy->wait(5000));

    m_leController->disconnectFromDevice();

    if (m_leController->state() != QLowEnergyController::UnconnectedState) {
        spy.reset(new QSignalSpy(m_leController.data(), &QLowEnergyController::stateChanged));
        QVERIFY(spy->wait(3000));
    }
    QCOMPARE(m_leController->state(), QLowEnergyController::UnconnectedState);

    // Server now changes the characteristic values again while we're offline.
    // Note: We cannot test indications and notifications for this case, as the client does
    //       not cache the old information and thus does not yet know the characteristics
    //       at the time the notification/indication is received.

    QTest::qWait(3000);
    m_leController->connectToDevice();
    spy.reset(new QSignalSpy(m_leController.data(), &QLowEnergyController::connected));
    QVERIFY(spy->wait(30000));
    m_leController->discoverServices();
    spy.reset(new QSignalSpy(m_leController.data(), &QLowEnergyController::discoveryFinished));
    QVERIFY(spy->wait(30000));
    customService.reset(m_leController->createServiceObject(QBluetoothUuid(quint16(0x2000))));
    QVERIFY(!customService.isNull());
    customService->discoverDetails();
    while (customService->state() != QLowEnergyService::RemoteServiceDiscovered) {
        spy.reset(new QSignalSpy(customService.data(), &QLowEnergyService::stateChanged));
        QVERIFY(spy->wait(5000));
    }
    customChar3 = customService->characteristic(QBluetoothUuid(quint16(0x5002)));
    QVERIFY(customChar3.isValid());
    QCOMPARE(customChar3.value().constData(), "indicated2");
    customChar4 = customService->characteristic(QBluetoothUuid(quint16(0x5003)));
    QVERIFY(customChar4.isValid());
    QCOMPARE(customChar4.value().constData(), "notified2");
    cc3ClientConfig = customChar3.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
    QVERIFY(cc3ClientConfig.isValid());
    cc4ClientConfig = customChar4.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
    QVERIFY(cc4ClientConfig.isValid());

    const bool isBonded = QBluetoothLocalDevice().pairingStatus(m_serverAddress)
                != QBluetoothLocalDevice::Unpaired;
    if (isBonded) {
        QCOMPARE(cc3ClientConfig.value(), indicateValue);
        QCOMPARE(cc4ClientConfig.value(), notifyValue);

        // Do a third signed write to test sign counter persistence.
        customChar5 = customService->characteristic(QBluetoothUuid(quint16(0x5004)));
        QVERIFY(customChar5.isValid());
        QCOMPARE(customChar5.value(), QByteArray("2"));
        customService->writeCharacteristic(customChar5, "3", QLowEnergyService::WriteSigned);
        customService->readCharacteristic(customChar5);
        spy.reset(new QSignalSpy(customService.data(), &QLowEnergyService::characteristicRead));
        QVERIFY(spy->wait(3000));
        QCOMPARE(customChar5.value(), QByteArray("3"));

    } else {
        QCOMPARE(cc3ClientConfig.value(), QByteArray(2, 0));
        QCOMPARE(cc4ClientConfig.value(), QByteArray(2, 0));
    }
}

void TestQLowEnergyControllerGattServer::controllerType()
{
    const QScopedPointer<QLowEnergyController> controller(QLowEnergyController::createPeripheral());
    QVERIFY(!controller.isNull());
    QCOMPARE(controller->role(), QLowEnergyController::PeripheralRole);
}

void TestQLowEnergyControllerGattServer::serviceData()
{
    QLowEnergyDescriptorData descData;
    QVERIFY(!descData.isValid());

    descData.setUuid(QBluetoothUuid::DescriptorType::ValidRange);
    QCOMPARE(descData.uuid(), QBluetoothUuid(QBluetoothUuid::DescriptorType::ValidRange));
    QVERIFY(descData.isValid());
    QVERIFY(descData != QLowEnergyDescriptorData());

    descData.setValue("xyz");
    QCOMPARE(descData.value().constData(), "xyz");

    descData.setReadPermissions(true, AttAccessConstraint::AttAuthenticationRequired);
    QCOMPARE(descData.isReadable(), true);
    QCOMPARE(descData.readConstraints(), AttAccessConstraint::AttAuthenticationRequired);

    descData.setWritePermissions(false);
    QCOMPARE(descData.isWritable(), false);

    QLowEnergyDescriptorData descData2(QBluetoothUuid::DescriptorType::ReportReference, "abc");
    QVERIFY(descData2 != QLowEnergyDescriptorData());
    QVERIFY(descData2.isValid());
    QCOMPARE(descData2.uuid(), QBluetoothUuid(QBluetoothUuid::DescriptorType::ReportReference));
    QCOMPARE(descData2.value().constData(), "abc");

    QLowEnergyCharacteristicData charData;
    QVERIFY(!charData.isValid());

    charData.setUuid(QBluetoothUuid::CharacteristicType::BatteryLevel);
    QVERIFY(charData != QLowEnergyCharacteristicData());
    QCOMPARE(charData.uuid(), QBluetoothUuid(QBluetoothUuid::CharacteristicType::BatteryLevel));
    QVERIFY(charData.isValid());

    charData.setValue("value");
    QCOMPARE(charData.value().constData(), "value");

    charData.setValueLength(4, 7);
    QCOMPARE(charData.minimumValueLength(), 4);
    QCOMPARE(charData.maximumValueLength(), 7);
    charData.setValueLength(5, 2);
    QCOMPARE(charData.minimumValueLength(), 5);
    QCOMPARE(charData.maximumValueLength(), 5);

    const QLowEnergyCharacteristic::PropertyTypes props
            = QLowEnergyCharacteristic::Read | QLowEnergyCharacteristic::WriteSigned;
    charData.setProperties(props);
    QCOMPARE(charData.properties(),  props);

    charData.setReadConstraints(AttAccessConstraint::AttEncryptionRequired);
    QCOMPARE(charData.readConstraints(), AttAccessConstraint::AttEncryptionRequired);
    charData.setWriteConstraints(AttAccessConstraint::AttAuthenticationRequired
                                 | AttAccessConstraint::AttAuthorizationRequired);
    QCOMPARE(charData.writeConstraints(),
             AttAccessConstraint::AttAuthenticationRequired
                     | AttAccessConstraint::AttAuthorizationRequired);

    charData.addDescriptor(descData);
    QCOMPARE(charData.descriptors().size(), 1);
    charData.setDescriptors(QList<QLowEnergyDescriptorData>());
    QCOMPARE(charData.descriptors().size(), 0);
    charData.setDescriptors(QList<QLowEnergyDescriptorData>() << descData << descData2);
    QLowEnergyDescriptorData descData3(QBluetoothUuid::DescriptorType::ExternalReportReference, "someval");
    charData.addDescriptor(descData3);
    charData.addDescriptor(QLowEnergyDescriptorData()); // Invalid.
    QCOMPARE(charData.descriptors(),
             QList<QLowEnergyDescriptorData>() << descData << descData2 << descData3);

    QLowEnergyServiceData secondaryData;
    QVERIFY(!secondaryData.isValid());

    secondaryData.setUuid(QBluetoothUuid::ServiceClassUuid::SerialPort);
    QCOMPARE(secondaryData.uuid(), QBluetoothUuid(QBluetoothUuid::ServiceClassUuid::SerialPort));
    QVERIFY(secondaryData.isValid());
    QVERIFY(secondaryData != QLowEnergyServiceData());

    secondaryData.setType(QLowEnergyServiceData::ServiceTypeSecondary);
    QCOMPARE(secondaryData.type(), QLowEnergyServiceData::ServiceTypeSecondary);

    secondaryData.addCharacteristic(charData);
    QCOMPARE(secondaryData.characteristics().size(), 1);
    secondaryData.setCharacteristics(QList<QLowEnergyCharacteristicData>());
    QCOMPARE(secondaryData.characteristics().size(), 0);
    secondaryData.setCharacteristics(QList<QLowEnergyCharacteristicData>()
                                     << charData << QLowEnergyCharacteristicData());
    QCOMPARE(secondaryData.characteristics(), QList<QLowEnergyCharacteristicData>() << charData);

    QLowEnergyServiceData backupData;
    backupData.setUuid(QBluetoothUuid::ServiceClassUuid::SerialPort);
    QCOMPARE(backupData.uuid(), QBluetoothUuid(QBluetoothUuid::ServiceClassUuid::SerialPort));
    QVERIFY(backupData.isValid());
    QVERIFY(backupData != QLowEnergyServiceData());

    backupData.setType(QLowEnergyServiceData::ServiceTypeSecondary);
    QCOMPARE(backupData.type(), QLowEnergyServiceData::ServiceTypeSecondary);

    backupData.setCharacteristics(QList<QLowEnergyCharacteristicData>()
                                     << charData << QLowEnergyCharacteristicData());
    QCOMPARE(backupData.characteristics(), QList<QLowEnergyCharacteristicData>() << charData);
    QVERIFY(backupData == secondaryData);

#ifdef Q_OS_DARWIN
    QSKIP("GATT server functionality not implemented for Apple platforms");
#endif
    const QScopedPointer<QLowEnergyController> controller(QLowEnergyController::createPeripheral());
    QVERIFY(!controller->addService(QLowEnergyServiceData()));
    const QScopedPointer<QLowEnergyService> secondaryService(controller->addService(secondaryData));
    QVERIFY(!secondaryService.isNull());
    QCOMPARE(secondaryService->serviceUuid(), secondaryData.uuid());
    const QList<QLowEnergyCharacteristic> characteristics = secondaryService->characteristics();
    QCOMPARE(characteristics.size(), 1);
    QCOMPARE(characteristics.first().uuid(), charData.uuid());
    const QList<QLowEnergyDescriptor> descriptors = characteristics.first().descriptors();
    QCOMPARE(descriptors.size(), 3);
    const auto inUuids = QSet<QBluetoothUuid>() << descData.uuid() << descData2.uuid()
                                                << descData3.uuid();
    QSet<QBluetoothUuid> outUuids;
    for (const QLowEnergyDescriptor &desc : descriptors)
        outUuids << desc.uuid();
    QCOMPARE(inUuids, outUuids);

    QLowEnergyServiceData primaryData;
    primaryData.setUuid(QBluetoothUuid::ServiceClassUuid::Headset);
    primaryData.addIncludedService(secondaryService.data());
    const QScopedPointer<QLowEnergyService> primaryService(controller->addService(primaryData));
    QVERIFY(!primaryService.isNull());
    QCOMPARE(primaryService->characteristics().size(), 0);
    const QList<QBluetoothUuid> includedServices = primaryService->includedServices();
    QCOMPARE(includedServices.size(), 1);
    QCOMPARE(includedServices.first(), secondaryService->serviceUuid());
}

QTEST_MAIN(TestQLowEnergyControllerGattServer)

#include "tst_qlowenergycontroller-gattserver.moc"
