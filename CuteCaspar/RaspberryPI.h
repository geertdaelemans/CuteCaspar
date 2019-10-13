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

private slots:
    void processPendingDatagrams();

private:
    static RaspberryPI* s_inst;
    QUdpSocket* udpSocket;
};

#endif // RASPBERRYPI_H
