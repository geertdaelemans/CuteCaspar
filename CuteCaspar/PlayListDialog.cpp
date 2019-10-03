#include "PlayListDialog.h"
#include "ui_PlayList.h"

#include <QSqlQueryModel>
#include <QSqlQuery>
#include <QtSql>

#include "DatabaseManager.h"

PlayList::PlayList(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PlayList)
{
    // Disable "What's This" button on Title bar
    this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    ui->setupUi(this);

    DatabaseManager::getInstance().getDevice();

    refreshMediaList();
    refreshPlayList();
}

PlayList::~PlayList()
{
    delete ui;
}


void PlayList::refreshMediaList()
{
    QSqlQueryModel * model = new QSqlQueryModel();

    QSqlQuery* qry = new QSqlQuery();

    qry->prepare("select Name, TypeId, Timecode, Fps from Library");
    qry->exec();

    model->setQuery(*qry);
    model->setHeaderData(1, Qt::Horizontal, tr("Duration"), Qt::DisplayRole);

    // Set proxy model to enable sorting columns:
    QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(model);
    proxyModel->sort(2, Qt::AscendingOrder);

    ui->clipList->setModel(proxyModel);
    ui->clipList->setColumnWidth(0, 400);
    ui->clipList->hideColumn(1);
    ui->clipList->setColumnWidth(2, 100);
    ui->clipList->hideColumn(3);
}

void PlayList::refreshPlayList()
{
    QSqlQueryModel * model = new QSqlQueryModel();

    QSqlQuery* qry = new QSqlQuery();

    qry->prepare("select Id, Name, TypeId, Timecode, Fps from Playlist");
    qry->exec();

    model->setQuery(*qry);

    // Set proxy model to enable sorting columns:
    QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(model);
    proxyModel->sort(0, Qt::AscendingOrder);

    ui->playList->setModel(proxyModel);
    ui->playList->hideColumn(0);
    ui->playList->hideColumn(2);
    ui->playList->hideColumn(3);
    ui->playList->hideColumn(4);

    emit playlistChanged();
}

void PlayList::on_clipList_doubleClicked(const QModelIndex &index)
{
    QSqlQuery query;
    if (!query.prepare("INSERT INTO Playlist (Name, TypeId, Timecode, Fps) VALUES (:name, :typeid, :timecode, :fps)"))
        qFatal("Failed to execute sql query: %s, Error: %s", qPrintable(query.lastQuery()), qPrintable(query.lastError().text()));
    query.bindValue(":name", index.siblingAtColumn(0).data(Qt::DisplayRole).toString());
    query.bindValue(":typeid", index.siblingAtColumn(1).data(Qt::DisplayRole).toString());
    query.bindValue(":timecode", index.siblingAtColumn(2).data(Qt::DisplayRole).toString());
    query.bindValue(":fps", index.siblingAtColumn(3).data(Qt::DisplayRole).toString());
    query.exec();

    refreshPlayList();
}

void PlayList::on_clearPlaylistButton_clicked()
{
    QSqlQuery query;
    if (!query.prepare("DELETE FROM Playlist"))
        qFatal("Failed to execute sql query: %s, Error: %s", qPrintable(query.lastQuery()), qPrintable(query.lastError().text()));
    query.exec();

    refreshPlayList();
}

void PlayList::on_clipList_clicked(const QModelIndex &index)
{
    const QString clipName = index.siblingAtColumn(0).data(Qt::DisplayRole).toString();
}
