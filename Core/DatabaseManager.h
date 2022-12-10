#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QMutex>
#include <QObject>

#include "Models/DeviceModel.h"
#include "Models/LibraryModel.h"


class CORESHARED_EXPORT DatabaseManager : public QObject
{

    Q_OBJECT

public:
    explicit DatabaseManager();

    static DatabaseManager* getInstance();

    void initialize();
    void reset();
    void loadDatabase();
    void updateLibraryMedia(const QList<LibraryModel>& insertModels);

    QList<DeviceModel> getDevice();
    DeviceModel getDeviceByName(const QString &name);
    DeviceModel getDeviceById(int deviceId);
    void insertDevice(const DeviceModel &model);
    void updateDevice(const DeviceModel &model);
    void deleteDevice(int id);
    void updateMidiStatus(QString clipName, int midiNotes);
    void copyClipsTo(QStringList clipNames, QString tableName);
    void removeClipsFromList(QList<int> clipIds, QString tableName);

private:
    QMutex mutex;
    void createDatabase();
    void deleteDatabase();
    void upgradeDatabase();

signals:
    void databaseUpdated(QString table);
};

#endif // DATABASEMANAGER_H
