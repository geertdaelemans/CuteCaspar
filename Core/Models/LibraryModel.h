#ifndef LIBRARYMODEL_H
#define LIBRARYMODEL_H

#include "Shared.h"

#include <QtCore/QObject>
#include <QtCore/QString>

class CORESHARED_EXPORT LibraryModel
{
    public:
        explicit LibraryModel() { }
        explicit LibraryModel(int id, const QString& label, const QString& name, const QString& deviceName, const QString& type, int thumbnailId, const QString& timecode, const double fps, const int midi);

        int getId() const;
        const QString& getLabel() const;
        const QString& getName() const;
        const QString& getDeviceName() const;
        const QString& getType() const;
        int getThumbnailId() const;
        const QString& getTimecode() const;
        double getFPS() const;
        int getMidi() const;

        void setLabel(const QString& label);
        void setName(const QString& name);
        void setDeviceName(const QString& deviceName);
        void setTimecode(const QString& timecode);
        void setMidi(const int midi);

private:
        int id;
        QString label;
        QString name;
        QString deviceName;
        QString type;
        int thumbnailId;
        QString timecode;
        double fps;
        int midi;
};

#endif // LIBRARYMODEL_H
