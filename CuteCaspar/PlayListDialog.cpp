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

    DatabaseManager::getInstance()->getDevice();

    m_playlist = "Playlist";

    ui->cmbPlaylists->addItem("Main Playlist", "Playlist");
    ui->cmbPlaylists->addItem("Scares Playlist", "Scares");
    ui->cmbPlaylists->addItem("Extra Playlist", "Extras");

    refreshLibraryList();
    refreshPlayList();
}

PlayList::~PlayList()
{
    delete ui;
}


void PlayList::refreshLibraryList()
{
    QSqlQueryModel * model = new QSqlQueryModel();

    QSqlQuery* qry = new QSqlQuery();

    qry->prepare("SELECT Id, Name, TypeId, Timecode, Fps, Midi FROM Library");
    qry->exec();

    model->setQuery(*qry);
    model->setHeaderData(1, Qt::Horizontal, tr("Duration"), Qt::DisplayRole);

    // Set proxy model to enable sorting columns:
    QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(model);
    proxyModel->sort(2, Qt::AscendingOrder);

    ui->clipList->setModel(proxyModel);
    ui->clipList->hideColumn(0);
    ui->clipList->setColumnWidth(1, 400);
    ui->clipList->hideColumn(2);
    ui->clipList->setColumnWidth(3, 100);
    ui->clipList->hideColumn(4);
    ui->clipList->setColumnWidth(5, 100);
}

void PlayList::refreshPlayList()
{
    modelPlayList = new QSqlQueryModel();

    QSqlQuery* qry = new QSqlQuery();

    qry->prepare(QString("SELECT Id, Name, TypeId, Timecode, Fps, Midi FROM %1").arg(m_playlist));
    qry->exec();

    modelPlayList->setQuery(*qry);

    // Set proxy model to enable sorting columns:
    QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(modelPlayList);
    proxyModel->sort(0, Qt::AscendingOrder);

    ui->playList->setModel(proxyModel);
    ui->playList->hideColumn(0);
    ui->playList->setColumnWidth(1, 400);
    ui->playList->hideColumn(2);
    ui->playList->hideColumn(3);
    ui->playList->hideColumn(4);

    emit playlistChanged();
}

/**
 * @brief PlayList::on_btnAddToList_clicked
 * Button to add clips to the list, can be multi-select
 */
void PlayList::on_btnAddToList_clicked()
{
    QModelIndexList selectionList = ui->clipList->selectionModel()->selectedRows(0);
    QList<int> selectedIndices;
    foreach (QModelIndex index, selectionList) {
        selectedIndices.append(index.data(Qt::DisplayRole).toInt());
    }
    DatabaseManager::getInstance()->copyClipsTo(selectedIndices, m_playlist);
    refreshPlayList();
}

/**
 * @brief PlayList::on_clipList_doubleClicked
 * Copying the double-clicked clip to the selected list
 * @param index - QModelIndex of the field on which the user clicked
 */
void PlayList::on_clipList_doubleClicked(const QModelIndex &index)
{
    QList<int> selectedIndices;
    selectedIndices.append(index.siblingAtColumn(0).data(Qt::DisplayRole).toInt());
    DatabaseManager::getInstance()->copyClipsTo(selectedIndices, m_playlist);
    refreshPlayList();
}

bool PlayList::isMidiPresent(QString clipName)
{
    QFileInfo midiFile(QString("%1.midi").arg(clipName.replace("/","-")));
    if (midiFile.exists()) {
        if (midiFile.size() > 0) {
            return true;
        }
    }
    return false;
}

void PlayList::on_clearPlaylistButton_clicked()
{
    DatabaseManager::getInstance()->emptyList(m_playlist);
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
        int rowA = selected.begin()->row();
        int rowB = selected.begin()->row() - 1;
        int indexA = modelPlayList->data(modelPlayList->index(rowA, 0)).toInt();
        int indexB = modelPlayList->data(modelPlayList->index(rowB, 0)).toInt();
        DatabaseManager::getInstance()->moveClip(indexA, indexB, m_playlist);
        refreshPlayList();
        ui->playList->selectRow(rowB);
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
        DatabaseManager::getInstance()->moveClip(indexA, indexB, m_playlist);
        refreshPlayList();
        ui->playList->selectRow(rowB);
    }
}

void PlayList::on_btnDelete_clicked()
{
    QItemSelectionModel *selections = ui->playList->selectionModel();
    QModelIndexList selected = selections->selectedRows();
    if (selected.size() != 0) {
        QList<int> indexList;
        foreach (QModelIndex index, selected) {
            indexList.append(modelPlayList->data(modelPlayList->index(index.row(), 0)).toInt());
        }
        int row = selected.begin()->row();

        DatabaseManager::getInstance()->removeClipsFromList(indexList, m_playlist);
        refreshPlayList();
        if (row < modelPlayList->rowCount()) {
            ui->playList->selectRow(row);
        } else {
            ui->playList->selectRow(row - 1);
        }
    }
}
