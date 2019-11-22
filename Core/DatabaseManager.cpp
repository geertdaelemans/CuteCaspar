#include "DatabaseManager.h"

#include "Version.h"

#include <QtCore/QDebug>
#include <QtCore/QDir>

#include <QtSql/QSqlDriver>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

#include "Models/LibraryModel.h"

Q_GLOBAL_STATIC(DatabaseManager, databaseManager)

DatabaseManager::DatabaseManager()
    : mutex(QMutex::Recursive)
{
}

DatabaseManager& DatabaseManager::getInstance()
{
    return *databaseManager();
}

void DatabaseManager::initialize()
{
    QMutexLocker locker(&mutex);

    if (QSqlDatabase::database().tables().count() == 0)
        createDatabase();
    else {
        upgradeDatabase();
    }
}

void DatabaseManager::createDatabase()
{
    QFile file(":/Scripts/Sql/Schema.sql");
    if (file.open(QFile::ReadOnly))
    {
        QStringList queries = QString(file.readAll()).split(";");

        file.close();

        QSqlQuery sql;
        foreach (QString query, queries)
        {
            if (query.trimmed().isEmpty())
                continue;

            if (sql.driver()->dbmsType() == QSqlDriver::SQLite)
                query.remove("AUTO_INCREMENT");

            if (!sql.exec(query))
                qFatal("Failed to execute sql query: %s, Error: %s", qPrintable(sql.lastQuery()), qPrintable(sql.lastError().text()));
        }

#if defined(Q_OS_WIN)
        if (!sql.exec("INSERT INTO Configuration (Name, Value) VALUES('FontSize', '11')"))
            qFatal("Failed to execute sql query: %s, Error: %s", qPrintable(sql.lastQuery()), qPrintable(sql.lastError().text()));

        if (!sql.exec("INSERT INTO Device (Name, Address, Port, Username, Password, Description, Version, Shadow, Channels, ChannelFormats, PreviewChannel, LockedChannel) VALUES('Local CasparCG', '127.0.0.1', 5250, '', '', '', '', 'No', 0, '', 0, 0)"))
            qFatal("Failed to execute sql query: %s, Error: %s", qPrintable(sql.lastQuery()), qPrintable(sql.lastError().text()));
#else
        if (!sql.exec("INSERT INTO Configuration (Name, Value) VALUES('FontSize', '12')"))
            qFatal("Failed to execute sql query: %s, Error: %s", qPrintable(sql.lastQuery()), qPrintable(sql.lastError().text()));
#endif

        sql.prepare("UPDATE Configuration SET Value = :Value "
                    "WHERE Name = 'DatabaseVersion'");
        sql.bindValue(":Value", DATABASE_VERSION);

        if (!sql.exec())
            qFatal("Failed to execute sql query: %s, Error: %s", qPrintable(sql.lastQuery()), qPrintable(sql.lastError().text()));
    }
}

void DatabaseManager::upgradeDatabase()
{
    QSqlQuery sql;
    if (!sql.exec("SELECT c.Id, c.Name, c.Value FROM Configuration c WHERE c.Name = 'DatabaseVersion'"))
       qFatal("Failed to execute sql query: %s, Error: %s", qPrintable(sql.lastQuery()), qPrintable(sql.lastError().text()));

    sql.first();

    int version = sql.value(2).toInt();

    while (version + 1 <= QString("%1").arg(DATABASE_VERSION).toInt())
    {
        QFile file(QString(":/Scripts/Sql/ChangeScript-%1.sql").arg(version + 1));
        if (file.open(QFile::ReadOnly))
        {
            QStringList queries = QString(file.readAll()).split(";");

            file.close();

            foreach(const QString& query, queries)
            {
                 if (query.trimmed().isEmpty())
                     continue;

                 if (!sql.exec(query))
                    qFatal("Failed to execute sql query: %s, Error: %s", qPrintable(sql.lastQuery()), qPrintable(sql.lastError().text()));
            }

            sql.prepare("UPDATE Configuration SET Value = :Value "
                        "WHERE Name = 'DatabaseVersion'");
            sql.bindValue(":Value", version + 1);

            if (!sql.exec())
                qFatal("Failed to execute sql query: %s, Error: %s", qPrintable(sql.lastQuery()), qPrintable(sql.lastError().text()));

            qDebug("Successfully upgraded to ChangeScript-%d", version + 1);
        }

        version++;
    }
}


