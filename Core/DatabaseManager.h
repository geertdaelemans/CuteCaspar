#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QMutex>
#include <QObject>

#include "Models/DeviceModel.h"
#include "Models/LibraryModel.h"


class CORESHARED_EXPORT DatabaseManager
{
public:
    explicit DatabaseManager();

    static DatabaseManager& getInstance();

    void initialize();
    void reset();
    void updateLibraryMedia(const QList<LibraryModel>& insertModels);

    QList<DeviceModel> getDevice();
    DeviceModel getDeviceByName(const QString &name);
    DeviceModel getDeviceById(int deviceId);
    void insertDevice(const DeviceModel &model);
    void updateDevice(const DeviceModel &model);
    void deleteDevice(int id);



private:
    QMutex mutex;
    void createDatabase();
    void deleteDatabase();
    void upgradeDatabase();
};

#endif // DATABASEMANAGER_H
