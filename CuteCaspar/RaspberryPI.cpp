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
    m_connected = true;
    setButtonActive(true);
}

void RaspberryPI::stopConnection()
{
    sendMessage("quit");
    m_connected = false;
    setButtonActive(false);
}

bool RaspberryPI::isButtonActive() const
{
    return m_buttonActive;
}

bool RaspberryPI::isMagnetActive() const
{
    return m_magnetActive;
}

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

    m_connected = timer.isActive();

    return m_connected;
}

void RaspberryPI::setButtonActive(bool buttonActive)
{
    if (buttonActive) {
        sendMessage("on");
        m_buttonActive = true;
    } else {
        sendMessage("off");
        m_buttonActive = false;
    }
    sendStatus();
}

void RaspberryPI::setMagnetActive(bool magnetActive)
{
    if (magnetActive) {
        sendMessage("magnet_on");
        m_magnetActive = true;
    } else {
        sendMessage("magnet_off");
        m_magnetActive = false;
    }
    sendStatus();
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
    qDebug() << "Insert finished()";
    setButtonActive(true);
}

void RaspberryPI::sendStatus()
{
    status stat;
    stat.connected = m_connected;
    stat.buttonActive = m_buttonActive;
    stat.magnetActive = m_magnetActive;
    emit statusUpdate(stat);
}
