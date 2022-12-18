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
    model->setHeaderData(1, Qt::Horizontal, tr("Clip Name"), Qt::DisplayRole);
    model->setHeaderData(3, Qt::Horizontal, tr("Duration"), Qt::DisplayRole);
    model->setHeaderData(5, Qt::Horizontal, tr("Midi Notes"), Qt::DisplayRole);

    // Set proxy model to enable sorting columns:
    QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(model);
    proxyModel->sort(1, Qt::AscendingOrder);

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

    qry->prepare(QString("SELECT Id, DisplayOrder, Name, TypeId, Timecode, Fps, Midi FROM %1 ORDER BY DisplayOrder").arg(m_playlist));
    qry->exec();

    modelPlayList->setQuery(*qry);
    modelPlayList->setHeaderData(2, Qt::Horizontal, tr("Clip Name"), Qt::DisplayRole);
    modelPlayList->setHeaderData(6, Qt::Horizontal, tr("Midi Notes"), Qt::DisplayRole);

    ui->playList->setModel(modelPlayList);
    ui->playList->hideColumn(0);
    ui->playList->hideColumn(1);
    ui->playList->setColumnWidth(2, 300);
    ui->playList->hideColumn(3);
    ui->playList->hideColumn(4);
    ui->playList->hideColumn(5);
    ui->playList->setColumnWidth(6, 50);

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

void PlayList::on_cmbPlaylists_currentIndexChanged(int index)
{
    m_playlist = ui->cmbPlaylists->itemData(index).toString();
    refreshPlayList();
}

void PlayList::on_btnUp_clicked()
{
    QModelIndexList selected = ui->playList->selectionModel()->selectedRows();

    if (selected.size() != 0 && selected[0].row() > 0) {
        int index = selected[0].siblingAtColumn(1).data().toInt();
        int toRow = selected[0].row() - 1;
        int newIndex = modelPlayList->index(toRow, 1).data().toInt();
        DatabaseManager::getInstance()->moveClip(index, newIndex, m_playlist);
        refreshPlayList();
        ui->playList->selectRow(toRow);
    }
}

void PlayList::on_btnDown_clicked()
{
    QModelIndexList selected = ui->playList->selectionModel()->selectedRows();

    if (selected.size() != 0 && selected[0].row() < modelPlayList->rowCount() - 1) {
        int index = selected[0].siblingAtColumn(1).data().toInt();
        int toRow = selected[0].row() + 1;
        int newIndex = modelPlayList->index(toRow, 1).data().toInt();
        DatabaseManager::getInstance()->moveClip(index, newIndex, m_playlist);
        refreshPlayList();
        ui->playList->selectRow(toRow);
    }
}

void PlayList::on_btnDelete_clicked()
{
    QItemSelectionModel *selections = ui->playList->selectionModel();
    QModelIndexList selected = selections->selectedRows();
    if (selected.size() != 0) {
        QList<int> indexList;
        foreach (QModelIndex index, selected) {
            indexList.append(index.siblingAtColumn(0).data().toInt());
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
