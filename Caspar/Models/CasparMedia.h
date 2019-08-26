#ifndef CASPARMEDIA_H
#define CASPARMEDIA_H

#include "Shared.h"

#include <QtCore/QString>

class CASPARSHARED_EXPORT CasparMedia
{
    public:
        explicit CasparMedia(const QString& name, const QString& type, const QString& timecode, const double fps);

        const QString& getName() const;
        const QString& getType() const;
        const QString& getTimecode() const;
        double getFPS() const;

    private:
        QString name;
        QString type;
        QString timecode;
        double fps;
};

#endif // CASPARMEDIA_H
