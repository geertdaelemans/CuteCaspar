#include "CasparOSCListener.h"

void CasparOscListener::ProcessMessage( const osc::ReceivedMessage& m,
        const IpEndpointName& remoteEndpoint )
{
    (void)remoteEndpoint; // suppress unused parameter warning

    try {
        QStringList address = QString(m.AddressPattern()+1).split("/");
        if (address[2] != "mixer") // exclude mixer messages
        {
            QStringList list;
            osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
            for (int i = 0; i < static_cast<int>(m.ArgumentCount()); i++)
            {
                QString types = m.TypeTags();
                if (types[i] == 'f')
                    list.append(QString::number(static_cast<double>((arg++)->AsFloat())));
                else if (types[i] == 'h')
                    list.append(QString::number((arg++)->AsInt64()));
                else if (types[i] == 's')
                    list.append((arg++)->AsString());
                else
                    arg++;
            }
            emit messageAvailable(address, list);
        }
    } catch( osc::Exception& e ){
        qDebug() << "error while parsing message: " << m.AddressPattern() << ": " << e.what();
    }
}
