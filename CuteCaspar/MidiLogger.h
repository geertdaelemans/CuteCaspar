#ifndef MIDILOGGER_H
#define MIDILOGGER_H

#include <QFile>
#include <QTextStream>

class MidiLogger
{
public:
    MidiLogger();
    void openMidiLog(QString videoName);
    void closeMidiLog();
    bool isReady() const;
    void writeNote(QString note);

private:
    bool m_ready = false;
    QFile logFile;
    QTextStream out;
};

#endif // MIDILOGGER_H
