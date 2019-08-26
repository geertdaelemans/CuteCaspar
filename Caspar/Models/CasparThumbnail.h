#ifndef CASPARTHUMBNAIL_H
#define CASPARTHUMBNAIL_H

#include "Shared.h"

#include <QtCore/QString>

class CASPARSHARED_EXPORT CasparThumbnail
{
    public:
        explicit CasparThumbnail(const QString& name, const QString& timestamp, const QString& size);

        const QString& getName() const;
        const QString& getTimestamp() const;
        const QString& getSize() const;

    private:
        QString name;
        QString timestamp;
        QString size;
};

#endif // CASPARTHUMBNAIL_H
