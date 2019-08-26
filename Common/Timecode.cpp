#include "Timecode.h"

#include <QtCore/QString>

QString Timecode::fromTime(const QTime& time, bool useDropFrameNotation)
{
    QString result = time.toString("hh:mm:ss").append((useDropFrameNotation == true) ? "." : ":");

    int msec = time.msec() / 10;
    if (msec < 10)
        result.append("0");

    return result.append(QString("%1").arg(msec));
}

QString Timecode::fromTime(double time, double fps, bool useDropFrameNotation)
{
    int hour;
    int minutes;
    int seconds;
    int frames;

    QString smpteFormat;

    hour = static_cast<int>(time / 3600);
    minutes = static_cast<int>((time - hour * 3600) / 60);
    seconds = static_cast<int>(time - hour * 3600 - minutes * 60);
    frames = static_cast<int>((time - hour * 3600 - minutes * 60 - seconds) * fps);

    if (useDropFrameNotation)
        return smpteFormat.sprintf("%02d:%02d:%02d.%02d", hour, minutes, seconds, frames);
    else
        return smpteFormat.sprintf("%02d:%02d:%02d:%02d", hour, minutes, seconds, frames);
}
