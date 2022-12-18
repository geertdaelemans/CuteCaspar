#include "DatabaseManager.h"

#include "Version.h"

#include <QtCore/QDebug>
#include <QtCore/QDir>

#include <QtSql/QSqlDriver>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

#include "Models/LibraryModel.h"

Q_GLOBAL_STATIC(DatabaseManager, s_databaseManager)

DatabaseManager::DatabaseManager()
    : mutex(QMutex::Recursive)
{
}

DatabaseManager* DatabaseManager::getInstance()
{
    return s_databaseManager();
}


/*******************************************
 * Data functions for handling media clips *
 *******************************************/

/**
 * @brief DatabaseManager::initializeDatabase
 * Set-up and configure the SQL database
 */
void DatabaseManager::initializeDatabase()
{
    QString path = QString("%1/.CasparCG/ClientNew").arg(QDir::homePath());

    QDir directory(path);
    if (!directory.exists())
        directory.mkpath(".");

    const bool dbmemory = false;

    QSqlDatabase database;
    if (dbmemory)
    {
        qDebug("Using SQLite in memory database");

        database = QSqlDatabase::addDatabase("QSQLITE");
        database.setDatabaseName(":memory:");
    }
    else
    {
        qDebug("Using SQLite database");

        database = QSqlDatabase::addDatabase("QSQLITE");
        QString databaseLocation = QString("%1/Database.sqlite").arg(path);

        database.setDatabaseName(databaseLocation);
    }

    if (!database.open())
        qCritical("Unable to open database");

    QMutexLocker locker(&mutex);

    if (QSqlDatabase::database().tables().count() == 0)
        createDatabase();
    else {
        upgradeDatabase();
    }
}

/**
 * @brief DatabaseManager::createDatabase
 * Create a SQL database based upon a schema
 */
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

/**
 * @brief DatabaseManager::upgradeDatabase
 * Upgrade the existing databae to a new schema
 */
