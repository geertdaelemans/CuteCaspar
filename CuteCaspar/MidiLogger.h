#ifndef MIDILOGGER_H
#define MIDILOGGER_H

#include <QtCore>

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
    QString m_videoName;
    QFile* logFile;
    QTextStream* out;
};

#endif // MIDILOGGER_H
