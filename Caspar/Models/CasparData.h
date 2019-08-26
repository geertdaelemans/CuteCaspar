#ifndef CASPARDATA_H
#define CASPARDATA_H

#include "Shared.h"

#include <QtCore/QString>

class CASPARSHARED_EXPORT CasparData
{
    public:
        explicit CasparData(const QString& name);

        const QString& getName() const;

    private:
        QString name;
};

#endif // CASPARDATA_H
