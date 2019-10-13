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
    unsigned short port = static_cast<unsigned short>(settings.value("raspport", "1234").toInt());
    settings.endGroup();

    // Open UDP socket to Raspberry PI
    if (udpSocket == nullptr) {
        udpSocket = new QUdpSocket(this);
    }
    if (udpSocket->state() != QAbstractSocket::UnconnectedState) {
        udpSocket->close();
    }
    udpSocket->bind(QHostAddress(address), port);

    // When a datagram has been received, go and process it
    QObject::connect(udpSocket, SIGNAL(readyRead()),
            this, SLOT(processPendingDatagrams()), Qt::UniqueConnection);

    // Report connection
    qDebug() << QString("Raspberry PI connected on %1:%2").arg(address).arg(port);
}


/**
 * @brief RaspberryPI::processPendingDatagrams
 */
void RaspberryPI::processPendingDatagrams()
{
    while (udpSocket->hasPendingDatagrams())
    {
        QByteArray datagram;

        datagram.resize(static_cast<int>(udpSocket->pendingDatagramSize()));

        udpSocket->readDatagram(datagram.data(), datagram.size());

        qDebug() << QString("Received datagram: %1").arg(datagram.data());
    }
}
