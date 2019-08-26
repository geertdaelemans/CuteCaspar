#ifndef AMCPDEVICE_H
#define AMCPDEVICE_H

#include "Shared.h"

#include <QtCore/QObject>
#include <QAbstractSocket>

class QObject;
class QTcpSocket;
class QTextDecoder;

class CASPARSHARED_EXPORT AmcpDevice : public QObject
{
    Q_OBJECT

    public:
        explicit AmcpDevice(const QString& address, int port, QObject* parent = nullptr);
        virtual ~AmcpDevice();

        void disconnectDevice();

        void setDisableCommands(bool disable);

        bool isConnected() const;
        int getPort() const;
        const QString& getAddress() const;
        QAbstractSocket::SocketState getState() const;

        Q_SLOT void connectDevice();

    protected:
        enum class AmcpDeviceCommand
        {
            NONE,
            CONNECTIONSTATE,
            LOAD,
            LOADBG,
            PLAY,
            STOP,
            CG,
            CLS,
            CINF,
            VERSION,
            TLS,
            INFO,
            INFOSYSTEM,
            DATALIST,
            DATARETRIEVE,
            CLEAR,
            SET,
            MIXER,
            CALL,
            REMOVE,
            ADD,
            SWAP,
            STATUS,
            ERROR,
            THUMBNAILLIST,
            THUMBNAILRETRIEVE
        };

        QTcpSocket* socket = nullptr;
        AmcpDeviceCommand command = AmcpDeviceCommand::NONE;

        QList<QString> response;

        virtual void sendNotification() = 0;

        void resetDevice();
        void writeMessage(const QString& message);

    private:
        enum class AmcpDeviceParserState
        {
            ExpectingHeader,
            ExpectingOneline,
            ExpectingTwoline,
            ExpectingMultiline
        };

        QString address;

        int port;
        int code;

        bool connected = false;
        bool disableCommands = false;

        QString fragments;

        QTextDecoder* decoder = nullptr;

        AmcpDeviceParserState state = AmcpDeviceParserState::ExpectingHeader;

        void parseLine(const QString& line);
        void parseHeader(const QString& line);
        void parseOneline(const QString& line);
        void parseTwoline(const QString& line);
        void parseMultiline(const QString& line);

        AmcpDeviceCommand translateCommand(const QString& command);

        Q_SLOT void readMessage();
        Q_SLOT void setConnected();
        Q_SLOT void setDisconnected();
};


#endif // AMCPDEVICE_H
