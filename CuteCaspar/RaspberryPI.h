#ifndef RASPBERRYPI_H
#define RASPBERRYPI_H

#include <QObject>
#include <QUdpSocket>

class RaspberryPI : public QObject
{

    Q_OBJECT

public:
    RaspberryPI();
    static RaspberryPI *getInstance();
    void connect();

signals:
    void statusButton(QString msg);

public slots:
    void sendMessage(QString msg);

private slots:
    void processPendingDatagrams();

private:
    static RaspberryPI* s_inst;
    QUdpSocket* udpSocketIn = nullptr;
    QUdpSocket* udpSocketOut = nullptr;
    QHostAddress m_address = QHostAddress("127.0.0.1");
    unsigned short m_portIn = 1234;
    unsigned short m_portOut = 1235;
    void parseMessage(QString msg);
};

#endif // RASPBERRYPI_H
