#include "RaspberryPI.h"

#include <QUdpSocket>
#include <QSettings>
#include <QMqttClient>

#include <QHostInfo>
#include <QNetworkInterface>
#include <QTimer>
#include <QEventLoop>
#include <QRandomGenerator>

RaspberryPI* RaspberryPI::s_inst = nullptr;

RaspberryPI::RaspberryPI()
{
    setup();
}

RaspberryPI* RaspberryPI::getInstance()
{
    if (!s_inst) {
        s_inst = new RaspberryPI();
    }
    return s_inst;
}

/**
 * @brief RaspberryPI::connect
 */
void RaspberryPI::setup()
{
    // Load server information from registry
    QSettings settings("VRT", "CasparCGClient");
    settings.beginGroup("Configuration");
    QString address = settings.value("raspaddress", "127.0.0.1").toString();
    m_address = QHostAddress(address);
    m_portIn = static_cast<unsigned short>(settings.value("raspport_in", "1234").toInt());
    m_portOut = static_cast<unsigned short>(settings.value("raspport_out", "1235").toInt());
    settings.endGroup();

    // Find real LocalHost Address
    QHostAddress localhost;
    foreach (const QHostAddress &address, QNetworkInterface::allAddresses()) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address != QHostAddress(QHostAddress::LocalHost)) {
            localhost = address;
        }
    }
    qDebug() << "Localhost IP:" << localhost.toString();

    // Open incoming UDP socket to Raspberry PI
    if (udpSocketIn == nullptr) {
        udpSocketIn = new QUdpSocket(this);
    }
    if (udpSocketIn->state() != QAbstractSocket::UnconnectedState) {
        udpSocketIn->close();
    }
    udpSocketIn->bind(localhost, m_portIn);

    // Open outgoing UDP socket to Raspberry PI
    if (udpSocketOut == nullptr) {
        udpSocketOut = new QUdpSocket(this);
    }
    if (udpSocketOut->state() != QAbstractSocket::UnconnectedState) {
        udpSocketOut->close();
    }

    // When a datagram has been received, go and process it
    QObject::connect(udpSocketIn, SIGNAL(readyRead()),
            this, SLOT(processPendingDatagrams()), Qt::UniqueConnection);

    // Initialize MQTT client (Step 1: Connection only)
    if (mqttClient == nullptr) {
        mqttClient = new QMqttClient(this);
        mqttClient->setHostname("127.0.0.1");
        mqttClient->setPort(1883);
        mqttClient->setClientId("CuteCaspar");
        
        // Connect to MQTT broker
        qDebug() << "Connecting to MQTT broker at 127.0.0.1:1883...";
        mqttClient->connectToHost();
        
        // Add connection status logging
        QObject::connect(mqttClient, &QMqttClient::connected, [this]() {
            qDebug() << "✅ MQTT connected successfully!";
        });
        
        QObject::connect(mqttClient, &QMqttClient::disconnected, [this]() {
            qDebug() << "❌ MQTT disconnected";
        });
        
        QObject::connect(mqttClient, &QMqttClient::errorChanged, [this](QMqttClient::ClientError error) {
            qDebug() << "MQTT Error:" << error;
        });
    }

    // Report connection
    qDebug() << QString("Raspberry PI connected on %1:%2").arg(address).arg(m_portIn);
}

void RaspberryPI::startConnection()
{
    sendMessage("start");
    m_status.connected = true;
    setButtonActive(true);
}

void RaspberryPI::stopConnection()
{
    sendMessage("quit");
    m_status.connected = false;
    setButtonActive(false);
}

/**
 * STATUS CHECKS
 */

bool RaspberryPI::isConnected()
{
    sendMessage("alive");

    QTimer timer;
    timer.setSingleShot(true);
    QEventLoop loop;
    connect(this, SIGNAL(heartBeat()), &loop, SLOT(quit()));
    connect( &timer, &QTimer::timeout, &loop, &QEventLoop::quit );
    timer.start(250);
    loop.exec();

    m_status.connected = timer.isActive();

    return m_status.connected;
}

bool RaspberryPI::isButtonActive() const
{
    return m_status.buttonActive;
}

bool RaspberryPI::isLightActive() const
{
    return m_status.lightActive;
}

bool RaspberryPI::isMagnetActive() const
{
    return m_status.magnetActive;
}

bool RaspberryPI::isMotionActive() const
{
    return m_status.motionActive;
}

bool RaspberryPI::isSmokeActive() const
{
    return m_status.smokeActive;
}


/**
 * SET/UNSET ACTIONS FOR RASPBERRY PI
 */