void DatabaseManager::reset()
{
    QMutexLocker locker(&mutex);

    QSqlQuery sql("DELETE FROM Library");
    if (!sql.exec())
        qDebug("Failed to execute sql query: %s, Error: %s", qPrintable(sql.lastQuery()), qPrintable(sql.lastError().text()));

    QSqlQuery sql2("VACUUM");
    if (!sql2.exec())
        qDebug("Failed to execute sql query: %s, Error: %s", qPrintable(sql2.lastQuery()), qPrintable(sql2.lastError().text()));
//    deleteDatabase();
//    initialize();
}

void DatabaseManager::deleteDatabase()
{
    QSqlQuery sql("DROP TABLE Library");
    if (!sql.exec())
        qDebug("Failed to execute sql query: %s, Error: %s", qPrintable(sql.lastQuery()), qPrintable(sql.lastError().text()));
}

void DatabaseManager::updateLibraryMedia(const QList<LibraryModel>& insertModels)
{
    QMutexLocker locker(&mutex);

//    int deviceId = getDeviceByAddress(address).getId();
//    QList<TypeModel> typeModels = getType();

    QSqlDatabase::database().transaction();

    QSqlQuery sql;

//    if (deleteModels.count() > 0)
//    {
//        for (int i = 0; i < deleteModels.count(); i++)
//        {
//            sql.prepare("DELETE FROM Library "
//                        "WHERE Id = :Id");
//            sql.bindValue(":Id", deleteModels.at(i).getId());

//            if (!sql.exec())
//               qCritical("Failed to execute sql query: %s, Error: %s", qPrintable(sql.lastQuery()), qPrintable(sql.lastError().text()));
//        }
//    }

    if (insertModels.count() > 0)
    {
        if (!sql.exec("DELETE FROM Library"))
           qCritical("Failed to execute sql query: %s, Error: %s", qPrintable(sql.lastQuery()), qPrintable(sql.lastError().text()));

//        int typeId = 0;
        for (int i = 0; i < insertModels.count(); i++)
        {
//            if (insertModels.at(i).getType() == Rundown::AUDIO)
//                typeId = std::find_if(typeModels.begin(), typeModels.end(), TypeModel::ByName(Rundown::AUDIO))->getId();
//            else if (insertModels.at(i).getType() == Rundown::MOVIE)
//                typeId = std::find_if(typeModels.begin(), typeModels.end(), TypeModel::ByName(Rundown::MOVIE))->getId();
//            else if (insertModels.at(i).getType() == Rundown::STILL)
//                typeId = std::find_if(typeModels.begin(), typeModels.end(), TypeModel::ByName(Rundown::STILL))->getId();

            sql.prepare("INSERT INTO Library (Name, DeviceId, TypeId, ThumbnailId, Timecode, Fps) "
                        "VALUES(:Name, :DeviceId, :TypeId, :ThumbnailId, :Timecode, :Fps)");
            sql.bindValue(":Name", insertModels.at(i).getName());
//            sql.bindValue(":DeviceId", deviceId);
//            sql.bindValue(":TypeId", typeId);
            sql.bindValue(":TypeId", insertModels.at(i).getType());
            sql.bindValue(":ThumbnailId", insertModels.at(i).getThumbnailId());
            sql.bindValue(":Timecode", insertModels.at(i).getTimecode());
            sql.bindValue(":Fps", insertModels.at(i).getFPS());

            if (!sql.exec())
               qCritical("Failed to execute sql query: %s, Error: %s", qPrintable(sql.lastQuery()), qPrintable(sql.lastError().text()));
        }
    }

    QSqlDatabase::database().commit();
}


/**
 * @brief DatabaseManager::getDevice
 * Get a list of all available devices
 * @return List of DeviceModel class objects
 */
