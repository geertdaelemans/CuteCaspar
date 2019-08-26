#ifndef CASPAROSCLISTENER_H
#define CASPAROSCLISTENER_H

#include <QDebug>

#include <osc/OscReceivedElements.h>
#include <osc/OscPacketListener.h>
#include <ip/UdpSocket.h>

class CasparOscListener : public QObject, public osc::OscPacketListener
{
    Q_OBJECT
signals:
    void messageAvailable(QStringList address, QStringList values);
protected:
    virtual void ProcessMessage( const osc::ReceivedMessage& m,
                                 const IpEndpointName& remoteEndpoint );
};

#endif // CASPAROSCLISTENER_H
