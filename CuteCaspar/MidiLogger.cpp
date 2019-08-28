#include "MidiLogger.h"

#include <QtDebug>

MidiLogger::MidiLogger()
{
}

void MidiLogger::openMidiLog(QString videoName)
{
    qDebug() << "Ready to write to" << videoName;
    logFile = new QFile(QString("%1.midi").arg(videoName));
    logFile->open(QIODevice::WriteOnly | QIODevice::Text);
    out = new QTextStream(logFile);
    m_ready = true;
}

void MidiLogger::closeMidiLog()
{
    qDebug() << "Closing log";
    logFile->close();
    m_ready = false;
}

bool MidiLogger::isReady() const
{
    return m_ready;
}

void MidiLogger::writeNote(QString note)
{
    qDebug() << "Write note" << note;
    *out << note << "\n";
}
