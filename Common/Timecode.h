#ifndef TIMECODE_H
#define TIMECODE_H

#include <QtCore/QString>
#include <QtCore/QTime>

#include "Share.h"


class COMMONSHARED_EXPORT Timecode
{
    public:
        static QString fromTime(const QTime& time, bool useDropFrameNotation);
        static QString fromTime(double time, double fps, bool useDropFrameNotation);

    private:
        Timecode() {}
};

#endif // TIMECODE_H
