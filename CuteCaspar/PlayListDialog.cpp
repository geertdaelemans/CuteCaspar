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

    model->setQuery("SELECT Id, Name, TypeId, Timecode, Fps, Midi FROM Library");
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

    modelPlayList->setQuery(QString("SELECT Id, DisplayOrder, Name, TypeId, Timecode, Fps, Midi FROM %1 ORDER BY DisplayOrder").arg(m_playlist));
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
        // List clip ID's to be moved
        QList<int> indexList;
        foreach (QModelIndex index, selected) {
            indexList.append(index.siblingAtColumn(0).data().toInt());
        }

        // Prepare the reorder command
        int numberOfMovedClips = selected.size();
        int toRow = selected[0].row() - 1;
        int newIndex = modelPlayList->index(toRow, 1).data().toInt();
        int lastLinePosition = DatabaseManager::getInstance()->reorderClips(indexList, newIndex, m_playlist);

        refreshPlayList();

        // Select the moved clips
        QItemSelection newSelection;
        newSelection.select(modelPlayList->index(lastLinePosition - (numberOfMovedClips - 1), 0), modelPlayList->index(lastLinePosition, 0));
        ui->playList->selectionModel()->select(newSelection, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    }
}

void PlayList::on_btnDown_clicked()
{
    QModelIndexList selected = ui->playList->selectionModel()->selectedRows();

    if (selected.size() != 0 && selected[0].row() + selected.size() < modelPlayList->rowCount()) {
        // List clip ID's to be moved
        QList<int> indexList;
        foreach (QModelIndex index, selected) {
            indexList.append(index.siblingAtColumn(0).data().toInt());
        }

        // Prepare reorder command
        int numberOfMovedClips = selected.size();
        int toRow = selected[0].row() + numberOfMovedClips + 1;
        int newIndex = (modelPlayList->index(toRow, 1).isValid() ? modelPlayList->index(toRow, 1).data().toInt() : modelPlayList->rowCount() + 1);
        int lastLinePosition = DatabaseManager::getInstance()->reorderClips(indexList, newIndex, m_playlist);

        refreshPlayList();

        // Select the moved clips
        QItemSelection newSelection;
        newSelection.select(modelPlayList->index(lastLinePosition - (numberOfMovedClips - 1), 0), modelPlayList->index(lastLinePosition, 0));
        ui->playList->selectionModel()->select(newSelection, QItemSelectionModel::Select | QItemSelectionModel::Rows);
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
