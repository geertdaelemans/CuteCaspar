#include "MidiEditorDialog.h"
#include "ui_MidiEditorDialog.h"

#include "MidiNotes.h"
#include "Timecode.h"
#include "EffectsDelegate.h"


MidiEditorDialog::MidiEditorDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MidiEditorDialog)
{
    ui->setupUi(this);

    // Disable "What's This" button on Title bar
    this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    m_model = new QStandardItemModel();
    m_model->setHorizontalHeaderLabels(QStringList({"Time Code", "Status", "Effect"}));

    QSortFilterProxyModel* proxyModel = new QSortFilterProxyModel();
    proxyModel->setSourceModel(m_model);
    proxyModel->sort(0, Qt::AscendingOrder);

    ui->tableView->setModel(proxyModel);

    ui->tableView->setColumnWidth(0, 80);
    ui->tableView->setColumnWidth(1, 50);
    ui->tableView->setColumnWidth(2, 150);

    ui->tableView->show();
}

MidiEditorDialog::~MidiEditorDialog()
{
    delete ui;
}

void MidiEditorDialog::newMidiPlaylist(QMap<QString, message> midiPlayList)
{
    m_model->setRowCount(0);
    m_newMidiPlaylist = midiPlayList;

    int row = 0;

    MidiNotes* midiNotes = MidiNotes::getInstance();

    for(QString time : midiPlayList.keys()) {
        m_model->setItem(row, 0, new QStandardItem(time));
        m_model->setItem(row, 1, new QStandardItem(midiPlayList.value(time).type));
        m_model->setItem(row, 2, new QStandardItem(midiNotes->getNoteNameByPitch(midiPlayList.value(time).pitch)));
        row++;
    }


    EffectsDelegate * cbid = new EffectsDelegate();

    ui->tableView->setItemDelegateForColumn(1, cbid);
    ui->tableView->setItemDelegateForColumn(2, cbid);
}

void MidiEditorDialog::currentNote(QString timecode, bool noteOn, unsigned int pitch)
{
    Q_UNUSED(noteOn)
    Q_UNUSED(pitch)

    QModelIndexList matches = m_model->match(m_model->index(0,0), Qt::DisplayRole, timecode);
    if (matches.size() == 0) {
        addNewNote(timecode, noteOn, pitch);
    } else {
        foreach(const QModelIndex &index, matches) {
            ui->tableView->selectRow(index.row());
            m_currentIndex = index.row();
        }
    }
}

void MidiEditorDialog::addNewNote(QString timecode, bool noteOn, unsigned int pitch)
{
    m_currentIndex++;
    QList<QStandardItem*> items;
    items.append(new QStandardItem(timecode));
    items.append(new QStandardItem(noteOn ? "ON" : "OFF"));
    items.append(new QStandardItem(MidiNotes::getInstance()->getNoteNameByPitch(pitch)));
    m_model->insertRow(m_currentIndex, items);
    ui->tableView->selectRow(m_currentIndex);
}

void MidiEditorDialog::on_btnAdd_clicked()
{
    QItemSelectionModel *selections = ui->tableView->selectionModel();
    QModelIndexList selected = selections->selectedIndexes();
    int row;
    if (selected.size() != 0) {
        row = selected.begin()->row();
    } else {
        row = m_model->rowCount(QModelIndex());
    }
    double timeBefore, timeAfter;
    if (row > 0) {
        timeBefore = Timecode::toTime(m_model->data(m_model->index(row-1, 0)).toString(), 29.97);
    } else {
        timeBefore = 0.0;
    }
    if (row >= m_model->rowCount()) {
        timeAfter = timeBefore + 1;
    } else {
        timeAfter = Timecode::toTime(m_model->data(m_model->index(row, 0)).toString(), 29.97);
    }
    m_model->insertRow(row);
    ui->tableView->selectRow(row);

    m_model->setItem(row, 0, new QStandardItem(Timecode::fromTime((timeAfter + timeBefore)/2.0, 29.97, false)));
    m_model->setItem(row, 1, new QStandardItem("ON"));
    m_model->setItem(row, 2, new QStandardItem(MidiNotes::getInstance()->getNoteNameByPitch(0)));

}

void MidiEditorDialog::on_btnDelete_clicked()
{
    QItemSelectionModel *selections = ui->tableView->selectionModel();
    QModelIndexList selected = selections->selectedIndexes();
    if (selected.size() != 0) {
        int row = selected.begin()->row();
        m_model->removeRow(row);
        if (row - 1 >= 0) {
            ui->tableView->selectRow(row-1);
        }
    }
}


void MidiEditorDialog::on_btnSave_clicked()
{
    int numberOfRows = m_model->rowCount();
    QMap<QString, message> output;
    for (int i = 0; i < numberOfRows; i++) {
        qDebug() << m_model->data(m_model->index(i,0)).toString()
                 << m_model->data(m_model->index(i,1)).toString()
                 << MidiNotes::getInstance()->getNotePitchByName(m_model->data(m_model->index(i,2)).toString());
        message newNote;
        newNote.timeCode = m_model->data(m_model->index(i,0)).toString();
        newNote.type = m_model->data(m_model->index(i,1)).toString();
        newNote.pitch = MidiNotes::getInstance()->getNotePitchByName(m_model->data(m_model->index(i,2)).toString());
        output[m_model->data(m_model->index(i,0)).toString()] = newNote;
    }
    emit saveMidiPlayList(output);
}
