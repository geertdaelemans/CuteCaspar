#include "PlayListDialog.h"
#include "ui_PlayList.h"

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

    m_playlist = "Playlist";

    ui->cmbPlaylists->addItem("Main Playlist", "Playlist");
    ui->cmbPlaylists->addItem("Scares Playlist", "Scares");
    ui->cmbPlaylists->addItem("Extra Playlist", "Extras");

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
    modelPlayList = new QSqlQueryModel();

    QSqlQuery* qry = new QSqlQuery();

    qry->prepare(QString("select Id, Name, TypeId, Timecode, Fps from %1").arg(m_playlist));
    qry->exec();

    modelPlayList->setQuery(*qry);

    // Set proxy model to enable sorting columns:
    QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(modelPlayList);
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
    if (!query.prepare(QString("INSERT INTO %1 (Name, TypeId, Timecode, Fps) VALUES (:name, :typeid, :timecode, :fps)").arg(m_playlist)))
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
    if (!query.prepare(QString("DELETE FROM %1").arg(m_playlist)))
        qFatal("Failed to execute sql query: %s, Error: %s", qPrintable(query.lastQuery()), qPrintable(query.lastError().text()));
    query.exec();

    refreshPlayList();
}

void PlayList::deleteRow(int row)
{
    QSqlQuery query;
    if (!query.prepare(QString("DELETE FROM %1 WHERE Id = %2").arg(m_playlist).arg(row)))
        qFatal("Failed to execute sql query: %s, Error: %s", qPrintable(query.lastQuery()), qPrintable(query.lastError().text()));
    query.exec();

    refreshPlayList();
}

void PlayList::swapRows(int rowA, int rowB)
{
    QSqlQuery query;
    if (!query.prepare(QString("UPDATE %1 SET Id = (CASE WHEN Id = %2 THEN -%3 ELSE -%2 END) WHERE Id IN (%2, %3)").arg(m_playlist).arg(QString::number(rowA), QString::number(rowB))))
        qFatal("Failed to execute sql query: %s, Error: %s", qPrintable(query.lastQuery()), qPrintable(query.lastError().text()));
    query.exec();
    if (!query.prepare(QString("UPDATE %1 SET Id = -Id WHERE Id < 0").arg(m_playlist)))
        qFatal("Failed to execute sql query: %s, Error: %s", qPrintable(query.lastQuery()), qPrintable(query.lastError().text()));
    query.exec();
    refreshPlayList();
}


void PlayList::on_clipList_clicked(const QModelIndex &index)
{
    const QString clipName = index.siblingAtColumn(0).data(Qt::DisplayRole).toString();
}

void PlayList::on_cmbPlaylists_currentIndexChanged(int index)
{
    m_playlist = ui->cmbPlaylists->itemData(index).toString();
    refreshPlayList();
}

void PlayList::on_btnUp_clicked()
{
    QItemSelectionModel *selections = ui->playList->selectionModel();
    QModelIndexList selected = selections->selectedIndexes();
    if (selected.size() != 0 && selected.begin()->row() != 0) {
        int rowA = selected.begin()->row() - 1;
        int rowB = selected.begin()->row();
        int indexA = modelPlayList->data(modelPlayList->index(rowA, 0)).toInt();
        int indexB = modelPlayList->data(modelPlayList->index(rowB, 0)).toInt();
        swapRows(indexA, indexB);
        ui->playList->selectRow(rowA);
    }
}

void PlayList::on_btnDown_clicked()
{
    QItemSelectionModel *selections = ui->playList->selectionModel();
    QModelIndexList selected = selections->selectedIndexes();
    if (selected.size() != 0 && selected.begin()->row() != modelPlayList->rowCount() - 1) {
        int rowA = selected.begin()->row();
        int rowB = selected.begin()->row() + 1;
        int indexA = modelPlayList->data(modelPlayList->index(rowA, 0)).toInt();
        int indexB = modelPlayList->data(modelPlayList->index(rowB, 0)).toInt();
        swapRows(indexA, indexB);
        ui->playList->selectRow(rowB);
    }
}

void PlayList::on_btnDelete_clicked()
{
    QItemSelectionModel *selections = ui->playList->selectionModel();
    QModelIndexList selected = selections->selectedIndexes();
    if (selected.size() != 0) {
        int row = selected.begin()->row();
        int index = modelPlayList->data(modelPlayList->index(row, 0)).toInt();
        deleteRow(index);
        refreshPlayList();
        if (row < modelPlayList->rowCount()) {
            ui->playList->selectRow(row);
        } else {
            ui->playList->selectRow(row - 1);
        }
    }
}