void DatabaseManager::upgradeDatabase()
{
    QSqlQuery sql;
    if (!sql.exec("SELECT c.Id, c.Name, c.Value FROM Configuration c WHERE c.Name = 'DatabaseVersion'"))
        qFatal("Failed to execute sql query: %s, Error: %s", qPrintable(sql.lastQuery()), qPrintable(sql.lastError().text()));

    sql.first();

    int version = sql.value(2).toInt();

    while (version + 1 <= QString("%1").arg(DATABASE_VERSION).toInt()) {

        QFile file(QString(":/Scripts/Sql/ChangeScript-%1.sql").arg(version + 1));
        if (file.open(QFile::ReadOnly)) {
            QStringList queries = QString(file.readAll()).split(";");

            file.close();

            foreach(const QString& query, queries) {
                if (query.trimmed().isEmpty())
                    continue;
                sql.prepare(query);
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

/**
 * @brief DatabaseManager::reset
 * Reset the database and clean up
 */
void DatabaseManager::reset()
{
    QMutexLocker locker(&mutex);

    QSqlQuery sql("DELETE FROM Library");
    if (!sql.exec())
        qDebug("Failed to execute sql query: %s, Error: %s", qPrintable(sql.lastQuery()), qPrintable(sql.lastError().text()));

    QSqlQuery sql2("VACUUM");
    if (!sql2.exec())
        qDebug("Failed to execute sql query: %s, Error: %s", qPrintable(sql2.lastQuery()), qPrintable(sql2.lastError().text()));
}

/**
 * @brief DatabaseManager::deleteDatabase
 * Delete the Library database
 */
void DatabaseManager::deleteDatabase()
{
    QSqlQuery sql("DROP TABLE Library");
    if (!sql.exec())
        qDebug("Failed to execute sql query: %s, Error: %s", qPrintable(sql.lastQuery()), qPrintable(sql.lastError().text()));
}


/*******************************************
 * Functions for handling CasparCG devices *
 *******************************************/

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

/**
 * @brief DatabaseManager::updateDevice
 * Update the device settings
 * @param model - device model to be applied
 */
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
 * @param id - id of the device
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


/*******************************************
 * Data functions for handling media clips *
 *******************************************/

/**
 * @brief DatabaseManager::updateLibraryMedia
 * This function adds all media data into the Library table after is has been
 * fetched from the CasparCG server by another process
 * @param insertModels - a list of all media in the LibraryModel format
 */
void DatabaseManager::updateLibraryMedia(const QList<LibraryModel>& insertModels)
{
    if (insertModels.count() > 0) {

        QMutexLocker locker(&mutex);

        QSqlDatabase::database().transaction();

        QSqlQuery sql;

        if (!sql.exec("DELETE FROM Library"))
            qCritical("Failed to execute sql query: %s, Error: %s", qPrintable(sql.lastQuery()), qPrintable(sql.lastError().text()));

        for (int i = 0; i < insertModels.count(); i++) {
            sql.prepare("INSERT INTO Library (Name, DeviceId, TypeId, ThumbnailId, Timecode, Fps, Midi) "
                        "VALUES(:Name, :DeviceId, :TypeId, :ThumbnailId, :Timecode, :Fps, :Midi)");
            sql.bindValue(":Name", insertModels.at(i).getName());
            sql.bindValue(":DeviceId", insertModels.at(i).getDeviceName());
            sql.bindValue(":TypeId", insertModels.at(i).getType());
            sql.bindValue(":ThumbnailId", insertModels.at(i).getThumbnailId());
            sql.bindValue(":Timecode", insertModels.at(i).getTimecode());
            sql.bindValue(":Fps", insertModels.at(i).getFPS());
            sql.bindValue(":Midi", insertModels.at(i).getMidi());

            if (!sql.exec())
                qCritical("Failed to execute sql query: %s, Error: %s", qPrintable(sql.lastQuery()), qPrintable(sql.lastError().text()));
        }

        QSqlDatabase::database().commit();

        emit databaseUpdated("Library");
    }
}

/**
 * @brief DatabaseManager::copyClipsTo
 * Copy a set of one or more clips into another table
 * @param clipIds - the ids of the clips to be copied
 * @param tableName - the destination table
 */
void DatabaseManager::copyClipsTo(QList<int> clipIds, QString tableName)
{
    QMutexLocker locker(&mutex);

    // Convert QList clipIds into a string to be used in the sql statement
    QString clipIdsString;
    for (int i = 0; i < clipIds.size(); i++) {
        clipIdsString += QString::number(clipIds[i]) + (i < clipIds.size() - 1 ? "," : "");
    }

    QSqlDatabase::database().transaction();

    QSqlQuery sql;
    sql.prepare("INSERT INTO " + tableName + " (Name, DeviceId, TypeId, ThumbnailId, Timecode, Fps, Midi)"
                "SELECT Name, DeviceId, TypeId, ThumbnailId, Timecode, Fps, Midi FROM Library "
                "WHERE Id IN (" + clipIdsString + ")");

    if (!sql.exec())
        qCritical("Failed to execute sql query: %s, Error: %s", qPrintable(sql.lastQuery()), qPrintable(sql.lastError().text()));

    sql.prepare("UPDATE " + tableName + " SET DisplayOrder = Id WHERE DisplayOrder IS NULL");

    if (!sql.exec())
        qCritical("Failed to execute sql query: %s, Error: %s", qPrintable(sql.lastQuery()), qPrintable(sql.lastError().text()));

    QSqlDatabase::database().commit();

    emit databaseUpdated(tableName);
}

/**
 * @brief DatabaseManager::removeClipsFromList
 * Simply remove a set of one or more clips from a certain table
 * @param clipIds the ids of the clips to be removed
 * @param tableName the table name the clip has to be removed from
 */
void DatabaseManager::removeClipsFromList(QList<int> clipIds, QString tableName)
{
    QMutexLocker locker(&mutex);

    // Convert QList clipIds into a string to be used in the sql statement
    QString clipIdsString = "";
    for (int i = 0; i < clipIds.size(); i++) {
        clipIdsString += QString::number(clipIds[i]) + (i < clipIds.size() - 1 ? "," : "");
    }

    QSqlDatabase::database().transaction();

    QSqlQuery sql;
    sql.prepare("DELETE FROM " + tableName + " "
                                             "WHERE Id IN (" + clipIdsString + ")");

    if (!sql.exec())
        qCritical("Failed to execute removeClipFromList query: %s, Error: %s", qPrintable(sql.lastQuery()), qPrintable(sql.lastError().text()));

    QSqlDatabase::database().commit();

    emit databaseUpdated(tableName);
}

/**
 * @brief DatabaseManager::moveClip
 * Move clip to a new position in the table
 * @param from - current location od the clip
 * @param to - new position of the clip in the table
 * @param tableName - name of the table that contains the clip
 */
void DatabaseManager::moveClip(int from, int to, QString tableName)
{
    QMutexLocker locker(&mutex);

    QSqlDatabase::database().transaction();

    QSqlQuery sql;

    if (from > to) {
        sql.prepare(QString("UPDATE %1 SET DisplayOrder = (CASE DisplayOrder WHEN %3 THEN %2 ELSE DisplayOrder + 1 END) WHERE DisplayOrder >= %2 AND DisplayOrder <= %3").arg(tableName).arg(to).arg(from));
    } else {
        sql.prepare(QString("UPDATE %1 SET DisplayOrder = (CASE DisplayOrder WHEN %3 THEN %2 ELSE DisplayOrder - 1 END) WHERE DisplayOrder <= %2 AND DisplayOrder >= %3").arg(tableName).arg(to).arg(from));
    }
    if (!sql.exec())
        qCritical("Failed to execute sql query: %s, Error: %s", qPrintable(sql.lastQuery()), qPrintable(sql.lastError().text()));

    QSqlDatabase::database().commit();

    emit databaseUpdated(tableName);
}

/**
 * @brief DatabaseManager::emptyList
 * Empties a table without removing the definition
 * @param tableName - name of the table to be emptied
 */
void DatabaseManager::emptyList(QString tableName)
{
    QMutexLocker locker(&mutex);

    QSqlDatabase::database().transaction();

    QSqlQuery sql;
    sql.prepare(QString("DELETE FROM %1").arg(tableName));

    if (!sql.exec())
        qCritical("Failed to execute sql query: %s, Error: %s", qPrintable(sql.lastQuery()), qPrintable(sql.lastError().text()));

    QSqlDatabase::database().commit();

    emit databaseUpdated(tableName);
}

/**
 * @brief DatabaseManager::updateMidiStatus
 * @param clipName - name of the clip for which midi data needs to be saved
 * @param midiNotes - number of notes available for this clip
 */
void DatabaseManager::updateMidiStatus(QString clipName, int midiNotes)
{
    QMutexLocker locker(&mutex);

    QSqlDatabase::database().transaction();

    QSqlQuery sql;
    sql.prepare("UPDATE Playlist SET Midi = :Midi "
                "WHERE Name = :Name");
    sql.bindValue(":Name", clipName);
    sql.bindValue(":Midi", midiNotes);

    if (!sql.exec())
        qCritical("Failed to execute sql query: %s, Error: %s", qPrintable(sql.lastQuery()), qPrintable(sql.lastError().text()));

    QSqlDatabase::database().commit();

    emit databaseUpdated("Playlist");
}
