#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QMutex>
#include <QObject>

#include "Models/DeviceModel.h"
#include "Models/LibraryModel.h"
#include "Models/ClipInfo.h"


class CORESHARED_EXPORT DatabaseManager : public QObject
{

    Q_OBJECT

public:
    explicit DatabaseManager();

    static DatabaseManager* getInstance();
    void initializeDatabase();
    void reset();

    // Functions for handling CasparCG devices
    QList<DeviceModel> getDevice();
    DeviceModel getDeviceById(int deviceId);
    DeviceModel getDeviceByName(const QString &name);
    void insertDevice(const DeviceModel &model);
    void updateDevice(const DeviceModel &model);
    void deleteDevice(int id);

    // Data functions for handling media clips
    void updateLibraryMedia(const QList<LibraryModel>& insertModels);
    void copyClipsTo(QList<int> clipIds, QString tableName);
    void removeClipsFromList(QList<int> clipIds, QString tableName);
    int reorderClips(QList<int> from, int to, QString tableName);
    void emptyList(QString tableName);
    void updateMidiStatus(QString clipName, int midiNotes);
    int getNumberOfClips(QString playlist) const;
    ClipInfo getClipInfo(QString clipName, QString tableName);

private:
    QMutex mutex;
    void createDatabase();
    void deleteDatabase();
    void upgradeDatabase();

signals:
    void databaseUpdated(QString table);
};

#endif // DATABASEMANAGER_H
