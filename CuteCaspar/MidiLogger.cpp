#include "MidiLogger.h"

#include "MidiReader.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>

MidiLogger::MidiLogger()
{
}

void MidiLogger::openMidiLog(QString videoName)
{
    closeMidiLog();

    const QString filePath = MidiReader::midiFilePath(videoName);
    if (!QDir().mkpath(QFileInfo(filePath).absolutePath())) {
        qCritical() << "Unable to create MIDI directory for" << filePath;
        m_ready = false;
        return;
    }

    logFile.setFileName(filePath);
    if (!logFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCritical() << "Unable to open MIDI file" << filePath << logFile.errorString();
        m_ready = false;
        return;
    }

    qDebug() << "Ready to write to" << filePath;
    out.setDevice(&logFile);
    m_ready = true;
}

void MidiLogger::closeMidiLog()
{
    if (m_ready) {
        qDebug() << "Closing log";
        out.flush();
        out.setDevice(nullptr);
        logFile.close();
        m_ready = false;
    }
}

bool MidiLogger::isReady() const
{
    return m_ready;
}

void MidiLogger::writeNote(QString note)
{
    if (!m_ready) {
        qWarning() << "Cannot write MIDI note: no MIDI file is open";
        return;
    }
    qDebug() << "Write note" << note;
    out << note << "\n";
}
