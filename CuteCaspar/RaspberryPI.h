#ifndef RASPBERRYPI_H
#define RASPBERRYPI_H

#include <QObject>
#include <QUdpSocket>

struct status {
    bool connected = false;
    bool buttonActive = false;
    bool lightActive = false;
    bool magnetActive = false;
    bool motionActive = false;
    bool smokeActive = false;
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
    bool isConnected();
    bool isButtonActive() const;
    bool isLightActive() const;
    bool isMagnetActive() const;
    bool isMotionActive() const;
    bool isSmokeActive() const;
    void setButtonActive(bool buttonActive);
    void setLightActive(bool lightActive);
    void setMagnetActive(bool magnetActive);
    void setMotionActive(bool motionActive);
    void setSmokeActive(bool smokeActive);
    void reboot();
    void shutdown();

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
    status m_status;
    void parseMessage(QString msg);
    void sendStatus();
};

#endif // RASPBERRYPI_H
