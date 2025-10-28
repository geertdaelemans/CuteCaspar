#include "RaspberryPI.h"

#include <QUdpSocket>
#include <QSettings>
#include <QMqttClient>
#include <QMqttSubscription>

#include <QHostInfo>
#include <QNetworkInterface>
#include <QTimer>
#include <QEventLoop>
#include <QRandomGenerator>

RaspberryPI *RaspberryPI::s_inst = nullptr;

RaspberryPI::RaspberryPI()
{
    setup();
}

RaspberryPI *RaspberryPI::getInstance()
{
    if (!s_inst)
    {
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

    // Load MQTT Configuration from settings
    m_mqttEnabled = settings.value("mqtt_enabled", true).toBool();
    m_mqttBrokerHost = settings.value("mqtt_broker_host", "192.168.0.220").toString();
    m_mqttBrokerPort = static_cast<quint16>(settings.value("mqtt_broker_port", 1883).toInt());
    m_mqttClientId = settings.value("mqtt_client_id", "CuteCaspar").toString();
    m_mqttTopicPrefix = settings.value("mqtt_topic_prefix", "cutecaspar/raspi").toString();
    settings.endGroup();

    // Find real LocalHost Address
    QHostAddress localhost;
    foreach (const QHostAddress &address, QNetworkInterface::allAddresses())
    {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address != QHostAddress(QHostAddress::LocalHost))
        {
            localhost = address;
        }
    }
    qDebug() << "Localhost IP:" << localhost.toString();

    // Open incoming UDP socket to Raspberry PI
    if (udpSocketIn == nullptr)
    {
        udpSocketIn = new QUdpSocket(this);
    }
    if (udpSocketIn->state() != QAbstractSocket::UnconnectedState)
    {
        udpSocketIn->close();
    }
    udpSocketIn->bind(localhost, m_portIn);

    // Open outgoing UDP socket to Raspberry PI
    if (udpSocketOut == nullptr)
    {
        udpSocketOut = new QUdpSocket(this);
    }
    if (udpSocketOut->state() != QAbstractSocket::UnconnectedState)
    {
        udpSocketOut->close();
    }

    // When a datagram has been received, go and process it
    QObject::connect(udpSocketIn, SIGNAL(readyRead()),
                     this, SLOT(processPendingDatagrams()), Qt::UniqueConnection);

    // Initialize MQTT client (Step 5: Using configurable settings)
    if (m_mqttEnabled && mqttClient == nullptr)
    {
        mqttClient = new QMqttClient(this);
        mqttClient->setHostname(m_mqttBrokerHost);
        mqttClient->setPort(m_mqttBrokerPort);
        mqttClient->setClientId(m_mqttClientId);

        // Connect to MQTT broker
        qDebug() << QString("Connecting to MQTT broker at %1:%2 (Client ID: %3)...")
                        .arg(m_mqttBrokerHost)
                        .arg(m_mqttBrokerPort)
                        .arg(m_mqttClientId);
        mqttClient->connectToHost();

        // Add connection status logging and subscription
        QObject::connect(mqttClient, &QMqttClient::connected, [this]()
                         {
                             qDebug() << "✅ MQTT connected successfully!";

                             // Subscribe to status topic for incoming messages
                             QString statusTopic = QString("%1/status").arg(m_mqttTopicPrefix);
                             QMqttSubscription *rpiStatusSubscription = mqttClient->subscribe(statusTopic, 1); // QoS 1

                             if (rpiStatusSubscription)
                             {
                                 qDebug() << QString("Subscribed to MQTT topic: %1").arg(statusTopic);
                                 QObject::connect(rpiStatusSubscription, &QMqttSubscription::messageReceived,
                                                  [this](const QMqttMessage &mqttMessage)
                                                  {
                                                      handleMqttMessage(mqttMessage.topic().name(), QString::fromUtf8(mqttMessage.payload()));
                                                  });
                             }
                             else
                             {
                                 qDebug() << "❌ Failed to subscribe to MQTT status topic";
                             }

                             // Subscribe to doorbell/button topic for direct doorbell events
                             QString doorbellTopic = "doorbell/button";
                             QMqttSubscription *doorbellSubscription = mqttClient->subscribe(doorbellTopic, 1);

                             if (doorbellSubscription)
                             {
                                 qDebug() << QString("Subscribed to MQTT topic: %1").arg(doorbellTopic);
                                 QObject::connect(doorbellSubscription, &QMqttSubscription::messageReceived,
                                                  [this](const QMqttMessage &mqttMessage)
                                                  {
                                                      handleMqttMessage(mqttMessage.topic().name(), QString::fromUtf8(mqttMessage.payload()));
                                                  });
                             }
                             else
                             {
                                 qDebug() << "❌ Failed to subscribe to MQTT doorbell topic";
                             }
                         });

        QObject::connect(mqttClient, &QMqttClient::disconnected, [this]()
                         { qDebug() << "❌ MQTT disconnected"; });

        QObject::connect(mqttClient, &QMqttClient::errorChanged, [this](QMqttClient::ClientError error)
                         { qDebug() << "MQTT Error:" << error; });
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
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
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
    // open the latch if not active (this can be overruled)
    if ((randomNumber <= getMagnetProbability() && !magnetActive) || overRule)
    {
        sendMessage("latch_open"); // Temporarily open the latch
        m_status.magnetActive = false;
    }
    else if (magnetActive)
    {
        sendMessage("latch_close"); // Manually close the latch
        m_status.magnetActive = true;
    }
    sendStatus();
}

void RaspberryPI::setMotionActive(bool motionActive)
{
    sendMessage(motionActive ? "motion_on" : "motion_off");
    m_status.motionActive = motionActive;
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
    while (udpSocketIn->hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(static_cast<int>(udpSocketIn->pendingDatagramSize()));
        udpSocketIn->readDatagram(datagram.data(), datagram.size());
        parseMessage(&datagram.data()[6]);
    }
}

/**
 * @brief RaspberryPI::publishMqtt
 * @param topic - the MQTT topic
 * @param payload - the message payload
 * @param qos - the Quality of Service level
 */

void RaspberryPI::publishMqtt(const QString& topic, const QString& payload, int qos)
{
    if (m_mqttEnabled && mqttClient && mqttClient->state() == QMqttClient::Connected) {
        mqttClient->publish(QMqttTopicName(topic), payload.toUtf8(), qos);
        qDebug() << QString("📡 Published MQTT message '%1' to topic '%2'").arg(payload).arg(topic);
    } else {
        qDebug() << "❌ MQTT not connected, cannot publish to" << topic;
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
    if (m_mqttEnabled && mqttClient && mqttClient->state() == QMqttClient::Connected)
    {
        QString topic = QString("%1/command").arg(m_mqttTopicPrefix);
        QByteArray mqttMessage = msg.toUtf8();
        mqttClient->publish(QMqttTopicName(topic), mqttMessage, 1); // QoS 1 for reliability
        qDebug() << QString("📡 Published MQTT message '%1' to topic '%2'").arg(msg).arg(topic);
    }
    else if (m_mqttEnabled && mqttClient)
    {
        qDebug() << QString("⚠️ MQTT client not connected (state: %1), cannot send '%2'").arg(mqttClient->state()).arg(msg);
    }
    else if (m_mqttEnabled)
    {
        qDebug() << QString("❌ MQTT client is null, cannot send '%1'").arg(msg);
    }
}

void RaspberryPI::parseMessage(QString msg)
{
    if (msg == "high" && isButtonActive())
    {
        qDebug() << "Insert clip" << msg;
        emit insertPlaylist();
        setButtonActive(false);
    }
    else if (msg == "high2")
    {
        qDebug() << "Insert doorbell clip";
        emit insertPlaylist("EXTRAS/TRICK OR TREAT", "extras");
        setButtonActive(false);
    }
    else if (msg == "latch2_closed")
    {
        qDebug() << "Latch 2 closed";
        m_status.motionActive = false;
        sendStatus();
    }
    else if (msg == "latch1_closed")
    {
        qDebug() << "Latch 1 closed";
        m_status.magnetActive = true;
        sendStatus();
    }
    emit statusButton(msg);
    if (msg == "ok")
    {
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

// MQTT Configuration Methods Implementation

bool RaspberryPI::isMqttEnabled() const
{
    return m_mqttEnabled;
}

void RaspberryPI::setMqttEnabled(bool enabled)
{
    if (m_mqttEnabled != enabled)
    {
        m_mqttEnabled = enabled;

        // Save to settings
        QSettings settings("VRT", "CasparCGClient");
        settings.beginGroup("Configuration");
        settings.setValue("mqtt_enabled", enabled);
        settings.endGroup();

        qDebug() << QString("MQTT %1").arg(enabled ? "enabled" : "disabled");

        // Note: Restart required for changes to take effect
        qDebug() << "⚠️ Restart CuteCaspar for MQTT settings to take effect";
    }
}

QString RaspberryPI::getMqttBrokerHost() const
{
    return m_mqttBrokerHost;
}

void RaspberryPI::setMqttBrokerHost(const QString &host)
{
    if (m_mqttBrokerHost != host)
    {
        m_mqttBrokerHost = host;

        // Save to settings
        QSettings settings("VRT", "CasparCGClient");
        settings.beginGroup("Configuration");
        settings.setValue("mqtt_broker_host", host);
        settings.endGroup();

        qDebug() << QString("MQTT broker host set to: %1").arg(host);
        qDebug() << "⚠️ Restart CuteCaspar for MQTT settings to take effect";
    }
}

quint16 RaspberryPI::getMqttBrokerPort() const
{
    return m_mqttBrokerPort;
}

void RaspberryPI::setMqttBrokerPort(quint16 port)
{
    if (m_mqttBrokerPort != port)
    {
        m_mqttBrokerPort = port;

        // Save to settings
        QSettings settings("VRT", "CasparCGClient");
        settings.beginGroup("Configuration");
        settings.setValue("mqtt_broker_port", port);
        settings.endGroup();

        qDebug() << QString("MQTT broker port set to: %1").arg(port);
        qDebug() << "⚠️ Restart CuteCaspar for MQTT settings to take effect";
    }
}

QString RaspberryPI::getMqttClientId() const
{
    return m_mqttClientId;
}

void RaspberryPI::setMqttClientId(const QString &clientId)
{
    if (m_mqttClientId != clientId)
    {
        m_mqttClientId = clientId;

        // Save to settings
        QSettings settings("VRT", "CasparCGClient");
        settings.beginGroup("Configuration");
        settings.setValue("mqtt_client_id", clientId);
        settings.endGroup();

        qDebug() << QString("MQTT client ID set to: %1").arg(clientId);
        qDebug() << "⚠️ Restart CuteCaspar for MQTT settings to take effect";
    }
}

QString RaspberryPI::getMqttTopicPrefix() const
{
    return m_mqttTopicPrefix;
}

void RaspberryPI::setMqttTopicPrefix(const QString &prefix)
{
    if (m_mqttTopicPrefix != prefix)
    {
        m_mqttTopicPrefix = prefix;

        // Save to settings
        QSettings settings("VRT", "CasparCGClient");
        settings.beginGroup("Configuration");
        settings.setValue("mqtt_topic_prefix", prefix);
        settings.endGroup();

        qDebug() << QString("MQTT topic prefix set to: %1").arg(prefix);
        qDebug() << QString("Command topic: %1/command").arg(prefix);
        qDebug() << QString("Status topic: %1/status").arg(prefix);
        qDebug() << "⚠️ Restart CuteCaspar for MQTT settings to take effect";
    }
}

bool RaspberryPI::isMqttConnected() const
{
    return mqttClient && mqttClient->state() == QMqttClient::Connected;
}

void RaspberryPI::handleMqttMessage(const QString &topic, const QString &payload)
{
    qDebug() << QString("📨 Received MQTT message on topic '%1': %2").arg(topic).arg(payload);
    if (topic.endsWith("doorbell/button"))
    {
        if (payload == "1")
        {
            parseMessage("high2");
        }
        else if (payload == "0")
        {
            parseMessage("low2");
        }
    }
    else
    {
        // For backward compatibility, pass other messages to parseMessage
        parseMessage(payload);
    }
    // Add more topic/payload handling here as needed
}