QList<DeviceModel> DatabaseManager::getDevice()
{
    QSqlQuery sql;
    if (!sql.exec("SELECT d.Id, d.Name, d.Address, d.Port, d.Username, d.Password, d.Description, d.Version, d.Shadow, d.Channels, d.ChannelFormats, d.PreviewChannel, d.LockedChannel FROM Device d ORDER BY d.Name"))
       qCritical("Failed to execute sql query: %s, Error: %s", qPrintable(sql.lastQuery()), qPrintable(sql.lastError().text()));

    QList<DeviceModel> models;
    while (sql.next())
        models.push_back(DeviceModel(sql.value(0).toInt(), sql.value(1).toString(), sql.value(2).toString(), sql.value(3).toInt(),
                                     sql.value(4).toString(), sql.value(5).toString(), sql.value(6).toString(), sql.value(7).toString(),
                                     sql.value(8).toString(), sql.value(9).toInt(), sql.value(10).toString(), sql.value(11).toInt(), sql.value(12).toInt()));

    return models;
}

/**
 * @brief DatabaseManager::getDeviceById
 * Search Device database for specific device and return all information
 * @param deviceId of the device
 * @return DeviceModel class object
 */
DeviceModel DatabaseManager::getDeviceById(int deviceId)
{
    QMutexLocker locker(&mutex);

    QSqlQuery sql;
    sql.prepare("SELECT d.Id, d.Name, d.Address, d.Port, d.Username, d.Password, d.Description, d.Version, d.Shadow, d.Channels, d.ChannelFormats, d.PreviewChannel, d.LockedChannel FROM Device d "
                "WHERE d.Id = :Id");
    sql.bindValue(":Id", deviceId);

    if (!sql.exec())
       qCritical("Failed to execute sql query: %s, Error: %s", qPrintable(sql.lastQuery()), qPrintable(sql.lastError().text()));

    sql.first();

    return DeviceModel(sql.value(0).toInt(), sql.value(1).toString(), sql.value(2).toString(), sql.value(3).toInt(),
                       sql.value(4).toString(), sql.value(5).toString(), sql.value(6).toString(), sql.value(7).toString(),
                       sql.value(8).toString(), sql.value(9).toInt(), sql.value(10).toString(), sql.value(11).toInt(), sql.value(12).toInt());
}

/**
 * @brief DatabaseManager::getDeviceByName
 * Search Device database for specific device and return all information
 * @param name of the device as given by the user
 * @return DeviceModel class object
 */
DeviceModel DatabaseManager::getDeviceByName(const QString& name)
{
    QMutexLocker locker(&mutex);

    QSqlQuery sql;
    sql.prepare("SELECT d.Id, d.Name, d.Address, d.Port, d.Username, d.Password, d.Description, d.Version, d.Shadow, d.Channels, d.ChannelFormats, d.PreviewChannel, d.LockedChannel FROM Device d "
                "WHERE d.Name = :Name");
    sql.bindValue(":Name", name);

    if (!sql.exec())
       qCritical("Failed to execute sql query: %s, Error: %s", qPrintable(sql.lastQuery()), qPrintable(sql.lastError().text()));

    sql.first();

    return DeviceModel(sql.value(0).toInt(), sql.value(1).toString(), sql.value(2).toString(), sql.value(3).toInt(),
                       sql.value(4).toString(), sql.value(5).toString(), sql.value(6).toString(), sql.value(7).toString(),
                       sql.value(8).toString(), sql.value(9).toInt(), sql.value(10).toString(), sql.value(11).toInt(), sql.value(12).toInt());
}

/**
 * @brief DatabaseManager::insertDevice
 * Insert a new device to the Device database
 * @param model
 */
