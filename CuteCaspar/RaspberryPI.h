#ifndef RASPBERRYPI_H
#define RASPBERRYPI_H

#include <QObject>
#include <QUdpSocket>
#include <QMqttClient>

struct status {
    bool connected = false;
    bool buttonActive = false;
    bool lightActive = false;
    bool magnetActive = false;
    bool motionActive = false;
    bool smokeActive = false;
    int magnetProbability = 50;
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
    void setMagnetActive(bool magnetActive, bool overRule = false);
    void setMotionActive(bool motionActive);
    void setSmokeActive(bool smokeActive);
    void setMagnetProbablilty(int probability);
    int getMagnetProbability() const;
    void reboot();
    void shutdown();
    
    // MQTT Configuration Methods
    bool isMqttEnabled() const;
    void setMqttEnabled(bool enabled);
    QString getMqttBrokerHost() const;
    void setMqttBrokerHost(const QString &host);
    quint16 getMqttBrokerPort() const;
    void setMqttBrokerPort(quint16 port);
    QString getMqttClientId() const;
    void setMqttClientId(const QString &clientId);
    QString getMqttTopicPrefix() const;
    void setMqttTopicPrefix(const QString &prefix);
    bool isMqttConnected() const;

    // Helper to publish arbitrary MQTT messages
    void publishMqtt(const QString& topic, const QString& payload, int qos = 1);

signals:
    void statusButton(QString msg);
    void statusUpdate(status stat);
    void insertPlaylist(QString clipName = "random", QString database = "scares");
    void heartBeat();

public slots:
    void sendMessage(QString msg);
    void insertFinished();
    void parseMessage(QString msg);
    void handleMqttMessage(const QString& topic, const QString& payload);

private slots:
    void processPendingDatagrams();

private:
    static RaspberryPI* s_inst;
    QUdpSocket* udpSocketIn = nullptr;
    QUdpSocket* udpSocketOut = nullptr;
    QMqttClient* mqttClient = nullptr;
    QHostAddress m_address = QHostAddress("127.0.0.1");
    unsigned short m_portIn = 1234;
    unsigned short m_portOut = 1235;
    // MQTT Configuration
    bool m_mqttEnabled = true;
    QString m_mqttBrokerHost = "192.168.0.220";
    quint16 m_mqttBrokerPort = 1883;
    QString m_mqttClientId = "CuteCaspar";
    QString m_mqttTopicPrefix = "cutecaspar/raspi";
    status m_status;
    void sendStatus();    
};

#endif // RASPBERRYPI_H
