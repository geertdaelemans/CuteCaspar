#ifndef MIDIREADER_H
#define MIDIREADER_H

#include <QObject>

struct message {
    QString timeCode;
    QString type;
    unsigned int pitch;
};

class MidiReader
{
public:
    MidiReader();
    QMap<QString, message> openLog(QString videoFile);
    bool isReady() const;

private:
    bool m_ready = false;
};

#endif // MIDIREADER_H
