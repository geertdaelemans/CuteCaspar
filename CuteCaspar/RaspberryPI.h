#ifndef RASPBERRYPI_H
#define RASPBERRYPI_H

#include <QObject>
#include <QUdpSocket>

struct status {
    bool buttonActive = false;
    bool magnetActive = false;
    bool lightActive = false;
    bool connected = false;
};

class RaspberryPI : public QObject
{

    Q_OBJECT

public:
    RaspberryPI();
    static RaspberryPI *getInstance();
    void setup();
    void startConnection();
    void stopConnection();
    bool isButtonActive() const;
    bool isMagnetActive() const;
    bool isLightActive() const;
    void setButtonActive(bool buttonActive);
    void setMagnetActive(bool magnetActive);
    void setLightActive(bool lightActive);
    bool isConnected();

signals:
    void statusButton(QString msg);
    void statusUpdate(status stat);
    void insertPlaylist(QString clipName = "random");
    void heartBeat();

public slots:
    void sendMessage(QString msg);
    void insertFinished();

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
    bool m_buttonActive = false;
    bool m_magnetActive = false;
    bool m_lightActive = false;
    bool m_connected = false;
    void sendStatus();
};

#endif // RASPBERRYPI_H
