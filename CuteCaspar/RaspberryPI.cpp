#include "RaspberryPI.h"

#include <QUdpSocket>
#include <QSettings>

RaspberryPI* RaspberryPI::s_inst = nullptr;

RaspberryPI::RaspberryPI()
{
    connect();
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
void RaspberryPI::connect()
{
    // Load server information from registry
    QSettings settings("VRT", "CasparCGClient");
    settings.beginGroup("Configuration");
    QString address = settings.value("raspaddress", "127.0.0.1").toString();
    m_address = QHostAddress(address);
    m_portIn = static_cast<unsigned short>(settings.value("raspport_in", "1234").toInt());
    m_portOut = static_cast<unsigned short>(settings.value("raspport_out", "1235").toInt());
    settings.endGroup();

    // Open UDP socket to Raspberry PI
    if (udpSocketIn == nullptr) {
        udpSocketIn = new QUdpSocket(this);
    }
    if (udpSocketIn->state() != QAbstractSocket::UnconnectedState) {
        udpSocketIn->close();
    }
    udpSocketIn->bind(QHostAddress("192.168.0.184"), m_portIn);


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

        parseMessage(datagram.data());
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
    emit statusButton(msg);
}