void RaspberryPI::setButtonActive(bool buttonActive)
{
    sendMessage(buttonActive ? "on" : "off");
    m_status.buttonActive = buttonActive;
    sendStatus();
}

void RaspberryPI::setLightActive(bool lightActive)
{
    sendMessage(lightActive ? "light_on" : "light_off");
    m_status.lightActive = lightActive;
    sendStatus();
}

void RaspberryPI::setMagnetActive(bool magnetActive, bool overRule)
{
    // Generate a number in de range [1..100]
    int randomNumber = QRandomGenerator::global()->bounded(100) + 1;

    // When random number is smaller or equal to the probability as set by user
    // switch the magnet off is active (this can be overruled)
    if ((randomNumber <= getMagnetProbability() && !magnetActive) || overRule) {
        sendMessage("magnet_off");
        m_status.magnetActive = false;
    } else if (magnetActive) {
        sendMessage("magnet_on");
        m_status.magnetActive = true;
    }
    sendStatus();
}

void RaspberryPI::setMotionActive(bool motionActive)
{
    sendMessage(motionActive ? "motion_off" : "motion_on");
    m_status.motionActive = !motionActive;
    sendStatus();
}

void RaspberryPI::setSmokeActive(bool smokeActive)
{
    sendMessage(smokeActive ? "smoke_on" : "smoke_off");
    m_status.smokeActive = smokeActive;
    sendStatus();
}

void RaspberryPI::setMagnetProbablilty(int probability)
{
    m_status.magnetProbability = probability;
}

int RaspberryPI::getMagnetProbability() const
{
    return m_status.magnetProbability;
}

void RaspberryPI::reboot()
{
    sendMessage("reboot");
}

void RaspberryPI::shutdown()
{
    sendMessage("shutdown");
}


/**
 * @brief RaspberryPI::processPendingDatagrams
 */
void RaspberryPI::processPendingDatagrams()
{
    while (udpSocketIn->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(static_cast<int>(udpSocketIn->pendingDatagramSize()));
        udpSocketIn->readDatagram(datagram.data(), datagram.size());
        parseMessage(&datagram.data()[6]);
    }
}


/**
 * @brief RaspberryPI::sendMessage
 * @param msg
 */
void RaspberryPI::sendMessage(QString msg)
{
    qDebug() << QString("🔄 sendMessage called with: '%1'").arg(msg);
    
    // Always send via UDP (keep existing functionality)
    QByteArray Data;
    Data.append("raspi/");
    Data.append(msg.toUtf8());
    udpSocketOut->writeDatagram(Data, Data.size(), m_address, m_portOut);
    qDebug() << QString("📤 Sending UDP Message '%1' to %2:%3").arg(msg).arg(m_address.toString()).arg(m_portOut);
    
    // Step 3: Send ALL commands via MQTT (not just "start" and "alive")
    if (mqttClient && mqttClient->state() == QMqttClient::Connected) {
        QString topic = "cutecaspar/raspi/command";
        QByteArray mqttMessage = msg.toUtf8();
        mqttClient->publish(QMqttTopicName(topic), mqttMessage, 1); // QoS 1 for reliability
        qDebug() << QString("📡 Published MQTT message '%1' to topic '%2'").arg(msg).arg(topic);
    } else if (mqttClient) {
        qDebug() << QString("⚠️ MQTT client not connected (state: %1), cannot send '%2'").arg(mqttClient->state()).arg(msg);
    } else {
        qDebug() << QString("❌ MQTT client is null, cannot send '%1'").arg(msg);
    }
}


void RaspberryPI::parseMessage(QString msg)
{
    if (msg == "high" && isButtonActive()) {
        qDebug() << "Insert clip" << msg;
        emit insertPlaylist();
        setButtonActive(false);
    } else if (msg == "high2") {
        qDebug() << "Insert doorbell clip";
        emit insertPlaylist("EXTRAS/TRICK OR TREAT", "extras");
        setButtonActive(false);
    } else if (msg == "latch2_closed") {
        qDebug() << "Latch 2 closed";
        m_status.motionActive = false;
        sendStatus();
    } else if (msg == "latch1_closed") {
        qDebug() << "Latch 1 closed";
        m_status.magnetActive = true;
        sendStatus();
    }
    emit statusButton(msg);
    if (msg == "ok") {
        emit heartBeat();
    }
}

void RaspberryPI::insertFinished()
{
    // When inserted clip has finised, release the push button
    setButtonActive(true);
}

void RaspberryPI::sendStatus()
{
    emit statusUpdate(m_status);
}
