#include "MidiReader.h"

#include "QFile"
#include "QTextStream"
#include "QDebug"

MidiReader::MidiReader()
{
    m_ready = false;
}

void MidiReader::openLog(QString videoFile)
{
    m_ready = false;
    QFile logFile(QString("%1.midi").arg(videoFile));
    if (logFile.open(QIODevice::ReadOnly))
    {
       QTextStream in(&logFile);
       while (!in.atEnd())
       {
          QString line = in.readLine();
          qDebug() << line;
       }
       logFile.close();
       m_ready = true;
    }
}

bool MidiReader::isReady() const
{
    return m_ready;
}
