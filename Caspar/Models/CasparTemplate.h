#ifndef CASPARTEMPLATE_H
#define CASPARTEMPLATE_H

#include "Shared.h"

#include <QtCore/QString>

class CASPARSHARED_EXPORT CasparTemplate
{
    public:
        explicit CasparTemplate(const QString& name);

        const QString& getName() const;

    private:
        QString name;
};

#endif // CASPARTEMPLATE_H
