#include "MidiReader.h"

#include "QFile"
#include "QTextStream"
#include "QDebug"

MidiReader::MidiReader()
{
    m_ready = false;
}

QMap<QString, message> MidiReader::openLog(QString videoFile)
{
    m_ready = false;
    QMap<QString, message> output;
    QFile logFile(QString("%1.midi").arg(videoFile));
    if (logFile.open(QIODevice::ReadOnly))
    {
       QTextStream in(&logFile);
       while (!in.atEnd())
       {
          QString line = in.readLine();
          QStringList list = line.split(",");
          message message;
          message.timeCode = list.at(0);
          message.type = list.at(1);
          message.pitch = list.at(2).toUInt();
          output[list.at(0)] = message;
       }
       logFile.close();
       m_ready = true;
    }
    return output;
}

bool MidiReader::isReady() const
{
    return m_ready;
}
