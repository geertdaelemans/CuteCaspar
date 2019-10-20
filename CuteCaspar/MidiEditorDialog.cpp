#include "MidiEditorDialog.h"
#include "ui_MidiEditorDialog.h"



MidiEditorDialog::MidiEditorDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MidiEditorDialog)
{
    ui->setupUi(this);

    // Disable "What's This" button on Title bar
    this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    m_model = new QStandardItemModel();
    m_model->setHorizontalHeaderLabels(QStringList({"Time Code", "Status", "Pitch"}));

    QSortFilterProxyModel* proxyModel = new QSortFilterProxyModel();
    proxyModel->setSourceModel(m_model);
    proxyModel->sort(0, Qt::AscendingOrder);

    ui->tableView->setModel(proxyModel);
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

    for(QString time : midiPlayList.keys()) {
        m_model->setItem(row, 0, new QStandardItem(time));
        m_model->setItem(row, 1, new QStandardItem(midiPlayList.value(time).type));
        m_model->setItem(row, 2, new QStandardItem(QString::number(midiPlayList.value(time).pitch)));
        row++;
    }
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
        }
    }
}

void MidiEditorDialog::addNewNote(QString timecode, bool noteOn, unsigned int pitch)
{
    int numberOfRows = m_model->rowCount();
    m_model->setItem(numberOfRows, 0, new QStandardItem(timecode));
    m_model->setItem(numberOfRows, 1, new QStandardItem((noteOn ? "ON" : "OFF")));
    m_model->setItem(numberOfRows, 2, new QStandardItem(QString::number(pitch)));
}
