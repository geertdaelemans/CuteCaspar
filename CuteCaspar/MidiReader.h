#ifndef MIDIREADER_H
#define MIDIREADER_H

#include <QObject>

class MidiReader
{
public:
    MidiReader();
    void openLog(QString videoFile);
    bool isReady() const;

private:
    bool m_ready = false;
};

#endif // MIDIREADER_H
