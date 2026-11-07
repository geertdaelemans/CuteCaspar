#include "MidiReader.h"

#include <QDir>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>
#include <QTextStream>

MidiReader::MidiReader()
{
    m_ready = false;
}

QString MidiReader::midiDirectory()
{
    const QString defaultDirectory = QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
                                        .filePath("MIDI");
    QSettings settings("VRT", "CasparCGClient");
    settings.beginGroup("Configuration");
    const QString configuredDirectory = settings.value("midi_directory", defaultDirectory).toString().trimmed();
    return configuredDirectory.isEmpty() ? defaultDirectory : configuredDirectory;
}

QString MidiReader::midiFilePath(QString videoFile)
{
    videoFile.replace("/", "-");
    return QDir(midiDirectory()).filePath(QString("%1.midi").arg(videoFile));
}

QMap<QString, message> MidiReader::openLog(QString videoFile)
{
    m_ready = false;
    QMap<QString, message> output;
    QFile logFile(midiFilePath(videoFile));
    if (logFile.open(QIODevice::ReadOnly)) {
       QTextStream in(&logFile);
       while (!in.atEnd()) {
          QString line = in.readLine();
          QStringList list = line.split(",");
          if (list.size() < 3)
              continue;
          message message;
          message.timeCode = list.at(0);
          message.type = list.at(1);
          message.pitch = list.at(2).toUInt();
          output[list.at(0)] = message;
       }
       logFile.close();
       if (output.size() != 0) {
            m_ready = true;
       }
    }
    return output;
}

bool MidiReader::isReady() const
{
    return m_ready;
}