void DatabaseManager::insertDevice(const DeviceModel& model)
{
    QMutexLocker locker(&mutex);

    QSqlDatabase::database().transaction();

    QSqlQuery sql;
    sql.prepare("INSERT INTO Device (Name, Address, Port, Username, Password, Description, Version, Shadow, Channels, ChannelFormats, PreviewChannel, LockedChannel) "
                "VALUES(:Name, :Address, :Port, :Username, :Password, :Description, :Version, :Shadow, :Channels, :ChannelFormats, :PreviewChannel, :LockedChannel)");
    sql.bindValue(":Name", model.getName());
    sql.bindValue(":Address", model.getAddress());
    sql.bindValue(":Port", model.getPort());
    sql.bindValue(":Username", model.getUsername());
    sql.bindValue(":Password", model.getPassword());
    sql.bindValue(":Description", model.getDescription());
    sql.bindValue(":Version", model.getVersion());
    sql.bindValue(":Shadow", model.getShadow());
    sql.bindValue(":Channels", model.getChannels());
    sql.bindValue(":ChannelFormats", model.getChannelFormats());
    sql.bindValue(":PreviewChannel", model.getPreviewChannel());
    sql.bindValue(":LockedChannel", model.getLockedChannel());

    if (!sql.exec())
       qCritical("Failed to execute sql query: %s, Error: %s", qPrintable(sql.lastQuery()), qPrintable(sql.lastError().text()));

    QSqlDatabase::database().commit();
}


void DatabaseManager::updateDevice(const DeviceModel& model)
{
    QMutexLocker locker(&mutex);

    QSqlDatabase::database().transaction();

    QSqlQuery sql;
    sql.prepare("UPDATE Device SET Name = :Name, Address = :Address, Port = :Port, Username = :Username, Password = :Password, Description = :Description, Version = :Version, Shadow = :Shadow, Channels = :Channels, ChannelFormats = :ChannelFormats, PreviewChannel = :PreviewChannel, LockedChannel = :LockedChannel "
                "WHERE Id = :Id");
    sql.bindValue(":Name", model.getName());
    sql.bindValue(":Address", model.getAddress());
    sql.bindValue(":Port", model.getPort());
    sql.bindValue(":Username", model.getUsername());
    sql.bindValue(":Password", model.getPassword());
    sql.bindValue(":Description", model.getDescription());
    sql.bindValue(":Version", model.getVersion());
    sql.bindValue(":Shadow", model.getShadow());
    sql.bindValue(":Channels", model.getChannels());
    sql.bindValue(":ChannelFormats", model.getChannelFormats());
    sql.bindValue(":PreviewChannel", model.getPreviewChannel());
    sql.bindValue(":LockedChannel", model.getLockedChannel());
    sql.bindValue(":Id", model.getId());

    if (!sql.exec())
       qCritical("Failed to execute sql query: %s, Error: %s", qPrintable(sql.lastQuery()), qPrintable(sql.lastError().text()));

    QSqlDatabase::database().commit();
}


/**
 * @brief DatabaseManager::deleteDevice
 * Remove the device from the Device, Thumbnail and Library database
 * @param id of the device
 */
void DatabaseManager::deleteDevice(int id)
{
    QMutexLocker locker(&mutex);

    QSqlDatabase::database().transaction();

    QSqlQuery sql;
    sql.prepare("DELETE FROM Device "
                "WHERE Id = :Id");
    sql.bindValue(":Id",id);

    if (!sql.exec())
       qCritical("Failed to execute sql query: %s, Error: %s", qPrintable(sql.lastQuery()), qPrintable(sql.lastError().text()));

    sql.prepare("DELETE FROM Thumbnail "
                "WHERE Id IN (SELECT l.ThumbnailId FROM Library l WHERE DeviceId = :DeviceId)");
    sql.bindValue(":DeviceId",id);

    if (!sql.exec())
       qCritical("Failed to execute sql query: %s, Error: %s", qPrintable(sql.lastQuery()), qPrintable(sql.lastError().text()));

    sql.prepare("DELETE FROM Library "
                "WHERE DeviceId = :DeviceId");
    sql.bindValue(":DeviceId",id);

    if (!sql.exec())
       qCritical("Failed to execute sql query: %s, Error: %s", qPrintable(sql.lastQuery()), qPrintable(sql.lastError().text()));

    QSqlDatabase::database().commit();
}
