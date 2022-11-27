#include "MainWindow.h"

#include "Version.h"

#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include <QtSql/QSqlDatabase>

#include "DatabaseManager.h"


void loadDatabase()
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
        QString databaseLocation = QString("%1/Database.s3db").arg(path);

        database.setDatabaseName(databaseLocation);
    }

    if (!database.open())
        qCritical("Unable to open database");
}


int main(int argc, char *argv[])
{

    loadDatabase();
    DatabaseManager::getInstance().initialize();

    QApplication application(argc, argv);
    application.setApplicationName("Cute Caspar");
    application.setApplicationVersion(QString("%1.%2.%3.%4").arg(MAJOR_VERSION).arg(MINOR_VERSION).arg(REVISION_VERSION).arg(BUILD_VERSION));

    qDebug("Starting %s %s", qPrintable(application.applicationName()), qPrintable(application.applicationVersion()));

    qApp->setStyle(QStyleFactory::create("Fusion"));

    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53,53,53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25,25,25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53,53,53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Disabled, QPalette::Text, Qt::black);
    darkPalette.setColor(QPalette::Button, QColor(53,53,53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, Qt::black);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);

    qApp->setPalette(darkPalette);

    qApp->setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");

    MainWindow w;
    w.show();

    return application.exec();
}


