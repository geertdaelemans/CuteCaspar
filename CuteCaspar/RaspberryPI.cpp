#include "RaspberryPI.h"

#include <QUdpSocket>
#include <QSettings>

#include <QHostInfo>
#include <QNetworkInterface>
#include <QTimer>
#include <QEventLoop>

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

void RaspberryPI::setMagnetActive(bool magnetActive)
{
    sendMessage(magnetActive ? "magnet_on" : "magnet_off");
    m_status.magnetActive = magnetActive;
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
    QByteArray Data;
    Data.append("raspi/");
    Data.append(msg);
    udpSocketOut->writeDatagram(Data, Data.size(), m_address, m_portOut);
    qDebug() << QString("Sending UDP Message '%1' to %2:%3").arg(msg).arg(m_address.toString()).arg(m_portOut);
}


void RaspberryPI::parseMessage(QString msg)
{
    if (msg == "high" && isButtonActive()) {
        qDebug() << "Insert clip" << msg;
        emit insertPlaylist();
        setButtonActive(false);
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
